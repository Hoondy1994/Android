#include "view_engine.h"
#include <sstream>

namespace android::viewdemo {

ViewEngine::ViewEngine(ViewEngineConfig cfg) : cfg_(cfg) {
    particles_.setGravity(cfg_.gravity);
    waveform_.setHarmonics(cfg_.harmonics);
    layout_.setMode(cfg_.layoutMode);
}

void ViewEngine::resize(float width, float height) {
    width_ = std::max(width, 1.f);
    height_ = std::max(height, 1.f);
    particles_.configure(cfg_.particleCount, width_, height_);
}

void ViewEngine::setTouch(float x, float y, bool active) {
    particles_.setTouch({x, y}, active);
}

void ViewEngine::tick(float dtSec) {
    particles_.tick(dtSec);
    frameCount_.fetch_add(1, std::memory_order_relaxed);
}

void ViewEngine::setParticleCount(int count) {
    cfg_.particleCount = std::clamp(count, 1, kMaxParticles);
    particles_.configure(cfg_.particleCount, width_, height_);
}

void ViewEngine::setHarmonics(int h) {
    cfg_.harmonics = h;
    waveform_.setHarmonics(h);
}

void ViewEngine::setGravity(float g) {
    cfg_.gravity = g;
    particles_.setGravity(g);
}

void ViewEngine::setLayoutMode(LayoutMode mode) {
    cfg_.layoutMode = mode;
    layout_.setMode(mode);
}

void ViewEngine::advancePhase(float deltaRad) {
    phaseRad_ += deltaRad;
    orbitRotationRad_ += deltaRad * 0.35f;
}

ViewResult<> ViewEngine::copyParticles(std::span<float> out) const {
    return particles_.fillBuffer(out);
}

ViewResult<> ViewEngine::copyWaveform(std::span<float> out) const {
    return waveform_.generate(out, phaseRad_);
}

ViewResult<> ViewEngine::copyOrbitLayout(std::span<float> out) const {
    return layout_.solve(out, cfg_.orbitItemCount, orbitRotationRad_);
}

std::string ViewEngine::statsJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"particles\":" << cfg_.particleCount
        << ",\"waveSamples\":" << cfg_.waveformSamples
        << ",\"orbitItems\":" << cfg_.orbitItemCount
        << ",\"frames\":" << frameCount_.load(std::memory_order_relaxed)
        << ",\"phase\":" << phaseRad_
        << "}";
    return oss.str();
}

} // namespace android::viewdemo
