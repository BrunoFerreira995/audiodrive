#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

namespace audio32 {

struct DecodedAudio {
    std::vector<float> samples;
    double sampleRate = 0.0;
    std::uint32_t channels = 0;
};

std::vector<float> pcm16ToFloat(std::span<const std::int16_t> samples);
std::vector<float> pcm24LittleEndianToFloat(std::span<const std::uint8_t> bytes);
std::vector<float> pcm32ToFloat(std::span<const std::int32_t> samples);
std::vector<float> float64ToFloat(std::span<const double> samples);

std::optional<DecodedAudio> decodeAudioFile(const std::filesystem::path& path);

} // namespace audio32
