package com.example.amstest

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.widget.Button
import android.widget.ScrollView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {

    private lateinit var tvLog: TextView
    private lateinit var tvTop: TextView
    private lateinit var scrollLog: ScrollView
    private val mainHandler = Handler(Looper.getMainLooper())
    private var refreshRunnable: Runnable? = null

    // ── Activity lifecycle ─────────────────────────────────────────────────
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        tvLog     = findViewById(R.id.tv_log)
        tvTop     = findViewById(R.id.tv_top_activity)
        scrollLog = findViewById(R.id.scroll_log)

        // Init native AMS
        nativeInit()
        appendLog("=== AMS Native initialized ===")

        bindButtons()
        startAutoRefresh()
    }

    override fun onDestroy() {
        super.onDestroy()
        refreshRunnable?.let { mainHandler.removeCallbacks(it) }
        nativeShutdown()
    }

    // ── Button wiring ──────────────────────────────────────────────────────
    private fun bindButtons() {
        // Start activities with different launch modes
        findViewById<Button>(R.id.btn_start_settings).setOnClickListener {
            val r = nativeStartActivity(
                "com.android.settings",
                "com.android.settings.Settings",
                "android.intent.action.MAIN", 0)  // STANDARD
            appendLog("[UI] startActivity(Settings) => $r")
            refreshTop()
        }
        findViewById<Button>(R.id.btn_start_maps).setOnClickListener {
            val r = nativeStartActivity(
                "com.google.android.maps",
                "com.google.android.maps.MapsActivity",
                "android.intent.action.VIEW", 0)
            appendLog("[UI] startActivity(Maps) => $r")
            refreshTop()
        }
        findViewById<Button>(R.id.btn_start_camera).setOnClickListener {
            val r = nativeStartActivity(
                "com.android.camera2",
                "com.android.camera2.CameraActivity",
                "android.intent.action.MAIN", 0)
            appendLog("[UI] startActivity(Camera) => $r")
            refreshTop()
        }
        findViewById<Button>(R.id.btn_start_single_task).setOnClickListener {
            // LaunchMode 2 = SINGLE_TASK
            val r = nativeStartActivity(
                "com.android.browser",
                "com.android.browser.BrowserActivity",
                "android.intent.action.VIEW", 2)
            appendLog("[UI] startActivity(Browser, SINGLE_TASK) => $r")
            refreshTop()
        }
        findViewById<Button>(R.id.btn_start_single_instance).setOnClickListener {
            // LaunchMode 3 = SINGLE_INSTANCE
            val r = nativeStartActivity(
                "com.android.dialer",
                "com.android.dialer.DialtactsActivity",
                "android.intent.action.MAIN", 3)
            appendLog("[UI] startActivity(Phone, SINGLE_INSTANCE) => $r")
            refreshTop()
        }

        // Finish activities
        findViewById<Button>(R.id.btn_finish_top).setOnClickListener {
            val top = nativeGetTopActivity()
            appendLog("[UI] Finishing top: $top")
            val cls = top.substringAfter("/").substringBefore(" ")
            val pkg = top.substringBefore("/")
            val r = nativeFinishActivity("$pkg.$cls")
                .takeIf { it != 0 }
                ?: run { nativeFinishActivity(top.substringBefore(" ").substringAfter("/")); 0 }
            appendLog("[UI] finishTop => $r")
            refreshTop()
        }
        findViewById<Button>(R.id.btn_finish_settings).setOnClickListener {
            val r = nativeFinishActivity("com.android.settings.Settings")
            appendLog("[UI] finishActivity(Settings) => $r")
            refreshTop()
        }

        // Services
        findViewById<Button>(R.id.btn_start_fg_svc).setOnClickListener {
            val r = nativeStartService(
                "com.android.music",
                "com.android.music.MediaPlaybackService",
                true)
            appendLog("[UI] startForegroundService(Music) => $r")
        }
        findViewById<Button>(R.id.btn_stop_svc).setOnClickListener {
            val r = nativeStopService(
                "com.android.music",
                "com.android.music.MediaPlaybackService")
            appendLog("[UI] stopService(Music) => $r")
        }

        // Broadcasts
        findViewById<Button>(R.id.btn_broadcast_boot).setOnClickListener {
            nativeSendBroadcast("android.intent.action.BOOT_COMPLETED")
            appendLog("[UI] sendBroadcast(BOOT_COMPLETED)")
        }
        findViewById<Button>(R.id.btn_broadcast_batt).setOnClickListener {
            nativeSendBroadcast("android.intent.action.BATTERY_LOW")
            appendLog("[UI] sendBroadcast(BATTERY_LOW)")
        }

        // Queries
        findViewById<Button>(R.id.btn_show_procs).setOnClickListener {
            val procs = nativeGetRunningProcesses()
            appendLog("=== Processes ===\n$procs")
        }
        findViewById<Button>(R.id.btn_tasks).setOnClickListener {
            val tasks = nativeGetRunningTasks(10)
            appendLog("=== Tasks ===\n$tasks")
        }
        findViewById<Button>(R.id.btn_dump).setOnClickListener {
            val dump = nativeDumpState()
            appendLog(dump)
        }

        // Clear log
        findViewById<Button>(R.id.btn_clear_log).setOnClickListener {
            tvLog.text = ""
        }
    }

    // ── Auto-refresh log & top activity every 2 seconds ───────────────────
    private fun startAutoRefresh() {
        refreshRunnable = object : Runnable {
            override fun run() {
                refreshTop()
                // Pull the last 30 lines from native log buffer
                val nativeLogs = nativeGetLogs(30)
                if (nativeLogs.isNotBlank()) {
                    // Avoid duplication – only append if different from current tail
                    val current = tvLog.text.toString()
                    val tail = current.takeLast(nativeLogs.length)
                    if (tail != nativeLogs) {
                        tvLog.text = nativeLogs
                        scrollLog.post { scrollLog.fullScroll(ScrollView.FOCUS_DOWN) }
                    }
                }
                mainHandler.postDelayed(this, 2000)
            }
        }
        mainHandler.post(refreshRunnable!!)
    }

    private fun refreshTop() {
        val top = nativeGetTopActivity()
        tvTop.text = "Top: $top"
    }

    private fun appendLog(msg: String) {
        val current = tvLog.text.toString()
        tvLog.text = if (current.isBlank()) msg else "$current\n$msg"
        scrollLog.post { scrollLog.fullScroll(ScrollView.FOCUS_DOWN) }
    }

    // ── JNI declarations ───────────────────────────────────────────────────
    external fun nativeInit()
    external fun nativeShutdown()
    external fun nativeStartActivity(pkg: String, cls: String, action: String, launchMode: Int): Int
    external fun nativeFinishActivity(cls: String): Int
    external fun nativeStartService(pkg: String, cls: String, foreground: Boolean): Int
    external fun nativeStopService(pkg: String, cls: String): Int
    external fun nativeSendBroadcast(action: String)
    external fun nativeGetTopActivity(): String
    external fun nativeDumpState(): String
    external fun nativeGetRunningTasks(maxNum: Int): String
    external fun nativeGetRunningProcesses(): String
    external fun nativeGetLogs(lines: Int): String
    external fun nativeMoveTaskToFront(taskId: Int): Int

    companion object {
        init { System.loadLibrary("amstest") }
    }
}
