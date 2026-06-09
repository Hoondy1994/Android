#include "layout_solver.h"
#include <cmath>
#include <numbers>

namespace android::viewdemo {

ViewResult<> LayoutSolver::solve(std::span<float> out, int itemCount, float rotationRad) const {
    if (itemCount <= 0) return std::unexpected(ViewError::BAD_SIZE);
    const auto needed = static_cast<std::size_t>(itemCount) * 2u;
    if (out.size() < needed) return std::unexpected(ViewError::BUFFER_TOO_SMALL);

    const float cx = 0.5f;
    const float cy = 0.5f;

    for (int i = 0; i < itemCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(itemCount);
        const float angle = rotationRad + t * 2.f * std::numbers::pi_v<float>;

        float nx = 0.f;
        float ny = 0.f;
        switch (mode_) {
            case LayoutMode::Circle: {
                const float r = 0.38f;
                nx = cx + r * std::cos(angle);
                ny = cy + r * std::sin(angle);
                break;
            }
            case LayoutMode::Spiral: {
                const float r = 0.08f + 0.32f * t;
                nx = cx + r * std::cos(angle * 2.2f);
                ny = cy + r * std::sin(angle * 2.2f);
                break;
            }
            case LayoutMode::Heart: {
                const float a = angle;
                const float hx = 0.32f * std::pow(std::sin(a), 3.f);
                const float hy = 0.28f * (std::cos(a) - 0.5f * std::cos(2.f * a) - 0.2f * std::cos(3.f * a));
                nx = cx + hx;
                ny = cy - hy;
                break;
            }
        }
        const std::size_t base = static_cast<std::size_t>(i) * 2u;
        out[base] = std::clamp(nx, 0.f, 1.f);
        out[base + 1] = std::clamp(ny, 0.f, 1.f);
    }
    return {};
}

} // namespace android::viewdemo
