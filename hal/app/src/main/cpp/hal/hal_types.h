#pragma once

#include <cstdint>

namespace hal {

constexpr int kMaxPins = 16;
constexpr int kMaxModules = 8;

enum class HalError : int32_t {
    Ok = 0,
    NotOpen = 2,
    InvalidPin = 10,
    InvalidArg = 11,
    NotFound = 20,
    NativeFail = 100,
};

enum class PowerRail : int32_t { Cpu = 0, Gpu = 1, Peripheral = 2, Display = 3 };
enum class PowerState : int32_t { FullOn = 0, LowPower = 1, Suspend = 2, Off = 3 };

struct SensorReading {
    float temperature_c;
    float humidity_pct;
    float pressure_hpa;
    float lux;
};

struct GpioPinConfig {
    int direction;  // 0=input 1=output
    bool pull_up;
};

}  // namespace hal
