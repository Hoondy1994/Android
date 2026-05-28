package com.example.filefuse

import android.content.Context
import java.io.File

object FileFuseCommands {

    fun ensureDemoDir(context: Context): File {
        val demoDir = File(context.filesDir, "vfs-demo")
        if (!demoDir.exists()) {
            demoDir.mkdirs()
            File(demoDir, "readme.txt").writeText("FileFuse VFS demo\nFUSE -> VFS -> local/NFS\n")
            File(demoDir, "docs").mkdirs()
            File(demoDir, "docs/notes.txt").writeText("hello from local backend")
        }
        return demoDir
    }

    fun defaultLocalPath(context: Context): String = ensureDemoDir(context).absolutePath

    fun mountLocal(path: String): CommandResult {
        val code = FileFuseNative.nativeMountLocal(path)
        return CommandResult(
            code = code,
            message = if (code == FileFuseNative.ERR_OK) {
                "mounted local: $path"
            } else {
                "mount local failed: $code"
            }
        )
    }

    fun mountNfs(host: String, exportPath: String, port: Int): CommandResult {
        val code = FileFuseNative.nativeMountNfs(host, exportPath, port)
        return CommandResult(
            code = code,
            message = if (code == FileFuseNative.ERR_OK) {
                "mounted nfs: $host:$exportPath"
            } else {
                "mount nfs failed: $code"
            }
        )
    }

    fun unmount(): CommandResult {
        FileFuseNative.nativeUnmount()
        return CommandResult(code = FileFuseNative.ERR_OK, message = "unmounted")
    }

    fun status(): CommandResult {
        return CommandResult(
            code = FileFuseNative.ERR_OK,
            message = FileFuseNative.nativeGetStatus()
        )
    }

    fun listDir(path: String): CommandResult {
        val json = FileFuseNative.nativeListDir(path)
        return CommandResult(code = FileFuseNative.ERR_OK, message = json)
    }

    data class CommandResult(val code: Int, val message: String)
}
