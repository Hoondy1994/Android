package com.example.filefuse

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.util.Log

class FileFuseCommandReceiver : BroadcastReceiver() {

    override fun onReceive(context: Context, intent: Intent) {
        val action = intent.action ?: return
        val pending = goAsync()

        Thread {
            try {
                val result = execute(context, intent, action)
                pending.resultCode = result.code
                pending.resultData = result.message
                Log.i(TAG, "${action.substringAfterLast('.')}: code=${result.code} msg=${result.message}")
            } catch (error: Exception) {
                pending.resultCode = FileFuseNative.ERR_IO
                pending.resultData = error.message ?: "error"
                Log.e(TAG, "command failed", error)
            } finally {
                pending.finish()
            }
        }.start()
    }

    private fun execute(context: Context, intent: Intent, action: String): FileFuseCommands.CommandResult {
        return when (action) {
            ACTION_MOUNT_LOCAL -> {
                val path = intent.getStringExtra(EXTRA_PATH)?.trim()
                    ?.takeIf { it.isNotEmpty() }
                    ?: FileFuseCommands.defaultLocalPath(context)
                FileFuseCommands.mountLocal(path)
            }

            ACTION_MOUNT_NFS -> {
                val host = intent.getStringExtra(EXTRA_HOST)?.trim().orEmpty()
                val export = intent.getStringExtra(EXTRA_EXPORT)?.trim().orEmpty()
                val port = intent.getIntExtra(EXTRA_PORT, 2049)
                if (host.isEmpty() || export.isEmpty()) {
                    FileFuseCommands.CommandResult(
                        code = FileFuseNative.ERR_IO,
                        message = "missing --es host or --es export"
                    )
                } else {
                    FileFuseCommands.mountNfs(host, export, port)
                }
            }

            ACTION_UNMOUNT -> FileFuseCommands.unmount()

            ACTION_STATUS -> FileFuseCommands.status()

            ACTION_LIST_DIR -> {
                val path = intent.getStringExtra(EXTRA_PATH)?.trim().orEmpty().ifEmpty { "/" }
                FileFuseCommands.listDir(path)
            }

            else -> FileFuseCommands.CommandResult(
                code = FileFuseNative.ERR_IO,
                message = "unknown action: $action"
            )
        }
    }

    companion object {
        private const val TAG = "filefuse-cmd"

        const val ACTION_MOUNT_LOCAL = "com.example.filefuse.MOUNT_LOCAL"
        const val ACTION_MOUNT_NFS = "com.example.filefuse.MOUNT_NFS"
        const val ACTION_UNMOUNT = "com.example.filefuse.UNMOUNT"
        const val ACTION_STATUS = "com.example.filefuse.STATUS"
        const val ACTION_LIST_DIR = "com.example.filefuse.LIST_DIR"

        const val EXTRA_PATH = "path"
        const val EXTRA_HOST = "host"
        const val EXTRA_EXPORT = "export"
        const val EXTRA_PORT = "port"
    }
}
