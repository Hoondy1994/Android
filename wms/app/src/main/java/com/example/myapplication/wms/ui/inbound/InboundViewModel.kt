package com.example.myapplication.wms.ui.inbound

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.myapplication.wms.data.entity.InboundOrderEntity
import com.example.myapplication.wms.data.repository.WmsRepository
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch

class InboundViewModel(private val repo: WmsRepository) : ViewModel() {
    val orders: StateFlow<List<InboundOrderEntity>> = repo.observeInboundOrders()
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), emptyList())
}

class InboundDetailViewModel(private val repo: WmsRepository) : ViewModel() {
    private val _message = kotlinx.coroutines.flow.MutableStateFlow<String?>(null)
    val message = _message

    suspend fun loadLines(orderId: Long) = repo.getInboundLines(orderId)

    fun receive(lineId: Long, qty: Int, locationId: Long) {
        viewModelScope.launch {
            repo.receiveInbound(lineId, qty, locationId)
                .onSuccess { _message.value = "收货成功" }
                .onFailure { _message.value = it.message }
        }
    }
}
