#pragma once

#include "RefBase.h"
#include "StrongPointer.h"

namespace android {

// wp<T>：弱智能指针，对应 AOSP utils/WeakPointer.h
template <typename T>
class wp {
public:
    wp() : m_ptr(nullptr) {}
    wp(T* other);
    wp(const wp<T>& other);
    wp(const sp<T>& other);
    ~wp();

    wp& operator=(T* other);
    wp& operator=(const wp<T>& other);
    wp& operator=(const sp<T>& other);

    sp<T> promote() const;
    T* unsafe_get() const { return m_ptr; }

    void clear();
    bool operator!() const { return m_ptr == nullptr; }
    explicit operator bool() const { return m_ptr != nullptr; }

private:
    T* m_ptr;
};

template <typename T>
wp<T>::wp(T* other) : m_ptr(other) {
    if (other) other->incWeak(this);
}

template <typename T>
wp<T>::wp(const wp<T>& other) : m_ptr(other.m_ptr) {
    if (m_ptr) m_ptr->incWeak(this);
}

template <typename T>
wp<T>::wp(const sp<T>& other) : m_ptr(other.m_ptr) {
    if (m_ptr) m_ptr->incWeak(this);
}

template <typename T>
wp<T>::~wp() {
    if (m_ptr) m_ptr->decWeak(this);
}

template <typename T>
wp<T>& wp<T>::operator=(T* other) {
    if (other) other->incWeak(this);
    if (m_ptr) m_ptr->decWeak(this);
    m_ptr = other;
    return *this;
}

template <typename T>
wp<T>& wp<T>::operator=(const wp<T>& other) {
    return operator=(other.m_ptr);
}

template <typename T>
wp<T>& wp<T>::operator=(const sp<T>& other) {
    return operator=(other.m_ptr);
}

template <typename T>
sp<T> wp<T>::promote() const {
    sp<T> result;
    if (m_ptr && m_ptr->attemptIncStrong(this)) {
        result.m_ptr = m_ptr;
    }
    return result;
}

template <typename T>
void wp<T>::clear() {
    if (m_ptr) {
        m_ptr->decWeak(this);
        m_ptr = nullptr;
    }
}

}  // namespace android
