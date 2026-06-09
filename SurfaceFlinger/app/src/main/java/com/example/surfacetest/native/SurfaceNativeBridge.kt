package com.example.surfacetest.native

import android.view.Surface

object SurfaceNativeBridge {
    init {
        System.loadLibrary("surfacetest")
    }

    const val MODE_PLASMA = 0
    const val MODE_GRID = 1
    const val MODE_PULSE_ALPHA = 2

    external fun nativeAttach(surface: Surface): Boolean
    external fun nativeDetach()
    external fun nativeResize(width: Int, height: Int)
    external fun nativeDraw(timeSec: Float, stressLevel: Int, mode: Int)
    external fun nativeGetStats(): LongArray
}
