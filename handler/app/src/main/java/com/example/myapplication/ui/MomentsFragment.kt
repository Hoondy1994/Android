package com.example.myapplication.ui

import android.os.Bundle
import android.os.Handler
import android.os.Message
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.example.myapplication.R
import com.example.myapplication.handler.AppMessage
import com.example.myapplication.model.MomentPost

/**
 * 朋友圈列表：滑动时通过主程序 [Handler] 向 MessageQueue 投递消息，
 * 由 [com.example.myapplication.handler.MainAppHandler] 在主线程处理。
 */
class MomentsFragment : Fragment() {

    private var mainHandler: Handler? = null
    private lateinit var recyclerView: RecyclerView
    private lateinit var layoutManager: LinearLayoutManager

    /** 合并高频滑动，避免刷屏 MessageQueue */
    private val scrollReportRunnable = Runnable { reportScrollToMain() }

    fun attachMainHandler(handler: Handler) {
        mainHandler = handler
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? = inflater.inflate(R.layout.fragment_moments, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        recyclerView = view.findViewById(R.id.rv_moments)
        layoutManager = LinearLayoutManager(requireContext())
        recyclerView.layoutManager = layoutManager
        recyclerView.adapter = MomentsAdapter(buildDemoPosts())
        recyclerView.addOnScrollListener(momentsScrollListener)
    }

    private val momentsScrollListener = object : RecyclerView.OnScrollListener() {
        override fun onScrolled(recyclerView: RecyclerView, dx: Int, dy: Int) {
            val handler = mainHandler ?: return
            // post(Runnable)：把任务放进 MessageQueue，由 Looper 调度
            handler.removeCallbacks(scrollReportRunnable)
            handler.postDelayed(scrollReportRunnable, SCROLL_DEBOUNCE_MS)
            // 同时用 Message 传递本次 dy（即时量）
            val msg = Message.obtain(handler, AppMessage.MSG_MOMENTS_SCROLL)
            msg.arg1 = dy
            msg.arg2 = recyclerView.computeVerticalScrollOffset()
            handler.sendMessage(msg)
        }

        override fun onScrollStateChanged(recyclerView: RecyclerView, newState: Int) {
            val handler = mainHandler ?: return
            val msg = Message.obtain(handler, AppMessage.MSG_MOMENTS_SCROLL_STATE)
            msg.arg1 = newState
            handler.sendMessage(msg)
            if (newState == RecyclerView.SCROLL_STATE_IDLE) {
                reportVisibleItem(handler)
            }
        }
    }

    private fun reportScrollToMain() {
        val handler = mainHandler ?: return
        val msg = Message.obtain(handler, AppMessage.MSG_MOMENTS_VISIBLE_ITEM)
        val first = layoutManager.findFirstVisibleItemPosition()
        msg.arg1 = first
        val adapter = recyclerView.adapter as? MomentsAdapter
        msg.obj = adapter?.authorAt(first) ?: ""
        handler.sendMessage(msg)
    }

    private fun reportVisibleItem(handler: Handler) {
        val msg = Message.obtain(handler, AppMessage.MSG_MOMENTS_VISIBLE_ITEM)
        val first = layoutManager.findFirstVisibleItemPosition()
        msg.arg1 = first
        val adapter = recyclerView.adapter as? MomentsAdapter
        msg.obj = adapter?.authorAt(first) ?: ""
        handler.sendMessage(msg)
    }

    private fun buildDemoPosts(): List<MomentPost> {
        val names = listOf("张三", "李四", "王五", "赵六", "产品部", "设计组", "Android 学习")
        return (1..30).map { i ->
            MomentPost(
                author = names[i % names.size],
                content = "这是第 $i 条朋友圈，滑动我会把状态发给主程序 Handler。",
                time = "${i} 小时前"
            )
        }
    }

    companion object {
        private const val SCROLL_DEBOUNCE_MS = 80L

        fun newInstance(): MomentsFragment = MomentsFragment()
    }
}
