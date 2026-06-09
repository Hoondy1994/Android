package com.example.myapplication.wms.ui.pick

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.myapplication.wms.data.model.PickTaskDetail
import com.example.myapplication.wms.data.repository.WmsRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch

class PickViewModel(private val repo: WmsRepository) : ViewModel() {
    val tasks: StateFlow<List<PickTaskDetail>> = repo.observePickTasks()
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), emptyList())

    private val _message = MutableStateFlow<String?>(null)
    val message: StateFlow<String?> = _message.asStateFlow()

    fun confirmPick(taskId: Long, qty: Int) {
        viewModelScope.launch {
            repo.confirmPick(taskId, qty)
                .onSuccess { _message.value = "拣货确认成功" }
                .onFailure { _message.value = it.message }
        }
    }

    fun clearMessage() {
        _message.value = null
    }
}
