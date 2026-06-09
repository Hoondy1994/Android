package com.example.inputtest.ui

import android.os.Bundle
import android.view.KeyEvent
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.fragment.app.Fragment
import com.example.inputtest.R
import com.example.inputtest.input.EventLogBuffer
import com.google.android.material.button.MaterialButton

class KeyEventFragment : Fragment() {

    private val log = EventLogBuffer(80)

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?,
    ): View = inflater.inflate(R.layout.fragment_key_event, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        val tvLog = view.findViewById<TextView>(R.id.tvKeyLog)
        val focusTarget = view.findViewById<View>(R.id.keyFocusTarget)

        fun refresh() {
            tvLog.text = log.text()
        }

        focusTarget.setOnClickListener { it.requestFocus() }
        focusTarget.setOnKeyListener { _, keyCode, event ->
            if (event.action == KeyEvent.ACTION_DOWN) {
                log.append(formatKeyEvent(keyCode, event))
                refresh()
            }
            true
        }

        view.findViewById<MaterialButton>(R.id.btnClearKeyLog).setOnClickListener {
            log.clear()
            refresh()
        }

        focusTarget.requestFocus()
    }

    private fun formatKeyEvent(keyCode: Int, event: KeyEvent): String {
        val name = KeyEvent.keyCodeToString(keyCode)
        val action = when (event.action) {
            KeyEvent.ACTION_DOWN -> "DOWN"
            KeyEvent.ACTION_UP -> "UP"
            KeyEvent.ACTION_MULTIPLE -> "MULTIPLE"
            else -> event.action.toString()
        }
        val meta = event.metaState
        val unicode = if (event.unicodeChar != 0) event.unicodeChar.toChar() else '∅'
        return "$action $name ($keyCode) meta=0x${meta.toString(16)} char=$unicode repeat=${event.repeatCount}"
    }
}
