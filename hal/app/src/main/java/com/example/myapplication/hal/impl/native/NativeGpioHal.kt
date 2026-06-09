package com.example.myapplication.hal.impl.native

import com.example.myapplication.hal.HalEvent
import com.example.myapplication.hal.HalEventBus
import com.example.myapplication.hal.HalModuleType
import com.example.myapplication.hal.core.HalCapability
import com.example.myapplication.hal.core.HalDeviceState
import com.example.myapplication.hal.core.HalResult
import com.example.myapplication.hal.gpio.GpioDirection
import com.example.myapplication.hal.gpio.IGpioHal
import com.example.myapplication.hal.impl.BaseHalModule
import com.example.myapplication.hal.native.NativeHalBridge

class NativeGpioHal(eventBus: HalEventBus) : BaseHalModule(
    HalModuleType.GPIO,
    eventBus,
    setOf(HalCapability.READ, HalCapability.WRITE)
), IGpioHal {

    override val version = "native-gpio-cpp"
    private var handle: Long = 0L

    override fun open(): HalResult<Unit> {
        transition(HalDeviceState.OPENING)
        handle = NativeHalBridge.nativeOpenModule(moduleId)
        if (handle == 0L) {
            state = HalDeviceState.ERROR
            return HalResult.Err(-100, "Native open gpio 失败")
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

    override fun configurePin(pin: Int, direction: GpioDirection, pullUp: Boolean): HalResult<Unit> {
        if (handle == 0L) return HalResult.Err(2, "未打开")
        val dir = if (direction == GpioDirection.OUTPUT) 1 else 0
        val rc = NativeHalBridge.nativeGpioConfigure(handle, pin, dir, pullUp)
        return if (rc == 0) HalResult.Ok(Unit) else HalResult.Err(rc, "nativeGpioConfigure 失败")
    }

    override fun readPin(pin: Int): HalResult<Int> {
        if (handle == 0L) return HalResult.Err(2, "未打开")
        val level = NativeHalBridge.nativeGpioRead(handle, pin)
        return if (level < 0) HalResult.Err(level, "readPin 失败") else HalResult.Ok(level)
    }

    override fun writePin(pin: Int, level: Int): HalResult<Unit> {
        if (handle == 0L) return HalResult.Err(2, "未打开")
        val rc = NativeHalBridge.nativeGpioWrite(handle, pin, level)
        if (rc == 0) eventBus.publish(HalEvent.GpioChanged(pin, level))
        return if (rc == 0) HalResult.Ok(Unit) else HalResult.Err(rc, "writePin 失败")
    }

    override fun readPort(mask: Int): HalResult<Int> {
        if (handle == 0L) return HalResult.Err(2, "未打开")
        val value = NativeHalBridge.nativeGpioReadPort(handle, mask)
        return if (value < 0) HalResult.Err(value, "readPort 失败") else HalResult.Ok(value)
    }

    override fun writePort(mask: Int, value: Int): HalResult<Unit> {
        for (pin in 0..15) {
            if (mask and (1 shl pin) != 0) {
                writePin(pin, (value shr pin) and 1)
            }
        }
        return HalResult.Ok(Unit)
    }

    override fun dumpState(): Map<String, String> = mapOf(
        "backend" to "native",
        "handle" to handle.toString(),
        "state" to state.name
    )
}
