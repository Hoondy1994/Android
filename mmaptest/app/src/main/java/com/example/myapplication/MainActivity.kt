package com.example.myapplication

import android.os.Bundle
import android.widget.Button
import androidx.appcompat.app.AppCompatActivity
import com.example.myapplication.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val filesDir = applicationContext.filesDir.absolutePath

        binding.btnRunAll.setOnClickListener {
            runDemo { runAllMmapDemos(filesDir) }
        }

        val demoButtons = listOf(
            binding.btnDemo1 to 1,
            binding.btnDemo2 to 2,
            binding.btnDemo3 to 3,
            binding.btnDemo4 to 4,
            binding.btnDemo5 to 5,
            binding.btnDemo6 to 6,
            binding.btnDemo7 to 7,
            binding.btnDemo8 to 8,
            binding.btnDemo9 to 9,
        )
        demoButtons.forEach { (button, id) ->
            button.setOnClickListener {
                runDemo { runSingleMmapDemo(filesDir, id) }
            }
        }
    }

    private fun runDemo(block: () -> String) {
        binding.outputText.text = "运行中..."
        Thread {
            val result = try {
                block()
            } catch (e: Exception) {
                "错误: ${e.message}"
            }
            runOnUiThread {
                binding.outputText.text = result
            }
        }.start()
    }

    external fun runAllMmapDemos(filesDir: String): String
    external fun runSingleMmapDemo(filesDir: String, demoId: Int): String

    companion object {
        init {
            System.loadLibrary("myapplication")
        }
    }
}
