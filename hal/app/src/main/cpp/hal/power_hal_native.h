#pragma once

#include "hal_module.h"
#include "hal_types.h"

namespace hal {

struct RailState {
    PowerState state = PowerState::FullOn;
    int32_t milliwatts = 0;
};

class PowerHalNative : public IHalModule {
public:
    int32_t open() override;
    int32_t close() override;
    const char* module_id() const override { return "power.mgmt"; }
    int32_t set_rail(PowerRail rail, PowerState state);
    void get_rail(PowerRail rail, int32_t* out_state, int32_t* out_mw);
    int32_t wake_all();

private:
    bool open_ = false;
    RailState rails_[4]{};
    static int32_t default_mw(PowerRail rail);
};

}  // namespace hal
