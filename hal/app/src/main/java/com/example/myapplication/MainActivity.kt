package com.example.myapplication

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.example.myapplication.databinding.ActivityMainBinding
import com.example.myapplication.hal.HalBackend
import com.example.myapplication.hal.HalEvent
import com.example.myapplication.hal.HalManager
import com.example.myapplication.hal.HalModuleType
import com.example.myapplication.hal.core.HalResult
import com.example.myapplication.hal.core.onFailure
import com.example.myapplication.hal.core.onSuccess
import com.example.myapplication.hal.gpio.GpioDirection
import com.example.myapplication.hal.gpio.IGpioHal
import com.example.myapplication.hal.power.IPowerHal
import com.example.myapplication.hal.power.PowerRail
import com.google.android.material.button.MaterialButtonToggleGroup
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private val halManager = HalManager()
    private val mainHandler = Handler(Looper.getMainLooper())
    private var streaming = false
    private var gpioPin7Level = 0
    private var suppressVerboseEvents = false
    private var halReady = false
    private val logLines = ArrayDeque<String>(80)
    private var unsubscribeEvents: (() -> Unit)? = null
    private var backendToggleListener: MaterialButtonToggleGroup.OnButtonCheckedListener? = null

    private val flushLogRunnable = Runnable {
        if (!isUiSafe()) return@Runnable
        binding.logText.text = logLines.joinToString("\n")
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        setSupportActionBar(binding.toolbar)

        setupEventBus()
        setupActions()
        setupBackendToggle()
        startHalAsync(HalBackend.MOCK)
    }

    override fun onPause() {
        if (streaming) stopStream()
        super.onPause()
    }

    override fun onDestroy() {
        unsubscribeEvents?.invoke()
        unsubscribeEvents = null
        mainHandler.removeCallbacks(flushLogRunnable)
        if (streaming) stopStream()
        halManager.shutdown()
        super.onDestroy()
    }

    private fun setupEventBus() {
        unsubscribeEvents = halManager.eventBus.subscribe { event ->
            mainHandler.post {
                if (isUiSafe()) handleHalEvent(event)
            }
        }
    }

    private fun setupBackendToggle() {
        backendToggleListener =
            MaterialButtonToggleGroup.OnButtonCheckedListener { _, checkedId, isChecked ->
                if (!isChecked || !halReady) return@OnButtonCheckedListener
                val backend = if (checkedId == R.id.backend_native) {
                    HalBackend.NATIVE
                } else {
                    HalBackend.MOCK
                }
                switchBackendAsync(backend)
            }
        binding.backendToggle.addOnButtonCheckedListener(backendToggleListener!!)
    }

    private fun setBackendToggleChecked(buttonId: Int) {
        backendToggleListener?.let { binding.backendToggle.removeOnButtonCheckedListener(it) }
        binding.backendToggle.check(buttonId)
        backendToggleListener?.let { binding.backendToggle.addOnButtonCheckedListener(it) }
    }

    private fun startHalAsync(backend: HalBackend) {
        binding.statusText.text = getString(R.string.hal_status_starting)
        lifecycleScope.launch(Dispatchers.Default) {
            suppressVerboseEvents = true
            val initResult = halManager.initialize(backend)
            val openResult: HalResult<Unit> = when (initResult) {
                is HalResult.Ok -> halManager.openAllDefaultModules()
                is HalResult.Err -> HalResult.Err(initResult.code, initResult.message)
            }
            withContext(Dispatchers.Main) {
                suppressVerboseEvents = false
                when (initResult) {
                    is HalResult.Ok -> {
                        binding.statusText.text = initResult.value
                        appendLog("INFO", initResult.value)
                    }
                    is HalResult.Err -> toast(initResult.message)
                }
                if (openResult is HalResult.Err) {
                    toast(openResult.message)
                } else {
                    halManager.power().onSuccess { refreshPowerStatus(it) }
                }
                halReady = true
                appendLog("INFO", "HAL 就绪，可操作")
            }
        }
    }

    private fun switchBackendAsync(backend: HalBackend) {
        halReady = false
        binding.statusText.text = getString(R.string.hal_status_switching)
        lifecycleScope.launch(Dispatchers.Default) {
            suppressVerboseEvents = true
            val switchResult = halManager.switchBackend(backend)
            val openResult: HalResult<Unit> = when (switchResult) {
                is HalResult.Ok -> halManager.openAllDefaultModules()
                is HalResult.Err -> HalResult.Err(switchResult.code, switchResult.message)
            }
            withContext(Dispatchers.Main) {
                suppressVerboseEvents = false
                when (switchResult) {
                    is HalResult.Ok -> {
                        binding.statusText.text = halManager.dumpAll().lines().firstOrNull()
                        appendLog("INFO", "切换后端 -> $backend")
                        halManager.power().onSuccess { refreshPowerStatus(it) }
                        halReady = true
                    }
                    is HalResult.Err -> {
                        toast(switchResult.message)
                        val restoreId = if (halManager.currentBackend() == HalBackend.MOCK) {
                            R.id.backend_mock
                        } else {
                            R.id.backend_native
                        }
                        setBackendToggleChecked(restoreId)
                        halReady = true
                    }
                }
                if (openResult is HalResult.Err) {
                    toast(openResult.message)
                }
            }
        }
    }

    private fun setupActions() {
        binding.btnSensorOpen.setOnClickListener {
            if (!halReady) return@setOnClickListener
            lifecycleScope.launch(Dispatchers.Default) {
                val result = halManager.openModule(HalModuleType.SENSOR)
                withContext(Dispatchers.Main) {
                    result.onSuccess {
                        appendLog("INFO", "传感器模块已打开 (${it.version})")
                    }.onFailure { _, m -> toast(m) }
                }
            }
        }

        binding.btnSensorRead.setOnClickListener {
            if (!halReady) return@setOnClickListener
            lifecycleScope.launch(Dispatchers.Default) {
                halManager.sensor().onSuccess { sensor ->
                    sensor.readSample().onSuccess { sample ->
                        withContext(Dispatchers.Main) {
                            binding.sensorReading.text = sample.toString()
                        }
                    }.onFailure { _, m -> withContext(Dispatchers.Main) { toast(m) } }
                }.onFailure { _, m -> withContext(Dispatchers.Main) { toast(m) } }
            }
        }

        binding.btnSensorStream.setOnClickListener {
            if (!halReady) return@setOnClickListener
            if (streaming) {
                stopStream()
                return@setOnClickListener
            }
            halManager.sensor().onSuccess { sensor ->
                sensor.calibrate(-0.5f, 2f)
                sensor.startStream(800) { sample ->
                    mainHandler.post {
                        if (isUiSafe()) binding.sensorReading.text = sample.toString()
                    }
                }.onSuccess {
                    streaming = true
                    binding.btnSensorStream.text = getString(R.string.hal_stream) + " ■"
                    appendLog("INFO", "传感器流已启动 800ms")
                }.onFailure { _, m -> toast(m) }
            }.onFailure { _, m -> toast(m) }
        }

        binding.btnGpioSetup.setOnClickListener {
            if (!halReady) return@setOnClickListener
            lifecycleScope.launch(Dispatchers.Default) {
                halManager.openModule(HalModuleType.GPIO).onSuccess { module ->
                    val gpio = module as IGpioHal
                    gpio.configurePin(7, GpioDirection.OUTPUT, pullUp = false)
                    gpio.writePin(7, 0)
                    withContext(Dispatchers.Main) {
                        gpioPin7Level = 0
                        updateGpioDisplay(gpio)
                        appendLog("INFO", "GPIO Pin7 配置为输出")
                    }
                }.onFailure { _, m -> withContext(Dispatchers.Main) { toast(m) } }
            }
        }

        binding.btnGpioToggle.setOnClickListener {
            if (!halReady) return@setOnClickListener
            lifecycleScope.launch(Dispatchers.Default) {
                halManager.gpio().onSuccess { gpio ->
                    gpioPin7Level = 1 - gpioPin7Level
                    gpio.writePin(7, gpioPin7Level).onSuccess {
                        withContext(Dispatchers.Main) { updateGpioDisplay(gpio) }
                    }.onFailure { _, m -> withContext(Dispatchers.Main) { toast(m) } }
                }.onFailure { _, m -> withContext(Dispatchers.Main) { toast(m) } }
            }
        }

        binding.btnPowerSuspend.setOnClickListener {
            if (!halReady) return@setOnClickListener
            lifecycleScope.launch(Dispatchers.Default) {
                halManager.power().onSuccess { power ->
                    power.suspendSystem()
                    withContext(Dispatchers.Main) {
                        refreshPowerStatus(power)
                        appendLog("WARN", "系统进入 SUSPEND")
                    }
                }.onFailure { _, m -> withContext(Dispatchers.Main) { toast(m) } }
            }
        }

        binding.btnPowerWake.setOnClickListener {
            if (!halReady) return@setOnClickListener
            lifecycleScope.launch(Dispatchers.Default) {
                halManager.power().onSuccess { power ->
                    power.wakeAll()
                    withContext(Dispatchers.Main) {
                        refreshPowerStatus(power)
                        appendLog("INFO", "电源轨已全部唤醒")
                    }
                }.onFailure { _, m -> withContext(Dispatchers.Main) { toast(m) } }
            }
        }

        binding.btnDump.setOnClickListener {
            if (!halReady) return@setOnClickListener
            lifecycleScope.launch(Dispatchers.Default) {
                val dump = halManager.dumpAll()
                withContext(Dispatchers.Main) {
                    binding.statusText.text = dump
                    appendLog("INFO", "Dump 完成 (${dump.lines().size} 行)")
                }
            }
        }

        binding.btnClearLog.setOnClickListener {
            logLines.clear()
            binding.logText.text = ""
        }
    }

    private fun stopStream() {
        halManager.sensor().onSuccess { it.stopStream() }
        streaming = false
        binding.btnSensorStream.text = getString(R.string.hal_stream)
        appendLog("INFO", "传感器流已停止")
    }

    private fun updateGpioDisplay(gpio: IGpioHal) {
        val port = gpio.readPort(0xFF80).getOrNull() ?: 0
        val pin7 = gpio.readPin(7).getOrNull() ?: -1
        binding.gpioStatus.text = "Pin7=$pin7  port(7..15)=0x${port.toString(16)}"
    }

    private fun refreshPowerStatus(power: IPowerHal) {
        if (!isUiSafe()) return
        val lines = PowerRail.entries.mapNotNull { rail ->
            power.getRailStatus(rail).getOrNull()?.let {
                "${rail.name}: ${it.state} ${it.milliwatts}mW"
            }
        }
        binding.powerStatus.text = lines.joinToString("\n")
    }

    private fun handleHalEvent(event: HalEvent) {
        when (event) {
            is HalEvent.Log -> appendLog(event.level, event.message)
            is HalEvent.Error -> appendLog("ERROR", "${event.source}: ${event.message}")
            is HalEvent.SensorSampleReceived -> Unit
            is HalEvent.GpioChanged -> appendLog("GPIO", "Pin${event.pin} -> ${event.level}")
            is HalEvent.PowerTransition -> {
                appendLog("PWR", "${event.rail} -> ${event.state}")
            }
            is HalEvent.StateChanged -> {
                if (!suppressVerboseEvents) {
                    appendLog("STATE", "${event.type.id} ${event.from}->${event.to}")
                }
            }
            is HalEvent.ModuleOpened -> {
                if (!suppressVerboseEvents) {
                    appendLog("INFO", "模块打开 ${event.type.displayName} [${event.backend.id}]")
                }
            }
            is HalEvent.ModuleClosed -> appendLog("INFO", "模块关闭 ${event.type.displayName}")
        }
    }

    private fun appendLog(level: String, message: String) {
        if (!isUiSafe()) return
        val line = "[${System.currentTimeMillis() % 100000}] $level $message"
        logLines.addLast(line)
        while (logLines.size > 60) {
            logLines.removeFirst()
        }
        mainHandler.removeCallbacks(flushLogRunnable)
        mainHandler.postDelayed(flushLogRunnable, 80L)
    }

    private fun isUiSafe(): Boolean = !isDestroyed && ::binding.isInitialized

    private fun toast(msg: String) {
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
    }
}
