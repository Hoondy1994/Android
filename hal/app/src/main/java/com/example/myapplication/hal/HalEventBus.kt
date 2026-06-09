package com.example.myapplication.hal

import java.util.concurrent.CopyOnWriteArrayList

class HalEventBus {
    private val listeners = CopyOnWriteArrayList<(HalEvent) -> Unit>()

    fun subscribe(listener: (HalEvent) -> Unit): () -> Unit {
        listeners.add(listener)
        return { listeners.remove(listener) }
    }

    fun publish(event: HalEvent) {
        listeners.forEach { it.invoke(event) }
    }

    fun log(message: String, level: String = "INFO") {
        publish(HalEvent.Log(level, message))
    }
}
