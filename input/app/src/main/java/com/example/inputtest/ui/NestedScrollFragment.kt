package com.example.inputtest.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.widget.HorizontalScrollView
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import com.example.inputtest.R
import androidx.appcompat.widget.SwitchCompat

class NestedScrollFragment : Fragment() {

    private var disallowIntercept = true

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?,
    ): View = inflater.inflate(R.layout.fragment_nested_scroll, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        val content = view.findViewById<LinearLayout>(R.id.nestedContent)
        val status = view.findViewById<TextView>(R.id.tvNestedStatus)
        val outer = view.findViewById<ScrollView>(R.id.outerScroll)
        val horizontal = view.findViewById<HorizontalScrollView>(R.id.innerHorizontal)

        val colors = intArrayOf(
            R.color.pointer_0,
            R.color.pointer_1,
            R.color.pointer_2,
            R.color.pointer_3,
        )

        for (col in 0 until 4) {
            val column = ScrollView(requireContext()).apply {
                layoutParams = LinearLayout.LayoutParams(dp(220), dp(520)).apply {
                    marginEnd = dp(12)
                }
                isFillViewport = true
                setOnTouchListener(touchLogger("垂直列#$col", status, ::disallowIntercept))
            }
            val inner = LinearLayout(requireContext()).apply {
                orientation = LinearLayout.VERTICAL
            }
            for (row in 0 until 8) {
                val block = View(requireContext()).apply {
                    layoutParams = LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        dp(140),
                    ).apply { bottomMargin = dp(8) }
                    setBackgroundColor(
                        ContextCompat.getColor(requireContext(), colors[(col + row) % colors.size]),
                    )
                    alpha = 0.85f
                }
                inner.addView(block)
            }
            column.addView(inner)
            content.addView(column)
        }

        horizontal.setOnTouchListener(touchLogger("横向 ScrollView", status, ::disallowIntercept))
        outer.setOnTouchListener(touchLogger("外层垂直 ScrollView", status, ::disallowIntercept))

        view.findViewById<SwitchCompat>(R.id.switchDisallowIntercept).apply {
            isChecked = disallowIntercept
            setOnCheckedChangeListener { _, checked ->
                disallowIntercept = checked
                text = if (checked) {
                    getString(R.string.disallow_intercept_on)
                } else {
                    getString(R.string.disallow_intercept_off)
                }
                status.text = if (checked) {
                    "子 View 在 DOWN 时 requestDisallowInterceptTouchEvent(true)"
                } else {
                    "父 ScrollView 可参与嵌套滑动仲裁"
                }
            }
        }
    }

    private fun touchLogger(
        name: String,
        status: TextView,
        disallow: () -> Boolean,
    ): View.OnTouchListener = View.OnTouchListener { v, event ->
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                if (disallow()) {
                    v.parent?.requestDisallowInterceptTouchEvent(true)
                }
                status.text = "$name ← DOWN @ (${event.x.toInt()}, ${event.y.toInt()})"
            }
            MotionEvent.ACTION_MOVE -> {
                status.text = "$name ← MOVE Δ=(${event.x.toInt()}, ${event.y.toInt()})"
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                v.parent?.requestDisallowInterceptTouchEvent(false)
                status.text = "$name ← ${if (event.actionMasked == MotionEvent.ACTION_UP) "UP" else "CANCEL"}"
            }
        }
        false
    }

    private fun dp(value: Int): Int =
        (value * resources.displayMetrics.density).toInt()
}
