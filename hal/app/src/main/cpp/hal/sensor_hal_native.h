#pragma once

#include "hal_module.h"
#include "hal_types.h"

namespace hal {

class SensorHalNative : public IHalModule {
public:
    int32_t open() override;
    int32_t close() override;
    const char* module_id() const override { return "sensor.env"; }
    SensorReading read();

private:
    bool open_ = false;
    int64_t sample_count_ = 0;
};

}  // namespace hal
