#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace audio32 {

struct EqualizerSettings {
    float lowGain = 1.0F;
    float midGain = 1.0F;
    float highGain = 1.0F;
    float lowFrequency = 150.0F;
    float midFrequency = 1000.0F;
    float highFrequency = 6000.0F;
    float midQ = 0.707F;
};

struct CompressorSettings {
    float threshold = 1.0F;
    float ratio = 1.0F;
    float makeupGain = 1.0F;
    float attackMs = 10.0F;
    float releaseMs = 100.0F;
    float kneeDb = 6.0F;
};

struct ReverbSettings {
    float mix = 0.0F;
    float feedback = 0.25F;
    float damping = 0.4F;
};

struct DelaySettings {
    float mix = 0.0F;
    float feedback = 0.25F;
    std::uint32_t delayFrames = 0;
};

class DspEngine {
public:
    DspEngine();
    // Configuration and reset require exclusive access; process never allocates.
    void reset() noexcept;
    void setFormat(double sampleRate, std::uint32_t channels);

    void setGain(float gain) noexcept;
    float gain() const noexcept;

    void setLimiterCeiling(float ceiling) noexcept;
    float limiterCeiling() const noexcept;

    void setEqualizer(EqualizerSettings settings) noexcept;
    EqualizerSettings equalizer() const noexcept;

    void setCompressor(CompressorSettings settings) noexcept;
    CompressorSettings compressor() const noexcept;

    void setReverb(ReverbSettings settings);
    ReverbSettings reverb() const noexcept;

    void setDelay(DelaySettings settings);
    DelaySettings delay() const noexcept;

    void process(std::span<float> interleavedSamples) noexcept;

    std::vector<float> fftMagnitudes(std::span<const float> monoSamples) const;

private:
    void resizeState();

    double sampleRate_{48000.0};
    std::uint32_t channels_{2};
    float gain_{1.0F};
    float limiterCeiling_{1.0F};
    EqualizerSettings equalizer_;
    CompressorSettings compressor_;
    ReverbSettings reverb_;
    DelaySettings delay_;
    struct Biquad {
        std::array<double, 5> coefficients{1, 0, 0, 0, 0};
        std::array<double, 5> target{1, 0, 0, 0, 0};
    };
    struct FilterState { double z1 = 0, z2 = 0; };
    struct ReverbLine {
        std::vector<float> samples;
        std::size_t index = 0;
        float damped = 0;
    };
    void updateEqualizer() noexcept;
    std::array<Biquad, 3> filters_;
    std::vector<std::array<FilterState, 3>> filterState_;
    std::vector<std::array<ReverbLine, 6>> reverbLines_;
    std::vector<float> frame_;
    std::vector<float> delayLine_;
    std::size_t delayWriteIndex_{0};
    double delayFramesCurrent_{0};
    float envelope_{0};
    float reverbMixCurrent_{0}, delayMixCurrent_{0};
    float reverbFeedbackCurrent_{0.25F}, delayFeedbackCurrent_{0.25F};
    double smoothing_{0};

};

} // namespace audio32
