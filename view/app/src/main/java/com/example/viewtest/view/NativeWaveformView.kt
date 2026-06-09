package com.example.viewtest.view

import android.content.Context
import android.graphics.Canvas
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.Path
import android.graphics.Shader
import android.util.AttributeSet
import android.view.View
import com.example.viewtest.native.ViewNativeBridge

/**
 * 波形自定义 View，采样数据由 C++ 生成。
 */
class NativeWaveformView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0,
) : View(context, attrs, defStyleAttr) {

    private val samples = FloatArray(128)
    private val linePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 4f
        strokeCap = Paint.Cap.ROUND
        strokeJoin = Paint.Join.ROUND
    }
    private val fillPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val path = Path()

    fun refreshFromNative() {
        ViewNativeBridge.nativeFillWaveform(samples)
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val w = width.toFloat()
        val h = height.toFloat()
        if (w <= 0f || h <= 0f) return

        linePaint.shader = LinearGradient(
            0f, 0f, w, 0f,
            intArrayOf(0xFF58A6FF.toInt(), 0xFFBC8CFF.toInt(), 0xFF3FB950.toInt()),
            null,
            Shader.TileMode.CLAMP,
        )
        fillPaint.shader = LinearGradient(
            0f, 0f, 0f, h,
            0x4058A6FF,
            0x000D1117,
            Shader.TileMode.CLAMP,
        )

        path.reset()
        val n = samples.size
        for (i in 0 until n) {
            val x = i / (n - 1f) * w
            val y = (1f - samples[i]) * h * 0.85f + h * 0.075f
            if (i == 0) path.moveTo(x, y) else path.lineTo(x, y)
        }
        val fillPath = Path(path).apply {
            lineTo(w, h)
            lineTo(0f, h)
            close()
        }
        canvas.drawPath(fillPath, fillPaint)
        canvas.drawPath(path, linePaint)
    }
}
