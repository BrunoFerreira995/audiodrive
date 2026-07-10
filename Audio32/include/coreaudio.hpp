#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

#ifdef __APPLE__
struct OpaqueAudioQueue;
using AudioQueueRef = OpaqueAudioQueue*;
struct AudioQueueBuffer;
using AudioQueueBufferRef = AudioQueueBuffer*;
#endif

namespace audio32 {

class CoreAudioBackend {
public:
    using RenderCallback = std::function<void(float* output, std::uint32_t frames, std::uint32_t channels)>;

    CoreAudioBackend();
    ~CoreAudioBackend();

    CoreAudioBackend(const CoreAudioBackend&) = delete;
    CoreAudioBackend& operator=(const CoreAudioBackend&) = delete;

    bool initialize(double sampleRate, std::uint32_t channels, RenderCallback callback);
    void setOutputDeviceUid(std::string deviceUid);
    const std::string& outputDeviceUid() const noexcept;
    bool start();
    void stop();
    bool isRunning() const noexcept;
    std::uint64_t underrunCount() const noexcept;
    std::int32_t lastError() const noexcept;

private:
#ifdef __APPLE__
    static void handleOutputBuffer(void* userData, AudioQueueRef queue, AudioQueueBufferRef buffer);
#endif

    struct Impl;
    Impl* impl_{nullptr};
    std::atomic<bool> running_{false};
    std::atomic<std::uint64_t> underruns_{0};
    std::atomic<std::int32_t> lastError_{0};
};

} // namespace audio32
