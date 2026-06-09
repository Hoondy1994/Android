package com.example.myapplication.hal.impl.native

import com.example.myapplication.hal.HalEvent
import com.example.myapplication.hal.HalEventBus
import com.example.myapplication.hal.HalModuleType
import com.example.myapplication.hal.core.HalCapability
import com.example.myapplication.hal.core.HalDeviceState
import com.example.myapplication.hal.core.HalResult
import com.example.myapplication.hal.impl.BaseHalModule
import com.example.myapplication.hal.native.NativeHalBridge
import com.example.myapplication.hal.power.IPowerHal
import com.example.myapplication.hal.power.PowerRail
import com.example.myapplication.hal.power.PowerRailStatus
import com.example.myapplication.hal.power.PowerState

class NativePowerHal(eventBus: HalEventBus) : BaseHalModule(
    HalModuleType.POWER,
    eventBus,
    setOf(HalCapability.POWER_MANAGEMENT, HalCapability.READ, HalCapability.WRITE)
), IPowerHal {

    override val version = "native-power-cpp"
    private var handle: Long = 0L

    override fun open(): HalResult<Unit> {
        transition(HalDeviceState.OPENING)
        handle = NativeHalBridge.nativeOpenModule(moduleId)
        if (handle == 0L) {
            state = HalDeviceState.ERROR
            return HalResult.Err(-100, "Native open power 失败")
        }
        return transition(HalDeviceState.OPEN)
    }

    override fun close(): HalResult<Unit> {
        if (handle != 0L) {
            NativeHalBridge.nativeCloseModule(handle)
            handle = 0L
        }
        return transition(HalDeviceState.CLOSED)
    }

    override fun getRailStatus(rail: PowerRail): HalResult<PowerRailStatus> {
        if (handle == 0L) return HalResult.Err(2, "未打开")
        val arr = NativeHalBridge.nativePowerGetRail(handle, rail.ordinal)
            ?: return HalResult.Err(3, "nativePowerGetRail 失败")
        val state = PowerState.entries.getOrElse(arr[0]) { PowerState.OFF }
        return HalResult.Ok(PowerRailStatus(rail, state, arr[1]))
    }

    override fun setRailState(rail: PowerRail, state: PowerState): HalResult<Unit> {
        if (handle == 0L) return HalResult.Err(2, "未打开")
        val rc = NativeHalBridge.nativePowerSetRail(handle, rail.ordinal, state.ordinal)
        if (rc == 0) eventBus.publish(HalEvent.PowerTransition(rail.name, state.name))
        return if (rc == 0) HalResult.Ok(Unit) else HalResult.Err(rc, "setRailState 失败")
    }

    override fun wakeAll(): HalResult<Unit> {
        if (handle == 0L) return HalResult.Err(2, "未打开")
        val rc = NativeHalBridge.nativePowerWakeAll(handle)
        if (rc == 0) transition(HalDeviceState.OPEN)
        return if (rc == 0) HalResult.Ok(Unit) else HalResult.Err(rc, "wakeAll 失败")
    }

    override fun suspendSystem(): HalResult<Unit> {
        PowerRail.entries.forEach { setRailState(it, PowerState.SUSPEND) }
        transition(HalDeviceState.SUSPENDED)
        return HalResult.Ok(Unit)
    }

    override fun dumpState(): Map<String, String> {
        val lines = PowerRail.entries.mapNotNull { rail ->
            getRailStatus(rail).getOrNull()?.let { "${rail.name}=${it.state}/${it.milliwatts}mW" }
        }
        return mapOf("backend" to "native", "state" to state.name, "rails" to lines.joinToString())
    }
}
