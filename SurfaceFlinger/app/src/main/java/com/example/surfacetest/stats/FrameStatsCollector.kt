package com.example.surfacetest.stats

import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.Choreographer
import android.view.FrameMetrics
import android.view.Window
import kotlin.math.roundToInt

/**
 * 汇总 Choreographer 帧间隔与 [Window.OnFrameAvailableListener] 的 SF 合成指标。
 */
class FrameStatsCollector(
    private val window: Window,
) {
    private val mainHandler = Handler(Looper.getMainLooper())
    private val choreographer = Choreographer.getInstance()
    private var metricsRegistered = false
    private var lastFrameNs = 0L
    private var frameCount = 0
    private var intervalSumMs = 0.0
    private var jankCount = 0

    var displayFps = 0f
        private set
    var avgIntervalMs = 0.0
        private set
    var jankFrames = 0
        private set
    var lastPresentMs = 0.0
        private set
    var lastGpuMs = 0.0
        private set
    var lastTotalMs = 0.0
        private set

    private val frameCallback = object : Choreographer.FrameCallback {
        override fun doFrame(frameTimeNs: Long) {
            if (lastFrameNs > 0L) {
                val dtMs = (frameTimeNs - lastFrameNs) / 1_000_000.0
                intervalSumMs += dtMs
                frameCount++
                if (dtMs > 20.0) jankCount++
                if (frameCount >= 30) {
                    avgIntervalMs = intervalSumMs / frameCount
                    displayFps = if (avgIntervalMs > 0) (1000.0 / avgIntervalMs).toFloat() else 0f
                    jankFrames = jankCount
                    frameCount = 0
                    intervalSumMs = 0.0
                    jankCount = 0
                }
            }
            lastFrameNs = frameTimeNs
            choreographer.postFrameCallback(this)
        }
    }

    private val metricsListener = Window.OnFrameMetricsAvailableListener { _, frameMetrics, _ ->
        lastGpuMs = nsToMs(
            frameMetrics.getMetric(FrameMetrics.GPU_DURATION) +
                frameMetrics.getMetric(FrameMetrics.SYNC_DURATION),
        )
        lastPresentMs = nsToMs(frameMetrics.getMetric(FrameMetrics.SWAP_BUFFERS_DURATION))
        lastTotalMs = nsToMs(frameMetrics.getMetric(FrameMetrics.TOTAL_DURATION))
    }

    fun start() {
        choreographer.postFrameCallback(frameCallback)
        if (metricsRegistered) return
        try {
            // MIUI 等设备不接受 null Handler，必须显式指定主线程 Looper。
            window.addOnFrameMetricsAvailableListener(metricsListener, mainHandler)
            metricsRegistered = true
        } catch (e: Exception) {
            Log.w(TAG, "FrameMetrics unavailable on this device", e)
        }
    }

    fun stop() {
        choreographer.removeFrameCallback(frameCallback)
        if (!metricsRegistered) return
        try {
            window.removeOnFrameMetricsAvailableListener(metricsListener)
        } catch (e: Exception) {
            Log.w(TAG, "removeOnFrameMetricsAvailableListener failed", e)
        } finally {
            metricsRegistered = false
        }
    }

    companion object {
        private const val TAG = "FrameStatsCollector"
    }

    fun snapshotLine(): String {
        return buildString {
            append("UI≈")
            append(displayFps.roundToInt())
            append("fps  Δ=")
            append(String.format("%.1f", avgIntervalMs))
            append("ms  jank=")
            append(jankFrames)
            append("  SF total=")
            append(String.format("%.1f", lastTotalMs))
            append("ms gpu=")
            append(String.format("%.1f", lastGpuMs))
            append("ms")
        }
    }

    private fun nsToMs(ns: Long): Double = ns / 1_000_000.0
}
