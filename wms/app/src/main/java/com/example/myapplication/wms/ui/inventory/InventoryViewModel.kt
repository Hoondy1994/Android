package com.example.myapplication.wms.ui.inventory

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.myapplication.wms.data.model.InventoryDetail
import com.example.myapplication.wms.data.repository.WmsRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch

class InventoryViewModel(private val repo: WmsRepository) : ViewModel() {
    private val query = MutableStateFlow("")

    val inventory: StateFlow<List<InventoryDetail>> = combine(
        repo.observeInventory(),
        query
    ) { list, q ->
        if (q.isBlank()) list
        else list.filter {
            it.sku.name.contains(q, true) ||
                it.sku.skuCode.contains(q, true) ||
                it.location.locationCode.contains(q, true)
        }
    }.stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), emptyList())

    private val _uiMessage = MutableStateFlow<String?>(null)
    val uiMessage: StateFlow<String?> = _uiMessage.asStateFlow()

    fun setQuery(text: String) {
        query.value = text
    }

    fun transfer(skuId: Long, fromId: Long, toId: Long, qty: Int) {
        viewModelScope.launch {
            repo.transferStock(skuId, fromId, toId, qty)
                .onSuccess { _uiMessage.value = "移库成功" }
                .onFailure { _uiMessage.value = it.message }
        }
    }

    fun clearMessage() {
        _uiMessage.value = null
    }
}
