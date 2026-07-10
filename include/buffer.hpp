#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace audio32 {

class AudioBuffer {
public:
    explicit AudioBuffer(std::size_t capacityBytes = 1024 * 1024);

    std::size_t capacityFrames(std::uint32_t channels) const noexcept;
    std::size_t availableRead() const noexcept;
    std::size_t availableWrite() const noexcept;
    void clear() noexcept;

    std::size_t write(std::span<const float> frames) noexcept;
    std::size_t read(std::span<float> frames) noexcept;

private:
    std::vector<float> buffer_;
    const std::size_t capacity_;
    std::atomic<std::size_t> readIndex_{0};
    std::atomic<std::size_t> writeIndex_{0};
};

} // namespace audio32
