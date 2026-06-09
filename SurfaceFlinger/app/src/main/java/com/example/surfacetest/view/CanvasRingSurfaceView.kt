package com.example.surfacetest.view

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.PixelFormat
import android.graphics.PorterDuff
import android.util.AttributeSet
import android.view.Choreographer
import android.view.SurfaceHolder
import android.view.SurfaceView
import kotlin.math.cos
import kotlin.math.min
import kotlin.math.sin

/**
 * 中层 Surface：CPU Canvas + unlockCanvasAndPost，测试半透明叠层与旋转环绘制压力。
 */
class CanvasRingSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
) : SurfaceView(context, attrs), SurfaceHolder.Callback {

    var ringCount = 12
    var layerAlpha = 180
    var renderingEnabled = true
    var postedFrames = 0
        private set

    private val strokePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 4f
    }
    private val fillPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL
    }

    private var phase = 0f
    private var running = false

    private val frameCallback = object : Choreographer.FrameCallback {
        override fun doFrame(frameTimeNs: Long) {
            if (renderingEnabled && holder.surface.isValid) {
                drawFrame(frameTimeNs)
            }
            if (running) Choreographer.getInstance().postFrameCallback(this)
        }
    }

    init {
        setZOrderMediaOverlay(true)
        holder.setFormat(PixelFormat.TRANSLUCENT)
        holder.addCallback(this)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        running = true
        Choreographer.getInstance().postFrameCallback(frameCallback)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) = Unit

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        running = false
        Choreographer.getInstance().removeFrameCallback(frameCallback)
    }

    private fun drawFrame(frameTimeNs: Long) {
        phase = (frameTimeNs % 6_000_000_000L) / 6_000_000_000f * (Math.PI * 2).toFloat()
        var canvas: Canvas? = null
        try {
            canvas = holder.lockCanvas()
            if (canvas == null) return
            canvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR)
            val cx = canvas.width * 0.5f
            val cy = canvas.height * 0.5f
            val baseR = min(canvas.width, canvas.height) * 0.22f
            for (i in 0 until ringCount) {
                val t = i.toFloat() / ringCount.coerceAtLeast(1)
                val angle = phase + t * 6.28f
                val r = baseR + i * 14f
                val x = cx + cos(angle) * r * 0.35f
                val y = cy + sin(angle) * r * 0.35f
                strokePaint.color = Color.argb(
                    layerAlpha,
                    (80 + 175 * sin(angle + t)).toInt().coerceIn(0, 255),
                    (120 + 100 * cos(angle * 1.3f)).toInt().coerceIn(0, 255),
                    (200 + 55 * sin(angle * 0.7f)).toInt().coerceIn(0, 255),
                )
                canvas.drawCircle(x, y, 18f + i * 2f, strokePaint)
                fillPaint.color = Color.argb(layerAlpha / 3, 255, 255, 255)
                canvas.drawCircle(x, y, 6f, fillPaint)
            }
            postedFrames++
        } finally {
            canvas?.let { holder.unlockCanvasAndPost(it) }
        }
    }
}
