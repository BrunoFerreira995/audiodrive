#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace audio32 {

struct EqualizerSettings {
    float lowGain = 1.0F;
    float midGain = 1.0F;
    float highGain = 1.0F;
};

struct CompressorSettings {
    float threshold = 1.0F;
    float ratio = 1.0F;
    float makeupGain = 1.0F;
};

struct ReverbSettings {
    float mix = 0.0F;
    float feedback = 0.25F;
};

struct DelaySettings {
    float mix = 0.0F;
    float feedback = 0.25F;
    std::uint32_t delayFrames = 0;
};

class DspEngine {
public:
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
    std::vector<float> lowState_;
    std::vector<float> reverbState_;
    std::vector<float> delayLine_;
    std::size_t delayWriteIndex_{0};
};

} // namespace audio32
