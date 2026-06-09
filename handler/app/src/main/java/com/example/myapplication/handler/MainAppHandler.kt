package com.example.myapplication.handler

import android.os.Handler
import android.os.Looper
import android.os.Message
import com.example.myapplication.MainActivity

/**
 * 主程序 Handler：绑定主线程 [Looper]，从 [MessageQueue] 取消息执行 [Runnable]。
 *
 * 子页面（朋友圈）不直接改主界面，而是 sendMessage / post，由这里统一处理。
 */
class MainAppHandler(
    looper: Looper,
    private val activity: MainActivity
) : Handler(looper) {

    override fun handleMessage(msg: Message) {
        when (msg.what) {
            AppMessage.MSG_APP_READY ->
                activity.onMainHandlerReady()

            AppMessage.MSG_TAB_CHANGED -> {
                val tabId = msg.data.getInt(AppMessage.KEY_TAB_ID, 0)
                activity.onTabChangedFromHandler(tabId)
            }

            AppMessage.MSG_MOMENTS_SCROLL -> {
                val dy = msg.arg1
                val offset = msg.arg2
                activity.onMomentsScroll(dy, offset)
            }

            AppMessage.MSG_MOMENTS_SCROLL_STATE -> {
                val state = msg.arg1
                activity.onMomentsScrollState(state)
            }

            AppMessage.MSG_MOMENTS_VISIBLE_ITEM -> {
                val position = msg.arg1
                val author = msg.obj as? String ?: ""
                activity.onMomentsVisibleItem(position, author)
            }

            else -> super.handleMessage(msg)
        }
    }
}
