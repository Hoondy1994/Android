package com.example.inputtest.view

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Path
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.VelocityTracker
import android.view.View
import androidx.core.content.ContextCompat
import com.example.inputtest.R
import com.example.inputtest.input.MotionEventFormatter
/**
 * 多指触摸可视化：轨迹、压力圆、历史点、VelocityTracker。
 */
class MultiTouchCanvasView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0,
) : View(context, attrs, defStyleAttr) {

    private data class TrailPoint(val x: Float, val y: Float, val pressure: Float)

    private val pointerColors = intArrayOf(
        R.color.pointer_0,
        R.color.pointer_1,
        R.color.pointer_2,
        R.color.pointer_3,
        R.color.pointer_4,
    )
    private val trails = Array(10) { ArrayDeque<TrailPoint>(64) }
    private val fillPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { style = Paint.Style.FILL }
    private val strokePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 3f
    }
    private val textPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.lab_text)
        textSize = 28f
    }
    private val path = Path()
    private var velocityTracker: VelocityTracker? = null

    var onEventLogged: ((String) -> Unit)? = null

    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (velocityTracker == null) {
            velocityTracker = VelocityTracker.obtain()
        }
        velocityTracker?.addMovement(event)

        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                updateTrails(event)
                logEvent(event)
                invalidate()
            }
            MotionEvent.ACTION_MOVE -> {
                updateTrails(event)
                logEvent(event)
                invalidate()
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> {
                val index = if (event.actionMasked == MotionEvent.ACTION_UP) 0
                else event.actionIndex
                velocityTracker?.computeCurrentVelocity(1000)
                val id = event.getPointerId(index)
                val vx = velocityTracker?.getXVelocity(id) ?: 0f
                val vy = velocityTracker?.getYVelocity(id) ?: 0f
                onEventLogged?.invoke(MotionEventFormatter.formatVelocity(vx, vy))
                clearTrail(id)
                logEvent(event)
                if (event.actionMasked == MotionEvent.ACTION_UP) {
                    velocityTracker?.recycle()
                    velocityTracker = null
                }
                invalidate()
            }
            MotionEvent.ACTION_CANCEL -> {
                trails.forEach { it.clear() }
                velocityTracker?.recycle()
                velocityTracker = null
                logEvent(event)
                invalidate()
            }
        }
        return true
    }

    private fun updateTrails(event: MotionEvent) {
        for (i in 0 until event.pointerCount) {
            val id = event.getPointerId(i)
            if (id !in trails.indices) continue
            val trail = trails[id]
            trail.addLast(TrailPoint(event.getX(i), event.getY(i), event.getPressure(i)))
            while (trail.size > 48) trail.removeFirst()
        }
    }

    private fun clearTrail(pointerId: Int) {
        if (pointerId in trails.indices) trails[pointerId].clear()
    }

    private fun logEvent(event: MotionEvent) {
        onEventLogged?.invoke(MotionEventFormatter.formatHeader(event))
        for (i in 0 until event.pointerCount) {
            onEventLogged?.invoke("  " + MotionEventFormatter.formatPointer(event, i))
        }
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        canvas.drawColor(ContextCompat.getColor(context, R.color.lab_surface))
        for (id in trails.indices) {
            val trail = trails[id]
            if (trail.isEmpty()) continue
            val color = ContextCompat.getColor(context, pointerColors[id % pointerColors.size])
            strokePaint.color = color
            fillPaint.color = color
            path.reset()
            trail.forEachIndexed { idx, p ->
                if (idx == 0) path.moveTo(p.x, p.y) else path.lineTo(p.x, p.y)
            }
            canvas.drawPath(path, strokePaint)
            val last = trail.last()
            val radius = 24f + last.pressure * 48f
            fillPaint.alpha = 90
            canvas.drawCircle(last.x, last.y, radius, fillPaint)
            fillPaint.alpha = 255
            canvas.drawCircle(last.x, last.y, 10f, fillPaint)
            canvas.drawText(
                "id=$id",
                last.x + 14f,
                last.y - 14f,
                textPaint,
            )
        }
        canvas.drawText(
            "多指触摸 · 最多显示 5 色轨迹",
            16f,
            height - 24f,
            textPaint.apply { textSize = 24f },
        )
        textPaint.textSize = 28f
    }
}
