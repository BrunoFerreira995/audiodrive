#include "audio_driver.hpp"

#include <algorithm>
#include <vector>

namespace audio32 {

AudioDriver::AudioDriver(std::size_t bufferBytes) : buffer_(bufferBytes) {}

AudioDriver::~AudioDriver() {
    stop();
}

bool AudioDriver::initialize() {
    mixer_.setOutputChannels(channels_);
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
    initialized_ = false;
}

double AudioDriver::sampleRate() const noexcept {
    return sampleRate_;
}

void AudioDriver::setChannels(std::uint32_t channels) {
    channels_ = std::max<std::uint32_t>(1, channels);
    mixer_.setOutputChannels(channels_);
    initialized_ = false;
}

std::uint32_t AudioDriver::channels() const noexcept {
    return channels_;
}

std::size_t AudioDriver::write(std::span<const float> interleavedSamples) noexcept {
    return buffer_.write(interleavedSamples);
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
}

} // namespace audio32
