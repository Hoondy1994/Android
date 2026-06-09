package com.example.myapplication.wms.data.seed

import com.example.myapplication.wms.data.WmsDatabase
import com.example.myapplication.wms.data.entity.InboundLineEntity
import com.example.myapplication.wms.data.entity.InboundOrderEntity
import com.example.myapplication.wms.data.entity.InventoryBalanceEntity
import com.example.myapplication.wms.data.entity.LocationEntity
import com.example.myapplication.wms.data.entity.OutboundLineEntity
import com.example.myapplication.wms.data.entity.OutboundOrderEntity
import com.example.myapplication.wms.data.entity.PickTaskEntity
import com.example.myapplication.wms.data.entity.SkuEntity
import com.example.myapplication.wms.data.entity.WarehouseEntity
import com.example.myapplication.wms.domain.LocationType
import com.example.myapplication.wms.domain.OrderStatus
import com.example.myapplication.wms.domain.PickTaskStatus
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/** 首次启动注入演示数据，便于直接体验完整 WMS 流程 */
object DatabaseSeeder {
    suspend fun seedIfEmpty(db: WmsDatabase) = withContext(Dispatchers.IO) {
        if (db.skuDao().count() > 0) return@withContext

        val warehouseId = db.warehouseDao().insert(
            WarehouseEntity(code = "WH-SH01", name = "华东一号仓", address = "上海市浦东新区")
        )

        db.locationDao().insertAll(buildLocations(warehouseId))
        val storageLocations = db.locationDao().getStorageLocations()

        db.skuDao().insertAll(buildSkus())
        val skus = db.skuDao().getAllSync()

        val now = System.currentTimeMillis()
        skus.forEachIndexed { index, sku ->
            val loc = storageLocations[index % storageLocations.size]
            db.inventoryDao().upsert(
                InventoryBalanceEntity(
                    skuId = sku.id,
                    locationId = loc.id,
                    quantity = 80 + index * 7,
                    reservedQty = if (index % 4 == 0) 10 else 0,
                    updatedAt = now
                )
            )
        }

        seedInbound(db, skus, now)
        seedOutboundWithPicks(db, skus, storageLocations, now)
    }

    private fun buildLocations(warehouseId: Long): List<LocationEntity> {
        val list = mutableListOf<LocationEntity>()
        list += LocationEntity(
            warehouseId = warehouseId, locationCode = "RCV-01",
            zone = "R", aisle = "00", rack = "00", bin = "01",
            type = LocationType.RECEIVING, maxCapacity = 9999
        )
        list += LocationEntity(
            warehouseId = warehouseId, locationCode = "SHIP-01",
            zone = "S", aisle = "00", rack = "00", bin = "01",
            type = LocationType.SHIPPING, maxCapacity = 9999
        )
        for (zone in listOf("A", "B")) {
            for (aisle in 1..2) {
                for (rack in 1..3) {
                    for (bin in 1..2) {
                        list += LocationEntity(
                            warehouseId = warehouseId,
                            locationCode = "$zone-${aisle.toString().padStart(2, '0')}-${rack.toString().padStart(2, '0')}-${bin.toString().padStart(2, '0')}",
                            zone = zone,
                            aisle = aisle.toString().padStart(2, '0'),
                            rack = rack.toString().padStart(2, '0'),
                            bin = bin.toString().padStart(2, '0'),
                            type = LocationType.STORAGE,
                            maxCapacity = 500
                        )
                    }
                }
            }
        }
        return list
    }

    private fun buildSkus(): List<SkuEntity> = listOf(
        SkuEntity(skuCode = "SKU-10001", name = "蓝牙耳机 Pro", barcode = "69010001", unit = "EA", category = "电子", safetyStock = 20),
        SkuEntity(skuCode = "SKU-10002", name = "机械键盘 K8", barcode = "69010002", unit = "EA", category = "电子", safetyStock = 15),
        SkuEntity(skuCode = "SKU-20001", name = "仓储周转箱-大", barcode = "69020001", unit = "EA", category = "包材", safetyStock = 50),
        SkuEntity(skuCode = "SKU-20002", name = "标签纸 100*60", barcode = "69020002", unit = "ROLL", category = "包材", safetyStock = 30),
        SkuEntity(skuCode = "SKU-30001", name = "工业手套 L", barcode = "69030001", unit = "PAIR", category = "劳保", safetyStock = 100),
        SkuEntity(skuCode = "SKU-30002", name = "安全帽 白", barcode = "69030002", unit = "EA", category = "劳保", safetyStock = 40),
        SkuEntity(skuCode = "SKU-40001", name = "A4 复印纸", barcode = "69040001", unit = "BOX", category = "办公", safetyStock = 25),
        SkuEntity(skuCode = "SKU-40002", name = "记号笔 黑", barcode = "69040002", unit = "EA", category = "办公", safetyStock = 60),
        SkuEntity(skuCode = "SKU-50001", name = "USB-C 数据线", barcode = "69050001", unit = "EA", category = "电子", safetyStock = 35),
        SkuEntity(skuCode = "SKU-50002", name = "移动电源 20W", barcode = "69050002", unit = "EA", category = "电子", safetyStock = 18),
        SkuEntity(skuCode = "SKU-60001", name = "封箱胶带", barcode = "69060001", unit = "EA", category = "包材", safetyStock = 80),
        SkuEntity(skuCode = "SKU-60002", name = "气泡膜卷", barcode = "69060002", unit = "EA", category = "包材", safetyStock = 20)
    )

    private suspend fun seedInbound(db: WmsDatabase, skus: List<SkuEntity>, now: Long) {
        val orders = listOf(
            Triple("IN-20250603-001", "深圳芯联供应商", OrderStatus.CONFIRMED),
            Triple("IN-20250603-002", "苏州包材科技", OrderStatus.IN_PROGRESS),
            Triple("IN-20250602-003", "东莞劳保批发", OrderStatus.DRAFT)
        )
        orders.forEachIndexed { idx, (no, supplier, status) ->
            val orderId = db.inboundDao().insertOrder(
                InboundOrderEntity(
                    orderNo = no,
                    supplier = supplier,
                    status = status,
                    createdAt = now - idx * 86_400_000L,
                    remark = "演示入库单"
                )
            )
            db.inboundDao().insertLines(
                listOf(
                    InboundLineEntity(
                        inboundOrderId = orderId,
                        skuId = skus[idx % skus.size].id,
                        expectedQty = 50,
                        receivedQty = if (status == OrderStatus.IN_PROGRESS) 20 else 0
                    ),
                    InboundLineEntity(
                        inboundOrderId = orderId,
                        skuId = skus[(idx + 3) % skus.size].id,
                        expectedQty = 30,
                        receivedQty = 0
                    )
                )
            )
        }
    }

    private suspend fun seedOutboundWithPicks(
        db: WmsDatabase,
        skus: List<SkuEntity>,
        storage: List<LocationEntity>,
        now: Long
    ) {
        val sku = skus.first()
        val loc = storage.first()
        val orderId = db.outboundDao().insertOrder(
            OutboundOrderEntity(
                orderNo = "OUT-20250603-101",
                customer = "天猫旗舰店",
                status = OrderStatus.CONFIRMED,
                priority = 9,
                createdAt = now
            )
        )
        val lineId = db.outboundDao().insertLines(
            listOf(
                OutboundLineEntity(
                    outboundOrderId = orderId,
                    skuId = sku.id,
                    orderedQty = 15,
                    allocatedQty = 15,
                    pickedQty = 0
                )
            )
        )
        val balance = db.inventoryDao().getBalance(sku.id, loc.id)
        if (balance != null) {
            db.inventoryDao().update(balance.copy(reservedQty = balance.reservedQty + 15))
            val lines = db.outboundDao().getOrderWithLines(orderId)?.lines.orEmpty()
            val line = lines.first()
            db.pickTaskDao().insertAll(
                listOf(
                    PickTaskEntity(
                        outboundLineId = line.id,
                        locationId = loc.id,
                        skuId = sku.id,
                        qty = 15,
                        pickedQty = 0,
                        status = PickTaskStatus.PENDING,
                        assignee = ""
                    )
                )
            )
        }

        val order2 = db.outboundDao().insertOrder(
            OutboundOrderEntity(
                orderNo = "OUT-20250603-102",
                customer = "京东 POP 店",
                status = OrderStatus.CONFIRMED,
                priority = 5,
                createdAt = now
            )
        )
        db.outboundDao().insertLines(
            listOf(
                OutboundLineEntity(
                    outboundOrderId = order2,
                    skuId = skus[2].id,
                    orderedQty = 8,
                    allocatedQty = 0,
                    pickedQty = 0
                )
            )
        )
    }
}
