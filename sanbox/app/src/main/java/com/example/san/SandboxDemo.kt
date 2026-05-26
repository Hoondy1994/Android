package com.example.san

import android.content.Context
import android.os.Process
import java.io.File

object SandboxDemo {

    fun appInfo(context: Context): String {
        val filesDir = context.filesDir.absolutePath
        val uid = Process.myUid()
        val userId = uid / 100000
        return buildString {
            appendLine("应用 UID: $uid (userId=$userId)")
            appendLine("私有目录: $filesDir")
            appendLine("其他应用无法直接读写此目录")
        }.trimEnd()
    }

    fun demoPrivateFileAccess(context: Context): String {
        val file = File(context.filesDir, "sandbox_secret.txt")
        val secret = "token=${System.currentTimeMillis()}"

        return try {
            file.writeText(secret)
            val readBack = file.readText()
            if (readBack == secret) {
                "成功：写入并读回私有文件\n路径: ${file.absolutePath}\n内容: $readBack"
            } else {
                "异常：读回内容不一致"
            }
        } catch (e: Exception) {
            "失败: ${e.message}"
        }
    }

    fun demoCrossBoundaryRead(context: Context): String {
        val ownDir = context.filesDir.absolutePath
        val foreignDir = "/data/data/com.android.settings"
        val foreignFile = File(foreignDir, "shared_prefs")

        return buildString {
            appendLine("尝试读取其他应用目录:")
            appendLine("目标: $foreignDir")
            appendLine("结果: ${if (foreignFile.canRead()) "可读（异常）" else "不可读 ✓"}")
            appendLine()
            appendLine("对比本应用私有目录:")
            appendLine("目标: $ownDir")
            appendLine("结果: ${if (File(ownDir).canRead()) "可读 ✓" else "不可读"}")
            appendLine()
            append("结论: Android 按 UID 隔离 /data/data/ 下各应用数据")
        }
    }

    fun prepareNativeTestFile(context: Context): File {
        val file = File(context.cacheDir, "native_test.txt")
        file.writeText("native sandbox demo payload")
        return file
    }
}
