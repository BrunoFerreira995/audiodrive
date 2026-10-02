#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace audio32 {
struct NativeDeviceInfo { std::string id; std::string name; bool capture; bool isDefault; };
struct NativeAudioConfig {
    std::uint32_t sampleRate = 48000, channels = 2, periodFrames = 128;
    bool playback = true, capture = false;
    std::string outputDeviceId, inputDeviceId;
};
// macOS AudioUnit, Linux ALSA, Windows WASAPI. Control methods require exclusive access.
class NativeAudioBackend {
public:
    using Callback = std::function<void(float*, const float*, std::uint32_t, std::uint32_t)>;
    NativeAudioBackend();
    ~NativeAudioBackend();
    NativeAudioBackend(const NativeAudioBackend&) = delete;
    NativeAudioBackend& operator=(const NativeAudioBackend&) = delete;
    std::vector<NativeDeviceInfo> devices();
    bool initialize(const NativeAudioConfig&, Callback);
    bool start();
    void stop() noexcept;
    bool isRunning() const noexcept;
    std::int32_t lastError() const noexcept;
    std::string backendName() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
