#include "coreaudio.hpp"

#ifdef __APPLE__
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#endif

#include <algorithm>
#include <vector>

namespace audio32 {

struct CoreAudioBackend::Impl {
#ifdef __APPLE__
    AudioQueueRef queue = nullptr;
#endif
    RenderCallback callback;
    double sampleRate = 48000.0;
    std::uint32_t channels = 2;
};

CoreAudioBackend::CoreAudioBackend() : impl_(new Impl) {}

CoreAudioBackend::~CoreAudioBackend() {
    stop();
    delete impl_;
}

bool CoreAudioBackend::initialize(double sampleRate, std::uint32_t channels, RenderCallback callback) {
    impl_->sampleRate = sampleRate;
    impl_->channels = channels;
    impl_->callback = std::move(callback);
    return static_cast<bool>(impl_->callback);
}

bool CoreAudioBackend::start() {
    if (!impl_->callback) {
        return false;
    }
    running_.store(true, std::memory_order_release);
    return true;
}

void CoreAudioBackend::stop() {
    running_.store(false, std::memory_order_release);
#ifdef __APPLE__
    if (impl_ != nullptr && impl_->queue != nullptr) {
        AudioQueueStop(impl_->queue, true);
        AudioQueueDispose(impl_->queue, true);
        impl_->queue = nullptr;
    }
#endif
}

bool CoreAudioBackend::isRunning() const noexcept {
    return running_.load(std::memory_order_acquire);
}

} // namespace audio32
