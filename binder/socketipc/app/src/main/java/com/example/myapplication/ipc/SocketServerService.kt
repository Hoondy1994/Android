package com.example.myapplication.ipc

import android.app.Service
import android.content.Intent
import android.os.Binder
import android.os.IBinder
import android.util.Log

/**
 * Native Socket 服务端，运行在独立进程 :socket_remote 中。
 */
class SocketServerService : Service() {

    private val localBinder = LocalBinder()

    inner class LocalBinder : Binder()

    override fun onCreate() {
        super.onCreate()
        Log.d(TAG, "[2] onCreate: pid=${android.os.Process.myPid()}")
        if (!nativeStartSocketServer()) {
            Log.e(TAG, "Native socket server failed to start")
        } else {
            Log.i(TAG, "Native socket server started in pid=${nativeGetProcessId()}")
        }
    }

    override fun onBind(intent: Intent?): IBinder {
        Log.d(TAG, "[3] onBind: intent=$intent, pid=${android.os.Process.myPid()}")
        return localBinder
    }

    override fun onDestroy() {
        Log.d(TAG, "[Flow:Socket] onDestroy: pid=${android.os.Process.myPid()}")
        nativeStopSocketServer()
        super.onDestroy()
    }

    private external fun nativeStartSocketServer(): Boolean
    private external fun nativeStopSocketServer()
    private external fun nativeGetProcessId(): Int

    companion object {
        private const val TAG = "SocketServerService"

        init {
            System.loadLibrary("myapplication")
        }
    }
}
