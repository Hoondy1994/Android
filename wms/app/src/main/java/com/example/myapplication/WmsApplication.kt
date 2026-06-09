package com.example.myapplication

import android.app.Application
import com.example.myapplication.wms.data.WmsDatabase
import com.example.myapplication.wms.data.repository.WmsRepository
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch

class WmsApplication : Application() {
    private val appScope = CoroutineScope(SupervisorJob() + Dispatchers.IO)

    val database: WmsDatabase by lazy { WmsDatabase.get(this) }
    val repository: WmsRepository by lazy { WmsRepository(database) }

    override fun onCreate() {
        super.onCreate()
        appScope.launch { repository.initialize() }
    }
}
