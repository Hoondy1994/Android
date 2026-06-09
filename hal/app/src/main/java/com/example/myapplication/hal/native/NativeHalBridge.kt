package com.example.myapplication.hal.native

import com.example.myapplication.hal.sensor.SensorSample

object NativeHalBridge {

    @Volatile
    private var libraryLoaded = false

    private fun ensureLibraryLoaded() {
        if (!libraryLoaded) {
            synchronized(this) {
                if (!libraryLoaded) {
                    System.loadLibrary("myapplication")
                    libraryLoaded = true
                }
            }
        }
    }

    fun nativeInit(): Int {
        ensureLibraryLoaded()
        return nativeInitInternal()
    }

    fun nativeShutdown(): Int {
        ensureLibraryLoaded()
        return nativeShutdownInternal()
    }

    fun nativeGetVersion(): String {
        ensureLibraryLoaded()
        return nativeGetVersionInternal()
    }

    fun nativeOpenModule(moduleId: String): Long {
        ensureLibraryLoaded()
        return nativeOpenModuleInternal(moduleId)
    }

    fun nativeCloseModule(handle: Long): Int {
        ensureLibraryLoaded()
        return nativeCloseModuleInternal(handle)
    }

    fun nativeSensorRead(handle: Long): FloatArray? {
        ensureLibraryLoaded()
        return nativeSensorReadInternal(handle)
    }

    fun nativeGpioConfigure(handle: Long, pin: Int, direction: Int, pullUp: Boolean): Int {
        ensureLibraryLoaded()
        return nativeGpioConfigureInternal(handle, pin, direction, pullUp)
    }

    fun nativeGpioRead(handle: Long, pin: Int): Int {
        ensureLibraryLoaded()
        return nativeGpioReadInternal(handle, pin)
    }

    fun nativeGpioWrite(handle: Long, pin: Int, level: Int): Int {
        ensureLibraryLoaded()
        return nativeGpioWriteInternal(handle, pin, level)
    }

    fun nativeGpioReadPort(handle: Long, mask: Int): Int {
        ensureLibraryLoaded()
        return nativeGpioReadPortInternal(handle, mask)
    }

    fun nativePowerSetRail(handle: Long, rail: Int, state: Int): Int {
        ensureLibraryLoaded()
        return nativePowerSetRailInternal(handle, rail, state)
    }

    fun nativePowerGetRail(handle: Long, rail: Int): IntArray? {
        ensureLibraryLoaded()
        return nativePowerGetRailInternal(handle, rail)
    }

    fun nativePowerWakeAll(handle: Long): Int {
        ensureLibraryLoaded()
        return nativePowerWakeAllInternal(handle)
    }

    private external fun nativeInitInternal(): Int
    private external fun nativeShutdownInternal(): Int
    private external fun nativeGetVersionInternal(): String
    private external fun nativeOpenModuleInternal(moduleId: String): Long
    private external fun nativeCloseModuleInternal(handle: Long): Int
    private external fun nativeSensorReadInternal(handle: Long): FloatArray?
    private external fun nativeGpioConfigureInternal(
        handle: Long,
        pin: Int,
        direction: Int,
        pullUp: Boolean
    ): Int

    private external fun nativeGpioReadInternal(handle: Long, pin: Int): Int
    private external fun nativeGpioWriteInternal(handle: Long, pin: Int, level: Int): Int
    private external fun nativeGpioReadPortInternal(handle: Long, mask: Int): Int
    private external fun nativePowerSetRailInternal(handle: Long, rail: Int, state: Int): Int
    private external fun nativePowerGetRailInternal(handle: Long, rail: Int): IntArray?
    private external fun nativePowerWakeAllInternal(handle: Long): Int

    fun readSensor(handle: Long): SensorSample? {
        val arr = nativeSensorRead(handle) ?: return null
        if (arr.size < 4) return null
        return SensorSample(
            temperatureC = arr[0],
            humidityPct = arr[1],
            pressureHpa = arr[2],
            lux = arr[3]
        )
    }
}
