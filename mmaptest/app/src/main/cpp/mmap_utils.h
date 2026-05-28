#pragma once

#include <android/log.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <unistd.h>

#define MMAP_LOG_TAG "MmapDemo"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, MMAP_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, MMAP_LOG_TAG, __VA_ARGS__)

inline std::string errnoStr() {
    return std::string(strerror(errno));
}

inline size_t pageSize() {
    static const size_t ps = static_cast<size_t>(sysconf(_SC_PAGESIZE));
    return ps;
}

inline size_t alignUp(size_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

inline std::string hexDump(const void* data, size_t len, size_t maxBytes = 64) {
    auto* bytes = static_cast<const uint8_t*>(data);
    const size_t n = len < maxBytes ? len : maxBytes;
    std::ostringstream oss;
    for (size_t i = 0; i < n; ++i) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02x ", bytes[i]);
        oss << buf;
    }
    if (len > maxBytes) {
        oss << "...";
    }
    return oss.str();
}
