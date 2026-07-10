#include "midi.hpp"

#include <algorithm>

namespace audio32 {
namespace {

float scaleMidiValue(std::uint8_t value, float minimum, float maximum) noexcept {
    const auto normalized = static_cast<float>(value) / 127.0F;
    return minimum + ((maximum - minimum) * normalized);
}

} // namespace

void MidiMapper::mapControl(MidiControlMapping mapping) {
    if (mapping.maximum < mapping.minimum) {
        std::swap(mapping.minimum, mapping.maximum);
    }
    mappings_[mapping.controller] = mapping;
}

void MidiMapper::clearControl(std::uint8_t controller) {
    mappings_.erase(controller);
}

std::optional<MidiControlMapping> MidiMapper::mappingFor(std::uint8_t controller) const {
    const auto found = mappings_.find(controller);
    if (found == mappings_.end()) {
        return std::nullopt;
    }
    return found->second;
}

bool MidiMapper::applyControlChange(std::uint8_t controller, std::uint8_t value, DspEngine& dsp) const {
    const auto mapping = mappingFor(controller);
    if (!mapping) {
        return false;
    }

    const auto scaled = scaleMidiValue(value, mapping->minimum, mapping->maximum);
    switch (mapping->target) {
    case MidiTarget::Gain:
        dsp.setGain(scaled);
        break;
    case MidiTarget::LimiterCeiling:
        dsp.setLimiterCeiling(scaled);
        break;
    case MidiTarget::EqualizerLowGain: {
        auto settings = dsp.equalizer();
        settings.lowGain = scaled;
        dsp.setEqualizer(settings);
        break;
    }
    case MidiTarget::EqualizerMidGain: {
        auto settings = dsp.equalizer();
        settings.midGain = scaled;
        dsp.setEqualizer(settings);
        break;
    }
    case MidiTarget::EqualizerHighGain: {
        auto settings = dsp.equalizer();
        settings.highGain = scaled;
        dsp.setEqualizer(settings);
        break;
    }
    case MidiTarget::CompressorThreshold: {
        auto settings = dsp.compressor();
        settings.threshold = scaled;
        dsp.setCompressor(settings);
        break;
    }
    case MidiTarget::CompressorRatio: {
        auto settings = dsp.compressor();
        settings.ratio = scaled;
        dsp.setCompressor(settings);
        break;
    }
    case MidiTarget::ReverbMix: {
        auto settings = dsp.reverb();
        settings.mix = scaled;
        dsp.setReverb(settings);
        break;
    }
    case MidiTarget::DelayMix: {
        auto settings = dsp.delay();
        settings.mix = scaled;
        dsp.setDelay(settings);
        break;
    }
    }

    return true;
}

} // namespace audio32
