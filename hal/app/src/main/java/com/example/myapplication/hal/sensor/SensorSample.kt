package com.example.myapplication.hal.sensor

data class SensorSample(
    val temperatureC: Float,
    val humidityPct: Float,
    val pressureHpa: Float,
    val lux: Float,
    val timestampMs: Long = System.currentTimeMillis()
) {
    override fun toString(): String =
        "T=%.1f°C H=%.0f%% P=%.1fhPa Lux=%.0f @%d".format(
            temperatureC, humidityPct, pressureHpa, lux, timestampMs
        )
}
