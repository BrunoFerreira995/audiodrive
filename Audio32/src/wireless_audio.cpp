#include "wireless_audio.hpp"

namespace audio32 {

WirelessAudioCapabilities wirelessAudioCapabilities() {
#ifdef __APPLE__
    return WirelessAudioCapabilities{
        true,
        true,
        true
    };
#else
    return WirelessAudioCapabilities{};
#endif
}

} // namespace audio32
