#include "mixer.hpp"

#include <algorithm>
#include <stdexcept>

namespace audio32 {

void Mixer::setOutputChannels(std::uint32_t channels) {
    if (channels == 0) {
        throw std::invalid_argument("output channel count must be greater than zero");
    }
    outputChannels_ = channels;
}

std::uint32_t Mixer::outputChannels() const noexcept {
    return outputChannels_;
}

void Mixer::mixMonoToOutput(std::span<const float> monoInput, std::span<float> output) const noexcept {
    const auto frames = std::min(monoInput.size(), output.size() / outputChannels_);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        for (std::uint32_t channel = 0; channel < outputChannels_; ++channel) {
            output[frame * outputChannels_ + channel] = monoInput[frame];
        }
    }
}

void Mixer::mixInterleaved(std::span<const float> input, std::uint32_t inputChannels, std::span<float> output) const noexcept {
    if (inputChannels == 0) {
        std::fill(output.begin(), output.end(), 0.0F);
        return;
    }

    const auto frames = std::min(input.size() / inputChannels, output.size() / outputChannels_);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        for (std::uint32_t channel = 0; channel < outputChannels_; ++channel) {
            const auto sourceChannel = std::min<std::uint32_t>(channel, inputChannels - 1);
            output[frame * outputChannels_ + channel] = input[frame * inputChannels + sourceChannel];
        }
    }
}

} // namespace audio32
