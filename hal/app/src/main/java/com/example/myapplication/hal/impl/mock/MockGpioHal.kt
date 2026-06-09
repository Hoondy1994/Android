package com.example.myapplication.hal.impl.mock

import com.example.myapplication.hal.HalEvent
import com.example.myapplication.hal.HalEventBus
import com.example.myapplication.hal.HalModuleType
import com.example.myapplication.hal.core.HalCapability
import com.example.myapplication.hal.core.HalDeviceState
import com.example.myapplication.hal.core.HalResult
import com.example.myapplication.hal.gpio.GpioDirection
import com.example.myapplication.hal.gpio.IGpioHal
import com.example.myapplication.hal.impl.BaseHalModule

class MockGpioHal(eventBus: HalEventBus) : BaseHalModule(
    HalModuleType.GPIO,
    eventBus,
    setOf(HalCapability.READ, HalCapability.WRITE, HalCapability.INTERRUPT)
), IGpioHal {

    override val version = "mock-gpio-1.0.0"
    private val pinConfig = mutableMapOf<Int, Pair<GpioDirection, Boolean>>()
    private val pinLevels = mutableMapOf<Int, Int>()

    override fun open(): HalResult<Unit> {
        transition(HalDeviceState.OPENING)
        pinLevels.clear()
        (0..15).forEach { pinLevels[it] = 0 }
        return transition(HalDeviceState.OPEN)
    }

    override fun close(): HalResult<Unit> = transition(HalDeviceState.CLOSED)

    override fun configurePin(pin: Int, direction: GpioDirection, pullUp: Boolean): HalResult<Unit> {
        if (pin !in 0..15) return HalResult.Err(10, "引脚 $pin 无效")
        pinConfig[pin] = direction to pullUp
        if (direction == GpioDirection.INPUT && pullUp) pinLevels[pin] = 1
        eventBus.log("GPIO$pin -> $direction pullUp=$pullUp")
        return HalResult.Ok(Unit)
    }

    override fun readPin(pin: Int): HalResult<Int> {
        if (state != HalDeviceState.OPEN) return HalResult.Err(2, "GPIO 未打开")
        val level = pinLevels[pin] ?: return HalResult.Err(11, "引脚未配置")
        return HalResult.Ok(level)
    }

    override fun writePin(pin: Int, level: Int): HalResult<Unit> {
        val dir = pinConfig[pin]?.first
        if (dir != GpioDirection.OUTPUT) return HalResult.Err(12, "引脚 $pin 非输出模式")
        pinLevels[pin] = level.coerceIn(0, 1)
        eventBus.publish(HalEvent.GpioChanged(pin, pinLevels[pin]!!))
        return HalResult.Ok(Unit)
    }

    override fun readPort(mask: Int): HalResult<Int> {
        var value = 0
        for (pin in 0..15) {
            if (mask and (1 shl pin) != 0) {
                value = value or ((pinLevels[pin] ?: 0) shl pin)
            }
        }
        return HalResult.Ok(value)
    }

    override fun writePort(mask: Int, value: Int): HalResult<Unit> {
        for (pin in 0..15) {
            if (mask and (1 shl pin) != 0) {
                writePin(pin, (value shr pin) and 1)
            }
        }
        return HalResult.Ok(Unit)
    }

    override fun dumpState(): Map<String, String> = buildMap {
        put("backend", "mock")
        put("state", state.name)
        put("port", pinLevels.entries.sortedBy { it.key }.joinToString { "${it.key}=${it.value}" })
    }
}
