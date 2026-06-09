package com.example.viewtest.view

import android.animation.ValueAnimator
import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import android.view.animation.DecelerateInterpolator
import kotlin.math.hypot

/**
 * 纯 Kotlin 涟漪触摸效果，与 C++ 粒子区形成对比。
 */
class RippleTouchView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0,
) : View(context, attrs, defStyleAttr) {

    private data class Ripple(val x: Float, val y: Float, var progress: Float, val color: Int)

    private val ripples = mutableListOf<Ripple>()
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply { style = Paint.Style.STROKE }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        canvas.drawColor(0xFF161B22.toInt())
        val maxR = hypot(width.toDouble(), height.toDouble()).toFloat()
        for (r in ripples) {
            paint.color = r.color
            paint.strokeWidth = 4f * (1f - r.progress)
            paint.alpha = ((1f - r.progress) * 180).toInt().coerceIn(0, 255)
            canvas.drawCircle(r.x, r.y, maxR * r.progress * 0.45f, paint)
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (event.action == MotionEvent.ACTION_DOWN) {
            spawnRipple(event.x, event.y)
            return true
        }
        return super.onTouchEvent(event)
    }

    private fun spawnRipple(x: Float, y: Float) {
        val colors = intArrayOf(0xFF58A6FF.toInt(), 0xFF3FB950.toInt(), 0xFFF78166.toInt())
        val ripple = Ripple(x, y, 0f, colors[ripples.size % colors.size])
        ripples.add(ripple)
        ValueAnimator.ofFloat(0f, 1f).apply {
            duration = 700
            interpolator = DecelerateInterpolator()
            addUpdateListener {
                ripple.progress = it.animatedValue as Float
                invalidate()
            }
            doOnEnd {
                ripples.remove(ripple)
            }
            start()
        }
        invalidate()
    }

    private fun ValueAnimator.doOnEnd(block: () -> Unit) {
        addListener(object : android.animation.Animator.AnimatorListener {
            override fun onAnimationStart(animation: android.animation.Animator) = Unit
            override fun onAnimationCancel(animation: android.animation.Animator) = block()
            override fun onAnimationRepeat(animation: android.animation.Animator) = Unit
            override fun onAnimationEnd(animation: android.animation.Animator) = block()
        })
    }
}
