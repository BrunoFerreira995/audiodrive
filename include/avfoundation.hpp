#pragma once

#include <string>

namespace audio32 {

struct AVFoundationIntegration {
    bool available = false;
    bool audioEngineSupported = false;
    bool fileDecodingSupported = false;
    std::string backendName;
};

AVFoundationIntegration avFoundationIntegration();

} // namespace audio32
