package com.example.ipctest

import android.app.Service
import android.content.Intent
import android.os.IBinder
import android.os.Process

class CalculatorService : Service() {

    private val binder = object : ICalculator.Stub() {
        override fun add(a: Int, b: Int): Int = a + b

        override fun getMessage(): String {
            return "来自远程进程 (PID: ${Process.myPid()})"
        }
    }

    override fun onBind(intent: Intent?): IBinder = binder
}
