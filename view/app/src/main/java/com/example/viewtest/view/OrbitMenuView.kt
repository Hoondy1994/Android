package com.example.viewtest.view

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.util.AttributeSet
import android.view.View
import com.example.viewtest.native.ViewNativeBridge
import kotlin.math.min

/**
 * 轨道菜单：布局坐标由 C++ LayoutSolver 计算。
 */
class OrbitMenuView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0,
) : View(context, attrs, defStyleAttr) {

    private val positions = FloatArray(24) // 12 items * 2
    private val nodePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { style = Paint.Style.FILL }
    private val ringPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 2f
        color = 0x33FFFFFF
    }
    private val labelPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = 0xFFE6EDF3.toInt()
        textSize = 28f
        textAlign = Paint.Align.CENTER
    }

    private val labels = listOf(
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L",
    )

    fun refreshFromNative() {
        ViewNativeBridge.nativeFillOrbit(positions)
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val w = width.toFloat()
        val h = height.toFloat()
        val cx = w * 0.5f
        val cy = h * 0.5f
        val radius = min(w, h) * 0.42f
        canvas.drawCircle(cx, cy, radius, ringPaint)

        val itemCount = positions.size / 2
        for (i in 0 until itemCount) {
            val nx = positions[i * 2]
            val ny = positions[i * 2 + 1]
            val px = nx * w
            val py = ny * h
            nodePaint.color = hueColor(i, itemCount)
            canvas.drawCircle(px, py, 22f, nodePaint)
            val label = labels.getOrElse(i) { "${i + 1}" }
            canvas.drawText(label, px, py + 10f, labelPaint)
        }
    }

    private fun hueColor(index: Int, total: Int): Int {
        val hue = (index * 360f / total).toInt()
        return android.graphics.Color.HSVToColor(220, floatArrayOf(hue.toFloat(), 0.55f, 0.95f))
    }
}
