#pragma once

namespace audio32 {

struct WirelessAudioCapabilities {
    bool bluetoothOutputAvailable = false;
    bool spatialAudioAvailable = false;
    bool headTrackedSpatialAudioAvailable = false;
};

WirelessAudioCapabilities wirelessAudioCapabilities();

} // namespace audio32
