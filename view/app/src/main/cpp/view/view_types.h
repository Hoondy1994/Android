#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>

namespace android::viewdemo {

struct Vec2 {
    float x = 0.f;
    float y = 0.f;

    [[nodiscard]] constexpr Vec2 operator+(Vec2 o) const noexcept { return {x + o.x, y + o.y}; }
    [[nodiscard]] constexpr Vec2 operator-(Vec2 o) const noexcept { return {x - o.x, y - o.y}; }
    [[nodiscard]] constexpr Vec2 operator*(float s) const noexcept { return {x * s, y * s}; }

    [[nodiscard]] float length() const noexcept { return std::hypot(x, y); }
    [[nodiscard]] Vec2 normalized() const noexcept {
        const float len = length();
        return len > 1e-6f ? Vec2{x / len, y / len} : Vec2{};
    }
};

struct ColorRgba {
    float r = 1.f, g = 1.f, b = 1.f, a = 1.f;

    [[nodiscard]] static constexpr ColorRgba fromHsv(float h, float s, float v, float a = 1.f) noexcept {
        const float c = v * s;
        const float x = c * (1.f - std::fabs(std::fmod(h / 60.f, 2.f) - 1.f));
        const float m = v - c;
        float rp = 0.f, gp = 0.f, bp = 0.f;
        if (h < 60.f) {
            rp = c;
            gp = x;
        } else if (h < 120.f) {
            rp = x;
            gp = c;
        } else if (h < 180.f) {
            gp = c;
            bp = x;
        } else if (h < 240.f) {
            gp = x;
            bp = c;
        } else if (h < 300.f) {
            rp = x;
            bp = c;
        } else {
            rp = c;
            bp = x;
        }
        return {rp + m, gp + m, bp + m, a};
    }
};

enum class ViewError : int32_t {
    OK = 0,
    NOT_INIT = -1,
    BAD_SIZE = -2,
    BUFFER_TOO_SMALL = -3,
};

[[nodiscard]] constexpr std::string_view to_string(ViewError e) noexcept {
    switch (e) {
        case ViewError::OK: return "OK";
        case ViewError::NOT_INIT: return "NOT_INIT";
        case ViewError::BAD_SIZE: return "BAD_SIZE";
        case ViewError::BUFFER_TOO_SMALL: return "BUFFER_TOO_SMALL";
        default: return "UNKNOWN";
    }
}

template<typename T = void>
using ViewResult = std::expected<T, ViewError>;

inline constexpr int kFloatsPerParticle = 7; // x, y, radius, r, g, b, a
inline constexpr int kMaxParticles = 256;
inline constexpr int kMaxWaveSamples = 512;

} // namespace android::viewdemo
