#pragma once

#include <string>

namespace audio32 {

struct PlatformCapabilities {
    bool macOS = false;
    bool arm64 = false;
    bool appleSilicon = false;
    bool simdOptimized = false;
    std::string architecture;
};

PlatformCapabilities platformCapabilities();

} // namespace audio32
