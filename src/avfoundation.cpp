#include "avfoundation.hpp"

namespace audio32 {

AVFoundationIntegration avFoundationIntegration() {
#ifdef __APPLE__
    return AVFoundationIntegration{
        true,
        true,
        true,
        "AVFoundation"
    };
#else
    return AVFoundationIntegration{
        false,
        false,
        false,
        {}
    };
#endif
}

} // namespace audio32
