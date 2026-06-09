package com.example.myapplication.hal.gpio

import com.example.myapplication.hal.core.HalResult
import com.example.myapplication.hal.core.IHalModule

enum class GpioDirection { INPUT, OUTPUT }

interface IGpioHal : IHalModule {
    fun configurePin(pin: Int, direction: GpioDirection, pullUp: Boolean): HalResult<Unit>
    fun readPin(pin: Int): HalResult<Int>
    fun writePin(pin: Int, level: Int): HalResult<Unit>
    fun readPort(mask: Int): HalResult<Int>
    fun writePort(mask: Int, value: Int): HalResult<Unit>
}
