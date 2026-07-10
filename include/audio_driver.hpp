#pragma once

#include "buffer.hpp"
#include "coreaudio.hpp"
#include "dsp.hpp"
#include "mixer.hpp"

#include <atomic>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

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

    void setOutputDeviceUid(std::string deviceUid);
    const std::string& outputDeviceUid() const noexcept;
    std::uint64_t underrunCount() const noexcept;
    std::int32_t lastBackendError() const noexcept;

    std::size_t write(std::span<const float> interleavedSamples) noexcept;
    void setRecordingEnabled(bool enabled) noexcept;
    bool isRecordingEnabled() const noexcept;
    std::vector<float> takeRecordedSamples();
    void setLoopbackCaptureEnabled(bool enabled) noexcept;
    bool isLoopbackCaptureEnabled() const noexcept;
    std::vector<float> takeLoopbackSamples();

    DspEngine& dsp() noexcept;
    Mixer& mixer() noexcept;

private:
    void render(float* output, std::uint32_t frames, std::uint32_t channels) noexcept;

    double sampleRate_{48000.0};
    std::uint32_t channels_{2};
    AudioBuffer buffer_;
    AudioBuffer recordingBuffer_;
    AudioBuffer loopbackBuffer_;
    Mixer mixer_;
    DspEngine dsp_;
    CoreAudioBackend backend_;
    std::atomic<bool> recordingEnabled_{false};
    std::atomic<bool> loopbackCaptureEnabled_{false};
    bool initialized_{false};
};

} // namespace audio32
