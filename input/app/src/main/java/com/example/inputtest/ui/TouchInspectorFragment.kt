package com.example.inputtest.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ScrollView
import android.widget.TextView
import androidx.fragment.app.Fragment
import com.example.inputtest.R
import com.example.inputtest.input.EventLogBuffer
import com.example.inputtest.view.MultiTouchCanvasView
import com.google.android.material.button.MaterialButton

class TouchInspectorFragment : Fragment() {

    private val log = EventLogBuffer(100)

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?,
    ): View = inflater.inflate(R.layout.fragment_touch_inspector, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        val tvLog = view.findViewById<TextView>(R.id.tvLog)
        val scroll = view.findViewById<ScrollView>(R.id.logScroll)
        val canvas = view.findViewById<MultiTouchCanvasView>(R.id.touchCanvas)

        fun refresh() {
            tvLog.text = log.text()
            scroll.post { scroll.fullScroll(View.FOCUS_DOWN) }
        }

        canvas.onEventLogged = { line ->
            log.append(line)
            refresh()
        }

        view.findViewById<MaterialButton>(R.id.btnClearLog).setOnClickListener {
            log.clear()
            refresh()
        }
    }
}
