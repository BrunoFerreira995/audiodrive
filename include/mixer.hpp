#pragma once

#include <cstdint>
#include <span>

namespace audio32 {

class Mixer {
public:
    void setOutputChannels(std::uint32_t channels);
    std::uint32_t outputChannels() const noexcept;

    void mixMonoToOutput(std::span<const float> monoInput, std::span<float> output) const noexcept;
    void mixInterleaved(std::span<const float> input, std::uint32_t inputChannels, std::span<float> output) const noexcept;

private:
    std::uint32_t outputChannels_{2};
};

} // namespace audio32
