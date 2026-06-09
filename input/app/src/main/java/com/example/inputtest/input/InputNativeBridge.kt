package com.example.inputtest.input

object InputNativeBridge {
    init {
        System.loadLibrary("inputtest")
    }

    external fun actionName(action: Int): String
    external fun toolTypeName(toolType: Int): String
    external fun axisName(axis: Int): String
}
