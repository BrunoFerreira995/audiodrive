#pragma once

#include "buffer.hpp"
#include "coreaudio.hpp"
#include "dsp.hpp"
#include "mixer.hpp"

#include <cstdint>
#include <span>

namespace audio32 {

class AudioDriver {
public:
    explicit AudioDriver(std::size_t bufferBytes = 1024 * 1024);
    ~AudioDriver();

    bool initialize();
    bool start();
    void stop();

    void setSampleRate(double sampleRate);
    double sampleRate() const noexcept;

    void setChannels(std::uint32_t channels);
    std::uint32_t channels() const noexcept;

    std::size_t write(std::span<const float> interleavedSamples) noexcept;

    DspEngine& dsp() noexcept;
    Mixer& mixer() noexcept;

private:
    void render(float* output, std::uint32_t frames, std::uint32_t channels) noexcept;

    double sampleRate_{48000.0};
    std::uint32_t channels_{2};
    AudioBuffer buffer_;
    Mixer mixer_;
    DspEngine dsp_;
    CoreAudioBackend backend_;
    bool initialized_{false};
};

} // namespace audio32
