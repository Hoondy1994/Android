package com.example.inputtest.view

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import androidx.core.content.ContextCompat
import com.example.inputtest.R

class DispatchTargetView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0,
) : View(context, attrs, defStyleAttr) {

    var layerName: String = "Child"
    var fillColor: Int = ContextCompat.getColor(context, R.color.pointer_0)
    var onDispatchLogged: ((String) -> Unit)? = null

    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)

    override fun onTouchEvent(event: MotionEvent): Boolean {
        val action = when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> "DOWN"
            MotionEvent.ACTION_MOVE -> "MOVE"
            MotionEvent.ACTION_UP -> "UP"
            else -> event.actionMasked.toString()
        }
        onDispatchLogged?.invoke("[$layerName] onTouchEvent → $action")
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                parent?.requestDisallowInterceptTouchEvent(true)
                return true
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                parent?.requestDisallowInterceptTouchEvent(false)
            }
        }
        return true
    }

    override fun onDraw(canvas: Canvas) {
        paint.color = fillColor
        paint.alpha = 200
        canvas.drawRoundRect(0f, 0f, width.toFloat(), height.toFloat(), 24f, 24f, paint)
        paint.alpha = 255
        paint.color = ContextCompat.getColor(context, R.color.lab_text)
        paint.textSize = 32f
        canvas.drawText(layerName, 24f, height / 2f, paint)
    }
}
