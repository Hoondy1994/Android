package com.example.myapplication.wms.ui

import com.example.myapplication.wms.domain.OrderStatus
import com.example.myapplication.wms.domain.PickTaskStatus

fun OrderStatus.toLabel(): String = when (this) {
    OrderStatus.DRAFT -> "草稿"
    OrderStatus.CONFIRMED -> "已确认"
    OrderStatus.IN_PROGRESS -> "作业中"
    OrderStatus.PARTIAL -> "部分完成"
    OrderStatus.COMPLETED -> "已完成"
    OrderStatus.CANCELLED -> "已取消"
}

fun PickTaskStatus.toLabel(): String = when (this) {
    PickTaskStatus.PENDING -> "待拣"
    PickTaskStatus.ASSIGNED -> "已分配"
    PickTaskStatus.PICKING -> "拣货中"
    PickTaskStatus.COMPLETED -> "已完成"
    PickTaskStatus.SHORT -> "短拣"
}
