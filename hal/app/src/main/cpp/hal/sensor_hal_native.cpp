#include "sensor_hal_native.h"

#include <cmath>
#include <chrono>

namespace hal {

int32_t SensorHalNative::open() {
    open_ = true;
    sample_count_ = 0;
    return static_cast<int32_t>(HalError::Ok);
}

int32_t SensorHalNative::close() {
    open_ = false;
    return static_cast<int32_t>(HalError::Ok);
}

SensorReading SensorHalNative::read() {
    sample_count_++;
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    const double phase = static_cast<double>(sample_count_) * 0.12;
    const double noise = static_cast<double>((now / 97) % 100) / 50.0;

    SensorReading r{};
    r.temperature_c = static_cast<float>(24.0 + std::sin(phase) * 4.0 + noise * 0.3);
    r.humidity_pct = static_cast<float>(50.0 + std::cos(phase * 0.8) * 12.0);
    r.pressure_hpa = 1012.5f + static_cast<float>(std::sin(phase * 0.3) * 1.5);
    r.lux = static_cast<float>(280.0 + sample_count_ * 3.5 + noise * 10.0);
    return r;
}

}  // namespace hal
