#pragma once

#include <cstddef>
#include <span>

namespace audio32 {

struct SimdCapabilities {
    bool scalar = true;
    bool armNeon = false;
    bool appleSilicon = false;
};

SimdCapabilities simdCapabilities() noexcept;
void applyGainLimiter(std::span<float> samples, float gain, float ceiling) noexcept;

} // namespace audio32
