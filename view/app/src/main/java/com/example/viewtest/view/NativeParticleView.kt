package com.example.viewtest.view

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.util.AttributeSet
import android.view.Choreographer
import android.view.MotionEvent
import android.view.View
import com.example.viewtest.native.ViewNativeBridge
import kotlin.math.max

/**
 * 自定义 View：每帧从 C++ 粒子引擎拉取数据并绘制。
 */
class NativeParticleView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0,
) : View(context, attrs, defStyleAttr) {

    private val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply { style = Paint.Style.FILL }
    private val buffer = FloatArray(256 * ViewNativeBridge.FLOATS_PER_PARTICLE)
    private var lastFrameNs = 0L

    private val frameCallback = object : Choreographer.FrameCallback {
        override fun doFrame(frameTimeNs: Long) {
            if (ViewNativeBridge.isInitialized && lastFrameNs > 0L) {
                val dt = (frameTimeNs - lastFrameNs) / 1_000_000_000f
                ViewNativeBridge.nativeTick(dt.coerceIn(0.001f, 0.05f), dt * 2.5f)
                if (ViewNativeBridge.nativeFillParticles(buffer)) {
                    invalidate()
                }
            }
            lastFrameNs = frameTimeNs
            if (isAttachedToWindow) {
                choreographer.postFrameCallback(this)
            }
        }
    }

    private val choreographer = Choreographer.getInstance()

    override fun onAttachedToWindow() {
        super.onAttachedToWindow()
        choreographer.postFrameCallback(frameCallback)
    }

    override fun onDetachedFromWindow() {
        choreographer.removeFrameCallback(frameCallback)
        super.onDetachedFromWindow()
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        if (w > 0 && h > 0) {
            ViewNativeBridge.nativeResize(w.toFloat(), h.toFloat())
        }
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        canvas.drawColor(0xFF0D1117.toInt())
        val stride = ViewNativeBridge.FLOATS_PER_PARTICLE
        var i = 0
        while (i + stride <= buffer.size) {
            val base = i
            i += stride
            val radius = buffer[base + 2]
            if (radius <= 0f) continue
            val x = buffer[base]
            val y = buffer[base + 1]
            paint.color = argb(
                buffer[base + 6],
                buffer[base + 3],
                buffer[base + 4],
                buffer[base + 5],
            )
            canvas.drawCircle(x, y, max(radius, 2f), paint)
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN,
            MotionEvent.ACTION_MOVE -> {
                ViewNativeBridge.nativeSetTouch(event.x, event.y, true)
                return true
            }
            MotionEvent.ACTION_UP,
            MotionEvent.ACTION_CANCEL -> {
                ViewNativeBridge.nativeSetTouch(event.x, event.y, false)
            }
        }
        return super.onTouchEvent(event)
    }

    private fun argb(a: Float, r: Float, g: Float, b: Float): Int {
        fun c(v: Float) = (v.coerceIn(0f, 1f) * 255).toInt()
        return (c(a) shl 24) or (c(r) shl 16) or (c(g) shl 8) or c(b)
    }
}
