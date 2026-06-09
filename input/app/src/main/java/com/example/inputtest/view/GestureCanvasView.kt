package com.example.inputtest.view

import android.content.Context
import android.graphics.Canvas
import android.graphics.Matrix
import android.graphics.Paint
import android.util.AttributeSet
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.ScaleGestureDetector
import android.view.View
import androidx.core.content.ContextCompat
import com.example.inputtest.R
import kotlin.math.max

/**
 * GestureDetector + ScaleGestureDetector 演示画布。
 */
class GestureCanvasView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0,
) : View(context, attrs, defStyleAttr) {

    private val matrix = Matrix()
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val textPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.lab_text)
        textSize = 26f
    }
    private var scale = 1f
    private var offsetX = 0f
    private var offsetY = 0f
    private var statusLine = "等待手势…"

    var onGestureLogged: ((String) -> Unit)? = null

    private val gestureDetector = GestureDetector(context, object : GestureDetector.SimpleOnGestureListener() {
        override fun onDown(e: MotionEvent): Boolean = true

        override fun onScroll(
            e1: MotionEvent?,
            e2: MotionEvent,
            distanceX: Float,
            distanceY: Float,
        ): Boolean {
            offsetX -= distanceX
            offsetY -= distanceY
            log("onScroll Δ=(${(-distanceX).format()}, ${(-distanceY).format()})")
            invalidate()
            return true
        }

        override fun onFling(
            e1: MotionEvent?,
            e2: MotionEvent,
            velocityX: Float,
            velocityY: Float,
        ): Boolean {
            log("onFling v=(${velocityX.format()}, ${velocityY.format()})")
            return true
        }

        override fun onDoubleTap(e: MotionEvent): Boolean {
            scale = 1f
            offsetX = 0f
            offsetY = 0f
            log("onDoubleTap → 重置变换")
            invalidate()
            return true
        }

        override fun onLongPress(e: MotionEvent) {
            log("onLongPress @ (${e.x.format()}, ${e.y.format()})")
        }

        override fun onSingleTapConfirmed(e: MotionEvent): Boolean {
            log("onSingleTapConfirmed")
            return true
        }
    })

    private val scaleDetector = ScaleGestureDetector(context, object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
        override fun onScale(detector: ScaleGestureDetector): Boolean {
            scale = (scale * detector.scaleFactor).coerceIn(0.35f, 4f)
            log("onScale factor=${detector.scaleFactor.format()} total=${scale.format()}")
            invalidate()
            return true
        }
    })

    override fun onTouchEvent(event: MotionEvent): Boolean {
        scaleDetector.onTouchEvent(event)
        gestureDetector.onTouchEvent(event)
        return true
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        canvas.drawColor(ContextCompat.getColor(context, R.color.lab_surface))
        canvas.save()
        matrix.reset()
        matrix.postScale(scale, scale, width / 2f, height / 2f)
        matrix.postTranslate(offsetX, offsetY)
        canvas.concat(matrix)

        paint.style = Paint.Style.STROKE
        paint.strokeWidth = 3f / max(scale, 0.5f)
        val grid = 48
        val cols = 8
        val rows = 6
        for (r in 0 until rows) {
            for (c in 0 until cols) {
                val colorRes = when ((r + c) % 5) {
                    0 -> R.color.pointer_0
                    1 -> R.color.pointer_1
                    2 -> R.color.pointer_2
                    3 -> R.color.pointer_3
                    else -> R.color.pointer_4
                }
                paint.color = ContextCompat.getColor(context, colorRes)
                val left = c * grid * 4f + 40f
                val top = r * grid * 4f + 60f
                canvas.drawRect(left, top, left + grid * 3f, top + grid * 3f, paint)
            }
        }

        canvas.restore()
        canvas.drawText(statusLine, 16f, 40f, textPaint)
        canvas.drawText(
            "scale=${scale.format()} offset=(${offsetX.format()}, ${offsetY.format()})",
            16f,
            72f,
            textPaint,
        )
    }

    private fun log(msg: String) {
        statusLine = msg
        onGestureLogged?.invoke(msg)
        invalidate()
    }

    private fun Float.format() = "%.1f".format(this)
}
