package com.example.myapplication.hal

enum class HalModuleType(val id: String, val displayName: String) {
    SENSOR("sensor.env", "环境传感器"),
    GPIO("gpio.virtual", "虚拟 GPIO"),
    POWER("power.mgmt", "电源管理");

    companion object {
        fun fromId(id: String): HalModuleType? = entries.find { it.id == id }
    }
}

enum class HalBackend(val id: String) {
    MOCK("mock"),
    NATIVE("native");

    companion object {
        fun fromId(id: String): HalBackend? = entries.find { it.id == id }
    }
}
