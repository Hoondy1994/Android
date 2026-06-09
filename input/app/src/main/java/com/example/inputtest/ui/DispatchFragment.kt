package com.example.inputtest.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import android.widget.TextView
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import com.example.inputtest.R
import com.example.inputtest.input.EventLogBuffer
import com.example.inputtest.view.DispatchTargetView
import com.example.inputtest.view.TouchDispatchLayout
import com.google.android.material.button.MaterialButton
import androidx.appcompat.widget.SwitchCompat

class DispatchFragment : Fragment() {

    private val log = EventLogBuffer(120)

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?,
    ): View = inflater.inflate(R.layout.fragment_dispatch, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        val tvLog = view.findViewById<TextView>(R.id.tvDispatchLog)
        val root = view.findViewById<TouchDispatchLayout>(R.id.dispatchRoot)

        fun append(line: String) {
            log.append(line)
            tvLog.text = log.text()
        }

        root.layerName = "RootLayout"
        root.onDispatchLogged = ::append

        val colors = intArrayOf(
            R.color.pointer_0,
            R.color.pointer_1,
            R.color.pointer_2,
        )
        val names = arrayOf("Child-A", "Child-B", "Child-C")
        val margins = arrayOf(24, 120, 216)

        names.forEachIndexed { i, name ->
            val child = DispatchTargetView(requireContext()).apply {
                layerName = name
                fillColor = ContextCompat.getColor(requireContext(), colors[i])
                onDispatchLogged = ::append
                layoutParams = FrameLayout.LayoutParams(dp(200), dp(120)).apply {
                    leftMargin = dp(margins[i])
                    topMargin = dp(margins[i])
                }
            }
            root.addView(child)
        }

        view.findViewById<SwitchCompat>(R.id.switchParentIntercept).setOnCheckedChangeListener { _, checked ->
            root.interceptTouches = checked
            append(if (checked) "--- 父布局开启 MOVE 拦截 ---" else "--- 父布局关闭拦截 ---")
        }

        view.findViewById<MaterialButton>(R.id.btnClearDispatchLog).setOnClickListener {
            log.clear()
            tvLog.text = ""
        }
    }

    private fun dp(value: Int): Int =
        (value * resources.displayMetrics.density).toInt()
}
