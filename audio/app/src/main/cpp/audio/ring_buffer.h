#pragma once
#include <array>
#include <atomic>
#include <span>
#include <algorithm>
#include <concepts>

namespace android::audio {

// 无锁 SPSC 环形缓冲区，Capacity 必须是2的幂
// Producer: AAudio 回调线程写入
// Consumer: 文件写入 jthread 读取
template<std::semiregular T, std::size_t Capacity>
    requires (Capacity > 0 && (Capacity & (Capacity-1)) == 0)
class RingBuffer {
    static constexpr std::size_t kMask = Capacity - 1;
    alignas(64) std::array<T, Capacity>  buf_{};
    alignas(64) std::atomic<std::size_t> writePos_{0};
    alignas(64) std::atomic<std::size_t> readPos_ {0};
public:
    [[nodiscard]] std::size_t write(std::span<const T> src) noexcept {
        std::size_t wp    = writePos_.load(std::memory_order_relaxed);
        std::size_t rp    = readPos_ .load(std::memory_order_acquire);
        std::size_t avail = Capacity - (wp - rp);
        std::size_t n     = std::min(src.size(), avail);
        if(!n) return 0;
        std::size_t idx   = wp & kMask;
        std::size_t first = std::min(n, Capacity - idx);
        std::copy_n(src.begin(),         first,     buf_.begin() + idx);
        std::copy_n(src.begin() + first, n - first, buf_.begin());
        writePos_.store(wp + n, std::memory_order_release);
        return n;
    }

    [[nodiscard]] std::size_t read(std::span<T> dst) noexcept {
        std::size_t rp    = readPos_ .load(std::memory_order_relaxed);
        std::size_t wp    = writePos_.load(std::memory_order_acquire);
        std::size_t avail = wp - rp;
        std::size_t n     = std::min(dst.size(), avail);
        if(!n) return 0;
        std::size_t idx   = rp & kMask;
        std::size_t first = std::min(n, Capacity - idx);
        std::copy_n(buf_.begin() + idx, first,     dst.begin());
        std::copy_n(buf_.begin(),       n - first, dst.begin() + first);
        readPos_.store(rp + n, std::memory_order_release);
        return n;
    }

    [[nodiscard]] std::size_t available() const noexcept {
        return writePos_.load(std::memory_order_acquire)
             - readPos_ .load(std::memory_order_acquire);
    }
    [[nodiscard]] bool empty() const noexcept { return available() == 0; }
    void reset() noexcept {
        writePos_.store(0, std::memory_order_relaxed);
        readPos_ .store(0, std::memory_order_relaxed);
    }
};

} // namespace android::audio
