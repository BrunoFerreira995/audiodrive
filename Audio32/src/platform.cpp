#include "platform.hpp"

#include "simd.hpp"

namespace audio32 {

PlatformCapabilities platformCapabilities() {
    const auto simd = simdCapabilities();
    PlatformCapabilities capabilities;

#ifdef __APPLE__
    capabilities.macOS = true;
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
    capabilities.arm64 = true;
    capabilities.architecture = "arm64";
#elif defined(__x86_64__) || defined(_M_X64)
    capabilities.architecture = "x86_64";
#else
    capabilities.architecture = "unknown";
#endif

#if defined(__APPLE__) && defined(__aarch64__)
    capabilities.appleSilicon = true;
#endif

    capabilities.simdOptimized = simd.armNeon;
    return capabilities;
}

} // namespace audio32
