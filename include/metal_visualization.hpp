#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace audio32 {

struct MetalVisualizationCapabilities {
    bool available = false;
    bool offlineFramePreparation = true;
};

struct VisualizationBar {
    float x = 0.0F;
    float height = 0.0F;
    float red = 0.0F;
    float green = 0.0F;
    float blue = 0.0F;
};

MetalVisualizationCapabilities metalVisualizationCapabilities();
std::vector<VisualizationBar> makeSpectrumBars(std::span<const float> magnitudes, std::uint32_t maxBars);

} // namespace audio32
