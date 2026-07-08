#include "dsp.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace audio32 {

void DspEngine::setGain(float gain) noexcept {
    gain_ = gain;
}

float DspEngine::gain() const noexcept {
    return gain_;
}

void DspEngine::setLimiterCeiling(float ceiling) noexcept {
    limiterCeiling_ = std::max(0.0F, ceiling);
}

float DspEngine::limiterCeiling() const noexcept {
    return limiterCeiling_;
}

void DspEngine::process(std::span<float> interleavedSamples) noexcept {
    const auto ceiling = limiterCeiling_;
    for (auto& sample : interleavedSamples) {
        sample = std::clamp(sample * gain_, -ceiling, ceiling);
    }
}

std::vector<float> DspEngine::fftMagnitudes(std::span<const float> monoSamples) const {
    const auto n = monoSamples.size();
    std::vector<float> magnitudes(n / 2 + 1, 0.0F);

    for (std::size_t k = 0; k < magnitudes.size(); ++k) {
        float real = 0.0F;
        float imag = 0.0F;
        for (std::size_t t = 0; t < n; ++t) {
            const auto angle = 2.0F * std::numbers::pi_v<float> * static_cast<float>(k * t) / static_cast<float>(n);
            real += monoSamples[t] * std::cos(angle);
            imag -= monoSamples[t] * std::sin(angle);
        }
        magnitudes[k] = std::sqrt(real * real + imag * imag);
    }

    return magnitudes;
}

} // namespace audio32
