package com.example.myapplication.hal.sensor

import com.example.myapplication.hal.core.HalResult
import com.example.myapplication.hal.core.IHalModule

interface ISensorHal : IHalModule {
    fun readSample(): HalResult<SensorSample>
    fun startStream(intervalMs: Int, onSample: (SensorSample) -> Unit): HalResult<Unit>
    fun stopStream(): HalResult<Unit>
    fun calibrate(offsetTemp: Float, offsetHumidity: Float): HalResult<Unit>
}
