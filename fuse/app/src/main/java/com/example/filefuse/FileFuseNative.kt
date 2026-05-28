package com.example.filefuse

import org.json.JSONArray

object FileFuseNative {
    init {
        System.loadLibrary("filefuse")
    }

    const val ERR_OK = 0
    const val ERR_NOT_FOUND = -2
    const val ERR_IO = -5
    const val ERR_NOT_CONNECTED = -100

    external fun nativeMountLocal(rootPath: String): Int
    external fun nativeMountNfs(host: String, exportPath: String, port: Int): Int
    external fun nativeUnmount()
    external fun nativeGetStatus(): String
    external fun nativeListDir(path: String): String
    external fun nativeReadTextFile(path: String): String

    data class DirEntry(val name: String, val isDirectory: Boolean)

    fun listDir(path: String): List<DirEntry> {
        val json = nativeListDir(path)
        if (json.isBlank() || json == "[]") {
            return emptyList()
        }

        val array = JSONArray(json)
        return buildList {
            for (index in 0 until array.length()) {
                val item = array.getJSONObject(index)
                add(
                    DirEntry(
                        name = item.getString("name"),
                        isDirectory = item.getBoolean("dir")
                    )
                )
            }
        }
    }
}
