package com.example.san

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.example.san.databinding.ActivityMainBinding
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private val timeFormat = SimpleDateFormat("HH:mm:ss", Locale.getDefault())

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.infoText.text = SandboxDemo.appInfo(this)
        appendLog("就绪。点击下方按钮，对比「有沙箱保护」与「无沙箱限制」的行为差异。")

        binding.btnPrivateFile.setOnClickListener {
            runDemo("① 应用私有目录（Android 沙箱）") {
                SandboxDemo.demoPrivateFileAccess(this)
            }
        }

        binding.btnCrossBoundary.setOnClickListener {
            runDemo("② 越界访问其他应用数据") {
                SandboxDemo.demoCrossBoundaryRead(this)
            }
        }

        binding.btnNativeOpen.setOnClickListener {
            val testFile = SandboxDemo.prepareNativeTestFile(this)
            runDemo("③ Native 无 Seccomp（可正常 open）") {
                SandboxNative.runNativeFileRead(testFile.absolutePath)
            }
        }

        binding.btnSeccomp.setOnClickListener {
            val testFile = SandboxDemo.prepareNativeTestFile(this)
            runDemo("④ Native Seccomp 沙箱（拦截 open）") {
                SandboxNative.runSeccompBlockedRead(testFile.absolutePath)
            }
        }

        binding.btnClearLog.setOnClickListener {
            binding.logText.text = ""
        }
    }

    private fun runDemo(title: String, block: () -> String) {
        val result = block()
        appendLog("▶ $title\n$result")
    }

    private fun appendLog(message: String) {
        val timestamp = timeFormat.format(Date())
        val entry = "[$timestamp]\n$message\n\n"
        binding.logText.append(entry)
        binding.logScroll.post {
            binding.logScroll.fullScroll(android.view.View.FOCUS_DOWN)
        }
    }
}
