package com.example.myapplication.hal.core

sealed class HalResult<out T> {
    data class Ok<T>(val value: T) : HalResult<T>()
    data class Err(val code: Int, val message: String) : HalResult<Nothing>()

    fun getOrNull(): T? = (this as? Ok)?.value
    fun errorOrNull(): String? = (this as? Err)?.message
}

inline fun <T> HalResult<T>.onSuccess(block: (T) -> Unit): HalResult<T> {
    if (this is HalResult.Ok) block(value)
    return this
}

inline fun <T> HalResult<T>.onFailure(block: (Int, String) -> Unit): HalResult<T> {
    if (this is HalResult.Err) block(code, message)
    return this
}
