package com.example.inputtest.input

class EventLogBuffer(private val maxLines: Int = 80) {
    private val lines = ArrayDeque<String>(maxLines)

    fun append(line: String) {
        lines.addLast(line)
        while (lines.size > maxLines) lines.removeFirst()
    }

    fun clear() = lines.clear()

    fun text(): String = lines.joinToString("\n")
}
