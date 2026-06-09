package com.example.myapplication.handler

/**
 * 主程序 Handler 消息类型（类似微信主进程消息分发）。
 * 朋友圈滑动等子模块通过 [android.os.Message] 投递到主线程 MessageQueue。
 */
object AppMessage {
    const val MSG_APP_READY = 1
    const val MSG_TAB_CHANGED = 2
    const val MSG_MOMENTS_SCROLL = 10
    const val MSG_MOMENTS_SCROLL_STATE = 11
    const val MSG_MOMENTS_VISIBLE_ITEM = 12

    const val KEY_TAB_ID = "tab_id"
    const val KEY_SCROLL_DY = "scroll_dy"
    const val KEY_SCROLL_OFFSET = "scroll_offset"
    const val KEY_VISIBLE_POSITION = "visible_position"
    const val KEY_POST_AUTHOR = "post_author"
    const val KEY_SCROLL_STATE = "scroll_state"
}
