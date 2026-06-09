#pragma once

#include "layout_solver.h"
#include "particle_system.h"
#include "view_types.h"
#include "waveform.h"
#include <atomic>
#include <string>

namespace android::viewdemo {

struct ViewEngineConfig {
    int particleCount = 80;
    int waveformSamples = 128;
    int orbitItemCount = 12;
    int harmonics = 4;
    float gravity = 180.f;
    LayoutMode layoutMode = LayoutMode::Circle;
};

class ViewEngine {
public:
    explicit ViewEngine(ViewEngineConfig cfg = {});

    void resize(float width, float height);
    void setTouch(float x, float y, bool active);
    void tick(float dtSec);
    void setParticleCount(int count);
    void setHarmonics(int h);
    void setGravity(float g);
    void setLayoutMode(LayoutMode mode);
    void advancePhase(float deltaRad);

    [[nodiscard]] ViewResult<> copyParticles(std::span<float> out) const;
    [[nodiscard]] ViewResult<> copyWaveform(std::span<float> out) const;
    [[nodiscard]] ViewResult<> copyOrbitLayout(std::span<float> out) const;
    [[nodiscard]] std::string statsJson() const;

private:
    ViewEngineConfig cfg_;
    ParticleSystem particles_;
    WaveformGenerator waveform_;
    LayoutSolver layout_;
    float width_ = 1.f;
    float height_ = 1.f;
    float phaseRad_ = 0.f;
    float orbitRotationRad_ = 0.f;
    std::atomic<uint64_t> frameCount_{0};
};

} // namespace android::viewdemo
