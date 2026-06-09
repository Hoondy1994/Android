package com.example.audiotest

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.*
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.example.audiotest.databinding.ActivityMainBinding
import org.json.JSONArray

// ─── 数据模型 ──────────────────────────────────────────────────────────────────
data class Recording(
    val path: String,
    val name: String,
    val durationMs: Long,
    val sizeBytes: Long
) {
    val durationText: String get() {
        val s = durationMs / 1000; val m = s / 60; val h = m / 60
        return if (h > 0) "%d:%02d:%02d".format(h, m % 60, s % 60)
        else              "%02d:%02d".format(m, s % 60)
    }
    val sizeText: String get() = when {
        sizeBytes >= 1_048_576 -> "%.1f MB".format(sizeBytes / 1_048_576f)
        sizeBytes >= 1024      -> "%.1f KB".format(sizeBytes / 1024f)
        else                   -> "$sizeBytes B"
    }
}

// ─── RecyclerView Adapter ─────────────────────────────────────────────────────
class RecordingAdapter(
    private var items: List<Recording>,
    private val onPlay: (Recording) -> Unit,
    private val onDelete: (Recording) -> Unit
) : RecyclerView.Adapter<RecordingAdapter.VH>() {

    var playingPath: String? = null

    inner class VH(v: View) : RecyclerView.ViewHolder(v) {
        val tvName:     TextView  = v.findViewById(R.id.tv_rec_name)
        val tvInfo:     TextView  = v.findViewById(R.id.tv_rec_info)
        val btnPlay:    ImageButton = v.findViewById(R.id.btn_play)
        val btnDelete:  ImageButton = v.findViewById(R.id.btn_delete)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): VH {
        val v = LayoutInflater.from(parent.context).inflate(R.layout.item_recording, parent, false)
        return VH(v)
    }

    override fun onBindViewHolder(holder: VH, position: Int) {
        val rec = items[position]
        holder.tvName.text = rec.name
        holder.tvInfo.text = "${rec.durationText}  ·  ${rec.sizeText}"
        val isPlaying = rec.path == playingPath
        holder.btnPlay.setImageResource(
            if (isPlaying) android.R.drawable.ic_media_pause
            else           android.R.drawable.ic_media_play
        )
        holder.btnPlay.setOnClickListener   { onPlay(rec) }
        holder.btnDelete.setOnClickListener { onDelete(rec) }
    }

    override fun getItemCount() = items.size

    fun update(newItems: List<Recording>) { items = newItems; notifyDataSetChanged() }
}

// ─── MainActivity ─────────────────────────────────────────────────────────────
class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private val handler = Handler(Looper.getMainLooper())

    // 状态
    private var isRecording  = false
    private var isPlaying    = false
    private var currentPlayPath: String? = null
    private var adapter: RecordingAdapter? = null

    // 权限请求
    private val permLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { granted ->
        if (granted) toggleRecording() else toast("需要麦克风权限")
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // 初始化 Native 引擎，存储目录在应用私有目录
        val dir = getExternalFilesDir(null)?.absolutePath
            ?: filesDir.absolutePath
        nativeInit("$dir/recordings")

        setupRecyclerView()
        setupButtons()
        startUiRefresh()
        refreshList()
    }

    override fun onDestroy() {
        super.onDestroy()
        handler.removeCallbacksAndMessages(null)
        nativeDestroy()
    }

    // ── RecyclerView ──────────────────────────────────────────────────────────
    private fun setupRecyclerView() {
        adapter = RecordingAdapter(
            emptyList(),
            onPlay   = { rec -> togglePlayback(rec) },
            onDelete = { rec ->
                AlertDialog.Builder(this)
                    .setTitle("删除录音")
                    .setMessage("确定删除 ${rec.name}？")
                    .setPositiveButton("删除") { _, _ ->
                        nativeDeleteRecording(rec.path)
                        if (currentPlayPath == rec.path) stopPlayback()
                        refreshList()
                    }
                    .setNegativeButton("取消", null)
                    .show()
            }
        )
        binding.rvRecordings.layoutManager = LinearLayoutManager(this)
        binding.rvRecordings.adapter = adapter
    }

    // ── Buttons ───────────────────────────────────────────────────────────────
    private fun setupButtons() {
        binding.btnRecord.setOnClickListener {
            if (hasAudioPermission()) toggleRecording()
            else permLauncher.launch(Manifest.permission.RECORD_AUDIO)
        }
        binding.seekBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(sb: SeekBar, progress: Int, fromUser: Boolean) {
                if (fromUser) nativeSeekTo(nativeGetPlayDurationMs() * progress / 100)
            }
            override fun onStartTrackingTouch(sb: SeekBar) {}
            override fun onStopTrackingTouch(sb: SeekBar) {}
        })
    }

    // ── 录音切换 ──────────────────────────────────────────────────────────────
    private fun toggleRecording() {
        if (!isRecording) {
            stopPlayback()
            val path = nativeStartRecording()
            if (path.isEmpty()) { toast("录音启动失败"); return }
            isRecording = true
            binding.btnRecord.setImageResource(android.R.drawable.ic_media_pause)
            binding.tvRecTimer.visibility = View.VISIBLE
        } else {
            nativeStopRecording()
            isRecording = false
            binding.btnRecord.setImageResource(R.drawable.ic_mic)
            binding.tvRecTimer.visibility = View.GONE
            refreshList()
        }
    }

    // ── 播放切换 ──────────────────────────────────────────────────────────────
    private fun togglePlayback(rec: Recording) {
        if (isRecording) { toast("录音中，无法播放"); return }
        if (currentPlayPath == rec.path && isPlaying) {
            // 暂停
            nativePausePlayback()
            isPlaying = false
        } else if (currentPlayPath == rec.path && !isPlaying) {
            // 恢复
            nativeResumePlayback()
            isPlaying = true
        } else {
            // 播放新文件
            stopPlayback()
            val ok = nativeStartPlayback(rec.path)
            if (!ok) { toast("播放失败"); return }
            currentPlayPath = rec.path
            isPlaying = true
        }
        adapter?.playingPath = if (isPlaying) currentPlayPath else null
        adapter?.notifyDataSetChanged()
    }

    private fun stopPlayback() {
        nativeStopPlayback()
        isPlaying = false
        currentPlayPath = null
        adapter?.playingPath = null
        adapter?.notifyDataSetChanged()
        binding.seekBar.progress = 0
        binding.tvPlayPos.text = "00:00"
    }

    // ── UI 定时刷新 ──────────────────────────────────────────────────────────
    private fun startUiRefresh() {
        handler.post(object : Runnable {
            override fun run() {
                updateUi()
                handler.postDelayed(this, 200)
            }
        })
    }

    private fun updateUi() {
        // 录音计时
        if (isRecording) {
            val ms = nativeGetRecordingElapsedMs()
            binding.tvRecTimer.text = fmtMs(ms)
        }
        // 播放进度
        if (isPlaying) {
            val pos = nativeGetPlayPositionMs()
            val dur = nativeGetPlayDurationMs()
            binding.tvPlayPos.text = fmtMs(pos)
            binding.tvPlayDur.text = fmtMs(dur)
            if (dur > 0) binding.seekBar.progress = (pos * 100 / dur).toInt()
            // 检测播放结束
            val state = nativeGetState()
            if (state == 0 /*IDLE*/) {
                isPlaying = false
                currentPlayPath = null
                adapter?.playingPath = null
                adapter?.notifyDataSetChanged()
                binding.seekBar.progress = 0
            }
        }
    }

    private fun refreshList() {
        val json = nativeListRecordings()
        val arr  = JSONArray(json)
        val list = (0 until arr.length()).map { i ->
            val o = arr.getJSONObject(i)
            Recording(o.getString("path"), o.getString("name"),
                      o.getLong("durationMs"), o.getLong("sizeBytes"))
        }
        adapter?.update(list)
        binding.tvEmpty.visibility = if (list.isEmpty()) View.VISIBLE else View.GONE
    }

    private fun hasAudioPermission() =
        ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) ==
                PackageManager.PERMISSION_GRANTED

    private fun toast(msg: String) = Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()

    private fun fmtMs(ms: Long): String {
        val s = ms / 1000; val m = s / 60
        return "%02d:%02d".format(m, s % 60)
    }

    // ── JNI ──────────────────────────────────────────────────────────────────
    external fun nativeInit(storageDir: String)
    external fun nativeDestroy()
    external fun nativeStartRecording(): String
    external fun nativeStopRecording(): Boolean
    external fun nativeGetRecordingElapsedMs(): Long
    external fun nativeStartPlayback(path: String): Boolean
    external fun nativePausePlayback(): Boolean
    external fun nativeResumePlayback(): Boolean
    external fun nativeStopPlayback()
    external fun nativeSeekTo(posMs: Long)
    external fun nativeGetState(): Int
    external fun nativeGetPlayPositionMs(): Long
    external fun nativeGetPlayDurationMs(): Long
    external fun nativeListRecordings(): String
    external fun nativeDeleteRecording(path: String): Boolean

    companion object {
        init { System.loadLibrary("audiotest") }
    }
}
