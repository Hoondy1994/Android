package com.example.myapplication.wms.ui

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import com.example.myapplication.wms.data.repository.WmsRepository
import com.example.myapplication.wms.ui.dashboard.DashboardViewModel
import com.example.myapplication.wms.ui.inbound.InboundViewModel
import com.example.myapplication.wms.ui.inventory.InventoryViewModel
import com.example.myapplication.wms.ui.outbound.OutboundViewModel
import com.example.myapplication.wms.ui.pick.PickViewModel

class WmsViewModelFactory(
    private val repository: WmsRepository
) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T = when {
        modelClass.isAssignableFrom(DashboardViewModel::class.java) ->
            DashboardViewModel(repository) as T
        modelClass.isAssignableFrom(InventoryViewModel::class.java) ->
            InventoryViewModel(repository) as T
        modelClass.isAssignableFrom(InboundViewModel::class.java) ->
            InboundViewModel(repository) as T
        modelClass.isAssignableFrom(OutboundViewModel::class.java) ->
            OutboundViewModel(repository) as T
        modelClass.isAssignableFrom(PickViewModel::class.java) ->
            PickViewModel(repository) as T
        else -> error("Unknown ViewModel: ${modelClass.name}")
    }
}
