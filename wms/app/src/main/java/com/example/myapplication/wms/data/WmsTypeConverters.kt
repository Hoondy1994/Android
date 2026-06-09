package com.example.myapplication.wms.data

import androidx.room.TypeConverter
import com.example.myapplication.wms.domain.LocationType
import com.example.myapplication.wms.domain.MovementType
import com.example.myapplication.wms.domain.OrderStatus
import com.example.myapplication.wms.domain.PickTaskStatus

class WmsTypeConverters {
    @TypeConverter fun fromOrderStatus(v: OrderStatus) = v.name
    @TypeConverter fun toOrderStatus(v: String) = OrderStatus.valueOf(v)

    @TypeConverter fun fromPickStatus(v: PickTaskStatus) = v.name
    @TypeConverter fun toPickStatus(v: String) = PickTaskStatus.valueOf(v)

    @TypeConverter fun fromMovementType(v: MovementType) = v.name
    @TypeConverter fun toMovementType(v: String) = MovementType.valueOf(v)

    @TypeConverter fun fromLocationType(v: LocationType) = v.name
    @TypeConverter fun toLocationType(v: String) = LocationType.valueOf(v)
}
