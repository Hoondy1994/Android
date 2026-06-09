#pragma once

#include "view_types.h"
#include <random>
#include <vector>

namespace android::viewdemo {

struct Particle {
    Vec2 pos{};
    Vec2 vel{};
    float radius = 6.f;
    ColorRgba color{};
};

class ParticleSystem {
public:
    void configure(int count, float width, float height);
    void setTouch(Vec2 pos, bool active) noexcept;
    void setGravity(float g) noexcept { gravity_ = g; }
    void setDamping(float d) noexcept { damping_ = std::clamp(d, 0.8f, 0.999f); }
    void tick(float dtSec);
    [[nodiscard]] std::span<const Particle> particles() const noexcept {
        return particles_;
    }
    ViewResult<> fillBuffer(std::span<float> out) const;

private:
    std::vector<Particle> particles_;
    float width_ = 1.f;
    float height_ = 1.f;
    float gravity_ = 180.f;
    float damping_ = 0.992f;
    Vec2 touch_{};
    bool touchActive_ = false;
};

} // namespace android::viewdemo
