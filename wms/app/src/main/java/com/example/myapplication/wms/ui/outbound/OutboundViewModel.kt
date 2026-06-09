package com.example.myapplication.wms.ui.outbound

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.myapplication.wms.data.entity.OutboundOrderEntity
import com.example.myapplication.wms.data.repository.WmsRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch

class OutboundViewModel(private val repo: WmsRepository) : ViewModel() {
    val orders: StateFlow<List<OutboundOrderEntity>> = repo.observeOutboundOrders()
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), emptyList())

    private val _message = MutableStateFlow<String?>(null)
    val message: StateFlow<String?> = _message.asStateFlow()

    fun allocate(orderId: Long) {
        viewModelScope.launch {
            repo.allocateOutbound(orderId)
                .onSuccess { _message.value = "分配成功，已生成拣货任务" }
                .onFailure { _message.value = it.message }
        }
    }

    fun clearMessage() {
        _message.value = null
    }
}
