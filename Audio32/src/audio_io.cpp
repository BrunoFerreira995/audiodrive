#include "audio_io.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace audio32 {
namespace {

using Bytes = std::vector<std::uint8_t>;

constexpr std::uint32_t wavPcmFormat = 1;
constexpr std::uint32_t wavFloatFormat = 3;
constexpr std::uint32_t cafLinearPcmFormat = 0x6C70636D;
constexpr std::uint32_t cafFlagIsFloat = 1U << 0U;
constexpr std::uint32_t cafFlagIsBigEndian = 1U << 1U;

std::optional<Bytes> readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }

    return Bytes(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

bool hasBytes(const Bytes& bytes, std::size_t offset, std::size_t count) noexcept {
    return offset <= bytes.size() && count <= bytes.size() - offset;
}

bool matches(const Bytes& bytes, std::size_t offset, std::string_view value) noexcept {
    return hasBytes(bytes, offset, value.size()) &&
           std::equal(value.begin(), value.end(), bytes.begin() + static_cast<std::ptrdiff_t>(offset));
}

std::uint16_t readU16Le(const Bytes& bytes, std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(bytes[offset] | (bytes[offset + 1] << 8U));
}

std::uint32_t readU32Le(const Bytes& bytes, std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

std::uint16_t readU16Be(const Bytes& bytes, std::size_t offset) noexcept {
    return static_cast<std::uint16_t>((bytes[offset] << 8U) | bytes[offset + 1]);
}

std::uint32_t readU32Be(const Bytes& bytes, std::size_t offset) noexcept {
    return (static_cast<std::uint32_t>(bytes[offset]) << 24U) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 8U) |
           static_cast<std::uint32_t>(bytes[offset + 3]);
}

std::uint64_t readU64Be(const Bytes& bytes, std::size_t offset) noexcept {
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        value = (value << 8U) | bytes[offset + i];
    }
    return value;
}

double readFloat64Be(const Bytes& bytes, std::size_t offset) noexcept {
    const auto raw = readU64Be(bytes, offset);
    double value = 0.0;
    static_assert(sizeof(value) == sizeof(raw));
    std::copy_n(reinterpret_cast<const char*>(&raw), sizeof(raw), reinterpret_cast<char*>(&value));
    return value;
}

double readAiffExtendedSampleRate(const Bytes& bytes, std::size_t offset) noexcept {
    const auto exponent = static_cast<int>(readU16Be(bytes, offset) & 0x7FFFU);
    const auto highMantissa = readU32Be(bytes, offset + 2);
    const auto lowMantissa = readU32Be(bytes, offset + 6);
    if (exponent == 0 && highMantissa == 0 && lowMantissa == 0) {
        return 0.0;
    }

    const auto mantissa = static_cast<long double>(highMantissa) * 4294967296.0L + static_cast<long double>(lowMantissa);
    return static_cast<double>(std::ldexp(mantissa, exponent - 16414));
}

std::int32_t signExtend24(std::uint32_t value) noexcept {
    if ((value & 0x00800000U) != 0U) {
        value |= 0xFF000000U;
    }
    return static_cast<std::int32_t>(value);
}

float intToFloat(std::int32_t sample, double scale) noexcept {
    return static_cast<float>(std::clamp(static_cast<double>(sample) / scale, -1.0, 1.0));
}

std::optional<std::vector<float>> decodeInterleavedPcm(
    const Bytes& bytes,
    std::size_t offset,
    std::size_t size,
    std::uint16_t bitsPerSample,
    bool floatingPoint,
    bool bigEndian) {
    if (!hasBytes(bytes, offset, size)) {
        return std::nullopt;
    }

    std::vector<float> samples;
    if (bitsPerSample == 0 || (size * 8U) % bitsPerSample != 0U) {
        return std::nullopt;
    }
    samples.reserve((size * 8U) / bitsPerSample);

    for (std::size_t i = 0; i < size;) {
        if (floatingPoint) {
            if (bitsPerSample == 32 && i + 4 <= size) {
                const auto raw = bigEndian ? readU32Be(bytes, offset + i) : readU32Le(bytes, offset + i);
                float sample = 0.0F;
                std::copy_n(reinterpret_cast<const char*>(&raw), sizeof(raw), reinterpret_cast<char*>(&sample));
                samples.push_back(sample);
                i += 4;
            } else if (bitsPerSample == 64 && i + 8 <= size) {
                std::array<std::uint8_t, 8> raw{};
                if (bigEndian) {
                    std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(offset + i), 8, raw.begin());
                } else {
                    std::reverse_copy(bytes.begin() + static_cast<std::ptrdiff_t>(offset + i),
                        bytes.begin() + static_cast<std::ptrdiff_t>(offset + i + 8), raw.begin());
                }
                const auto integer = (static_cast<std::uint64_t>(raw[0]) << 56U) |
                                     (static_cast<std::uint64_t>(raw[1]) << 48U) |
                                     (static_cast<std::uint64_t>(raw[2]) << 40U) |
                                     (static_cast<std::uint64_t>(raw[3]) << 32U) |
                                     (static_cast<std::uint64_t>(raw[4]) << 24U) |
                                     (static_cast<std::uint64_t>(raw[5]) << 16U) |
                                     (static_cast<std::uint64_t>(raw[6]) << 8U) |
                                     static_cast<std::uint64_t>(raw[7]);
                double sample = 0.0;
                std::copy_n(reinterpret_cast<const char*>(&integer), sizeof(integer), reinterpret_cast<char*>(&sample));
                samples.push_back(static_cast<float>(sample));
                i += 8;
            } else {
                return std::nullopt;
            }
        } else if (bitsPerSample == 16 && i + 2 <= size) {
            const auto raw = bigEndian ? readU16Be(bytes, offset + i) : readU16Le(bytes, offset + i);
            samples.push_back(intToFloat(static_cast<std::int16_t>(raw), 32768.0));
            i += 2;
        } else if (bitsPerSample == 24 && i + 3 <= size) {
            const auto raw = bigEndian
                ? (static_cast<std::uint32_t>(bytes[offset + i]) << 16U) |
                      (static_cast<std::uint32_t>(bytes[offset + i + 1]) << 8U) |
                      static_cast<std::uint32_t>(bytes[offset + i + 2])
                : static_cast<std::uint32_t>(bytes[offset + i]) |
                      (static_cast<std::uint32_t>(bytes[offset + i + 1]) << 8U) |
                      (static_cast<std::uint32_t>(bytes[offset + i + 2]) << 16U);
            samples.push_back(intToFloat(signExtend24(raw), 8388608.0));
            i += 3;
        } else if (bitsPerSample == 32 && i + 4 <= size) {
            const auto raw = bigEndian ? readU32Be(bytes, offset + i) : readU32Le(bytes, offset + i);
            samples.push_back(intToFloat(static_cast<std::int32_t>(raw), 2147483648.0));
            i += 4;
        } else {
            return std::nullopt;
        }
    }

    return samples;
}

std::optional<DecodedAudio> decodeWav(const Bytes& bytes) {
    if (!matches(bytes, 0, "RIFF") || !matches(bytes, 8, "WAVE")) {
        return std::nullopt;
    }

    std::uint16_t format = 0;
    std::uint16_t channels = 0;
    std::uint32_t sampleRate = 0;
    std::uint16_t bitsPerSample = 0;
    std::size_t dataOffset = 0;
    std::size_t dataSize = 0;

    for (std::size_t offset = 12; hasBytes(bytes, offset, 8);) {
        const auto chunkSize = readU32Le(bytes, offset + 4);
        const auto payload = offset + 8;
        if (!hasBytes(bytes, payload, chunkSize)) {
            return std::nullopt;
        }

        if (matches(bytes, offset, "fmt ") && chunkSize >= 16) {
            format = readU16Le(bytes, payload);
            channels = readU16Le(bytes, payload + 2);
            sampleRate = readU32Le(bytes, payload + 4);
            bitsPerSample = readU16Le(bytes, payload + 14);
        } else if (matches(bytes, offset, "data")) {
            dataOffset = payload;
            dataSize = chunkSize;
        }

        offset = payload + chunkSize + (chunkSize % 2U);
    }

    if (channels == 0 || sampleRate == 0 || dataSize == 0 ||
        (format != wavPcmFormat && format != wavFloatFormat)) {
        return std::nullopt;
    }

    auto samples = decodeInterleavedPcm(bytes, dataOffset, dataSize, bitsPerSample, format == wavFloatFormat, false);
    if (!samples) {
        return std::nullopt;
    }

    return DecodedAudio{std::move(*samples), static_cast<double>(sampleRate), channels};
}

std::optional<DecodedAudio> decodeAiff(const Bytes& bytes) {
    if (!matches(bytes, 0, "FORM") || !matches(bytes, 8, "AIFF")) {
        return std::nullopt;
    }

    std::uint16_t channels = 0;
    double sampleRate = 0.0;
    std::uint16_t bitsPerSample = 0;
    std::size_t soundOffset = 0;
    std::size_t soundSize = 0;

    for (std::size_t offset = 12; hasBytes(bytes, offset, 8);) {
        const auto chunkSize = readU32Be(bytes, offset + 4);
        const auto payload = offset + 8;
        if (!hasBytes(bytes, payload, chunkSize)) {
            return std::nullopt;
        }

        if (matches(bytes, offset, "COMM") && chunkSize >= 18) {
            channels = readU16Be(bytes, payload);
            bitsPerSample = readU16Be(bytes, payload + 6);
            sampleRate = readAiffExtendedSampleRate(bytes, payload + 8);
        } else if (matches(bytes, offset, "SSND") && chunkSize >= 8) {
            const auto dataOffset = readU32Be(bytes, payload);
            soundOffset = payload + 8 + dataOffset;
            soundSize = chunkSize - 8 - dataOffset;
        }

        offset = payload + chunkSize + (chunkSize % 2U);
    }

    if (channels == 0 || sampleRate <= 0.0 || soundSize == 0) {
        return std::nullopt;
    }

    auto samples = decodeInterleavedPcm(bytes, soundOffset, soundSize, bitsPerSample, false, true);
    if (!samples) {
        return std::nullopt;
    }

    return DecodedAudio{std::move(*samples), sampleRate, channels};
}

std::optional<DecodedAudio> decodeCaf(const Bytes& bytes) {
    if (!matches(bytes, 0, "caff") || !hasBytes(bytes, 8, 0)) {
        return std::nullopt;
    }

    double sampleRate = 0.0;
    std::uint32_t formatId = 0;
    std::uint32_t formatFlags = 0;
    std::uint32_t channels = 0;
    std::uint32_t bitsPerSample = 0;
    std::size_t dataOffset = 0;
    std::size_t dataSize = 0;

    for (std::size_t offset = 8; hasBytes(bytes, offset, 12);) {
        const auto chunkSize = readU64Be(bytes, offset + 4);
        const auto payload = offset + 12;
        if (chunkSize > std::numeric_limits<std::size_t>::max() || !hasBytes(bytes, payload, static_cast<std::size_t>(chunkSize))) {
            return std::nullopt;
        }

        const auto payloadSize = static_cast<std::size_t>(chunkSize);
        if (matches(bytes, offset, "desc") && payloadSize >= 32) {
            sampleRate = readFloat64Be(bytes, payload);
            formatId = readU32Be(bytes, payload + 8);
            formatFlags = readU32Be(bytes, payload + 12);
            channels = readU32Be(bytes, payload + 24);
            bitsPerSample = readU32Be(bytes, payload + 28);
        } else if (matches(bytes, offset, "data") && payloadSize >= 4) {
            dataOffset = payload + 4;
            dataSize = payloadSize - 4;
        }

        offset = payload + payloadSize;
    }

    if (formatId != cafLinearPcmFormat || channels == 0 || sampleRate <= 0.0 || dataSize == 0) {
        return std::nullopt;
    }

    auto samples = decodeInterleavedPcm(
        bytes,
        dataOffset,
        dataSize,
        static_cast<std::uint16_t>(bitsPerSample),
        (formatFlags & cafFlagIsFloat) != 0U,
        (formatFlags & cafFlagIsBigEndian) != 0U);
    if (!samples) {
        return std::nullopt;
    }

    return DecodedAudio{std::move(*samples), sampleRate, channels};
}

} // namespace

std::vector<float> pcm16ToFloat(std::span<const std::int16_t> samples) {
    std::vector<float> output;
    output.reserve(samples.size());
    for (const auto sample : samples) {
        output.push_back(intToFloat(sample, 32768.0));
    }
    return output;
}

std::vector<float> pcm24LittleEndianToFloat(std::span<const std::uint8_t> bytes) {
    std::vector<float> output;
    output.reserve(bytes.size() / 3);
    for (std::size_t i = 0; i + 2 < bytes.size(); i += 3) {
        const auto raw = static_cast<std::uint32_t>(bytes[i]) |
                         (static_cast<std::uint32_t>(bytes[i + 1]) << 8U) |
                         (static_cast<std::uint32_t>(bytes[i + 2]) << 16U);
        output.push_back(intToFloat(signExtend24(raw), 8388608.0));
    }
    return output;
}

std::vector<float> pcm32ToFloat(std::span<const std::int32_t> samples) {
    std::vector<float> output;
    output.reserve(samples.size());
    for (const auto sample : samples) {
        output.push_back(intToFloat(sample, 2147483648.0));
    }
    return output;
}

std::vector<float> float64ToFloat(std::span<const double> samples) {
    std::vector<float> output;
    output.reserve(samples.size());
    for (const auto sample : samples) {
        output.push_back(static_cast<float>(std::clamp(sample, -1.0, 1.0)));
    }
    return output;
}

std::optional<DecodedAudio> decodeAudioFile(const std::filesystem::path& path) {
    auto bytes = readFile(path);
    if (!bytes || bytes->size() < 12) {
        return std::nullopt;
    }

    if (matches(*bytes, 0, "RIFF")) {
        return decodeWav(*bytes);
    }
    if (matches(*bytes, 0, "FORM")) {
        return decodeAiff(*bytes);
    }
    if (matches(*bytes, 0, "caff")) {
        return decodeCaf(*bytes);
    }

    return std::nullopt;
}

} // namespace audio32
