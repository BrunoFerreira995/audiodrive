#include "dsp.hpp"
#include "simd.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace audio32 {

namespace {
float bounded(float value, float lo, float hi, float fallback) noexcept {
    return std::isfinite(value) ? std::clamp(value, lo, hi) : fallback;
}
float clean(float value) noexcept {
    return std::isfinite(value) && std::fabs(value) >= 1e-20F ? value : 0.0F;
}
}

DspEngine::DspEngine() { resizeState(); }

void DspEngine::setFormat(double sampleRate, std::uint32_t channels) {
    sampleRate_ = std::isfinite(sampleRate) ? std::clamp(sampleRate, 8000.0, 384000.0) : 48000.0;
    channels_ = std::clamp<std::uint32_t>(channels, 1, 64);
    delay_.delayFrames = std::min(delay_.delayFrames, static_cast<std::uint32_t>(sampleRate_*2));
    resizeState();
    updateEqualizer();
    for (auto& filter : filters_) filter.coefficients = filter.target;
}
void DspEngine::setGain(float gain) noexcept { gain_ = bounded(gain, 0, 64, 1); }
float DspEngine::gain() const noexcept { return gain_; }
void DspEngine::setLimiterCeiling(float ceiling) noexcept { limiterCeiling_ = bounded(ceiling, 0, 64, 1); }
float DspEngine::limiterCeiling() const noexcept { return limiterCeiling_; }
void DspEngine::setEqualizer(EqualizerSettings settings) noexcept {
    settings.lowGain = bounded(settings.lowGain, 0.001F, 16, 1);
    settings.midGain = bounded(settings.midGain, 0.001F, 16, 1);
    settings.highGain = bounded(settings.highGain, 0.001F, 16, 1);
    const auto maximum = static_cast<float>(sampleRate_ * 0.45);
    settings.lowFrequency = bounded(settings.lowFrequency, 20, maximum, 150);
    settings.midFrequency = bounded(settings.midFrequency, 20, maximum, 1000);
    settings.highFrequency = bounded(settings.highFrequency, 20, maximum, std::min(6000.0F, maximum));
    settings.midQ = bounded(settings.midQ, 0.1F, 10, 0.707F);
    equalizer_ = settings;
    updateEqualizer();
}
EqualizerSettings DspEngine::equalizer() const noexcept { return equalizer_; }
void DspEngine::updateEqualizer() noexcept {
    const float gains[]{equalizer_.lowGain, equalizer_.midGain, equalizer_.highGain};
    const float frequencies[]{equalizer_.lowFrequency, equalizer_.midFrequency, equalizer_.highFrequency};
    for (std::size_t i = 0; i < 3; ++i) {
        const double A = std::sqrt(gains[i]);
        const double w = 2 * std::numbers::pi * std::min<double>(frequencies[i], sampleRate_ * 0.45) / sampleRate_;
        const double c = std::cos(w), sn = std::sin(w);
        double b0, b1, b2, a0, a1, a2;
        if (i == 1) {
            const double alpha = sn / (2 * equalizer_.midQ);
            b0 = 1 + alpha*A; b1 = -2*c; b2 = 1-alpha*A;
            a0 = 1 + alpha/A; a1 = -2*c; a2 = 1-alpha/A;
        } else {
            const double beta = std::sqrt(2*A)*sn;
            if (i == 0) {
                b0=A*((A+1)-(A-1)*c+beta); b1=2*A*((A-1)-(A+1)*c); b2=A*((A+1)-(A-1)*c-beta);
                a0=(A+1)+(A-1)*c+beta; a1=-2*((A-1)+(A+1)*c); a2=(A+1)+(A-1)*c-beta;
            } else {
                b0=A*((A+1)+(A-1)*c+beta); b1=-2*A*((A-1)+(A+1)*c); b2=A*((A+1)+(A-1)*c-beta);
                a0=(A+1)-(A-1)*c+beta; a1=2*((A-1)-(A+1)*c); a2=(A+1)-(A-1)*c-beta;
            }
        }
        filters_[i].target = {b0/a0,b1/a0,b2/a0,a1/a0,a2/a0};
    }
}
void DspEngine::setCompressor(CompressorSettings settings) noexcept {
    settings.threshold = bounded(settings.threshold, 0.000001F, 1, 1);
    settings.ratio = bounded(settings.ratio, 1, 100, 1);
    settings.makeupGain = bounded(settings.makeupGain, 0, 64, 1);
    settings.attackMs = bounded(settings.attackMs, 0.01F, 1000, 10);
    settings.releaseMs = bounded(settings.releaseMs, 1, 10000, 100);
    settings.kneeDb = bounded(settings.kneeDb, 0, 24, 6);
    compressor_ = settings;
}
CompressorSettings DspEngine::compressor() const noexcept { return compressor_; }
void DspEngine::setReverb(ReverbSettings settings) {
    settings.mix = bounded(settings.mix, 0, 1, 0);
    settings.feedback = bounded(settings.feedback, 0, 0.95F, 0.25F);
    settings.damping = bounded(settings.damping, 0, 0.99F, 0.4F);
    reverb_ = settings;
}
ReverbSettings DspEngine::reverb() const noexcept { return reverb_; }
void DspEngine::setDelay(DelaySettings settings) {
    settings.mix = bounded(settings.mix, 0, 1, 0);
    settings.feedback = bounded(settings.feedback, 0, 0.95F, 0.25F);
    settings.delayFrames = std::min(settings.delayFrames, static_cast<std::uint32_t>(sampleRate_*2));
    if (delay_.delayFrames == 0) delayFramesCurrent_ = settings.delayFrames;
    delay_ = settings;
}
DelaySettings DspEngine::delay() const noexcept { return delay_; }

void DspEngine::process(std::span<float> samples) noexcept {
    const double attack = std::exp(-1.0/(sampleRate_*compressor_.attackMs*0.001));
    const double release = std::exp(-1.0/(sampleRate_*compressor_.releaseMs*0.001));
    const double thresholdDb = 20*std::log10(compressor_.threshold);
    const auto capacity = delayLine_.size()/channels_;
    for (std::size_t offset = 0; offset + channels_ <= samples.size(); offset += channels_) {
        for (auto& filter : filters_)
            for (std::size_t k=0; k<5; ++k)
                filter.coefficients[k] += smoothing_*(filter.target[k]-filter.coefficients[k]);
        float peak = 0;
        for (std::size_t ch=0; ch<channels_; ++ch) {
            double x = clean(samples[offset+ch])*gain_;
            for (std::size_t band=0; band<3; ++band) {
                const auto& c = filters_[band].coefficients;
                auto& state = filterState_[ch][band];
                const double y = c[0]*x+state.z1;
                state.z1 = c[1]*x-c[3]*y+state.z2;
                state.z2 = c[2]*x-c[4]*y;
                if (std::fabs(state.z1)<1e-20) state.z1=0;
                if (std::fabs(state.z2)<1e-20) state.z2=0;
                x=y;
            }
            frame_[ch]=clean(static_cast<float>(x));
            peak=std::max(peak,std::fabs(frame_[ch]));
        }
        const double coeff = peak>envelope_ ? attack : release;
        envelope_ = clean(static_cast<float>(coeff*envelope_+(1-coeff)*peak));
        const double over = 20*std::log10(std::max(envelope_,1e-20F))-thresholdDb;
        const double knee = compressor_.kneeDb;
        double reduction = 0;
        if (over >= knee/2) reduction=(1.0/compressor_.ratio-1)*over;
        else if (knee>0 && over>-knee/2) reduction=(1.0/compressor_.ratio-1)*(over+knee/2)*(over+knee/2)/(2*knee);
        const float compression = static_cast<float>(std::pow(10.0,reduction/20))*compressor_.makeupGain;
        reverbMixCurrent_ += smoothing_*(reverb_.mix-reverbMixCurrent_);
        delayMixCurrent_ += smoothing_*(delay_.mix-delayMixCurrent_);
        reverbFeedbackCurrent_ += smoothing_*(reverb_.feedback-reverbFeedbackCurrent_);
        delayFeedbackCurrent_ += smoothing_*(delay_.feedback-delayFeedbackCurrent_);
        delayFramesCurrent_ += smoothing_*(std::max(1U,delay_.delayFrames)-delayFramesCurrent_);
        const double read = std::fmod(delayWriteIndex_+capacity-delayFramesCurrent_,static_cast<double>(capacity));
        const auto first = static_cast<std::size_t>(read);
        const float fraction = static_cast<float>(read-first);
        for (std::size_t ch=0; ch<channels_; ++ch) {
            float x=frame_[ch]*compression, wet=0;
            auto& lines=reverbLines_[ch];
            for (std::size_t i=0;i<4;++i) {
                auto& line=lines[i];
                const float delayed=line.samples[line.index];
                line.damped=clean(delayed*(1-reverb_.damping)+line.damped*reverb_.damping);
                line.samples[line.index]=clean(x+line.damped*reverbFeedbackCurrent_);
                line.index=(line.index+1)%line.samples.size();
                wet+=delayed*0.25F;
            }
            for (std::size_t i=4;i<6;++i) {
                auto& line=lines[i];
                const float delayed=line.samples[line.index];
                const float input=wet;
                wet=delayed-0.5F*input;
                line.samples[line.index]=clean(input+0.5F*wet);
                line.index=(line.index+1)%line.samples.size();
            }
            x=x*(1-reverbMixCurrent_)+wet*reverbMixCurrent_;
            const float delayed=delayLine_[first*channels_+ch]*(1-fraction)+delayLine_[((first+1)%capacity)*channels_+ch]*fraction;
            delayLine_[delayWriteIndex_*channels_+ch]=clean(x+delayed*delayFeedbackCurrent_);
            if (delay_.delayFrames>0) x=x*(1-delayMixCurrent_)+delayed*delayMixCurrent_;
            samples[offset+ch]=std::clamp(clean(x),-limiterCeiling_,limiterCeiling_);
        }
        delayWriteIndex_=(delayWriteIndex_+1)%capacity;
    }
}

std::vector<float> DspEngine::fftMagnitudes(std::span<const float> monoSamples) const {
    const auto n = monoSamples.size();
    std::vector<float> magnitudes(n / 2 + 1, 0.0F);

    for (std::size_t k = 0; k < magnitudes.size(); ++k) {
        float real = 0.0F;
        float imag = 0.0F;
        for (std::size_t t = 0; t < n; ++t) {
            const auto angle = 2.0F * std::numbers::pi_v<float> * static_cast<float>(k * t) / static_cast<float>(n);
            real += monoSamples[t] * std::cos(angle);
            imag -= monoSamples[t] * std::sin(angle);
        }
        magnitudes[k] = std::sqrt(real * real + imag * imag);
    }

    return magnitudes;
}

void DspEngine::resizeState() {
    smoothing_=1-std::exp(-1.0/(sampleRate_*0.01));
    filterState_.assign(channels_, {});
    frame_.assign(channels_, 0);
    delayLine_.assign((static_cast<std::size_t>(sampleRate_*2)+2)*channels_,0);
    reverbLines_.clear();
    reverbLines_.resize(channels_);
    constexpr double times[]{0.0297,0.0371,0.0411,0.0437,0.005,0.0017};
    for (std::size_t ch=0; ch<channels_; ++ch)
        for (std::size_t i=0;i<6;++i)
            reverbLines_[ch][i].samples.assign(static_cast<std::size_t>(sampleRate_*(times[i]+ch*0.00013))+1,0);
    reset();
}
void DspEngine::reset() noexcept {
    for (auto& channel : filterState_) channel={};
    for (auto& channel : reverbLines_) for (auto& line : channel) {
        std::fill(line.samples.begin(),line.samples.end(),0);
        line.index=0; line.damped=0;
    }
    std::fill(delayLine_.begin(),delayLine_.end(),0);
    delayWriteIndex_=0; envelope_=0;
    delayFramesCurrent_=std::max(1U,delay_.delayFrames);
    reverbMixCurrent_=reverb_.mix; delayMixCurrent_=delay_.mix;
    reverbFeedbackCurrent_=reverb_.feedback; delayFeedbackCurrent_=delay_.feedback;
    for (auto& filter : filters_) filter.coefficients=filter.target;
}

} // namespace audio32
