#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace audio32 {

class DspEngine {
public:
    void setGain(float gain) noexcept;
    float gain() const noexcept;

    void setLimiterCeiling(float ceiling) noexcept;
    float limiterCeiling() const noexcept;

    void process(std::span<float> interleavedSamples) noexcept;

    std::vector<float> fftMagnitudes(std::span<const float> monoSamples) const;

private:
    float gain_{1.0F};
    float limiterCeiling_{1.0F};
};

} // namespace audio32
