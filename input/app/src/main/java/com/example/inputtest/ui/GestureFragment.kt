package com.example.inputtest.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.fragment.app.Fragment
import com.example.inputtest.R
import com.example.inputtest.input.EventLogBuffer
import com.example.inputtest.view.GestureCanvasView

class GestureFragment : Fragment() {

    private val log = EventLogBuffer(60)

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?,
    ): View = inflater.inflate(R.layout.fragment_gesture, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        val tvLog = view.findViewById<TextView>(R.id.tvGestureLog)
        view.findViewById<GestureCanvasView>(R.id.gestureCanvas).onGestureLogged = { line ->
            log.append(line)
            tvLog.text = log.text()
        }
    }
}
