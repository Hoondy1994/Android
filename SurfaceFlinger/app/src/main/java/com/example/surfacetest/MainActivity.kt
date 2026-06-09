package com.example.surfacetest

import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import com.example.surfacetest.databinding.ActivityMainBinding
import com.example.surfacetest.native.SurfaceNativeBridge
import com.example.surfacetest.stats.FrameStatsCollector
import kotlin.math.roundToInt

/**
 * SurfaceFlinger 合成压力实验台：
 * - 底层 Native EGL Surface（swapBuffers → BufferQueue）
 * - 中层半透明 Canvas Surface（unlockCanvasAndPost）
 * - 顶层 Z-Order OnTop 小窗 Surface
 * - FrameMetrics + Choreographer 观测合成耗时与 jank
 */
class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var frameStats: FrameStatsCollector
    private val uiHandler = Handler(Looper.getMainLooper())
    private val statsRunnable = object : Runnable {
        override fun run() {
            refreshStats()
            uiHandler.postDelayed(this, 500L)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        frameStats = FrameStatsCollector(window)
        wireControls()
        uiHandler.post(statsRunnable)
    }

    override fun onResume() {
        super.onResume()
        frameStats.start()
    }

    override fun onPause() {
        frameStats.stop()
        super.onPause()
    }

    override fun onDestroy() {
        uiHandler.removeCallbacks(statsRunnable)
        binding.glesSurface.releaseRenderer()
        super.onDestroy()
    }

    private fun wireControls() {
        val gles = binding.glesSurface
        val canvas = binding.canvasSurface
        val overlay = binding.overlaySurface

        binding.modeChips.setOnCheckedStateChangeListener { _, checkedIds ->
            val mode = when (checkedIds.firstOrNull()) {
                R.id.chip_grid -> SurfaceNativeBridge.MODE_GRID
                R.id.chip_alpha -> SurfaceNativeBridge.MODE_PULSE_ALPHA
                else -> SurfaceNativeBridge.MODE_PLASMA
            }
            gles.renderMode = mode
        }
        binding.stressSlider.addOnChangeListener { _, value, _ ->
            gles.stressLevel = value.roundToInt()
        }

        binding.ringSlider.addOnChangeListener { _, value, _ ->
            canvas.ringCount = value.roundToInt()
        }

        binding.alphaSlider.addOnChangeListener { _, value, _ ->
            canvas.layerAlpha = value.roundToInt()
        }

        binding.switchGles.setOnCheckedChangeListener { _, on ->
            gles.renderingEnabled = on
            gles.visibility = if (on) View.VISIBLE else View.INVISIBLE
        }

        binding.switchCanvas.setOnCheckedChangeListener { _, on ->
            canvas.renderingEnabled = on
            canvas.visibility = if (on) View.VISIBLE else View.INVISIBLE
        }

        binding.switchOverlay.setOnCheckedChangeListener { _, on ->
            overlay.renderingEnabled = on
            overlay.visibility = if (on) View.VISIBLE else View.INVISIBLE
        }

        binding.switchFixedSize.setOnCheckedChangeListener { _, on ->
            if (on) {
                gles.setFixedBufferSize(1280, 720)
            } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                gles.holder.setSizeFromLayout()
            } else {
                gles.setFixedBufferSize(gles.width.coerceAtLeast(1), gles.height.coerceAtLeast(1))
            }
        }

        binding.btnStressBurst.setOnClickListener {
            val prevStress = gles.stressLevel
            val prevRings = canvas.ringCount
            gles.stressLevel = 16
            canvas.ringCount = 40
            uiHandler.postDelayed({
                gles.stressLevel = prevStress
                canvas.ringCount = prevRings
                binding.stressSlider.value = prevStress.toFloat()
                binding.ringSlider.value = prevRings.toFloat()
            }, 3000L)
        }
    }

    private fun refreshStats() {
        binding.statsText.text = frameStats.snapshotLine()
        val native = binding.glesSurface.readNativeStats()
        if (native.size >= 5) {
            binding.nativeStatsText.text = buildString {
                append("EGL ")
                append(native[2])
                append('x')
                append(native[3])
                append(" draw=")
                append(native[1])
                append("µs swap=")
                append(if (native[4] == 1L) "OK" else "FAIL")
                append("  canvasPosted=")
                append(binding.canvasSurface.postedFrames)
            }
        }
    }
}
