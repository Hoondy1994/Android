package com.example.myapplication.hal

import com.example.myapplication.hal.sensor.SensorSample

sealed class HalEvent {
    data class ModuleOpened(val type: HalModuleType, val backend: HalBackend) : HalEvent()
    data class ModuleClosed(val type: HalModuleType) : HalEvent()
    data class StateChanged(val type: HalModuleType, val from: String, val to: String) : HalEvent()
    data class SensorSampleReceived(val sample: SensorSample) : HalEvent()
    data class GpioChanged(val pin: Int, val level: Int) : HalEvent()
    data class PowerTransition(val rail: String, val state: String) : HalEvent()
    data class Log(val level: String, val message: String) : HalEvent()
    data class Error(val source: String, val message: String) : HalEvent()
}
