package com.example.myapplication.wms.data.repository

import com.example.myapplication.wms.data.WmsDatabase
import com.example.myapplication.wms.data.entity.InboundLineEntity
import com.example.myapplication.wms.data.entity.InboundOrderEntity
import com.example.myapplication.wms.data.entity.InventoryBalanceEntity
import com.example.myapplication.wms.data.entity.OperationLogEntity
import com.example.myapplication.wms.data.entity.OutboundLineEntity
import com.example.myapplication.wms.data.entity.OutboundOrderEntity
import com.example.myapplication.wms.data.entity.PickTaskEntity
import com.example.myapplication.wms.data.entity.StockMovementEntity
import com.example.myapplication.wms.data.model.InboundLineWithSku
import com.example.myapplication.wms.data.model.InboundOrderWithLines
import com.example.myapplication.wms.data.model.InventoryDetail
import com.example.myapplication.wms.data.model.MovementDetail
import com.example.myapplication.wms.data.model.OutboundOrderWithLines
import com.example.myapplication.wms.data.model.PickTaskDetail
import com.example.myapplication.wms.data.seed.DatabaseSeeder
import com.example.myapplication.wms.domain.MovementType
import com.example.myapplication.wms.domain.OrderStatus
import com.example.myapplication.wms.domain.PickTaskStatus
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.withContext

data class DashboardStats(
    val skuCount: Int,
    val locationCount: Int,
    val totalOnHand: Int,
    val openInbound: Int,
    val openOutbound: Int,
    val openPicks: Int
)

/**
 * WMS 业务中枢：把「入库 / 出库 / 拣货 / 移库」串成可追溯的库存变更。
 * 学习时可对照每个方法的：校验 → 改余额 → 写流水 → 记日志。
 */
class WmsRepository(private val db: WmsDatabase) {

    private val operator = "学员-01"

    suspend fun initialize() {
        DatabaseSeeder.seedIfEmpty(db)
    }

    fun observeInventory(): Flow<List<InventoryDetail>> = db.inventoryDao().observeDetails()
    fun observeInboundOrders() = db.inboundDao().observeOrders()
    fun observeOutboundOrders() = db.outboundDao().observeOrders()
    fun observePickTasks(): Flow<List<PickTaskDetail>> = db.pickTaskDao().observeOpenTasks()
    fun observeMovements(): Flow<List<MovementDetail>> = db.movementDao().observeRecent()
    fun observeLogs() = db.operationLogDao().observeRecent()
    fun observeSkus() = db.skuDao().observeAll()
    fun observeLocations() = db.locationDao().observeAll()

    suspend fun loadDashboard(): DashboardStats = withContext(Dispatchers.IO) {
        val d = db.dashboardDao()
        DashboardStats(
            skuCount = d.skuCount(),
            locationCount = d.locationCount(),
            totalOnHand = d.totalOnHand(),
            openInbound = d.openInboundCount(),
            openOutbound = d.openOutboundCount(),
            openPicks = d.openPickCount()
        )
    }

    suspend fun getInboundDetail(orderId: Long): InboundOrderWithLines? =
        withContext(Dispatchers.IO) { db.inboundDao().getOrderWithLines(orderId) }

    suspend fun getInboundLines(orderId: Long): List<InboundLineWithSku> =
        withContext(Dispatchers.IO) { db.inboundDao().getLinesWithSku(orderId) }

    suspend fun getOutboundDetail(orderId: Long): OutboundOrderWithLines? =
        withContext(Dispatchers.IO) { db.outboundDao().getOrderWithLines(orderId) }

    /**
     * 收货上架：增加目标库位库存，更新入库行，必要时推进单据状态。
     */
    suspend fun receiveInbound(
        lineId: Long,
        qty: Int,
        toLocationId: Long
    ): Result<Unit> = withContext(Dispatchers.IO) {
        runCatching {
            require(qty > 0) { "收货数量必须大于 0" }
            val line = db.inboundDao().getLine(lineId)
                ?: error("入库行不存在")
            val remain = line.expectedQty - line.receivedQty
            require(qty <= remain) { "超收：剩余可收 $remain" }

            val order = db.inboundDao().getOrderWithLines(line.inboundOrderId)?.order
                ?: error("入库单不存在")
            check(order.status != OrderStatus.COMPLETED && order.status != OrderStatus.CANCELLED) {
                "单据状态不允许收货"
            }

            addStock(line.skuId, toLocationId, qty)
            db.movementDao().insert(
                StockMovementEntity(
                    type = MovementType.INBOUND,
                    skuId = line.skuId,
                    fromLocationId = null,
                    toLocationId = toLocationId,
                    quantity = qty,
                    referenceNo = order.orderNo,
                    operator = operator,
                    createdAt = System.currentTimeMillis()
                )
            )

            val newReceived = line.receivedQty + qty
            db.inboundDao().updateLine(line.copy(receivedQty = newReceived))
            refreshInboundOrderStatus(order, line.inboundOrderId)
            log("入库", "收货", "${order.orderNo} 行#$lineId +$qty @库位$toLocationId")
        }
    }

    private suspend fun refreshInboundOrderStatus(order: InboundOrderEntity, orderId: Long) {
        val lines = db.inboundDao().getOrderWithLines(orderId)?.lines.orEmpty()
        val allDone = lines.all { it.receivedQty >= it.expectedQty }
        val anyReceived = lines.any { it.receivedQty > 0 }
        val newStatus = when {
            allDone -> OrderStatus.COMPLETED
            anyReceived -> OrderStatus.PARTIAL
            else -> order.status
        }
        if (newStatus != order.status) {
            db.inboundDao().updateOrder(order.copy(status = newStatus))
        }
    }

    /**
     * 确认出库单并分配库存：按 FIFO 思路从可用库位锁定 reservedQty，生成拣货任务。
     */
    suspend fun allocateOutbound(orderId: Long): Result<Unit> = withContext(Dispatchers.IO) {
        runCatching {
            val bundle = db.outboundDao().getOrderWithLines(orderId)
                ?: error("出库单不存在")
            val order = bundle.order
            require(order.status == OrderStatus.CONFIRMED) { "仅已确认出库单可分配" }

            val tasks = mutableListOf<PickTaskEntity>()
            bundle.lines.forEach { line ->
                var need = line.orderedQty - line.allocatedQty
                if (need <= 0) return@forEach
                var allocated = line.allocatedQty
                val balances = db.inventoryDao().findAllocatable(line.skuId, 1)
                for (bal in balances) {
                    if (need <= 0) break
                    val available = bal.quantity - bal.reservedQty
                    if (available <= 0) continue
                    val take = minOf(available, need)
                    db.inventoryDao().update(bal.copy(reservedQty = bal.reservedQty + take))
                    tasks += PickTaskEntity(
                        outboundLineId = line.id,
                        locationId = bal.locationId,
                        skuId = line.skuId,
                        qty = take,
                        pickedQty = 0,
                        status = PickTaskStatus.PENDING,
                        assignee = operator
                    )
                    allocated += take
                    need -= take
                }
                if (need > 0) error("SKU#${line.skuId} 库存不足，缺 $need")
                db.outboundDao().updateLine(line.copy(allocatedQty = allocated))
            }
            if (tasks.isNotEmpty()) db.pickTaskDao().insertAll(tasks)
            db.outboundDao().updateOrder(order.copy(status = OrderStatus.IN_PROGRESS))
            log("出库", "分配", "${order.orderNo} 生成 ${tasks.size} 个拣货任务")
        }
    }

    /** 拣货确认：扣减库位库存与预留量，累计出库行已拣数量 */
    suspend fun confirmPick(taskId: Long, pickedQty: Int): Result<Unit> = withContext(Dispatchers.IO) {
        runCatching {
            require(pickedQty > 0) { "拣货数量必须大于 0" }
            val task = db.pickTaskDao().getById(taskId) ?: error("拣货任务不存在")
            require(task.status != PickTaskStatus.COMPLETED) { "任务已完成" }
            val qty = minOf(pickedQty, task.qty - task.pickedQty)

            val balance = db.inventoryDao().getBalance(task.skuId, task.locationId)
                ?: error("库位库存不存在")
            require(balance.quantity >= qty) { "物理库存不足" }
            require(balance.reservedQty >= qty) { "预留库存不足" }

            db.inventoryDao().update(
                balance.copy(
                    quantity = balance.quantity - qty,
                    reservedQty = balance.reservedQty - qty,
                    updatedAt = System.currentTimeMillis()
                )
            )

            val line = db.outboundDao().getLine(task.outboundLineId) ?: error("出库行不存在")
            db.outboundDao().updateLine(line.copy(pickedQty = line.pickedQty + qty))

            val newPicked = task.pickedQty + qty
            val newStatus = if (newPicked >= task.qty) PickTaskStatus.COMPLETED else PickTaskStatus.PICKING
            db.pickTaskDao().update(
                task.copy(pickedQty = newPicked, status = newStatus)
            )

            val order = db.outboundDao().getOrderWithLines(line.outboundOrderId)?.order
            if (order != null) {
                db.movementDao().insert(
                    StockMovementEntity(
                        type = MovementType.OUTBOUND,
                        skuId = task.skuId,
                        fromLocationId = task.locationId,
                        toLocationId = null,
                        quantity = qty,
                        referenceNo = order.orderNo,
                        operator = operator,
                        createdAt = System.currentTimeMillis()
                    )
                )
                refreshOutboundOrderStatus(order, line.outboundOrderId)
            }
            log("拣货", "确认", "任务#$taskId 拣货 $qty")
        }
    }

    private suspend fun refreshOutboundOrderStatus(order: OutboundOrderEntity, orderId: Long) {
        val lines = db.outboundDao().getOrderWithLines(orderId)?.lines.orEmpty()
        val allPicked = lines.all { it.pickedQty >= it.orderedQty }
        val anyPicked = lines.any { it.pickedQty > 0 }
        val newStatus = when {
            allPicked -> OrderStatus.COMPLETED
            anyPicked -> OrderStatus.PARTIAL
            else -> order.status
        }
        if (newStatus != order.status) {
            db.outboundDao().updateOrder(order.copy(status = newStatus))
        }
    }

    /** 库内移库：源库位减少，目标库位增加 */
    suspend fun transferStock(
        skuId: Long,
        fromLocationId: Long,
        toLocationId: Long,
        qty: Int
    ): Result<Unit> = withContext(Dispatchers.IO) {
        runCatching {
            require(qty > 0) { "数量必须大于 0" }
            require(fromLocationId != toLocationId) { "源与目标库位不能相同" }
            val from = db.inventoryDao().getBalance(skuId, fromLocationId)
                ?: error("源库位无库存")
            val available = from.quantity - from.reservedQty
            require(qty <= available) { "可用库存不足（含预留）" }

            db.inventoryDao().update(
                from.copy(
                    quantity = from.quantity - qty,
                    updatedAt = System.currentTimeMillis()
                )
            )
            addStock(skuId, toLocationId, qty)
            db.movementDao().insert(
                StockMovementEntity(
                    type = MovementType.TRANSFER,
                    skuId = skuId,
                    fromLocationId = fromLocationId,
                    toLocationId = toLocationId,
                    quantity = qty,
                    referenceNo = "TR-${System.currentTimeMillis()}",
                    operator = operator,
                    createdAt = System.currentTimeMillis()
                )
            )
            log("库存", "移库", "SKU#$skuId $qty 从 $fromLocationId → $toLocationId")
        }
    }

    suspend fun getStorageLocations() = withContext(Dispatchers.IO) {
        db.locationDao().getStorageLocations()
    }

    private suspend fun addStock(skuId: Long, locationId: Long, qty: Int) {
        val now = System.currentTimeMillis()
        val existing = db.inventoryDao().getBalance(skuId, locationId)
        if (existing != null) {
            db.inventoryDao().update(
                existing.copy(quantity = existing.quantity + qty, updatedAt = now)
            )
        } else {
            db.inventoryDao().upsert(
                InventoryBalanceEntity(
                    skuId = skuId,
                    locationId = locationId,
                    quantity = qty,
                    reservedQty = 0,
                    updatedAt = now
                )
            )
        }
    }

    private suspend fun log(module: String, action: String, detail: String) {
        db.operationLogDao().insert(
            OperationLogEntity(
                module = module,
                action = action,
                detail = detail,
                createdAt = System.currentTimeMillis()
            )
        )
    }
}
