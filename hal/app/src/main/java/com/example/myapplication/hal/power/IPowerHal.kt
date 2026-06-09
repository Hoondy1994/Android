package com.example.myapplication.hal.power

import com.example.myapplication.hal.core.HalResult
import com.example.myapplication.hal.core.IHalModule

enum class PowerRail { CPU, GPU, PERIPHERAL, DISPLAY }
enum class PowerState { FULL_ON, LOW_POWER, SUSPEND, OFF }

data class PowerRailStatus(val rail: PowerRail, val state: PowerState, val milliwatts: Int)

interface IPowerHal : IHalModule {
    fun getRailStatus(rail: PowerRail): HalResult<PowerRailStatus>
    fun setRailState(rail: PowerRail, state: PowerState): HalResult<Unit>
    fun wakeAll(): HalResult<Unit>
    fun suspendSystem(): HalResult<Unit>
}
