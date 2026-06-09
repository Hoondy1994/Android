package com.example.inputtest.view

import android.content.Context
import android.util.AttributeSet
import android.view.MotionEvent
import android.widget.FrameLayout

/**
 * 记录触摸事件在 View 树中的派发顺序。
 */
class TouchDispatchLayout @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
) : FrameLayout(context, attrs) {

    var layerName: String = "Parent"
    var onDispatchLogged: ((String) -> Unit)? = null
    var interceptTouches: Boolean = false

    override fun dispatchTouchEvent(ev: MotionEvent): Boolean {
        log("dispatchTouchEvent", ev)
        return super.dispatchTouchEvent(ev)
    }

    override fun onInterceptTouchEvent(ev: MotionEvent): Boolean {
        log("onInterceptTouchEvent", ev)
        return interceptTouches && ev.actionMasked == MotionEvent.ACTION_MOVE
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        log("onTouchEvent", event)
        return interceptTouches
    }

    private fun log(stage: String, event: MotionEvent) {
        val action = when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> "DOWN"
            MotionEvent.ACTION_MOVE -> "MOVE"
            MotionEvent.ACTION_UP -> "UP"
            MotionEvent.ACTION_CANCEL -> "CANCEL"
            MotionEvent.ACTION_POINTER_DOWN -> "PTR_DOWN"
            MotionEvent.ACTION_POINTER_UP -> "PTR_UP"
            else -> event.actionMasked.toString()
        }
        onDispatchLogged?.invoke("[$layerName] $stage → $action")
    }
}
