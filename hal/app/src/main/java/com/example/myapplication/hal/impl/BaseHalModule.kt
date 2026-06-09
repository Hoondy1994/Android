package com.example.myapplication.hal.impl

import com.example.myapplication.hal.HalEvent
import com.example.myapplication.hal.HalEventBus
import com.example.myapplication.hal.HalModuleType
import com.example.myapplication.hal.core.HalCapability
import com.example.myapplication.hal.core.HalDeviceState
import com.example.myapplication.hal.core.HalResult
import com.example.myapplication.hal.core.IHalModule

abstract class BaseHalModule(
    protected val moduleType: HalModuleType,
    protected val eventBus: HalEventBus,
    override val capabilities: Set<HalCapability>
) : IHalModule {

    override val moduleId: String = moduleType.id
    override var state: HalDeviceState = HalDeviceState.UNINITIALIZED

    protected fun transition(next: HalDeviceState): HalResult<Unit> {
        if (!state.canTransitionTo(next)) {
            return HalResult.Err(-1, "非法状态迁移: $state -> $next")
        }
        val prev = state
        state = next
        eventBus.publish(
            HalEvent.StateChanged(moduleType, prev.name, next.name)
        )
        return HalResult.Ok(Unit)
    }

    override fun reset(): HalResult<Unit> {
        close()
        state = HalDeviceState.UNINITIALIZED
        return HalResult.Ok(Unit)
    }
}
