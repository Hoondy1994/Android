#pragma once

#include "hal_module.h"
#include "hal_types.h"

namespace hal {

class GpioHalNative : public IHalModule {
public:
    int32_t open() override;
    int32_t close() override;
    const char* module_id() const override { return "gpio.virtual"; }
    int32_t configure(int pin, int direction, bool pull_up);
    int32_t read_pin(int pin);
    int32_t write_pin(int pin, int level);
    int32_t read_port(int mask);

private:
    bool open_ = false;
    GpioPinConfig pins_[kMaxPins]{};
    int levels_[kMaxPins]{};
};

}  // namespace hal
