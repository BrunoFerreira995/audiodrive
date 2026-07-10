#include "coreaudio.hpp"

#ifdef __APPLE__
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <utility>
#include <vector>

namespace audio32 {

struct CoreAudioBackend::Impl {
    static constexpr std::uint32_t bufferCount = 3;
    static constexpr std::uint32_t framesPerBuffer = 512;

#ifdef __APPLE__
    AudioQueueRef queue = nullptr;
    std::array<AudioQueueBufferRef, bufferCount> buffers{};
    AudioStreamBasicDescription streamDescription{};
#endif
    RenderCallback callback;
    double sampleRate = 48000.0;
    std::uint32_t channels = 2;
    std::string outputDeviceUid;
    std::atomic<bool>* running = nullptr;
    std::atomic<std::uint64_t>* underruns = nullptr;
    std::atomic<std::int32_t>* lastError = nullptr;
};

CoreAudioBackend::CoreAudioBackend() : impl_(new Impl) {
    impl_->running = &running_;
    impl_->underruns = &underruns_;
    impl_->lastError = &lastError_;
}

CoreAudioBackend::~CoreAudioBackend() {
    stop();
    delete impl_;
}

bool CoreAudioBackend::initialize(double sampleRate, std::uint32_t channels, RenderCallback callback) {
    stop();
    impl_->sampleRate = sampleRate;
    impl_->channels = std::max<std::uint32_t>(1, channels);
    impl_->callback = std::move(callback);
    underruns_.store(0, std::memory_order_release);
    lastError_.store(0, std::memory_order_release);
    return static_cast<bool>(impl_->callback);
}

void CoreAudioBackend::setOutputDeviceUid(std::string deviceUid) {
    impl_->outputDeviceUid = std::move(deviceUid);
}

const std::string& CoreAudioBackend::outputDeviceUid() const noexcept {
    return impl_->outputDeviceUid;
}

#ifdef __APPLE__
namespace {

void setError(std::atomic<std::int32_t>& destination, OSStatus status) noexcept {
    destination.store(static_cast<std::int32_t>(status), std::memory_order_release);
}

bool configureOutputDevice(AudioQueueRef queue, const std::string& outputDeviceUid, std::atomic<std::int32_t>& lastError) {
    if (outputDeviceUid.empty()) {
        return true;
    }

    CFStringRef deviceUid = CFStringCreateWithCString(nullptr, outputDeviceUid.c_str(), kCFStringEncodingUTF8);
    if (deviceUid == nullptr) {
        lastError.store(-1, std::memory_order_release);
        return false;
    }

    const auto status = AudioQueueSetProperty(queue, kAudioQueueProperty_CurrentDevice, &deviceUid, sizeof(deviceUid));
    CFRelease(deviceUid);
    if (status != noErr) {
        setError(lastError, status);
        return false;
    }

    return true;
}

} // namespace
#endif

#ifdef __APPLE__
void CoreAudioBackend::handleOutputBuffer(void* userData, AudioQueueRef queue, AudioQueueBufferRef buffer) {
    auto* impl = static_cast<Impl*>(userData);
    const auto channels = impl->channels;
    const auto frames = Impl::framesPerBuffer;
    const auto byteSize = static_cast<std::uint32_t>(frames * channels * sizeof(float));

    if (buffer->mAudioDataBytesCapacity < byteSize) {
        impl->underruns->fetch_add(1, std::memory_order_acq_rel);
        std::memset(buffer->mAudioData, 0, buffer->mAudioDataBytesCapacity);
        buffer->mAudioDataByteSize = buffer->mAudioDataBytesCapacity;
    } else {
        auto* output = static_cast<float*>(buffer->mAudioData);
        impl->callback(output, frames, channels);
        buffer->mAudioDataByteSize = byteSize;
    }

    const auto status = AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
    if (status != noErr) {
        setError(*impl->lastError, status);
        impl->running->store(false, std::memory_order_release);
    }
}
#endif

bool CoreAudioBackend::start() {
    if (!impl_->callback) {
        return false;
    }

#ifdef __APPLE__
    if (impl_->queue != nullptr) {
        return running_.load(std::memory_order_acquire);
    }

    impl_->streamDescription = {};
    impl_->streamDescription.mSampleRate = impl_->sampleRate;
    impl_->streamDescription.mFormatID = kAudioFormatLinearPCM;
    impl_->streamDescription.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagsNativeEndian;
    impl_->streamDescription.mBytesPerPacket = impl_->channels * sizeof(float);
    impl_->streamDescription.mFramesPerPacket = 1;
    impl_->streamDescription.mBytesPerFrame = impl_->channels * sizeof(float);
    impl_->streamDescription.mChannelsPerFrame = impl_->channels;
    impl_->streamDescription.mBitsPerChannel = sizeof(float) * 8;

    auto status = AudioQueueNewOutput(&impl_->streamDescription, handleOutputBuffer, impl_, nullptr, nullptr, 0, &impl_->queue);
    if (status != noErr) {
        setError(lastError_, status);
        impl_->queue = nullptr;
        return false;
    }

    if (!configureOutputDevice(impl_->queue, impl_->outputDeviceUid, lastError_)) {
        stop();
        return false;
    }

    const auto bufferBytes = static_cast<std::uint32_t>(Impl::framesPerBuffer * impl_->channels * sizeof(float));
    for (auto& buffer : impl_->buffers) {
        status = AudioQueueAllocateBuffer(impl_->queue, bufferBytes, &buffer);
        if (status != noErr) {
            setError(lastError_, status);
            stop();
            return false;
        }

        handleOutputBuffer(impl_, impl_->queue, buffer);
    }

    status = AudioQueueStart(impl_->queue, nullptr);
    if (status != noErr) {
        setError(lastError_, status);
        stop();
        return false;
    }
#endif

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
    impl_->buffers = {};
#endif
}

bool CoreAudioBackend::isRunning() const noexcept {
    return running_.load(std::memory_order_acquire);
}

std::uint64_t CoreAudioBackend::underrunCount() const noexcept {
    return underruns_.load(std::memory_order_acquire);
}

std::int32_t CoreAudioBackend::lastError() const noexcept {
    return lastError_.load(std::memory_order_acquire);
}

} // namespace audio32
