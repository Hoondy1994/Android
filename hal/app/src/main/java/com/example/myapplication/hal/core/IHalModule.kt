package com.example.myapplication.hal.core

interface IHalModule {
    val moduleId: String
    val version: String
    val capabilities: Set<HalCapability>
    var state: HalDeviceState

    fun open(): HalResult<Unit>
    fun close(): HalResult<Unit>
    fun reset(): HalResult<Unit>
    fun dumpState(): Map<String, String>
}
