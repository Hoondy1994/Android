#include "particle_system.h"
#include <algorithm>
namespace android::viewdemo {

void ParticleSystem::configure(int count, float width, float height) {
    width_ = std::max(width, 1.f);
    height_ = std::max(height, 1.f);
    count = std::clamp(count, 1, kMaxParticles);
    particles_.resize(static_cast<std::size_t>(count));

    std::mt19937 rng{42};
    std::uniform_real_distribution<float> posX{0.f, width_};
    std::uniform_real_distribution<float> posY{0.f, height_};
    std::uniform_real_distribution<float> hue{0.f, 360.f};

    for (auto& p : particles_) {
        p.pos = {posX(rng), posY(rng)};
        p.vel = {0.f, 0.f};
        p.radius = 4.f + (hue(rng) / 360.f) * 6.f;
        p.color = ColorRgba::fromHsv(hue(rng), 0.75f, 0.95f, 0.85f);
    }
}

void ParticleSystem::setTouch(Vec2 pos, bool active) noexcept {
    touch_ = pos;
    touchActive_ = active;
}

void ParticleSystem::tick(float dtSec) {
    dtSec = std::clamp(dtSec, 0.001f, 0.05f);

    for (auto& p : particles_) {
        p.vel.y += gravity_ * dtSec;

        if (touchActive_) {
            const Vec2 delta = p.pos - touch_;
            const float dist = std::max(delta.length(), 1.f);
            const float force = 12000.f / (dist * dist);
            p.vel = p.vel + delta.normalized() * (force * dtSec);
        }

        p.pos = p.pos + p.vel * dtSec;
        p.vel = p.vel * damping_;

        if (p.pos.x < p.radius) {
            p.pos.x = p.radius;
            p.vel.x = std::abs(p.vel.x) * 0.6f;
        } else if (p.pos.x > width_ - p.radius) {
            p.pos.x = width_ - p.radius;
            p.vel.x = -std::abs(p.vel.x) * 0.6f;
        }
        if (p.pos.y < p.radius) {
            p.pos.y = p.radius;
            p.vel.y = std::abs(p.vel.y) * 0.6f;
        } else if (p.pos.y > height_ - p.radius) {
            p.pos.y = height_ - p.radius;
            p.vel.y = -std::abs(p.vel.y) * 0.6f;
        }
    }
}

ViewResult<> ParticleSystem::fillBuffer(std::span<float> out) const {
    const auto needed = particles_.size() * static_cast<std::size_t>(kFloatsPerParticle);
    if (out.size() < needed) return std::unexpected(ViewError::BUFFER_TOO_SMALL);

    std::size_t i = 0;
    for (const auto& p : particles_) {
        out[i++] = p.pos.x;
        out[i++] = p.pos.y;
        out[i++] = p.radius;
        out[i++] = p.color.r;
        out[i++] = p.color.g;
        out[i++] = p.color.b;
        out[i++] = p.color.a;
    }
    return {};
}

} // namespace android::viewdemo
