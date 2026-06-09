package com.example.surfacetest.view

import android.content.Context
import android.graphics.PixelFormat
import android.util.AttributeSet
import android.view.Choreographer
import android.view.SurfaceHolder
import android.view.SurfaceView
import com.example.surfacetest.native.SurfaceNativeBridge
import kotlin.math.min

/**
 * 底层 Surface：通过 EGL swapBuffers 向 BufferQueue 提交帧，由 SurfaceFlinger 合成。
 */
class NativeGlesSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
) : SurfaceView(context, attrs), SurfaceHolder.Callback {

    var stressLevel = 4
    var renderMode = SurfaceNativeBridge.MODE_PLASMA
    var renderingEnabled = true

    private val renderThread = android.os.HandlerThread("GlesProducer").apply { start() }
    private val renderHandler = android.os.Handler(renderThread.looper)
    private var attached = false
    private var choreographerRunning = false

    private val frameCallback = object : Choreographer.FrameCallback {
        override fun doFrame(frameTimeNs: Long) {
            if (!attached || !renderingEnabled) {
                scheduleFrame()
                return
            }
            val t = frameTimeNs / 1_000_000_000f
            renderHandler.post {
                SurfaceNativeBridge.nativeDraw(t, stressLevel, renderMode)
            }
            scheduleFrame()
        }
    }

    init {
        holder.addCallback(this)
        setZOrderOnTop(false)
        holder.setFormat(PixelFormat.RGBA_8888)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        renderHandler.post {
            attached = SurfaceNativeBridge.nativeAttach(holder.surface)
            if (attached) {
                val (w, h) = surfaceSize()
                SurfaceNativeBridge.nativeResize(w, h)
                startChoreographer()
            }
        }
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        renderHandler.post {
            if (attached) SurfaceNativeBridge.nativeResize(width, height)
        }
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        stopChoreographer()
        renderHandler.post {
            SurfaceNativeBridge.nativeDetach()
            attached = false
        }
    }

    fun setFixedBufferSize(width: Int, height: Int) {
        holder.setFixedSize(width, height)
    }

    fun releaseRenderer() {
        stopChoreographer()
        renderHandler.post { SurfaceNativeBridge.nativeDetach() }
        renderThread.quitSafely()
    }

    fun readNativeStats(): LongArray = SurfaceNativeBridge.nativeGetStats()

    private fun surfaceSize(): Pair<Int, Int> {
        val frame = holder.surfaceFrame
        return if (frame.width() > 0 && frame.height() > 0) {
            frame.width() to frame.height()
        } else {
            val w = min(width, 1920).coerceAtLeast(1)
            val h = min(height, 1080).coerceAtLeast(1)
            w to h
        }
    }

    private fun startChoreographer() {
        if (choreographerRunning) return
        choreographerRunning = true
        scheduleFrame()
    }

    private fun stopChoreographer() {
        choreographerRunning = false
        Choreographer.getInstance().removeFrameCallback(frameCallback)
    }

    private fun scheduleFrame() {
        if (choreographerRunning) {
            Choreographer.getInstance().postFrameCallback(frameCallback)
        }
    }
}
