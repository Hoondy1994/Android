#include "gpio_hal_native.h"

namespace hal {

int32_t GpioHalNative::open() {
    open_ = true;
    for (int i = 0; i < kMaxPins; ++i) {
        pins_[i] = {0, false};
        levels_[i] = 0;
    }
    return static_cast<int32_t>(HalError::Ok);
}

int32_t GpioHalNative::close() {
    open_ = false;
    return static_cast<int32_t>(HalError::Ok);
}

int32_t GpioHalNative::configure(int pin, int direction, bool pull_up) {
    if (!open_ || pin < 0 || pin >= kMaxPins) return static_cast<int32_t>(HalError::InvalidPin);
    pins_[pin].direction = direction;
    pins_[pin].pull_up = pull_up;
    if (direction == 0 && pull_up) levels_[pin] = 1;
    return static_cast<int32_t>(HalError::Ok);
}

int32_t GpioHalNative::read_pin(int pin) {
    if (!open_ || pin < 0 || pin >= kMaxPins) return static_cast<int32_t>(HalError::InvalidPin);
    return levels_[pin];
}

int32_t GpioHalNative::write_pin(int pin, int level) {
    if (!open_ || pin < 0 || pin >= kMaxPins) return static_cast<int32_t>(HalError::InvalidPin);
    if (pins_[pin].direction != 1) return static_cast<int32_t>(HalError::InvalidArg);
    levels_[pin] = level & 1;
    return static_cast<int32_t>(HalError::Ok);
}

int32_t GpioHalNative::read_port(int mask) {
    if (!open_) return static_cast<int32_t>(HalError::NotOpen);
    int value = 0;
    for (int pin = 0; pin < kMaxPins; ++pin) {
        if (mask & (1 << pin)) value |= (levels_[pin] << pin);
    }
    return value;
}

}  // namespace hal
