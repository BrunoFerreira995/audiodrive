#pragma once

#include <array>
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

enum class OutputBackend { AudioQueue, Native };

enum class BufferSize : std::uint32_t {
    Frames32 = 32,
    Frames64 = 64,
    Frames128 = 128,
    Frames256 = 256,
    Frames512 = 512,
    Frames1024 = 1024,
    Frames2048 = 2048,
};

struct BufferSizeProfile {
    BufferSize size;
    std::uint32_t frames;
    double latencyMs;
    const char* application;
};

inline constexpr std::array<BufferSizeProfile, 7> kBufferSizeProfiles{{
    {BufferSize::Frames32, 32, 32.0 * 1000.0 / 48000.0, "Highly responsive DSP"},
    {BufferSize::Frames64, 64, 64.0 * 1000.0 / 48000.0, "Virtual instruments"},
    {BufferSize::Frames128, 128, 128.0 * 1000.0 / 48000.0, "Production and monitoring"},
    {BufferSize::Frames256, 256, 256.0 * 1000.0 / 48000.0, "Recording and mixing"},
    {BufferSize::Frames512, 512, 512.0 * 1000.0 / 48000.0, "Stable playback"},
    {BufferSize::Frames1024, 1024, 1024.0 * 1000.0 / 48000.0, "Latency-insensitive processing"},
    {BufferSize::Frames2048, 2048, 2048.0 * 1000.0 / 48000.0, "Large-buffer operations"},
}};

constexpr std::uint32_t bufferSizeFrames(BufferSize size) noexcept {
    return static_cast<std::uint32_t>(size);
}

constexpr bool isSupportedBufferSize(std::uint32_t frames) noexcept {
    switch (frames) {
        case 32:
        case 64:
        case 128:
        case 256:
        case 512:
        case 1024:
        case 2048:
            return true;
        default:
            return false;
    }
}

constexpr double bufferLatencyMs(std::uint32_t frames, double sampleRate = 48000.0) noexcept {
    return sampleRate > 0.0 ? static_cast<double>(frames) * 1000.0 / sampleRate : 0.0;
}

constexpr double bufferLatencyMs(BufferSize size, double sampleRate = 48000.0) noexcept {
    return bufferLatencyMs(bufferSizeFrames(size), sampleRate);
}

class CoreAudioBackend {
public:
    using RenderCallback = std::function<void(float* output, std::uint32_t frames, std::uint32_t channels)>;

    CoreAudioBackend();
    ~CoreAudioBackend();

    CoreAudioBackend(const CoreAudioBackend&) = delete;
    CoreAudioBackend& operator=(const CoreAudioBackend&) = delete;

    void setBackend(OutputBackend backend);
    OutputBackend backend() const noexcept;
    void setPeriodFrames(std::uint32_t frames);
    std::uint32_t periodFrames() const noexcept;

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
