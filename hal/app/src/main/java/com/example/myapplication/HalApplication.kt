package com.example.myapplication

import android.app.Application
import android.util.Log

class HalApplication : Application() {

    override fun onCreate() {
        super.onCreate()
        val defaultHandler = Thread.getDefaultUncaughtExceptionHandler()
        Thread.setDefaultUncaughtExceptionHandler { thread, throwable ->
            Log.e(TAG, "Uncaught on ${thread.name}", throwable)
            defaultHandler?.uncaughtException(thread, throwable)
        }
    }

    companion object {
        private const val TAG = "HalApplication"
    }
}
