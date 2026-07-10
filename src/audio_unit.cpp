#include "audio_unit.hpp"

#include <sstream>

namespace audio32 {

bool isValidAudioUnitDescriptor(const AudioUnitPluginDescriptor& descriptor) noexcept {
    return !descriptor.name.empty() &&
           !descriptor.manufacturer.empty() &&
           !descriptor.bundleIdentifier.empty() &&
           descriptor.componentType != 0 &&
           descriptor.componentSubtype != 0 &&
           descriptor.manufacturerCode != 0 &&
           descriptor.version != 0;
}

std::string makeAudioUnitComponentIdentifier(const AudioUnitPluginDescriptor& descriptor) {
    std::ostringstream stream;
    stream << descriptor.bundleIdentifier << ":"
           << descriptor.componentType << ":"
           << descriptor.componentSubtype << ":"
           << descriptor.manufacturerCode;
    return stream.str();
}

} // namespace audio32
