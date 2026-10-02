#pragma once
#include "buffer.hpp"
#include "native_audio.hpp"
#include <atomic>
#include <span>
namespace audio32 {
// One control-thread consumer; the device callback is the sole producer.
class AudioInputCapture {
public:
    explicit AudioInputCapture(std::size_t capacityBytes=1024*1024);
    ~AudioInputCapture();
    bool initialize(std::uint32_t sampleRate,std::uint32_t channels,std::string deviceId={},std::uint32_t periodFrames=128);
    bool start();
    void stop() noexcept;
    std::size_t read(std::span<float>) noexcept;
    std::size_t availableSamples() const noexcept;
    std::uint64_t droppedSamples() const noexcept;
    std::int32_t lastError() const noexcept;
private:
    std::uint32_t channels_{1};
    AudioBuffer buffer_;
    std::atomic<std::uint64_t> dropped_{0};
    NativeAudioBackend backend_;
};
}
