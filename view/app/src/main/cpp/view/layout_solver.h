#pragma once

#include "view_types.h"
#include <span>

namespace android::viewdemo {

enum class LayoutMode : uint8_t { Circle, Spiral, Heart };

class LayoutSolver {
public:
    void setMode(LayoutMode mode) noexcept { mode_ = mode; }
    ViewResult<> solve(std::span<float> out, int itemCount, float rotationRad) const;

private:
    LayoutMode mode_ = LayoutMode::Circle;
};

} // namespace android::viewdemo
