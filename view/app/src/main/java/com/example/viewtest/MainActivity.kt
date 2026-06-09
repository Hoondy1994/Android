package com.example.viewtest

import android.os.Bundle
import android.view.Choreographer
import androidx.appcompat.app.AppCompatActivity
import com.example.viewtest.databinding.ActivityMainBinding
import com.example.viewtest.native.ViewNativeBridge
import com.google.android.material.chip.Chip
import com.google.android.material.slider.Slider

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private var uiFrameNs = 0L

    private val uiFrameCallback = object : Choreographer.FrameCallback {
        override fun doFrame(frameTimeNs: Long) {
            if (ViewNativeBridge.isInitialized && uiFrameNs > 0L) {
                binding.waveformView.refreshFromNative()
                binding.orbitView.refreshFromNative()
                binding.statsText.text = ViewNativeBridge.nativeGetStats()
            }
            uiFrameNs = frameTimeNs
            if (!isDestroyed && !isFinishing) {
                Choreographer.getInstance().postFrameCallback(this)
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // 必须在 inflate 自定义 View 之前初始化 native，避免 attach 时调用未就绪的 JNI
        ViewNativeBridge.nativeInit(
            particleCount = 80,
            waveSamples = 128,
            orbitItems = 12,
        )
        ViewNativeBridge.markInitialized()

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setSupportActionBar(binding.toolbar)
        supportActionBar?.setDisplayShowTitleEnabled(false)

        setupSliders()
        setupLayoutChips()
    }

    override fun onStart() {
        super.onStart()
        Choreographer.getInstance().postFrameCallback(uiFrameCallback)
    }

    override fun onStop() {
        Choreographer.getInstance().removeFrameCallback(uiFrameCallback)
        super.onStop()
    }

    override fun onDestroy() {
        ViewNativeBridge.markDestroyed()
        ViewNativeBridge.nativeDestroy()
        super.onDestroy()
    }

    private fun setupSliders() {
        binding.sliderParticles.addOnChangeListener { _: Slider, value, _ ->
            ViewNativeBridge.nativeSetParticleCount(value.toInt())
        }
        binding.sliderGravity.addOnChangeListener { _: Slider, value, _ ->
            ViewNativeBridge.nativeSetGravity(value)
        }
        binding.sliderHarmonics.addOnChangeListener { _: Slider, value, _ ->
            ViewNativeBridge.nativeSetHarmonics(value.toInt())
        }
    }

    private fun setupLayoutChips() {
        val listener = { chip: Chip ->
            val mode = when (chip.id) {
                R.id.chip_spiral -> ViewNativeBridge.LAYOUT_MODE_SPIRAL
                R.id.chip_heart -> ViewNativeBridge.LAYOUT_MODE_HEART
                else -> ViewNativeBridge.LAYOUT_MODE_CIRCLE
            }
            ViewNativeBridge.nativeSetLayoutMode(mode)
            binding.orbitView.refreshFromNative()
        }
        binding.chipCircle.setOnClickListener { listener(binding.chipCircle) }
        binding.chipSpiral.setOnClickListener { listener(binding.chipSpiral) }
        binding.chipHeart.setOnClickListener { listener(binding.chipHeart) }
    }
}
