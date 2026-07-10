#include "buffer.hpp"

#include <algorithm>

namespace audio32 {

AudioBuffer::AudioBuffer(std::size_t capacityBytes)
    : buffer_(std::max<std::size_t>(capacityBytes / sizeof(float), 2U), 0.0F),
      capacity_(buffer_.size()) {}

std::size_t AudioBuffer::capacityFrames(std::uint32_t channels) const noexcept {
    return channels == 0 ? 0 : capacity_ / channels;
}

std::size_t AudioBuffer::availableRead() const noexcept {
    const auto write = writeIndex_.load(std::memory_order_acquire);
    const auto read = readIndex_.load(std::memory_order_acquire);
    return write - read;
}

std::size_t AudioBuffer::availableWrite() const noexcept {
    return capacity_ - availableRead();
}

void AudioBuffer::clear() noexcept {
    readIndex_.store(0, std::memory_order_release);
    writeIndex_.store(0, std::memory_order_release);
}

std::size_t AudioBuffer::write(std::span<const float> frames) noexcept {
    const auto writable = std::min(frames.size(), availableWrite());
    auto write = writeIndex_.load(std::memory_order_relaxed);

    for (std::size_t i = 0; i < writable; ++i) {
        buffer_[(write + i) % capacity_] = frames[i];
    }

    writeIndex_.store(write + writable, std::memory_order_release);
    return writable;
}

std::size_t AudioBuffer::read(std::span<float> frames) noexcept {
    const auto readable = std::min(frames.size(), availableRead());
    auto read = readIndex_.load(std::memory_order_relaxed);

    for (std::size_t i = 0; i < readable; ++i) {
        frames[i] = buffer_[(read + i) % capacity_];
    }

    if (readable < frames.size()) {
        std::fill(frames.begin() + static_cast<std::ptrdiff_t>(readable), frames.end(), 0.0F);
    }

    readIndex_.store(read + readable, std::memory_order_release);
    return readable;
}

} // namespace audio32
