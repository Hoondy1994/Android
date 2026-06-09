#include "hal_registry.h"

#include <android/log.h>

#define LOG_TAG "HalRegistry"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace hal {

HalRegistry& HalRegistry::instance() {
    static HalRegistry reg;
    return reg;
}

ModuleHandle* HalRegistry::find_locked(int64_t handle) {
    auto it = modules_.find(handle);
    if (it == modules_.end()) return nullptr;
    return &it->second;
}

int32_t HalRegistry::init() {
    std::lock_guard<std::mutex> lock(mutex_);
    initialized_ = true;
    LOGI("HAL registry initialized");
    return static_cast<int32_t>(HalError::Ok);
}

int32_t HalRegistry::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& pair : modules_) {
        auto& entry = pair.second;
        switch (entry.kind) {
            case ModuleKind::Sensor:
                if (entry.sensor) entry.sensor->close();
                break;
            case ModuleKind::Gpio:
                if (entry.gpio) entry.gpio->close();
                break;
            case ModuleKind::Power:
                if (entry.power) entry.power->close();
                break;
            default:
                break;
        }
    }
    modules_.clear();
    initialized_ = false;
    LOGI("HAL registry shutdown");
    return static_cast<int32_t>(HalError::Ok);
}

int64_t HalRegistry::open_module(const std::string& module_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) return 0;

    ModuleHandle entry;
    if (module_id == "sensor.env") {
        entry.kind = ModuleKind::Sensor;
        entry.sensor = std::make_unique<SensorHalNative>();
        if (entry.sensor->open() != 0) return 0;
    } else if (module_id == "gpio.virtual") {
        entry.kind = ModuleKind::Gpio;
        entry.gpio = std::make_unique<GpioHalNative>();
        if (entry.gpio->open() != 0) return 0;
    } else if (module_id == "power.mgmt") {
        entry.kind = ModuleKind::Power;
        entry.power = std::make_unique<PowerHalNative>();
        if (entry.power->open() != 0) return 0;
    } else {
        return 0;
    }

    const int64_t handle = next_handle_++;
    modules_[handle] = std::move(entry);
    return handle;
}

int32_t HalRegistry::close_module(int64_t handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = modules_.find(handle);
    if (it == modules_.end()) return static_cast<int32_t>(HalError::NotFound);
    switch (it->second.kind) {
        case ModuleKind::Sensor:
            if (it->second.sensor) it->second.sensor->close();
            break;
        case ModuleKind::Gpio:
            if (it->second.gpio) it->second.gpio->close();
            break;
        case ModuleKind::Power:
            if (it->second.power) it->second.power->close();
            break;
        default:
            break;
    }
    modules_.erase(it);
    return static_cast<int32_t>(HalError::Ok);
}

int32_t HalRegistry::sensor_read(int64_t handle, SensorReading* out) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto* mod = find_locked(handle);
    if (mod == nullptr || mod->kind != ModuleKind::Sensor || !mod->sensor) {
        return static_cast<int32_t>(HalError::NotOpen);
    }
    *out = mod->sensor->read();
    return static_cast<int32_t>(HalError::Ok);
}

int32_t HalRegistry::gpio_configure(int64_t handle, int pin, int direction, bool pull_up) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto* mod = find_locked(handle);
    if (mod == nullptr || mod->kind != ModuleKind::Gpio || !mod->gpio) {
        return static_cast<int32_t>(HalError::NotOpen);
    }
    return mod->gpio->configure(pin, direction, pull_up);
}

int32_t HalRegistry::gpio_read_pin(int64_t handle, int pin) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto* mod = find_locked(handle);
    if (mod == nullptr || mod->kind != ModuleKind::Gpio || !mod->gpio) {
        return static_cast<int32_t>(HalError::NotOpen);
    }
    return mod->gpio->read_pin(pin);
}

int32_t HalRegistry::gpio_write_pin(int64_t handle, int pin, int level) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto* mod = find_locked(handle);
    if (mod == nullptr || mod->kind != ModuleKind::Gpio || !mod->gpio) {
        return static_cast<int32_t>(HalError::NotOpen);
    }
    return mod->gpio->write_pin(pin, level);
}

int32_t HalRegistry::gpio_read_port(int64_t handle, int mask) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto* mod = find_locked(handle);
    if (mod == nullptr || mod->kind != ModuleKind::Gpio || !mod->gpio) {
        return static_cast<int32_t>(HalError::NotOpen);
    }
    return mod->gpio->read_port(mask);
}

int32_t HalRegistry::power_set_rail(int64_t handle, PowerRail rail, PowerState state) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto* mod = find_locked(handle);
    if (mod == nullptr || mod->kind != ModuleKind::Power || !mod->power) {
        return static_cast<int32_t>(HalError::NotOpen);
    }
    return mod->power->set_rail(rail, state);
}

int32_t HalRegistry::power_get_rail(int64_t handle, PowerRail rail, int32_t* out_state,
                                  int32_t* out_mw) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto* mod = find_locked(handle);
    if (mod == nullptr || mod->kind != ModuleKind::Power || !mod->power) {
        return static_cast<int32_t>(HalError::NotOpen);
    }
    mod->power->get_rail(rail, out_state, out_mw);
    return static_cast<int32_t>(HalError::Ok);
}

int32_t HalRegistry::power_wake_all(int64_t handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto* mod = find_locked(handle);
    if (mod == nullptr || mod->kind != ModuleKind::Power || !mod->power) {
        return static_cast<int32_t>(HalError::NotOpen);
    }
    return mod->power->wake_all();
}

}  // namespace hal
