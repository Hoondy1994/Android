package com.example.myapplication.ipc

import android.app.Service
import android.content.Intent
import android.os.IBinder
import android.util.Log
import com.example.myapplication.ICalculator

/**
 * Binder IPC 服务端，运行在独立进程 :binder_remote 中。
 * 复杂对象编解码在 Native C++ 层完成。
 */
class CalculatorService : Service() {

    private val binder = object : ICalculator.Stub() {
        override fun add(a: Int, b: Int): Int {
            Log.i(TAG, "Binder add($a, $b) in pid=${android.os.Process.myPid()}")
            return a + b
        }

        override fun getProtocol(): String = "Binder (AIDL + Native C++)"

        override fun processCustomPayload(payload: ByteArray): ByteArray {
            Log.i(TAG, "Binder processCustomPayload size=${payload.size}, pid=${android.os.Process.myPid()}")
            return NativeIpc.nativeProcessCustomPayload(payload)
        }

        override fun processParcelPayload(payload: ByteArray): ByteArray {
            Log.i(TAG, "Binder processParcelPayload size=${payload.size}, pid=${android.os.Process.myPid()}")
            return NativeIpc.nativeProcessParcelPayload(payload)
        }
    }

    override fun onCreate() {
        super.onCreate()
        Log.d(TAG, "[Flow:Binder] onCreate: pid=${android.os.Process.myPid()}")
    }

    override fun onBind(intent: Intent?): IBinder {
        Log.d(TAG, "[Flow:Binder] onBind: intent=$intent, pid=${android.os.Process.myPid()}")
        return binder
    }

    override fun onDestroy() {
        Log.d(TAG, "[Flow:Binder] onDestroy: pid=${android.os.Process.myPid()}")
        super.onDestroy()
    }

    companion object {
        private const val TAG = "CalculatorService"
    }
}
