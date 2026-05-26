package com.example.myapplication

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.example.myapplication.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.runSandboxButton.setOnClickListener {
            runSandboxDemo()
        }
    }

    private fun runSandboxDemo() {
        binding.runSandboxButton.isEnabled = false
        binding.sampleText.text = getString(R.string.sandbox_running)

        Thread {
            val demoDir = filesDir.resolve("sandbox_demo").absolutePath
            val output = try {
                runSandboxDemo(demoDir)
            } catch (e: UnsatisfiedLinkError) {
                "Native 库加载失败: ${e.message}"
            } catch (e: Exception) {
                "演示异常: ${e.message}"
            }

            runOnUiThread {
                binding.sampleText.text = output
                binding.runSandboxButton.isEnabled = true
            }
        }.start()
    }

    private external fun runSandboxDemo(baseDir: String): String

    companion object {
        init {
            System.loadLibrary("myapplication")
        }
    }
}
