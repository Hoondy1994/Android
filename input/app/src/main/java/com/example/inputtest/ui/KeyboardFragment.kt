package com.example.inputtest.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager
import android.widget.TextView
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.fragment.app.Fragment
import com.example.inputtest.R
import com.example.inputtest.input.EventLogBuffer
import com.google.android.material.button.MaterialButton
import com.google.android.material.textfield.TextInputEditText

class KeyboardFragment : Fragment() {

    private val log = EventLogBuffer(40)

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?,
    ): View = inflater.inflate(R.layout.fragment_keyboard, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        val tvInset = view.findViewById<TextView>(R.id.tvInsetInfo)
        val tvLog = view.findViewById<TextView>(R.id.tvImeLog)
        val etPhone = view.findViewById<TextInputEditText>(R.id.etPhone)
        val etSearch = view.findViewById<TextInputEditText>(R.id.etSearch)
        val etMultiline = view.findViewById<TextInputEditText>(R.id.etMultiline)
        val imm = requireContext().getSystemService(InputMethodManager::class.java)

        fun refreshLog() {
            tvLog.text = log.text()
        }

        ViewCompat.setOnApplyWindowInsetsListener(view) { _, insets ->
            val ime = insets.getInsets(WindowInsetsCompat.Type.ime())
            val sys = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            val visible = insets.isVisible(WindowInsetsCompat.Type.ime())
            tvInset.text = buildString {
                append("IME visible: ")
                append(visible)
                append("\nime.bottom=")
                append(ime.bottom)
                append("  sys.bottom=")
                append(sys.bottom)
                append("\nusableHeight≈")
                append(view.height - ime.bottom)
            }
            insets
        }

        listOf(etPhone, etSearch, etMultiline).forEach { field ->
            field.setOnFocusChangeListener { _, hasFocus ->
                if (hasFocus) {
                    log.append("focus → ${field.hint ?: field.id}")
                    refreshLog()
                }
            }
        }

        etSearch.setOnEditorActionListener { _, actionId, _ ->
            if (actionId == EditorInfo.IME_ACTION_SEARCH) {
                log.append("IME_ACTION_SEARCH: query=${etSearch.text}")
                refreshLog()
                true
            } else false
        }

        etMultiline.setOnEditorActionListener { _, actionId, _ ->
            if (actionId == EditorInfo.IME_ACTION_DONE) {
                imm?.hideSoftInputFromWindow(etMultiline.windowToken, 0)
                log.append("IME_ACTION_DONE → hide keyboard")
                refreshLog()
                true
            } else false
        }

        view.findViewById<MaterialButton>(R.id.btnShowIme).setOnClickListener {
            etPhone.requestFocus()
            imm?.showSoftInput(etPhone, InputMethodManager.SHOW_IMPLICIT)
            log.append("showSoftInput(phone)")
            refreshLog()
        }

        view.findViewById<MaterialButton>(R.id.btnHideIme).setOnClickListener {
            imm?.hideSoftInputFromWindow(view.windowToken, 0)
            log.append("hideSoftInputFromWindow")
            refreshLog()
        }
    }
}
