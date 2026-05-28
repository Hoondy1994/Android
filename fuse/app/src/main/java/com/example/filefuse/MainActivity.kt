package com.example.filefuse

import android.os.Bundle
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.EditText
import android.widget.ListView
import android.widget.RadioButton
import android.widget.RadioGroup
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {

    private lateinit var backendGroup: RadioGroup
    private lateinit var backendLocal: RadioButton
    private lateinit var backendNfs: RadioButton
    private lateinit var localPathInput: EditText
    private lateinit var nfsHostInput: EditText
    private lateinit var nfsExportInput: EditText
    private lateinit var mountButton: Button
    private lateinit var unmountButton: Button
    private lateinit var statusText: TextView
    private lateinit var currentPathText: TextView
    private lateinit var fileList: ListView
    private lateinit var filePreview: TextView

    private var currentPath = "/"
    private var entries = emptyList<FileFuseNative.DirEntry>()
    private lateinit var adapter: ArrayAdapter<String>

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        bindViews()
        setupDefaults()
        setupListeners()
        refreshStatus()
    }

    private fun bindViews() {
        backendGroup = findViewById(R.id.backend_group)
        backendLocal = findViewById(R.id.backend_local)
        backendNfs = findViewById(R.id.backend_nfs)
        localPathInput = findViewById(R.id.local_path_input)
        nfsHostInput = findViewById(R.id.nfs_host_input)
        nfsExportInput = findViewById(R.id.nfs_export_input)
        mountButton = findViewById(R.id.mount_button)
        unmountButton = findViewById(R.id.unmount_button)
        statusText = findViewById(R.id.status_text)
        currentPathText = findViewById(R.id.current_path_text)
        fileList = findViewById(R.id.file_list)
        filePreview = findViewById(R.id.file_preview)
        adapter = ArrayAdapter(this, android.R.layout.simple_list_item_1, mutableListOf())
        fileList.adapter = adapter
    }

    private fun setupDefaults() {
        val demoDir = FileFuseCommands.ensureDemoDir(this)
        localPathInput.setText(demoDir.absolutePath)
        nfsHostInput.setText("127.0.0.1")
        nfsExportInput.setText("/export")
    }

    private fun setupListeners() {
        backendGroup.setOnCheckedChangeListener { _, checkedId ->
            val useNfs = checkedId == R.id.backend_nfs
            localPathInput.isEnabled = !useNfs
            nfsHostInput.isEnabled = useNfs
            nfsExportInput.isEnabled = useNfs
        }

        mountButton.setOnClickListener { mountSelectedBackend() }
        unmountButton.setOnClickListener {
            FileFuseCommands.unmount()
            currentPath = "/"
            refreshStatus()
            refreshDirectory()
        }

        fileList.onItemClickListener = AdapterView.OnItemClickListener { _, _, position, _ ->
            val entry = entries[position]
            if (entry.isDirectory) {
                currentPath = joinPath(currentPath, entry.name)
                refreshDirectory()
                filePreview.text = ""
            } else {
                val filePath = joinPath(currentPath, entry.name)
                filePreview.text = FileFuseNative.nativeReadTextFile(filePath)
            }
        }

        currentPathText.setOnClickListener {
            if (currentPath != "/") {
                currentPath = parentPath(currentPath)
                refreshDirectory()
                filePreview.text = ""
            }
        }
    }

    private fun mountSelectedBackend() {
        val result = if (backendNfs.isChecked) {
            FileFuseCommands.mountNfs(
                host = nfsHostInput.text.toString().trim(),
                exportPath = nfsExportInput.text.toString().trim(),
                port = 2049
            )
        } else {
            FileFuseCommands.mountLocal(localPathInput.text.toString().trim())
        }

        if (result.code == FileFuseNative.ERR_OK) {
            currentPath = "/"
            refreshStatus()
            refreshDirectory()
            Toast.makeText(this, R.string.mount_success, Toast.LENGTH_SHORT).show()
        } else {
            Toast.makeText(this, getString(R.string.mount_failed, result.code), Toast.LENGTH_LONG).show()
        }
    }

    private fun refreshStatus() {
        statusText.text = FileFuseNative.nativeGetStatus()
        currentPathText.text = getString(R.string.current_path, currentPath)
    }

    private fun refreshDirectory() {
        entries = FileFuseNative.listDir(currentPath)
        adapter.clear()
        adapter.addAll(entries.map { entry ->
            if (entry.isDirectory) "[DIR] ${entry.name}" else entry.name
        })
        adapter.notifyDataSetChanged()
        refreshStatus()
    }

    private fun joinPath(base: String, name: String): String {
        return if (base == "/") "/$name" else "$base/$name"
    }

    private fun parentPath(path: String): String {
        if (path == "/") {
            return "/"
        }
        val index = path.lastIndexOf('/')
        return if (index <= 0) "/" else path.substring(0, index)
    }
}
