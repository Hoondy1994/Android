package com.example.myapplication.hal.core

enum class HalDeviceState {
    UNINITIALIZED,
    CLOSED,
    OPENING,
    OPEN,
    STREAMING,
    ERROR,
    SUSPENDED;

    fun canTransitionTo(next: HalDeviceState): Boolean = when (this) {
        UNINITIALIZED -> next in setOf(CLOSED, OPENING, ERROR)
        CLOSED -> next in setOf(OPENING, ERROR)
        OPENING -> next in setOf(OPEN, CLOSED, ERROR)
        OPEN -> next in setOf(STREAMING, SUSPENDED, CLOSED, ERROR)
        STREAMING -> next in setOf(OPEN, CLOSED, ERROR)
        SUSPENDED -> next in setOf(OPEN, CLOSED, ERROR)
        ERROR -> next in setOf(CLOSED, UNINITIALIZED)
    }
}
