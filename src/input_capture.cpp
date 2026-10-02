#include "input_capture.hpp"
#include <utility>
#include <algorithm>
namespace audio32 {
AudioInputCapture::AudioInputCapture(std::size_t bytes):buffer_(bytes) {}
AudioInputCapture::~AudioInputCapture() { stop(); }
bool AudioInputCapture::initialize(std::uint32_t rate,std::uint32_t channels,std::string id,std::uint32_t period) {
    if (channels<1 || channels>64) {
        NativeAudioConfig invalid; invalid.channels=channels;
        stop(); return backend_.initialize(invalid,{});
    }
    stop(); buffer_.clear(); dropped_=0; channels_=channels;
    NativeAudioConfig config;
    config.sampleRate=rate; config.channels=channels; config.periodFrames=period;
    config.playback=false; config.capture=true; config.inputDeviceId=std::move(id);
    return backend_.initialize(config,[this](float*,const float* input,std::uint32_t frames,std::uint32_t count) {
        if (!input) { dropped_.fetch_add(static_cast<std::uint64_t>(frames)*count,std::memory_order_relaxed); return; }
        const std::span<const float> samples(input,static_cast<std::size_t>(frames)*count);
        const auto writable=std::min(samples.size(),buffer_.availableWrite());
        dropped_.fetch_add(samples.size()-buffer_.write(samples.first(writable-writable%channels_)),std::memory_order_relaxed);
    });
}
bool AudioInputCapture::start() { return backend_.start(); }
void AudioInputCapture::stop() noexcept { backend_.stop(); }
std::size_t AudioInputCapture::read(std::span<float> output) noexcept { return buffer_.read(output.first(output.size()-output.size()%channels_)); }
std::size_t AudioInputCapture::availableSamples() const noexcept { return buffer_.availableRead(); }
std::uint64_t AudioInputCapture::droppedSamples() const noexcept { return dropped_.load(); }
std::int32_t AudioInputCapture::lastError() const noexcept { return backend_.lastError(); }
}
