#pragma once
#include "dsp.hpp"
#include <optional>
#include <string>
#include <string_view>
namespace audio32 {
struct EffectChainSettings {
    float gain=1, ceiling=1;
    EqualizerSettings equalizer;
    CompressorSettings compressor;
    ReverbSettings reverb;
    DelaySettings delay;
};
enum class EffectPreset { Neutral, Voice, Room, Echo };
EffectChainSettings effectPreset(EffectPreset, double sampleRate=48000);
EffectChainSettings captureEffectChain(const DspEngine&) noexcept;
void applyEffectChain(DspEngine&, const EffectChainSettings&, bool clearTails=false);
// Versioned text format; parsing rejects malformed, unknown, missing, or nonfinite fields.
std::string serializeEffectChain(const EffectChainSettings&);
std::optional<EffectChainSettings> deserializeEffectChain(std::string_view);
}
