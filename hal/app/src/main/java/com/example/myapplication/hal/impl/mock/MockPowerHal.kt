package com.example.myapplication.hal.impl.mock

import com.example.myapplication.hal.HalEvent
import com.example.myapplication.hal.HalEventBus
import com.example.myapplication.hal.HalModuleType
import com.example.myapplication.hal.core.HalCapability
import com.example.myapplication.hal.core.HalDeviceState
import com.example.myapplication.hal.core.HalResult
import com.example.myapplication.hal.impl.BaseHalModule
import com.example.myapplication.hal.power.IPowerHal
import com.example.myapplication.hal.power.PowerRail
import com.example.myapplication.hal.power.PowerRailStatus
import com.example.myapplication.hal.power.PowerState

class MockPowerHal(eventBus: HalEventBus) : BaseHalModule(
    HalModuleType.POWER,
    eventBus,
    setOf(HalCapability.POWER_MANAGEMENT, HalCapability.READ, HalCapability.WRITE)
), IPowerHal {

    override val version = "mock-power-1.1.0"
    private val rails = mutableMapOf<PowerRail, PowerRailStatus>()

    override fun open(): HalResult<Unit> {
        transition(HalDeviceState.OPENING)
        PowerRail.entries.forEach { rail ->
            rails[rail] = PowerRailStatus(rail, PowerState.FULL_ON, defaultMw(rail))
        }
        return transition(HalDeviceState.OPEN)
    }

    override fun close(): HalResult<Unit> = transition(HalDeviceState.CLOSED)

    override fun getRailStatus(rail: PowerRail): HalResult<PowerRailStatus> {
        val status = rails[rail] ?: return HalResult.Err(20, "未知电源轨")
        return HalResult.Ok(status)
    }

    override fun setRailState(rail: PowerRail, state: PowerState): HalResult<Unit> {
        if (this.state == HalDeviceState.SUSPENDED && state == PowerState.FULL_ON) {
            transition(HalDeviceState.OPEN)
        }
        val mw = when (state) {
            PowerState.FULL_ON -> defaultMw(rail)
            PowerState.LOW_POWER -> defaultMw(rail) / 4
            PowerState.SUSPEND, PowerState.OFF -> 0
        }
        rails[rail] = PowerRailStatus(rail, state, mw)
        eventBus.publish(HalEvent.PowerTransition(rail.name, state.name))
        return HalResult.Ok(Unit)
    }

    override fun wakeAll(): HalResult<Unit> {
        PowerRail.entries.forEach { setRailState(it, PowerState.FULL_ON) }
        transition(HalDeviceState.OPEN)
        eventBus.log("Mock: 全部电源轨唤醒")
        return HalResult.Ok(Unit)
    }

    override fun suspendSystem(): HalResult<Unit> {
        PowerRail.entries.forEach { setRailState(it, PowerState.SUSPEND) }
        transition(HalDeviceState.SUSPENDED)
        eventBus.log("Mock: 系统进入 SUSPEND")
        return HalResult.Ok(Unit)
    }

    override fun dumpState(): Map<String, String> = buildMap {
        put("backend", "mock")
        put("state", state.name)
        rails.forEach { (rail, s) -> put(rail.name, "${s.state}/${s.milliwatts}mW") }
    }

    private fun defaultMw(rail: PowerRail) = when (rail) {
        PowerRail.CPU -> 2500
        PowerRail.GPU -> 1800
        PowerRail.PERIPHERAL -> 400
        PowerRail.DISPLAY -> 1200
    }
}
