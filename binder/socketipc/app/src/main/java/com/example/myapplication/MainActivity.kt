package com.example.myapplication

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.Bundle
import android.os.IBinder
import android.os.Process
import android.util.Log
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.example.myapplication.databinding.ActivityMainBinding
import com.example.myapplication.ipc.BinderBenchmark
import com.example.myapplication.ipc.CalculatorService
import com.example.myapplication.ipc.NativeIpc
import com.example.myapplication.ipc.SocketServerService
import com.example.myapplication.ICalculator
import java.util.concurrent.Executors

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private var calculator: ICalculator? = null
    private var binderBound = false
    private var socketServiceBound = false
    private val backgroundExecutor = Executors.newSingleThreadExecutor()

    private val binderServiceConnection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
            Log.d(TAG, "[Flow:Binder] onServiceConnected: name=$name, pid=${Process.myPid()}")
            calculator = ICalculator.Stub.asInterface(service)
            binderBound = true
            appendLog("Binder 服务已连接 (:binder_remote)")
        }

        override fun onServiceDisconnected(name: ComponentName?) {
            Log.d(TAG, "[Flow:Binder] onServiceDisconnected: name=$name, pid=${Process.myPid()}")
            calculator = null
            binderBound = false
            appendLog("Binder 服务已断开")
        }
    }

    private val socketServiceConnection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
            Log.d(TAG, "[4] onServiceConnected: name=$name, binder=$service, pid=${Process.myPid()}")
            socketServiceBound = true
            waitForSocketServerAsync()
        }

        override fun onServiceDisconnected(name: ComponentName?) {
            Log.d(TAG, "[5] onServiceDisconnected: name=$name, pid=${Process.myPid()}")
            socketServiceBound = false
            appendLog("Socket 服务已断开")
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.d(TAG, "[Flow:App] onCreate: pid=${Process.myPid()}")

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.tvProcessInfo.text =
            "主进程 PID: ${Process.myPid()}  |  Binder: :binder_remote  |  Socket: :socket_remote"

        startRemoteServices()
        bindCalculatorService()
        Log.d(TAG, "[Flow:App] onCreate 完成，等待远程服务异步回调")

        binding.btnBinderCalc.setOnClickListener { runBinderAdd() }
        binding.btnSocketCalc.setOnClickListener { runSocketAdd() }
        binding.btnBinderCustom.setOnClickListener { runBinderComplex(useParcel = false) }
        binding.btnBinderParcel.setOnClickListener { runBinderComplex(useParcel = true) }
        binding.btnSocketCustom.setOnClickListener { runSocketComplex(useParcel = false) }
        binding.btnSocketParcel.setOnClickListener { runSocketComplex(useParcel = true) }
        binding.btnBenchSerialization.setOnClickListener { runSerializationBenchmark() }
        binding.btnBenchBinder.setOnClickListener { runBinderBenchmark() }
        binding.btnBenchSocket.setOnClickListener { runSocketBenchmark() }
    }

    private fun startRemoteServices() {
        val bound = bindService(
            Intent(this, SocketServerService::class.java),
            socketServiceConnection,
            Context.BIND_AUTO_CREATE,
        )
        Log.d(TAG, "[1] bindService 已调用, 返回值=$bound, pid=${Process.myPid()}（此时还未 onServiceConnected）")
    }

    private fun waitForSocketServerAsync() {
        Log.d(TAG, "[Flow:Socket] waitForSocketServerAsync 开始")
        backgroundExecutor.execute {
            val ready = nativeWaitForSocketServer(10_000)
            Log.d(TAG, "[Flow:Socket] nativeWaitForSocketServer 返回 ready=$ready")
            runOnUiThread {
                if (ready) {
                    appendLog("Socket 服务已就绪 (:socket_remote, Native C++)")
                } else {
                    appendLog("Socket 服务连接超时，请杀掉 App 后重试")
                }
            }
        }
    }

    private fun bindCalculatorService() {
        val bound = bindService(
            Intent(this, CalculatorService::class.java),
            binderServiceConnection,
            Context.BIND_AUTO_CREATE,
        )
        Log.d(TAG, "[Flow:Binder] bindService(CalculatorService) 已调用, 返回值=$bound, pid=${Process.myPid()}")
    }

    private fun runBinderAdd() {
        Log.d(TAG, "[Flow:BinderAdd] 开始")
        val numbers = readInputNumbers() ?: return
        if (!ensureBinderReady()) return
        val (a, b) = numbers

        val start = System.nanoTime()
        val result = calculator!!.add(a, b)
        Log.d(TAG, "[Flow:BinderAdd] 完成: $a + $b = $result")
        val elapsedMs = (System.nanoTime() - start) / 1_000_000.0
        appendLog("Binder Add: $a + $b = $result (${"%.3f".format(elapsedMs)} ms)")
    }

    private fun runSocketAdd() {
        Log.d(TAG, "[Flow:SocketAdd] 开始")
        val numbers = readInputNumbers() ?: return
        val (a, b) = numbers

        val start = System.nanoTime()
        val result = nativeSocketAdd(a, b)
        Log.d(TAG, "[Flow:SocketAdd] nativeSocketAdd 返回 result=$result")
        val elapsedMs = (System.nanoTime() - start) / 1_000_000.0
        if (result < 0) {
            appendLog("Socket Add 失败")
            return
        }
        appendLog("Socket Add: $a + $b = $result (${"%.3f".format(elapsedMs)} ms)")
    }

    private fun runBinderComplex(useParcel: Boolean) {
        val label = if (useParcel) "Binder Parcel" else "Binder Custom"
        Log.d(TAG, "[Flow:BinderComplex] 开始: $label")
        if (!ensureBinderReady()) return
        val payload = if (useParcel) nativeBuildParcelPayload() else nativeBuildCustomPayload()
        Log.d(TAG, "[Flow:BinderComplex] payload 已构建, size=${payload.size}")

        val start = System.nanoTime()
        val result = if (useParcel) {
            calculator!!.processParcelPayload(payload)
        } else {
            calculator!!.processCustomPayload(payload)
        }
        val elapsedMs = (System.nanoTime() - start) / 1_000_000.0
        val verified = if (useParcel) {
            nativeVerifyParcelPayload(result)
        } else {
            nativeVerifyCustomPayload(result)
        }
        val desc = NativeIpc.nativeDescribePayload(result, useParcel)
        Log.d(TAG, "[Flow:BinderComplex] 完成: $label, verified=$verified")
        appendLog("$label: ${"%.3f".format(elapsedMs)} ms, verified=$verified, $desc")
    }

    private fun runSocketComplex(useParcel: Boolean) {
        val label = if (useParcel) "Socket Parcel" else "Socket Custom"
        Log.d(TAG, "[Flow:SocketComplex] 开始: $label")
        val start = System.nanoTime()
        val message = if (useParcel) nativeSocketProcessParcel() else nativeSocketProcessCustom()
        Log.d(TAG, "[Flow:SocketComplex] 完成: $label")
        val elapsedMs = (System.nanoTime() - start) / 1_000_000.0
        appendLog("$label: ${"%.3f".format(elapsedMs)} ms, $message")
    }

    private fun runSerializationBenchmark() {
        val config = readBenchmarkConfig() ?: return
        val (threads, iterations) = config
        Log.d(TAG, "[Flow:BenchSerialization] 开始: threads=$threads, iterations=$iterations")
        setButtonsEnabled(false)
        backgroundExecutor.execute {
            val custom = nativeRunSerializationBenchmark(threads, iterations, false)
            val parcel = nativeRunSerializationBenchmark(threads, iterations, true)
            Log.d(TAG, "[Flow:BenchSerialization] 完成")
            runOnUiThread {
                appendLog("[$threads x $iterations] $custom")
                appendLog("[$threads x $iterations] $parcel")
                setButtonsEnabled(true)
            }
        }
    }

    private fun runBinderBenchmark() {
        val config = readBenchmarkConfig() ?: return
        if (!ensureBinderReady()) return
        val (threads, iterations) = config
        val customPayload = nativeBuildCustomPayload()
        val parcelPayload = nativeBuildParcelPayload()
        Log.d(TAG, "[Flow:BenchBinder] 开始: threads=$threads, iterations=$iterations")
        setButtonsEnabled(false)
        backgroundExecutor.execute {
            val addStats = BinderBenchmark.runAdd(calculator!!, threads, iterations)
            val customStats = BinderBenchmark.runCustom(calculator!!, customPayload, threads, iterations)
            val parcelStats = BinderBenchmark.runParcel(calculator!!, parcelPayload, threads, iterations)
            Log.d(TAG, "[Flow:BenchBinder] 完成")
            runOnUiThread {
                appendLog("Binder Bench Add: ${addStats.format()}")
                appendLog("Binder Bench Custom: ${customStats.format()}")
                appendLog("Binder Bench Parcel: ${parcelStats.format()}")
                setButtonsEnabled(true)
            }
        }
    }

    private fun runSocketBenchmark() {
        val config = readBenchmarkConfig() ?: return
        val (threads, iterations) = config
        Log.d(TAG, "[Flow:BenchSocket] 开始: threads=$threads, iterations=$iterations")
        setButtonsEnabled(false)
        backgroundExecutor.execute {
            val add = nativeRunSocketBenchmark(threads, iterations, 0)
            val custom = nativeRunSocketBenchmark(threads, iterations, 1)
            val parcel = nativeRunSocketBenchmark(threads, iterations, 2)
            Log.d(TAG, "[Flow:BenchSocket] 完成")
            runOnUiThread {
                appendLog("Socket Bench Add: $add")
                appendLog("Socket Bench Custom: $custom")
                appendLog("Socket Bench Parcel: $parcel")
                setButtonsEnabled(true)
            }
        }
    }

    private fun readBenchmarkConfig(): Pair<Int, Int>? {
        val threads = binding.etThreadCount.text?.toString()?.trim()?.toIntOrNull()
        val iterations = binding.etIterations.text?.toString()?.trim()?.toIntOrNull()
        if (threads == null || iterations == null || threads <= 0 || iterations <= 0) {
            Toast.makeText(this, "请输入有效的线程数与迭代次数", Toast.LENGTH_SHORT).show()
            return null
        }
        return threads to iterations
    }

    private fun readInputNumbers(): Pair<Int, Int>? {
        val a = binding.etNumberA.text?.toString()?.trim()?.toIntOrNull()
        val b = binding.etNumberB.text?.toString()?.trim()?.toIntOrNull()
        if (a == null || b == null) {
            Toast.makeText(this, "请输入有效整数", Toast.LENGTH_SHORT).show()
            return null
        }
        return a to b
    }

    private fun ensureBinderReady(): Boolean {
        if (!binderBound || calculator == null) {
            Toast.makeText(this, "Binder 服务未就绪", Toast.LENGTH_SHORT).show()
            return false
        }
        return true
    }

    private fun setButtonsEnabled(enabled: Boolean) {
        binding.btnBinderCalc.isEnabled = enabled
        binding.btnSocketCalc.isEnabled = enabled
        binding.btnBinderCustom.isEnabled = enabled
        binding.btnBinderParcel.isEnabled = enabled
        binding.btnSocketCustom.isEnabled = enabled
        binding.btnSocketParcel.isEnabled = enabled
        binding.btnBenchSerialization.isEnabled = enabled
        binding.btnBenchBinder.isEnabled = enabled
        binding.btnBenchSocket.isEnabled = enabled
    }

    private fun appendLog(message: String) {
        val current = binding.tvLog.text?.toString().orEmpty()
        binding.tvLog.text = if (current.isEmpty()) message else "$current\n$message"
    }

    override fun onDestroy() {
        Log.d(TAG, "[Flow:App] onDestroy: pid=${Process.myPid()}")
        if (binderBound) {
            Log.d(TAG, "[Flow:Binder] unbindService(CalculatorService)")
            unbindService(binderServiceConnection)
            binderBound = false
        }
        if (socketServiceBound) {
            Log.d(TAG, "[Flow:Socket] unbindService(SocketServerService)")
            unbindService(socketServiceConnection)
            socketServiceBound = false
        }
        backgroundExecutor.shutdownNow()
        super.onDestroy()
    }

    private external fun nativeWaitForSocketServer(maxWaitMs: Int): Boolean
    private external fun nativeSocketAdd(a: Int, b: Int): Int
    private external fun nativeBuildCustomPayload(): ByteArray
    private external fun nativeBuildParcelPayload(): ByteArray
    private external fun nativeVerifyCustomPayload(data: ByteArray): Boolean
    private external fun nativeVerifyParcelPayload(data: ByteArray): Boolean
    private external fun nativeSocketProcessCustom(): String
    private external fun nativeSocketProcessParcel(): String
    private external fun nativeRunSerializationBenchmark(
        threadCount: Int,
        iterations: Int,
        useParcel: Boolean,
    ): String

    private external fun nativeRunSocketBenchmark(
        threadCount: Int,
        iterations: Int,
        mode: Int,
    ): String

    companion object {
        private const val TAG = "MainActivity"

        init {
            System.loadLibrary("myapplication")
        }
    }
}
