package com.example.myapplication.wms.domain

/** 入库单 / 出库单共用状态机（学习重点：状态驱动业务流程） */
enum class OrderStatus {
    DRAFT,
    CONFIRMED,
    IN_PROGRESS,
    PARTIAL,
    COMPLETED,
    CANCELLED
}

enum class PickTaskStatus {
    PENDING,
    ASSIGNED,
    PICKING,
    COMPLETED,
    SHORT
}

/** 库存流水类型：审计与对账的基础 */
enum class MovementType {
    INBOUND,
    OUTBOUND,
    TRANSFER,
    ADJUSTMENT
}

enum class LocationType {
    STORAGE,
    RECEIVING,
    SHIPPING,
    QUARANTINE
}
