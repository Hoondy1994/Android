#pragma once

#include "RefBase.h"

namespace android {

// sp<T>：强智能指针，对应 AOSP utils/StrongPointer.h
template <typename T>
class sp {
public:
    sp() : m_ptr(nullptr) {}
    sp(T* other);
    sp(const sp<T>& other);
    sp(sp<T>&& other) noexcept;
    ~sp();

    sp& operator=(T* other);
    sp& operator=(const sp<T>& other);
    sp& operator=(sp<T>&& other) noexcept;

    T& operator*() const { return *m_ptr; }
    T* operator->() const { return m_ptr; }
    T* get() const { return m_ptr; }

    void clear();
    bool operator!() const { return m_ptr == nullptr; }
    explicit operator bool() const { return m_ptr != nullptr; }

private:
    template <typename U>
    friend class sp;
    template <typename U>
    friend class wp;

    void setPointer(T* ptr);
    T* m_ptr;
};

template <typename T>
sp<T>::sp(T* other) : m_ptr(other) {
    if (other) other->incStrong(this);
}

template <typename T>
sp<T>::sp(const sp<T>& other) : m_ptr(other.m_ptr) {
    if (m_ptr) m_ptr->incStrong(this);
}

template <typename T>
sp<T>::sp(sp<T>&& other) noexcept : m_ptr(other.m_ptr) {
    other.m_ptr = nullptr;
}

template <typename T>
sp<T>::~sp() {
    if (m_ptr) m_ptr->decStrong(this);
}

template <typename T>
sp<T>& sp<T>::operator=(T* other) {
    if (other) other->incStrong(this);
    if (m_ptr) m_ptr->decStrong(this);
    m_ptr = other;
    return *this;
}

template <typename T>
sp<T>& sp<T>::operator=(const sp<T>& other) {
    return operator=(other.m_ptr);
}

template <typename T>
sp<T>& sp<T>::operator=(sp<T>&& other) noexcept {
    if (m_ptr) m_ptr->decStrong(this);
    m_ptr = other.m_ptr;
    other.m_ptr = nullptr;
    return *this;
}

template <typename T>
void sp<T>::clear() {
    if (m_ptr) {
        m_ptr->decStrong(this);
        m_ptr = nullptr;
    }
}

}  // namespace android
