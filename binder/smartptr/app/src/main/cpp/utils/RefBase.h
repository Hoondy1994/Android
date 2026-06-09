#pragma once

#include <atomic>
#include <cstdint>

namespace android {

// 与 AOSP RefBase 类似的引用计数基类（教学简化版）
class RefBase {
public:
    void incStrong(const void* id) const;
    void decStrong(const void* id) const;

    void incWeak(const void* id) const;
    void decWeak(const void* id) const;

    // 尝试从弱引用提升为强引用；对象已销毁时返回 false
    bool attemptIncStrong(const void* id) const;

    int32_t getStrongCount() const { return mStrong.load(std::memory_order_relaxed); }
    int32_t getWeakCount() const { return mWeak.load(std::memory_order_relaxed); }

protected:
    RefBase();
    virtual ~RefBase();

    // 派生类可覆盖：最后一处强引用释放时的回调
    virtual void onFirstRef();
    virtual void onLastStrongRef(const void* id);
    virtual bool onIncStrongAttempted(uint32_t flags, const void* id);

private:
    enum {
        INITIAL_STRONG_VALUE = 1 << 28,
    };

    mutable std::atomic<int32_t> mStrong{INITIAL_STRONG_VALUE};
    mutable std::atomic<int32_t> mWeak{0};
    std::atomic<uint32_t> mFlags{0};

    RefBase(const RefBase&) = delete;
    RefBase& operator=(const RefBase&) = delete;
};

}  // namespace android
