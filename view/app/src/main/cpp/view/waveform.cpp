#include "waveform.h"
#include <cmath>
#include <numbers>

namespace android::viewdemo {

ViewResult<> WaveformGenerator::generate(std::span<float> out, float phaseRad) const {
    if (out.empty()) return std::unexpected(ViewError::BAD_SIZE);
    if (out.size() > static_cast<std::size_t>(kMaxWaveSamples)) {
        return std::unexpected(ViewError::BAD_SIZE);
    }

    const auto n = out.size();
    for (std::size_t i = 0; i < n; ++i) {
        const float t = n > 1 ? static_cast<float>(i) / static_cast<float>(n - 1) : 0.f;
        float sum = 0.f;
        for (int h = 1; h <= harmonics_; ++h) {
            const float freq = static_cast<float>(h) * 2.f * std::numbers::pi_v<float>;
            sum += std::sin(freq * t + phaseRad + static_cast<float>(h) * 0.35f) / static_cast<float>(h);
        }
        const float envelope = 0.5f + 0.5f * std::sin(t * std::numbers::pi_v<float>);
        const float y = 0.5f + amplitude_ * 0.25f * sum * envelope;
        out[i] = std::clamp(y, 0.f, 1.f);
    }
    return {};
}

} // namespace android::viewdemo
