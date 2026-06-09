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

/**
 * 顶层小窗 Surface：setZOrderOnTop + 半透明脉冲块，验证最前层合成顺序。
 */
class OverlayPulseSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
) : SurfaceView(context, attrs), SurfaceHolder.Callback {

    var renderingEnabled = true

    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val choreographer: Choreographer = Choreographer.getInstance()
    private var pulse = 0f
    private var running = false

    private fun onVsync(frameTimeNs: Long) {
        if (renderingEnabled && holder.surface.isValid) {
            val phaseNs: Long = frameTimeNs % 2_000_000_000L
            pulse = phaseNs.toFloat() / 2_000_000_000f
            drawPulse()
        }
        if (running) choreographer.postFrameCallback(vsyncCallback)
    }

    private val vsyncCallback = Choreographer.FrameCallback { frameTimeNs -> onVsync(frameTimeNs) }

    init {
        setZOrderOnTop(true)
        holder.setFormat(PixelFormat.TRANSLUCENT)
        holder.addCallback(this)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        running = true
        choreographer.postFrameCallback(vsyncCallback)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) = Unit

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        running = false
        choreographer.removeFrameCallback(vsyncCallback)
    }

    private fun drawPulse() {
        var canvas: Canvas? = null
        try {
            canvas = holder.lockCanvas() ?: return
            canvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR)
            val alpha = (120 + 135 * pulse).toInt().coerceIn(0, 255)
            paint.color = Color.argb(alpha, 255, 64, 128)
            val pad = 12f
            canvas.drawRoundRect(pad, pad, canvas.width - pad, canvas.height - pad, 16f, 16f, paint)
            paint.color = Color.argb(220, 255, 255, 255)
            paint.textSize = 28f
            canvas.drawText("TOP LAYER", pad + 8f, canvas.height * 0.55f, paint)
        } finally {
            canvas?.let { holder.unlockCanvasAndPost(it) }
        }
    }
}
