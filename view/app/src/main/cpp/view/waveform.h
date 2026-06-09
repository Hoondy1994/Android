#pragma once

#include "view_types.h"
#include <span>

namespace android::viewdemo {

class WaveformGenerator {
public:
    void setHarmonics(int count) noexcept { harmonics_ = std::clamp(count, 1, 8); }
    void setAmplitude(float a) noexcept { amplitude_ = std::clamp(a, 0.1f, 1.f); }
    ViewResult<> generate(std::span<float> out, float phaseRad) const;

private:
    int harmonics_ = 4;
    float amplitude_ = 0.85f;
};

} // namespace android::viewdemo
