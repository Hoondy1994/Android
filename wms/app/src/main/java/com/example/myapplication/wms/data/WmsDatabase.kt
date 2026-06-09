package com.example.myapplication.wms.data

import android.content.Context
import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase
import androidx.room.TypeConverters
import com.example.myapplication.wms.data.dao.DashboardDao
import com.example.myapplication.wms.data.dao.InboundDao
import com.example.myapplication.wms.data.dao.InventoryDao
import com.example.myapplication.wms.data.dao.LocationDao
import com.example.myapplication.wms.data.dao.MovementDao
import com.example.myapplication.wms.data.dao.OperationLogDao
import com.example.myapplication.wms.data.dao.OutboundDao
import com.example.myapplication.wms.data.dao.PickTaskDao
import com.example.myapplication.wms.data.dao.SkuDao
import com.example.myapplication.wms.data.dao.WarehouseDao
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

@Database(
    entities = [
        WarehouseEntity::class,
        LocationEntity::class,
        SkuEntity::class,
        InventoryBalanceEntity::class,
        InboundOrderEntity::class,
        InboundLineEntity::class,
        OutboundOrderEntity::class,
        OutboundLineEntity::class,
        PickTaskEntity::class,
        StockMovementEntity::class,
        OperationLogEntity::class
    ],
    version = 1,
    exportSchema = false
)
@TypeConverters(WmsTypeConverters::class)
abstract class WmsDatabase : RoomDatabase() {
    abstract fun warehouseDao(): WarehouseDao
    abstract fun locationDao(): LocationDao
    abstract fun skuDao(): SkuDao
    abstract fun inventoryDao(): InventoryDao
    abstract fun inboundDao(): InboundDao
    abstract fun outboundDao(): OutboundDao
    abstract fun pickTaskDao(): PickTaskDao
    abstract fun movementDao(): MovementDao
    abstract fun operationLogDao(): OperationLogDao
    abstract fun dashboardDao(): DashboardDao

    companion object {
        @Volatile private var instance: WmsDatabase? = null

        fun get(context: Context): WmsDatabase =
            instance ?: synchronized(this) {
                instance ?: Room.databaseBuilder(
                    context.applicationContext,
                    WmsDatabase::class.java,
                    "wms_learning.db"
                ).build().also { instance = it }
            }
    }
}
