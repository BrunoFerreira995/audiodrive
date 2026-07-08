#pragma once

#include <atomic>
#include <cstdint>
#include <functional>

namespace audio32 {

class CoreAudioBackend {
public:
    using RenderCallback = std::function<void(float* output, std::uint32_t frames, std::uint32_t channels)>;

    CoreAudioBackend();
    ~CoreAudioBackend();

    CoreAudioBackend(const CoreAudioBackend&) = delete;
    CoreAudioBackend& operator=(const CoreAudioBackend&) = delete;

    bool initialize(double sampleRate, std::uint32_t channels, RenderCallback callback);
    bool start();
    void stop();
    bool isRunning() const noexcept;

private:
    struct Impl;
    Impl* impl_{nullptr};
    std::atomic<bool> running_{false};
};

} // namespace audio32
