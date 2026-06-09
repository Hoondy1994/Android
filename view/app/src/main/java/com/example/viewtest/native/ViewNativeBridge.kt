package com.example.viewtest.native

object ViewNativeBridge {
    @Volatile
    var isInitialized: Boolean = false
        private set

    const val FLOATS_PER_PARTICLE = 7
    const val LAYOUT_MODE_CIRCLE = 0
    const val LAYOUT_MODE_SPIRAL = 1
    const val LAYOUT_MODE_HEART = 2

    init {
        System.loadLibrary("viewtest")
    }

    external fun nativeInit(particleCount: Int, waveSamples: Int, orbitItems: Int)
    external fun nativeDestroy()
    external fun nativeResize(width: Float, height: Float)
    external fun nativeSetTouch(x: Float, y: Float, active: Boolean)
    external fun nativeTick(dtSec: Float, phaseDelta: Float)
    external fun nativeFillParticles(out: FloatArray): Boolean
    external fun nativeFillWaveform(out: FloatArray): Boolean
    external fun nativeFillOrbit(out: FloatArray): Boolean
    external fun nativeSetParticleCount(count: Int)
    external fun nativeSetHarmonics(count: Int)
    external fun nativeSetGravity(gravity: Float)
    external fun nativeSetLayoutMode(mode: Int)
    external fun nativeGetStats(): String

    fun markInitialized() {
        isInitialized = true
    }

    fun markDestroyed() {
        isInitialized = false
    }
}
