package com.example.myapplication.ipc

object NativeIpc {
    init {
        System.loadLibrary("myapplication")
    }

    external fun nativeProcessCustomPayload(data: ByteArray): ByteArray
    external fun nativeProcessParcelPayload(data: ByteArray): ByteArray
    external fun nativeDescribePayload(data: ByteArray, useParcel: Boolean): String
}
