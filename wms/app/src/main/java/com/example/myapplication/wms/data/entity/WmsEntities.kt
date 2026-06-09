package com.example.myapplication.wms.data.entity

import androidx.room.Entity
import androidx.room.ForeignKey
import androidx.room.Index
import androidx.room.PrimaryKey
import com.example.myapplication.wms.domain.LocationType
import com.example.myapplication.wms.domain.MovementType
import com.example.myapplication.wms.domain.OrderStatus
import com.example.myapplication.wms.domain.PickTaskStatus

@Entity(tableName = "warehouse")
data class WarehouseEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val code: String,
    val name: String,
    val address: String
)

@Entity(
    tableName = "location",
    foreignKeys = [
        ForeignKey(
            entity = WarehouseEntity::class,
            parentColumns = ["id"],
            childColumns = ["warehouseId"],
            onDelete = ForeignKey.CASCADE
        )
    ],
    indices = [Index(value = ["warehouseId"]), Index(value = ["locationCode"], unique = true)]
)
data class LocationEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val warehouseId: Long,
    val locationCode: String,
    val zone: String,
    val aisle: String,
    val rack: String,
    val bin: String,
    val type: LocationType,
    val maxCapacity: Int
)

@Entity(
    tableName = "sku",
    indices = [Index(value = ["skuCode"], unique = true)]
)
data class SkuEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val skuCode: String,
    val name: String,
    val barcode: String,
    val unit: String,
    val category: String,
    val safetyStock: Int
)

/** 库位级库存：WMS 核心表，可用量 = quantity - reservedQty */
@Entity(
    tableName = "inventory_balance",
    foreignKeys = [
        ForeignKey(entity = SkuEntity::class, parentColumns = ["id"], childColumns = ["skuId"]),
        ForeignKey(entity = LocationEntity::class, parentColumns = ["id"], childColumns = ["locationId"])
    ],
    indices = [Index("skuId"), Index("locationId"), Index(value = ["skuId", "locationId"], unique = true)]
)
data class InventoryBalanceEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val skuId: Long,
    val locationId: Long,
    val quantity: Int,
    val reservedQty: Int,
    val updatedAt: Long
)

@Entity(tableName = "inbound_order", indices = [Index(value = ["orderNo"], unique = true)])
data class InboundOrderEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val orderNo: String,
    val supplier: String,
    val status: OrderStatus,
    val createdAt: Long,
    val remark: String
)

@Entity(
    tableName = "inbound_line",
    foreignKeys = [ForeignKey(entity = InboundOrderEntity::class, parentColumns = ["id"], childColumns = ["inboundOrderId"])],
    indices = [Index("inboundOrderId")]
)
data class InboundLineEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val inboundOrderId: Long,
    val skuId: Long,
    val expectedQty: Int,
    val receivedQty: Int
)

@Entity(tableName = "outbound_order", indices = [Index(value = ["orderNo"], unique = true)])
data class OutboundOrderEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val orderNo: String,
    val customer: String,
    val status: OrderStatus,
    val priority: Int,
    val createdAt: Long
)

@Entity(
    tableName = "outbound_line",
    foreignKeys = [ForeignKey(entity = OutboundOrderEntity::class, parentColumns = ["id"], childColumns = ["outboundOrderId"])],
    indices = [Index("outboundOrderId")]
)
data class OutboundLineEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val outboundOrderId: Long,
    val skuId: Long,
    val orderedQty: Int,
    val allocatedQty: Int,
    val pickedQty: Int
)

@Entity(
    tableName = "pick_task",
    foreignKeys = [
        ForeignKey(entity = OutboundLineEntity::class, parentColumns = ["id"], childColumns = ["outboundLineId"]),
        ForeignKey(entity = LocationEntity::class, parentColumns = ["id"], childColumns = ["locationId"]),
        ForeignKey(entity = SkuEntity::class, parentColumns = ["id"], childColumns = ["skuId"])
    ],
    indices = [Index("outboundLineId"), Index("status")]
)
data class PickTaskEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val outboundLineId: Long,
    val locationId: Long,
    val skuId: Long,
    val qty: Int,
    val pickedQty: Int,
    val status: PickTaskStatus,
    val assignee: String
)

@Entity(
    tableName = "stock_movement",
    indices = [Index("skuId"), Index("createdAt")]
)
data class StockMovementEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val type: MovementType,
    val skuId: Long,
    val fromLocationId: Long?,
    val toLocationId: Long?,
    val quantity: Int,
    val referenceNo: String,
    val operator: String,
    val createdAt: Long
)

@Entity(tableName = "operation_log")
data class OperationLogEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val module: String,
    val action: String,
    val detail: String,
    val createdAt: Long
)
