package com.oplus.pantanal.namespace.demo

/**
 * JNI bridge for libpreopen path mapping / capability-style file access demo.
 */
object PreopenBridge {

    init {
        System.loadLibrary("preopen_demo")
        android.util.Log.i("PreopenDemo", "libpreopen_demo loaded")
    }

    /**
     * Runs a demo under [baseDir] (typically Context.filesDir.absolutePath).
     * Registers a real directory to a virtual prefix, then tries allowed vs denied paths.
     */
    @JvmStatic
    external fun runDemo(baseDir: String): String
}
