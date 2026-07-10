#include "dsp.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace audio32 {

void DspEngine::setFormat(double sampleRate, std::uint32_t channels) {
    sampleRate_ = std::max(1.0, sampleRate);
    channels_ = std::max<std::uint32_t>(1, channels);
    resizeState();
}

void DspEngine::setGain(float gain) noexcept {
    gain_ = gain;
}

float DspEngine::gain() const noexcept {
    return gain_;
}

void DspEngine::setLimiterCeiling(float ceiling) noexcept {
    limiterCeiling_ = std::max(0.0F, ceiling);
}

float DspEngine::limiterCeiling() const noexcept {
    return limiterCeiling_;
}

void DspEngine::setEqualizer(EqualizerSettings settings) noexcept {
    settings.lowGain = std::max(0.0F, settings.lowGain);
    settings.midGain = std::max(0.0F, settings.midGain);
    settings.highGain = std::max(0.0F, settings.highGain);
    equalizer_ = settings;
}

EqualizerSettings DspEngine::equalizer() const noexcept {
    return equalizer_;
}

void DspEngine::setCompressor(CompressorSettings settings) noexcept {
    settings.threshold = std::clamp(settings.threshold, 0.0F, 1.0F);
    settings.ratio = std::max(1.0F, settings.ratio);
    settings.makeupGain = std::max(0.0F, settings.makeupGain);
    compressor_ = settings;
}

CompressorSettings DspEngine::compressor() const noexcept {
    return compressor_;
}

void DspEngine::setReverb(ReverbSettings settings) {
    settings.mix = std::clamp(settings.mix, 0.0F, 1.0F);
    settings.feedback = std::clamp(settings.feedback, 0.0F, 0.95F);
    reverb_ = settings;
    resizeState();
}

ReverbSettings DspEngine::reverb() const noexcept {
    return reverb_;
}

void DspEngine::setDelay(DelaySettings settings) {
    settings.mix = std::clamp(settings.mix, 0.0F, 1.0F);
    settings.feedback = std::clamp(settings.feedback, 0.0F, 0.95F);
    delay_ = settings;
    resizeState();
}

DelaySettings DspEngine::delay() const noexcept {
    return delay_;
}

void DspEngine::process(std::span<float> interleavedSamples) noexcept {
    const auto ceiling = limiterCeiling_;
    const auto lowAlpha = 0.08F;
    const auto reverbMix = reverb_.mix;
    const auto reverbFeedback = reverb_.feedback;
    const auto delayMix = delay_.mix;
    const auto delayFeedback = delay_.feedback;
    const auto hasDelay = !delayLine_.empty();

    for (std::size_t index = 0; index < interleavedSamples.size(); ++index) {
        const auto channel = channels_ == 0 ? 0 : index % channels_;
        auto sample = interleavedSamples[index] * gain_;

        if (channel < lowState_.size()) {
            lowState_[channel] += lowAlpha * (sample - lowState_[channel]);
            const auto low = lowState_[channel];
            const auto high = sample - low;
            const auto mid = sample - low - (high * 0.5F);
            sample = (low * equalizer_.lowGain) + (mid * equalizer_.midGain) + (high * equalizer_.highGain);
        }

        const auto magnitude = std::fabs(sample);
        if (magnitude > compressor_.threshold && compressor_.threshold < 1.0F) {
            const auto compressed = compressor_.threshold + ((magnitude - compressor_.threshold) / compressor_.ratio);
            sample = std::copysign(compressed, sample);
        }
        sample *= compressor_.makeupGain;

        if (channel < reverbState_.size() && reverbMix > 0.0F) {
            const auto wet = reverbState_[channel];
            reverbState_[channel] = sample + (wet * reverbFeedback);
            sample = (sample * (1.0F - reverbMix)) + (wet * reverbMix);
        }

        if (hasDelay && delayMix > 0.0F) {
            const auto delayed = delayLine_[delayWriteIndex_];
            delayLine_[delayWriteIndex_] = sample + (delayed * delayFeedback);
            delayWriteIndex_ = (delayWriteIndex_ + 1) % delayLine_.size();
            sample = (sample * (1.0F - delayMix)) + (delayed * delayMix);
        }

        interleavedSamples[index] = std::clamp(sample, -ceiling, ceiling);
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
    lowState_.assign(channels_, 0.0F);
    reverbState_.assign(channels_, 0.0F);

    if (delay_.delayFrames == 0) {
        delayLine_.clear();
        delayWriteIndex_ = 0;
        return;
    }

    delayLine_.assign(static_cast<std::size_t>(delay_.delayFrames) * channels_, 0.0F);
    delayWriteIndex_ = 0;
}

} // namespace audio32
