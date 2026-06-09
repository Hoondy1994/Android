package com.example.myapplication

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.Message
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.RecyclerView
import com.example.myapplication.databinding.ActivityMainBinding
import com.example.myapplication.handler.AppMessage
import com.example.myapplication.handler.MainAppHandler
import com.example.myapplication.ui.MomentsFragment
import com.example.myapplication.ui.PlaceholderFragment
/**
 * 微信式主程序：主线程 Looper + MessageQueue + Handler。
 *
 * Activity 启动时系统已为 UI 线程准备好 Looper（等价于 Looper.prepare + Looper.loop）。
 * [mainHandler] 从 MessageQueue 取消息，在 handleMessage / Runnable 中更新主界面。
 */
class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var mainHandler: MainAppHandler
    private lateinit var tvHandlerLog: TextView
    private lateinit var tvTitle: TextView

    private var momentsFragment: MomentsFragment? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        tvHandlerLog = binding.tvHandlerLog
        tvTitle = binding.tvTitle

        // 主线程 Looper：微信主进程消息循环的简化模型
        val mainLooper = Looper.getMainLooper()
        mainHandler = MainAppHandler(mainLooper, this)

        setupBottomNav()
        if (savedInstanceState == null) {
            showTab(TAB_CHATS)
        }

        // Runnable 投递：应用就绪通知
        mainHandler.post {
            val msg = Message.obtain(mainHandler, AppMessage.MSG_APP_READY)
            mainHandler.sendMessage(msg)
        }
    }

    private fun setupBottomNav() {
        binding.bottomNav.setOnItemSelectedListener { item ->
            val tabId = item.itemId
            val msg = Message.obtain(mainHandler, AppMessage.MSG_TAB_CHANGED).apply {
                data.putInt(AppMessage.KEY_TAB_ID, tabId)
            }
            mainHandler.sendMessage(msg)
            true
        }
    }

    fun onMainHandlerReady() {
        tvHandlerLog.text = getString(R.string.main_looper_ready)
    }

    fun onTabChangedFromHandler(tabId: Int) {
        when (tabId) {
            R.id.nav_chats -> showTab(TAB_CHATS)
            R.id.nav_contacts -> showTab(TAB_CONTACTS)
            R.id.nav_discover -> showTab(TAB_DISCOVER)
            R.id.nav_me -> showTab(TAB_ME)
        }
    }

    fun onMomentsScroll(dy: Int, offset: Int) {
        val direction = when {
            dy > 0 -> "下滑"
            dy < 0 -> "上滑"
            else -> "—"
        }
        tvHandlerLog.text = getString(
            R.string.handler_log_scroll,
            direction,
            dy,
            offset
        )
    }

    fun onMomentsScrollState(state: Int) {
        val stateName = when (state) {
            RecyclerView.SCROLL_STATE_DRAGGING -> "拖动中"
            RecyclerView.SCROLL_STATE_SETTLING -> "惯性中"
            RecyclerView.SCROLL_STATE_IDLE -> "已停止"
            else -> "未知"
        }
        tvTitle.text = getString(R.string.moments_title) + " · $stateName"
    }

    fun onMomentsVisibleItem(position: Int, author: String) {
        if (author.isNotEmpty()) {
            tvHandlerLog.text = getString(R.string.handler_log_visible, position, author)
        }
    }

    /** 供 Fragment 获取主程序 Handler */
    fun getMainAppHandler(): Handler = mainHandler

    private fun showTab(tab: Int) {
        val (title, fragment) = when (tab) {
            TAB_CHATS -> getString(R.string.tab_chats) to
                PlaceholderFragment.newInstance("微信会话列表（演示）")
            TAB_CONTACTS -> getString(R.string.tab_contacts) to
                PlaceholderFragment.newInstance("通讯录（演示）")
            TAB_DISCOVER -> {
                val mf = momentsFragment ?: MomentsFragment.newInstance().also {
                    it.attachMainHandler(mainHandler)
                    momentsFragment = it
                }
                getString(R.string.moments_title) to mf
            }
            TAB_ME -> getString(R.string.tab_me) to
                PlaceholderFragment.newInstance("我（演示）")
            else -> "" to PlaceholderFragment.newInstance("")
        }
        tvTitle.text = title
        replaceFragment(fragment)
    }

    private fun replaceFragment(fragment: Fragment) {
        supportFragmentManager.beginTransaction()
            .replace(R.id.fragment_container, fragment)
            .commit()
    }

    companion object {
        private const val TAB_CHATS = 0
        private const val TAB_CONTACTS = 1
        private const val TAB_DISCOVER = 2
        private const val TAB_ME = 3
    }
}
