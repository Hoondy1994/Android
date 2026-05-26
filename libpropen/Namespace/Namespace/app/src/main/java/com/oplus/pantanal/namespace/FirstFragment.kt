package com.oplus.pantanal.namespace

import android.os.Bundle
import android.util.Log
import android.widget.Toast
import com.oplus.pantanal.namespace.BuildConfig
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.navigation.fragment.findNavController
import com.oplus.pantanal.namespace.databinding.FragmentFirstBinding
import com.oplus.pantanal.namespace.demo.PreopenBridge

/**
 * A simple [Fragment] subclass as the default destination in the navigation.
 */
class FirstFragment : Fragment() {

    companion object {
        private const val TAG = "PreopenDemo"
    }

    private var _binding: FragmentFirstBinding? = null

    private val binding get() = _binding!!

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentFirstBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.buttonPreopenDemo.setOnClickListener {
            runPreopenDemo()
        }

        binding.buttonFirst.setOnClickListener {
            findNavController().navigate(R.id.action_FirstFragment_to_SecondFragment)
        }

        if (BuildConfig.DEBUG) {
            runPreopenDemo()
        }
    }

    private fun runPreopenDemo() {
        Toast.makeText(requireContext(), "Preopen Demo 运行中，请看 Logcat: PreopenDemo", Toast.LENGTH_SHORT).show()
        Log.i(TAG, "button clicked, starting preopen demo")
        binding.buttonPreopenDemo.isEnabled = false
        binding.textPreopenResult.text = getString(R.string.preopen_demo_running)

        Thread {
            val demoDir = requireContext().filesDir.resolve("preopen_demo").absolutePath
            Log.i(TAG, "calling PreopenBridge.runDemo demoDir=$demoDir")
            val result = try {
                PreopenBridge.runDemo(demoDir)
            } catch (e: UnsatisfiedLinkError) {
                Log.e(TAG, "UnsatisfiedLinkError", e)
                "Native 库加载失败: ${e.message}"
            } catch (e: Exception) {
                Log.e(TAG, "demo failed", e)
                "Demo 异常: ${e.message}"
            }

            Log.i(TAG, "runDemo finished, result length=${result.length}")
            requireActivity().runOnUiThread {
                binding.textPreopenResult.text = result
                binding.buttonPreopenDemo.isEnabled = true
            }
        }.start()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
