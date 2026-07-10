#pragma once

#include "dsp.hpp"

#include <cstdint>
#include <optional>
#include <unordered_map>

namespace audio32 {

enum class MidiTarget {
    Gain,
    LimiterCeiling,
    EqualizerLowGain,
    EqualizerMidGain,
    EqualizerHighGain,
    CompressorThreshold,
    CompressorRatio,
    ReverbMix,
    DelayMix
};

struct MidiControlMapping {
    std::uint8_t controller = 0;
    MidiTarget target = MidiTarget::Gain;
    float minimum = 0.0F;
    float maximum = 1.0F;
};

class MidiMapper {
public:
    void mapControl(MidiControlMapping mapping);
    void clearControl(std::uint8_t controller);
    std::optional<MidiControlMapping> mappingFor(std::uint8_t controller) const;
    bool applyControlChange(std::uint8_t controller, std::uint8_t value, DspEngine& dsp) const;

private:
    std::unordered_map<std::uint8_t, MidiControlMapping> mappings_;
};

} // namespace audio32
