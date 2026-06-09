package com.example.myapplication.hal.impl.native

import com.example.myapplication.hal.HalEvent
import com.example.myapplication.hal.HalEventBus
import com.example.myapplication.hal.HalModuleType
import com.example.myapplication.hal.core.HalCapability
import com.example.myapplication.hal.core.HalDeviceState
import com.example.myapplication.hal.core.HalResult
import com.example.myapplication.hal.impl.BaseHalModule
import com.example.myapplication.hal.native.NativeHalBridge
import com.example.myapplication.hal.core.onSuccess
import com.example.myapplication.hal.sensor.ISensorHal
import com.example.myapplication.hal.sensor.SensorSample
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch

class NativeSensorHal(
    eventBus: HalEventBus,
    private val scope: CoroutineScope
) : BaseHalModule(
    HalModuleType.SENSOR,
    eventBus,
    setOf(HalCapability.READ, HalCapability.STREAM, HalCapability.CALIBRATION)
), ISensorHal {

    override val version = "native-sensor-cpp"
    private var handle: Long = 0L
    private var streamJob: Job? = null
    private var calTemp = 0f
    private var calHumidity = 0f

    override fun open(): HalResult<Unit> {
        transition(HalDeviceState.OPENING)
        handle = NativeHalBridge.nativeOpenModule(moduleId)
        if (handle == 0L) {
            state = HalDeviceState.ERROR
            return HalResult.Err(-100, "Native open sensor 失败")
        }
        return transition(HalDeviceState.OPEN)
    }

    override fun close(): HalResult<Unit> {
        stopStream()
        if (handle != 0L) {
            NativeHalBridge.nativeCloseModule(handle)
            handle = 0L
        }
        return transition(HalDeviceState.CLOSED)
    }

    override fun readSample(): HalResult<SensorSample> {
        if (handle == 0L) return HalResult.Err(2, "Native handle 无效")
        val raw = NativeHalBridge.readSensor(handle)
            ?: return HalResult.Err(3, "nativeSensorRead 失败")
        val sample = raw.copy(
            temperatureC = raw.temperatureC + calTemp,
            humidityPct = raw.humidityPct + calHumidity
        )
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
        if (state == HalDeviceState.STREAMING) transition(HalDeviceState.OPEN)
        return HalResult.Ok(Unit)
    }

    override fun calibrate(offsetTemp: Float, offsetHumidity: Float): HalResult<Unit> {
        calTemp = offsetTemp
        calHumidity = offsetHumidity
        return HalResult.Ok(Unit)
    }

    override fun dumpState(): Map<String, String> = mapOf(
        "backend" to "native",
        "handle" to handle.toString(),
        "state" to state.name
    )
}
