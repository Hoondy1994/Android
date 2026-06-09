#include "RefBase.h"

#include <android/log.h>

#define LOG_TAG "RefBase"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace android {

RefBase::RefBase() = default;

RefBase::~RefBase() {
    const int32_t s = mStrong.load(std::memory_order_relaxed);
    const int32_t w = mWeak.load(std::memory_order_relaxed);
    if ((s & 0xffff) != 0 || (w & 0xffff) != 0) {
        ALOGI("~RefBase: leaking refs strong=%d weak=%d", s, w);
    }
}

void RefBase::onFirstRef() {}
void RefBase::onLastStrongRef(const void* /*id*/) {}
bool RefBase::onIncStrongAttempted(uint32_t /*flags*/, const void* /*id*/) {
    return true;
}

void RefBase::incStrong(const void* id) const {
    const int32_t old = mStrong.fetch_add(1, std::memory_order_relaxed);
    if (old == INITIAL_STRONG_VALUE) {
        const_cast<RefBase*>(this)->onFirstRef();
    }
}

void RefBase::decStrong(const void* id) const {
    const int32_t old = mStrong.fetch_sub(1, std::memory_order_release);
    if (old == 1 || old == INITIAL_STRONG_VALUE + 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        const_cast<RefBase*>(this)->onLastStrongRef(id);
        const int32_t weak = mWeak.fetch_sub(INITIAL_STRONG_VALUE, std::memory_order_release);
        if (weak == INITIAL_STRONG_VALUE) {
            delete this;
        }
    }
}

void RefBase::incWeak(const void* id) const {
    mWeak.fetch_add(1, std::memory_order_relaxed);
}

void RefBase::decWeak(const void* id) const {
    const int32_t old = mWeak.fetch_sub(1, std::memory_order_release);
    if (old == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete this;
    }
}

bool RefBase::attemptIncStrong(const void* id) const {
    int32_t oldStrong = mStrong.load(std::memory_order_relaxed);
    for (;;) {
        if (oldStrong == 0) {
            return false;
        }
        if (oldStrong == INITIAL_STRONG_VALUE) {
            if (!const_cast<RefBase*>(this)->onIncStrongAttempted(0, id)) {
                return false;
            }
        }
        const int32_t desired = oldStrong + 1;
        if (mStrong.compare_exchange_weak(oldStrong, desired,
                                          std::memory_order_relaxed)) {
            if (oldStrong == INITIAL_STRONG_VALUE) {
                const_cast<RefBase*>(this)->onFirstRef();
            }
            return true;
        }
    }
}

}  // namespace android
