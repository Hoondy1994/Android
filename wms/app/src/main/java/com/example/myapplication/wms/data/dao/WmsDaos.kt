package com.example.myapplication.wms.data.dao

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Transaction
import androidx.room.Update
import com.example.myapplication.wms.data.entity.InboundLineEntity
import com.example.myapplication.wms.data.entity.InboundOrderEntity
import com.example.myapplication.wms.data.entity.InventoryBalanceEntity
import com.example.myapplication.wms.data.entity.LocationEntity
import com.example.myapplication.wms.data.entity.OperationLogEntity
import com.example.myapplication.wms.data.entity.OutboundLineEntity
import com.example.myapplication.wms.data.entity.OutboundOrderEntity
import com.example.myapplication.wms.data.entity.PickTaskEntity
import com.example.myapplication.wms.data.entity.SkuEntity
import com.example.myapplication.wms.data.entity.StockMovementEntity
import com.example.myapplication.wms.data.entity.WarehouseEntity
import com.example.myapplication.wms.data.model.InboundLineWithSku
import com.example.myapplication.wms.data.model.InboundOrderWithLines
import com.example.myapplication.wms.data.model.InventoryDetail
import com.example.myapplication.wms.data.model.MovementDetail
import com.example.myapplication.wms.data.model.OutboundOrderWithLines
import com.example.myapplication.wms.data.model.PickTaskDetail
import com.example.myapplication.wms.domain.OrderStatus
import com.example.myapplication.wms.domain.PickTaskStatus
import kotlinx.coroutines.flow.Flow

@Dao
interface WarehouseDao {
    @Insert suspend fun insert(entity: WarehouseEntity): Long
    @Query("SELECT * FROM warehouse LIMIT 1") suspend fun getDefault(): WarehouseEntity?
}

@Dao
interface LocationDao {
    @Insert suspend fun insertAll(list: List<LocationEntity>)
    @Query("SELECT * FROM location ORDER BY locationCode") fun observeAll(): Flow<List<LocationEntity>>
    @Query("SELECT * FROM location WHERE type = 'STORAGE' ORDER BY locationCode")
    suspend fun getStorageLocations(): List<LocationEntity>
    @Query("SELECT * FROM location WHERE id = :id") suspend fun getById(id: Long): LocationEntity?
    @Query("SELECT * FROM location WHERE type = 'RECEIVING' LIMIT 1")
    suspend fun getDefaultReceiving(): LocationEntity?
}

@Dao
interface SkuDao {
    @Insert suspend fun insertAll(list: List<SkuEntity>)
    @Query("SELECT * FROM sku ORDER BY skuCode") fun observeAll(): Flow<List<SkuEntity>>
    @Query("SELECT * FROM sku WHERE id = :id") suspend fun getById(id: Long): SkuEntity?
    @Query("SELECT COUNT(*) FROM sku") suspend fun count(): Int
    @Query("SELECT * FROM sku ORDER BY id") suspend fun getAllSync(): List<SkuEntity>
}

@Dao
interface InventoryDao {
    @Insert(onConflict = OnConflictStrategy.REPLACE) suspend fun upsert(entity: InventoryBalanceEntity): Long
    @Update suspend fun update(entity: InventoryBalanceEntity)
    @Transaction
    @Query("SELECT * FROM inventory_balance WHERE quantity > 0 ORDER BY updatedAt DESC")
    fun observeDetails(): Flow<List<InventoryDetail>>
    @Query("SELECT * FROM inventory_balance WHERE skuId = :skuId AND locationId = :locationId")
    suspend fun getBalance(skuId: Long, locationId: Long): InventoryBalanceEntity?
    @Query("SELECT SUM(quantity - reservedQty) FROM inventory_balance WHERE skuId = :skuId")
    suspend fun getTotalAvailable(skuId: Long): Int?
    @Query(
        """
        SELECT * FROM inventory_balance 
        WHERE skuId = :skuId AND (quantity - reservedQty) >= :needQty
        ORDER BY (quantity - reservedQty) DESC
        """
    )
    suspend fun findAllocatable(skuId: Long, needQty: Int): List<InventoryBalanceEntity>
}

@Dao
interface InboundDao {
    @Insert suspend fun insertOrder(entity: InboundOrderEntity): Long
    @Insert suspend fun insertLines(list: List<InboundLineEntity>)
    @Update suspend fun updateOrder(entity: InboundOrderEntity)
    @Update suspend fun updateLine(entity: InboundLineEntity)
    @Query("SELECT * FROM inbound_order ORDER BY createdAt DESC")
    fun observeOrders(): Flow<List<InboundOrderEntity>>
    @Transaction
    @Query("SELECT * FROM inbound_order WHERE id = :orderId")
    suspend fun getOrderWithLines(orderId: Long): InboundOrderWithLines?
    @Transaction
    @Query("SELECT * FROM inbound_line WHERE inboundOrderId = :orderId")
    suspend fun getLinesWithSku(orderId: Long): List<InboundLineWithSku>
    @Query("SELECT * FROM inbound_line WHERE id = :lineId") suspend fun getLine(lineId: Long): InboundLineEntity?
}

@Dao
interface OutboundDao {
    @Insert suspend fun insertOrder(entity: OutboundOrderEntity): Long
    @Insert suspend fun insertLines(list: List<OutboundLineEntity>)
    @Update suspend fun updateOrder(entity: OutboundOrderEntity)
    @Update suspend fun updateLine(entity: OutboundLineEntity)
    @Query("SELECT * FROM outbound_order ORDER BY priority DESC, createdAt DESC")
    fun observeOrders(): Flow<List<OutboundOrderEntity>>
    @Transaction
    @Query("SELECT * FROM outbound_order WHERE id = :orderId")
    suspend fun getOrderWithLines(orderId: Long): OutboundOrderWithLines?
    @Query("SELECT * FROM outbound_line WHERE id = :lineId") suspend fun getLine(lineId: Long): OutboundLineEntity?
}

@Dao
interface PickTaskDao {
    @Insert suspend fun insertAll(list: List<PickTaskEntity>)
    @Update suspend fun update(entity: PickTaskEntity)
    @Transaction
    @Query("SELECT * FROM pick_task WHERE status != 'COMPLETED' ORDER BY id")
    fun observeOpenTasks(): Flow<List<PickTaskDetail>>
    @Query("SELECT * FROM pick_task WHERE id = :id") suspend fun getById(id: Long): PickTaskEntity?
}

@Dao
interface MovementDao {
    @Insert suspend fun insert(entity: StockMovementEntity)
    @Transaction
    @Query("SELECT * FROM stock_movement ORDER BY createdAt DESC LIMIT 100")
    fun observeRecent(): Flow<List<MovementDetail>>
}

@Dao
interface OperationLogDao {
    @Insert suspend fun insert(entity: OperationLogEntity)
    @Query("SELECT * FROM operation_log ORDER BY createdAt DESC LIMIT 50")
    fun observeRecent(): Flow<List<OperationLogEntity>>
}

@Dao
interface DashboardDao {
    @Query("SELECT COUNT(*) FROM sku") suspend fun skuCount(): Int
    @Query("SELECT COUNT(*) FROM location") suspend fun locationCount(): Int
    @Query("SELECT COALESCE(SUM(quantity), 0) FROM inventory_balance") suspend fun totalOnHand(): Int
    @Query("SELECT COUNT(*) FROM inbound_order WHERE status IN ('CONFIRMED','IN_PROGRESS','PARTIAL')")
    suspend fun openInboundCount(): Int
    @Query("SELECT COUNT(*) FROM outbound_order WHERE status IN ('CONFIRMED','IN_PROGRESS','PARTIAL')")
    suspend fun openOutboundCount(): Int
    @Query("SELECT COUNT(*) FROM pick_task WHERE status != 'COMPLETED'") suspend fun openPickCount(): Int
}
