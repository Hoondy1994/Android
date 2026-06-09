package com.example.myapplication.hal

import com.example.myapplication.hal.core.HalResult
import com.example.myapplication.hal.core.IHalModule
import com.example.myapplication.hal.gpio.IGpioHal
import com.example.myapplication.hal.impl.mock.MockGpioHal
import com.example.myapplication.hal.impl.mock.MockPowerHal
import com.example.myapplication.hal.impl.mock.MockSensorHal
import com.example.myapplication.hal.impl.native.NativeGpioHal
import com.example.myapplication.hal.impl.native.NativePowerHal
import com.example.myapplication.hal.impl.native.NativeSensorHal
import com.example.myapplication.hal.native.NativeHalBridge
import com.example.myapplication.hal.power.IPowerHal
import com.example.myapplication.hal.sensor.ISensorHal
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import java.util.concurrent.ConcurrentHashMap

/**
 * HAL 服务入口：管理后端选择、模块生命周期与统一事件总线。
 */
class HalManager {

    val eventBus = HalEventBus()
    private var scope = CoroutineScope(SupervisorJob() + Dispatchers.Default)
    private val openModules = ConcurrentHashMap<HalModuleType, IHalModule>()
    private var backend: HalBackend = HalBackend.MOCK
    private var initialized = false

    fun currentBackend(): HalBackend = backend

    fun initialize(preferredBackend: HalBackend): HalResult<String> {
        if (initialized) return HalResult.Ok(currentInfo())
        backend = preferredBackend
        if (backend == HalBackend.NATIVE) {
            val rc = NativeHalBridge.nativeInit()
            if (rc != 0) {
                eventBus.publish(HalEvent.Error("HalManager", "Native init 失败 rc=$rc，回退 Mock"))
                backend = HalBackend.MOCK
            }
        }
        initialized = true
        eventBus.log("HAL 初始化完成 backend=$backend")
        return HalResult.Ok(currentInfo())
    }

    fun shutdown() {
        HalModuleType.entries.forEach { closeModule(it) }
        if (backend == HalBackend.NATIVE) {
            NativeHalBridge.nativeShutdown()
        }
        scope.cancel()
        scope = CoroutineScope(SupervisorJob() + Dispatchers.Default)
        initialized = false
        eventBus.log("HAL 已关闭")
    }

    fun switchBackend(newBackend: HalBackend): HalResult<Unit> {
        if (!initialized) return HalResult.Err(1, "请先 initialize")
        if (newBackend == backend) return HalResult.Ok(Unit)

        HalModuleType.entries.forEach { closeModule(it) }
        if (backend == HalBackend.NATIVE) {
            NativeHalBridge.nativeShutdown()
        }
        backend = newBackend
        if (backend == HalBackend.NATIVE) {
            val rc = NativeHalBridge.nativeInit()
            if (rc != 0) {
                backend = HalBackend.MOCK
                eventBus.publish(HalEvent.Error("HalManager", "Native init 失败 rc=$rc"))
                return HalResult.Err(rc, "Native init 失败")
            }
        }
        eventBus.log("切换后端 -> $backend")
        return HalResult.Ok(Unit)
    }

    fun openModule(type: HalModuleType): HalResult<IHalModule> {
        if (!initialized) return HalResult.Err(1, "HAL 未初始化")
        openModules[type]?.let { return HalResult.Ok(it) }
        val module = createModule(type)
        return module.open().let { result ->
            when (result) {
                is HalResult.Ok -> {
                    openModules[type] = module
                    eventBus.publish(HalEvent.ModuleOpened(type, backend))
                    HalResult.Ok(module)
                }
                is HalResult.Err -> HalResult.Err(result.code, result.message)
            }
        }
    }

    fun closeModule(type: HalModuleType): HalResult<Unit> {
        val module = openModules.remove(type) ?: return HalResult.Ok(Unit)
        (module as? ISensorHal)?.stopStream()
        module.close()
        eventBus.publish(HalEvent.ModuleClosed(type))
        return HalResult.Ok(Unit)
    }

    fun sensor(): HalResult<ISensorHal> = moduleOf(HalModuleType.SENSOR)
    fun gpio(): HalResult<IGpioHal> = moduleOf(HalModuleType.GPIO)
    fun power(): HalResult<IPowerHal> = moduleOf(HalModuleType.POWER)

    fun openAllDefaultModules(): HalResult<Unit> {
        for (type in HalModuleType.entries) {
            when (val r = openModule(type)) {
                is HalResult.Err -> return HalResult.Err(r.code, "打开 $type 失败: ${r.message}")
                is HalResult.Ok -> Unit
            }
        }
        return HalResult.Ok(Unit)
    }

    fun dumpAll(): String = buildString {
        appendLine("=== HAL Dump ===")
        appendLine("backend=$backend initialized=$initialized")
        val nativeVer = if (backend == HalBackend.NATIVE) {
            runCatching { NativeHalBridge.nativeGetVersion() }.getOrElse { "error: ${it.message}" }
        } else {
            "n/a"
        }
        appendLine("nativeVersion=$nativeVer")
        openModules.forEach { (type, mod) ->
            appendLine("[$type] v${mod.version} state=${mod.state}")
            mod.dumpState().forEach { (k, v) -> appendLine("  $k=$v") }
        }
    }

    private inline fun <reified T : IHalModule> moduleOf(type: HalModuleType): HalResult<T> {
        return when (val r = openModule(type)) {
            is HalResult.Ok -> (r.value as? T)?.let { HalResult.Ok(it) }
                ?: HalResult.Err(10, "模块类型不匹配")
            is HalResult.Err -> HalResult.Err(r.code, r.message)
        }
    }

    private fun createModule(type: HalModuleType): IHalModule = when (backend) {
        HalBackend.MOCK -> when (type) {
            HalModuleType.SENSOR -> MockSensorHal(eventBus, scope)
            HalModuleType.GPIO -> MockGpioHal(eventBus)
            HalModuleType.POWER -> MockPowerHal(eventBus)
        }
        HalBackend.NATIVE -> when (type) {
            HalModuleType.SENSOR -> NativeSensorHal(eventBus, scope)
            HalModuleType.GPIO -> NativeGpioHal(eventBus)
            HalModuleType.POWER -> NativePowerHal(eventBus)
        }
    }

    private fun currentInfo(): String =
        "HAL Demo | backend=$backend | ${HalModuleType.entries.size} modules"
}
