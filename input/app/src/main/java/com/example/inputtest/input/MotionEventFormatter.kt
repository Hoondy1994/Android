package com.example.inputtest.input

import android.view.MotionEvent
import kotlin.math.hypot

object MotionEventFormatter {

    fun formatHeader(event: MotionEvent): String {
        val action = InputNativeBridge.actionName(event.actionMasked)
        val pointerCount = event.pointerCount
        return buildString {
            append(action)
            append("  pointers=")
            append(pointerCount)
            append("  history=")
            append(event.historySize)
            append("  deviceId=")
            append(event.deviceId)
            append("  source=0x")
            append(event.source.toString(16))
        }
    }

    fun formatPointer(event: MotionEvent, index: Int): String {
        val id = event.getPointerId(index)
        val tool = InputNativeBridge.toolTypeName(event.getToolType(index))
        return buildString {
            append("#")
            append(id)
            append(" [")
            append(index)
            append("] (")
            append("%.1f".format(event.getX(index)))
            append(", ")
            append("%.1f".format(event.getY(index)))
            append(") pressure=")
            append("%.3f".format(event.getPressure(index)))
            append(" size=")
            append("%.3f".format(event.getSize(index)))
            append(" tool=")
            append(tool)
            if (event.actionMasked == MotionEvent.ACTION_MOVE) {
                append(" Δ(")
                append("%.1f".format(event.getHistoricalX(index, 0) - event.getX(index)))
                append(", ")
                append("%.1f".format(event.getHistoricalY(index, 0) - event.getY(index)))
                append(")")
            }
        }
    }

    fun formatVelocity(vx: Float, vy: Float): String {
        val speed = hypot(vx.toDouble(), vy.toDouble()).toFloat()
        return "velocity=(%.1f, %.1f) speed=%.1f px/s".format(vx, vy, speed)
    }
}
