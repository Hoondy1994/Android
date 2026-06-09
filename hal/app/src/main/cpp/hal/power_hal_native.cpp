#include "power_hal_native.h"

namespace hal {

int32_t PowerHalNative::default_mw(PowerRail rail) {
    switch (rail) {
        case PowerRail::Cpu: return 2800;
        case PowerRail::Gpu: return 2000;
        case PowerRail::Peripheral: return 450;
        case PowerRail::Display: return 1500;
        default: return 0;
    }
}

int32_t PowerHalNative::open() {
    open_ = true;
    for (int i = 0; i < 4; ++i) {
        rails_[i].state = PowerState::FullOn;
        rails_[i].milliwatts = default_mw(static_cast<PowerRail>(i));
    }
    return static_cast<int32_t>(HalError::Ok);
}

int32_t PowerHalNative::close() {
    open_ = false;
    return static_cast<int32_t>(HalError::Ok);
}

int32_t PowerHalNative::set_rail(PowerRail rail, PowerState state) {
    if (!open_) return static_cast<int32_t>(HalError::NotOpen);
    const int idx = static_cast<int>(rail);
    if (idx < 0 || idx >= 4) return static_cast<int32_t>(HalError::InvalidArg);
    rails_[idx].state = state;
    switch (state) {
        case PowerState::FullOn:
            rails_[idx].milliwatts = default_mw(rail);
            break;
        case PowerState::LowPower:
            rails_[idx].milliwatts = default_mw(rail) / 4;
            break;
        case PowerState::Suspend:
        case PowerState::Off:
            rails_[idx].milliwatts = 0;
            break;
    }
    return static_cast<int32_t>(HalError::Ok);
}

void PowerHalNative::get_rail(PowerRail rail, int32_t* out_state, int32_t* out_mw) {
    const int idx = static_cast<int>(rail);
    if (idx < 0 || idx >= 4) {
        *out_state = static_cast<int32_t>(PowerState::Off);
        *out_mw = 0;
        return;
    }
    *out_state = static_cast<int32_t>(rails_[idx].state);
    *out_mw = rails_[idx].milliwatts;
}

int32_t PowerHalNative::wake_all() {
    if (!open_) return static_cast<int32_t>(HalError::NotOpen);
    for (int i = 0; i < 4; ++i) {
        set_rail(static_cast<PowerRail>(i), PowerState::FullOn);
    }
    return static_cast<int32_t>(HalError::Ok);
}

}  // namespace hal
