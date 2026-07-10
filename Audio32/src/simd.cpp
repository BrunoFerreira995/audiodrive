#include "simd.hpp"

#include <algorithm>
#include <cmath>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

namespace audio32 {

SimdCapabilities simdCapabilities() noexcept {
    return SimdCapabilities{
        true,
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
        true,
#else
        false,
#endif
#if defined(__APPLE__) && defined(__aarch64__)
        true
#else
        false
#endif
    };
}

void applyGainLimiter(std::span<float> samples, float gain, float ceiling) noexcept {
    const auto limit = std::max(0.0F, ceiling);
    std::size_t index = 0;

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    const auto gainVector = vdupq_n_f32(gain);
    const auto high = vdupq_n_f32(limit);
    const auto low = vdupq_n_f32(-limit);
    for (; index + 4 <= samples.size(); index += 4) {
        auto values = vld1q_f32(samples.data() + index);
        values = vmulq_f32(values, gainVector);
        values = vmaxq_f32(low, vminq_f32(high, values));
        vst1q_f32(samples.data() + index, values);
    }
#endif

    for (; index < samples.size(); ++index) {
        samples[index] = std::clamp(samples[index] * gain, -limit, limit);
    }
}

} // namespace audio32
