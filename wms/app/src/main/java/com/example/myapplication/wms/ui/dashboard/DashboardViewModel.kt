package com.example.myapplication.wms.ui.dashboard

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.myapplication.wms.data.repository.DashboardStats
import com.example.myapplication.wms.data.repository.WmsRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

class DashboardViewModel(private val repo: WmsRepository) : ViewModel() {
    private val _stats = MutableStateFlow<DashboardStats?>(null)
    val stats: StateFlow<DashboardStats?> = _stats.asStateFlow()

    private val _message = MutableStateFlow<String?>(null)
    val message: StateFlow<String?> = _message.asStateFlow()

    init {
        refresh()
    }

    fun refresh() {
        viewModelScope.launch {
            _stats.value = repo.loadDashboard()
        }
    }

    fun clearMessage() {
        _message.value = null
    }
}
