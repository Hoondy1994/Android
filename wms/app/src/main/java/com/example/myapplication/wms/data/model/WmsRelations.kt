package com.example.myapplication.wms.data.model

import androidx.room.Embedded
import androidx.room.Relation
import com.example.myapplication.wms.data.entity.InboundLineEntity
import com.example.myapplication.wms.data.entity.InboundOrderEntity
import com.example.myapplication.wms.data.entity.InventoryBalanceEntity
import com.example.myapplication.wms.data.entity.LocationEntity
import com.example.myapplication.wms.data.entity.OutboundLineEntity
import com.example.myapplication.wms.data.entity.OutboundOrderEntity
import com.example.myapplication.wms.data.entity.PickTaskEntity
import com.example.myapplication.wms.data.entity.SkuEntity
import com.example.myapplication.wms.data.entity.StockMovementEntity

data class InventoryDetail(
    @Embedded val balance: InventoryBalanceEntity,
    @Relation(parentColumn = "skuId", entityColumn = "id")
    val sku: SkuEntity,
    @Relation(parentColumn = "locationId", entityColumn = "id")
    val location: LocationEntity
) {
    val availableQty: Int get() = balance.quantity - balance.reservedQty
}

data class InboundOrderWithLines(
    @Embedded val order: InboundOrderEntity,
    @Relation(parentColumn = "id", entityColumn = "inboundOrderId")
    val lines: List<InboundLineEntity>
)

data class OutboundOrderWithLines(
    @Embedded val order: OutboundOrderEntity,
    @Relation(parentColumn = "id", entityColumn = "outboundOrderId")
    val lines: List<OutboundLineEntity>
)

data class PickTaskDetail(
    @Embedded val task: PickTaskEntity,
    @Relation(parentColumn = "skuId", entityColumn = "id")
    val sku: SkuEntity,
    @Relation(parentColumn = "locationId", entityColumn = "id")
    val location: LocationEntity
)

data class MovementDetail(
    @Embedded val movement: StockMovementEntity,
    @Relation(parentColumn = "skuId", entityColumn = "id")
    val sku: SkuEntity
)

data class InboundLineWithSku(
    @Embedded val line: InboundLineEntity,
    @Relation(parentColumn = "skuId", entityColumn = "id")
    val sku: SkuEntity
)
