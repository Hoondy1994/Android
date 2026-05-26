package com.example.san

object SandboxNative {
    init {
        System.loadLibrary("san")
    }

    external fun runNativeFileRead(path: String): String
    external fun runSeccompBlockedRead(path: String): String
}
