#include "audio_driver.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace audio32 {

AudioDriver::AudioDriver(std::size_t bufferBytes)
    : buffer_(bufferBytes),
      recordingBuffer_(bufferBytes),
      loopbackBuffer_(bufferBytes) {}

AudioDriver::~AudioDriver() {
    stop();
}

bool AudioDriver::initialize() {
    mixer_.setOutputChannels(channels_);
    dsp_.setFormat(sampleRate_, channels_);
    initialized_ = backend_.initialize(sampleRate_, channels_, [this](float* output, std::uint32_t frames, std::uint32_t channels) {
        render(output, frames, channels);
    });
    return initialized_;
}

bool AudioDriver::start() {
    if (!initialized_ && !initialize()) {
        return false;
    }
    return backend_.start();
}

void AudioDriver::stop() {
    backend_.stop();
}

void AudioDriver::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    dsp_.setFormat(sampleRate_, channels_);
    initialized_ = false;
}

double AudioDriver::sampleRate() const noexcept {
    return sampleRate_;
}

void AudioDriver::setChannels(std::uint32_t channels) {
    channels_ = std::max<std::uint32_t>(1, channels);
    mixer_.setOutputChannels(channels_);
    dsp_.setFormat(sampleRate_, channels_);
    initialized_ = false;
}

std::uint32_t AudioDriver::channels() const noexcept {
    return channels_;
}

void AudioDriver::setOutputDeviceUid(std::string deviceUid) {
    backend_.setOutputDeviceUid(std::move(deviceUid));
}

const std::string& AudioDriver::outputDeviceUid() const noexcept {
    return backend_.outputDeviceUid();
}

std::uint64_t AudioDriver::underrunCount() const noexcept {
    return backend_.underrunCount();
}

std::int32_t AudioDriver::lastBackendError() const noexcept {
    return backend_.lastError();
}

std::size_t AudioDriver::write(std::span<const float> interleavedSamples) noexcept {
    return buffer_.write(interleavedSamples);
}

void AudioDriver::setRecordingEnabled(bool enabled) noexcept {
    if (!enabled) {
        recordingBuffer_.clear();
    }
    recordingEnabled_.store(enabled, std::memory_order_release);
}

bool AudioDriver::isRecordingEnabled() const noexcept {
    return recordingEnabled_.load(std::memory_order_acquire);
}

std::vector<float> AudioDriver::takeRecordedSamples() {
    std::vector<float> samples(recordingBuffer_.availableRead(), 0.0F);
    recordingBuffer_.read(samples);
    return samples;
}

void AudioDriver::setLoopbackCaptureEnabled(bool enabled) noexcept {
    if (!enabled) {
        loopbackBuffer_.clear();
    }
    loopbackCaptureEnabled_.store(enabled, std::memory_order_release);
}

bool AudioDriver::isLoopbackCaptureEnabled() const noexcept {
    return loopbackCaptureEnabled_.load(std::memory_order_acquire);
}

std::vector<float> AudioDriver::takeLoopbackSamples() {
    std::vector<float> samples(loopbackBuffer_.availableRead(), 0.0F);
    loopbackBuffer_.read(samples);
    return samples;
}

DspEngine& AudioDriver::dsp() noexcept {
    return dsp_;
}

Mixer& AudioDriver::mixer() noexcept {
    return mixer_;
}

void AudioDriver::render(float* output, std::uint32_t frames, std::uint32_t channels) noexcept {
    std::span<float> out(output, static_cast<std::size_t>(frames) * channels);
    buffer_.read(out);
    dsp_.process(out);
    if (recordingEnabled_.load(std::memory_order_acquire)) {
        recordingBuffer_.write(out);
    }
    if (loopbackCaptureEnabled_.load(std::memory_order_acquire)) {
        loopbackBuffer_.write(out);
    }
}

} // namespace audio32
