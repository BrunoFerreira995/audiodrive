#pragma once

#include <cstdint>
#include <string>

namespace audio32 {

struct AudioUnitPluginDescriptor {
    std::string name = "Audio32";
    std::string manufacturer = "Audio32";
    std::string bundleIdentifier = "com.audio32.driver";
    std::uint32_t componentType = 0x61756678; // aufx
    std::uint32_t componentSubtype = 0x61333220; // a32
    std::uint32_t manufacturerCode = 0x41333220; // A32
    std::uint32_t version = 0x00010000;
};

bool isValidAudioUnitDescriptor(const AudioUnitPluginDescriptor& descriptor) noexcept;
std::string makeAudioUnitComponentIdentifier(const AudioUnitPluginDescriptor& descriptor);

} // namespace audio32
