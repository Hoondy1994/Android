package com.example.myapplication.hal.impl.mock

import com.example.myapplication.hal.HalEvent
import com.example.myapplication.hal.HalEventBus
import com.example.myapplication.hal.HalModuleType
import com.example.myapplication.hal.core.HalCapability
import com.example.myapplication.hal.core.HalDeviceState
import com.example.myapplication.hal.core.HalResult
import com.example.myapplication.hal.impl.BaseHalModule
import com.example.myapplication.hal.core.onSuccess
import com.example.myapplication.hal.sensor.ISensorHal
import com.example.myapplication.hal.sensor.SensorSample
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlin.math.sin
import kotlin.random.Random

class MockSensorHal(
    eventBus: HalEventBus,
    private val scope: CoroutineScope
) : BaseHalModule(
    HalModuleType.SENSOR,
    eventBus,
    setOf(HalCapability.READ, HalCapability.STREAM, HalCapability.CALIBRATION)
), ISensorHal {

    override val version = "mock-sensor-1.2.0"
    private var offsetTemp = 0f
    private var offsetHumidity = 0f
    private var streamJob: Job? = null
    private var tick = 0

    override fun open(): HalResult<Unit> {
        val opening = transition(HalDeviceState.OPENING)
        if (opening is HalResult.Err) return opening
        tick = 0
        return transition(HalDeviceState.OPEN)
    }

    override fun close(): HalResult<Unit> {
        stopStream()
        return transition(HalDeviceState.CLOSED)
    }

    override fun readSample(): HalResult<SensorSample> {
        if (state != HalDeviceState.OPEN && state != HalDeviceState.STREAMING) {
            return HalResult.Err(2, "传感器未打开")
        }
        tick++
        val t = 22f + offsetTemp + (sin(tick * 0.15) * 3f).toFloat() + Random.nextFloat()
        val h = 55f + offsetHumidity + (sin(tick * 0.1) * 8f).toFloat()
        val sample = SensorSample(t, h, 1013.2f + Random.nextFloat() * 2f, 300f + tick * 5f)
        eventBus.publish(HalEvent.SensorSampleReceived(sample))
        return HalResult.Ok(sample)
    }

    override fun startStream(intervalMs: Int, onSample: (SensorSample) -> Unit): HalResult<Unit> {
        if (state != HalDeviceState.OPEN) return HalResult.Err(3, "需要先 open")
        stopStream()
        transition(HalDeviceState.STREAMING)
        streamJob = scope.launch {
            while (isActive) {
                readSample().onSuccess(onSample)
                delay(intervalMs.toLong())
            }
        }
        return HalResult.Ok(Unit)
    }

    override fun stopStream(): HalResult<Unit> {
        streamJob?.cancel()
        streamJob = null
        if (state == HalDeviceState.STREAMING) {
            transition(HalDeviceState.OPEN)
        }
        return HalResult.Ok(Unit)
    }

    override fun calibrate(offsetTemp: Float, offsetHumidity: Float): HalResult<Unit> {
        this.offsetTemp = offsetTemp
        this.offsetHumidity = offsetHumidity
        eventBus.log("Mock 传感器校准: T=$offsetTemp H=$offsetHumidity")
        return HalResult.Ok(Unit)
    }

    override fun dumpState(): Map<String, String> = mapOf(
        "backend" to "mock",
        "state" to state.name,
        "offsetTemp" to offsetTemp.toString(),
        "streaming" to (streamJob?.isActive == true).toString()
    )
}
