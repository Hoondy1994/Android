#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "gpio_hal_native.h"
#include "power_hal_native.h"
#include "sensor_hal_native.h"
#include "hal_types.h"

namespace hal {

enum class ModuleKind { Sensor, Gpio, Power, Unknown };

struct ModuleHandle {
    ModuleKind kind = ModuleKind::Unknown;
    std::unique_ptr<SensorHalNative> sensor;
    std::unique_ptr<GpioHalNative> gpio;
    std::unique_ptr<PowerHalNative> power;
};

class HalRegistry {
public:
    static HalRegistry& instance();

    int32_t init();
    int32_t shutdown();
    int64_t open_module(const std::string& module_id);
    int32_t close_module(int64_t handle);

    int32_t sensor_read(int64_t handle, SensorReading* out);
    int32_t gpio_configure(int64_t handle, int pin, int direction, bool pull_up);
    int32_t gpio_read_pin(int64_t handle, int pin);
    int32_t gpio_write_pin(int64_t handle, int pin, int level);
    int32_t gpio_read_port(int64_t handle, int mask);
    int32_t power_set_rail(int64_t handle, PowerRail rail, PowerState state);
    int32_t power_get_rail(int64_t handle, PowerRail rail, int32_t* out_state, int32_t* out_mw);
    int32_t power_wake_all(int64_t handle);

private:
    ModuleHandle* find_locked(int64_t handle);
    HalRegistry() = default;
    std::mutex mutex_;
    bool initialized_ = false;
    int64_t next_handle_ = 1;
    std::unordered_map<int64_t, ModuleHandle> modules_;
};

}  // namespace hal
