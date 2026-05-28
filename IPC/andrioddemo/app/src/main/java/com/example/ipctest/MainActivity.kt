package com.example.ipctest

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.Bundle
import android.os.IBinder
import android.os.Process
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.example.ipctest.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private var calculator: ICalculator? = null
    private var bound = false

    private val connection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
            calculator = ICalculator.Stub.asInterface(service)
            bound = true
            appendLog(getString(R.string.ipc_connected))
        }

        override fun onServiceDisconnected(name: ComponentName?) {
            calculator = null
            bound = false
            appendLog(getString(R.string.ipc_disconnected))
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.textLocalPid.text = getString(R.string.local_pid, Process.myPid())

        binding.btnBind.setOnClickListener { bindRemoteService() }
        binding.btnUnbind.setOnClickListener { unbindRemoteService() }
        binding.btnAdd.setOnClickListener { callAdd() }
        binding.btnGetMessage.setOnClickListener { callGetMessage() }
    }

    override fun onStart() {
        super.onStart()
        bindRemoteService()
    }

    override fun onStop() {
        super.onStop()
        unbindRemoteService()
    }

    private fun bindRemoteService() {
        if (bound) return
        val intent = Intent(this, CalculatorService::class.java)
        bindService(intent, connection, Context.BIND_AUTO_CREATE)
    }

    private fun unbindRemoteService() {
        if (!bound) return
        unbindService(connection)
        bound = false
        calculator = null
        appendLog(getString(R.string.ipc_unbound))
    }

    private fun callAdd() {
        val calc = calculator
        if (calc == null) {
            toast(getString(R.string.ipc_not_bound))
            return
        }

        val a = binding.editA.text.toString().toIntOrNull()
        val b = binding.editB.text.toString().toIntOrNull()
        if (a == null || b == null) {
            toast(getString(R.string.ipc_invalid_input))
            return
        }

        try {
            val result = calc.add(a, b)
            appendLog(getString(R.string.ipc_add_result, a, b, result))
        } catch (e: Exception) {
            appendLog(getString(R.string.ipc_call_failed, e.message))
        }
    }

    private fun callGetMessage() {
        val calc = calculator
        if (calc == null) {
            toast(getString(R.string.ipc_not_bound))
            return
        }

        try {
            val message = calc.getMessage()
            appendLog(getString(R.string.ipc_message_result, message))
        } catch (e: Exception) {
            appendLog(getString(R.string.ipc_call_failed, e.message))
        }
    }

    private fun appendLog(message: String) {
        val current = binding.textLog.text.toString()
        binding.textLog.text = if (current.isEmpty()) message else "$current\n$message"
    }

    private fun toast(message: String) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
    }
}
