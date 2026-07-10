#include "metal_visualization.hpp"

#include <algorithm>
#include <cmath>

namespace audio32 {

MetalVisualizationCapabilities metalVisualizationCapabilities() {
    return MetalVisualizationCapabilities{
#ifdef __APPLE__
        true,
#else
        false,
#endif
        true
    };
}

std::vector<VisualizationBar> makeSpectrumBars(std::span<const float> magnitudes, std::uint32_t maxBars) {
    if (magnitudes.empty() || maxBars == 0) {
        return {};
    }

    const auto barCount = std::min<std::size_t>(magnitudes.size(), maxBars);
    std::vector<VisualizationBar> bars(barCount);
    const auto stride = static_cast<float>(magnitudes.size()) / static_cast<float>(barCount);
    const auto width = barCount > 1 ? 2.0F / static_cast<float>(barCount - 1) : 0.0F;

    auto maxMagnitude = 0.0F;
    for (const auto magnitude : magnitudes) {
        maxMagnitude = std::max(maxMagnitude, std::fabs(magnitude));
    }
    maxMagnitude = std::max(maxMagnitude, 1.0F);

    for (std::size_t bar = 0; bar < barCount; ++bar) {
        const auto source = std::min<std::size_t>(static_cast<std::size_t>(static_cast<float>(bar) * stride), magnitudes.size() - 1);
        const auto normalized = std::clamp(std::fabs(magnitudes[source]) / maxMagnitude, 0.0F, 1.0F);
        bars[bar] = VisualizationBar{
            -1.0F + (width * static_cast<float>(bar)),
            normalized,
            0.15F + (0.65F * normalized),
            0.75F,
            1.0F - (0.55F * normalized)
        };
    }

    return bars;
}

} // namespace audio32
