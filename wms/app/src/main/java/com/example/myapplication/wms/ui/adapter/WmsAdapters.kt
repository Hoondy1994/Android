package com.example.myapplication.wms.ui.adapter

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.example.myapplication.databinding.ItemInboundOrderBinding
import com.example.myapplication.databinding.ItemInventoryBinding
import com.example.myapplication.databinding.ItemOutboundOrderBinding
import com.example.myapplication.databinding.ItemPickTaskBinding
import com.example.myapplication.wms.data.entity.InboundOrderEntity
import com.example.myapplication.wms.data.entity.OutboundOrderEntity
import com.example.myapplication.wms.data.model.InventoryDetail
import com.example.myapplication.wms.data.model.PickTaskDetail
import com.example.myapplication.wms.ui.toLabel
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

private val dateFmt = SimpleDateFormat("MM-dd HH:mm", Locale.CHINA)

class InventoryAdapter(
    private val onTransfer: (InventoryDetail) -> Unit
) : ListAdapter<InventoryDetail, InventoryAdapter.VH>(diff) {
    class VH(val binding: ItemInventoryBinding) : RecyclerView.ViewHolder(binding.root)

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int) = VH(
        ItemInventoryBinding.inflate(LayoutInflater.from(parent.context), parent, false)
    )

    override fun onBindViewHolder(holder: VH, position: Int) {
        val item = getItem(position)
        holder.binding.apply {
            textSku.text = "${item.sku.skuCode} · ${item.sku.name}"
            textLocation.text = "库位 ${item.location.locationCode} (${item.location.zone}区)"
            textQty.text = "在库 ${item.balance.quantity} | 预留 ${item.balance.reservedQty} | 可用 ${item.availableQty}"
            btnTransfer.setOnClickListener { onTransfer(item) }
        }
    }

    companion object {
        private val diff = object : DiffUtil.ItemCallback<InventoryDetail>() {
            override fun areItemsTheSame(a: InventoryDetail, b: InventoryDetail) = a.balance.id == b.balance.id
            override fun areContentsTheSame(a: InventoryDetail, b: InventoryDetail) = a == b
        }
    }
}

class InboundOrderAdapter(
    private val onClick: (InboundOrderEntity) -> Unit
) : ListAdapter<InboundOrderEntity, InboundOrderAdapter.VH>(diff) {
    class VH(val binding: ItemInboundOrderBinding) : RecyclerView.ViewHolder(binding.root)

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int) = VH(
        ItemInboundOrderBinding.inflate(LayoutInflater.from(parent.context), parent, false)
    )

    override fun onBindViewHolder(holder: VH, position: Int) {
        val o = getItem(position)
        holder.binding.apply {
            textOrderNo.text = o.orderNo
            textSupplier.text = o.supplier
            textStatus.text = o.status.toLabel()
            textTime.text = dateFmt.format(Date(o.createdAt))
            root.setOnClickListener { onClick(o) }
        }
    }

    companion object {
        private val diff = object : DiffUtil.ItemCallback<InboundOrderEntity>() {
            override fun areItemsTheSame(a: InboundOrderEntity, b: InboundOrderEntity) = a.id == b.id
            override fun areContentsTheSame(a: InboundOrderEntity, b: InboundOrderEntity) = a == b
        }
    }
}

class OutboundOrderAdapter(
    private val onAllocate: (OutboundOrderEntity) -> Unit
) : ListAdapter<OutboundOrderEntity, OutboundOrderAdapter.VH>(diff) {
    class VH(val binding: ItemOutboundOrderBinding) : RecyclerView.ViewHolder(binding.root)

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int) = VH(
        ItemOutboundOrderBinding.inflate(LayoutInflater.from(parent.context), parent, false)
    )

    override fun onBindViewHolder(holder: VH, position: Int) {
        val o = getItem(position)
        holder.binding.apply {
            textOrderNo.text = o.orderNo
            textCustomer.text = o.customer
            textStatus.text = o.status.toLabel()
            textPriority.text = "优先级 ${o.priority}"
            btnAllocate.isEnabled = o.status == com.example.myapplication.wms.domain.OrderStatus.CONFIRMED
            btnAllocate.setOnClickListener { onAllocate(o) }
        }
    }

    companion object {
        private val diff = object : DiffUtil.ItemCallback<OutboundOrderEntity>() {
            override fun areItemsTheSame(a: OutboundOrderEntity, b: OutboundOrderEntity) = a.id == b.id
            override fun areContentsTheSame(a: OutboundOrderEntity, b: OutboundOrderEntity) = a == b
        }
    }
}

class PickTaskAdapter(
    private val onConfirm: (PickTaskDetail) -> Unit
) : ListAdapter<PickTaskDetail, PickTaskAdapter.VH>(diff) {
    class VH(val binding: ItemPickTaskBinding) : RecyclerView.ViewHolder(binding.root)

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int) = VH(
        ItemPickTaskBinding.inflate(LayoutInflater.from(parent.context), parent, false)
    )

    override fun onBindViewHolder(holder: VH, position: Int) {
        val t = getItem(position)
        holder.binding.apply {
            textSku.text = "${t.sku.skuCode} · ${t.sku.name}"
            textLocation.text = "拣货库位 ${t.location.locationCode}"
            textQty.text = "任务 ${t.task.pickedQty}/${t.task.qty} · ${t.task.status.toLabel()}"
            btnConfirm.isEnabled = t.task.status != com.example.myapplication.wms.domain.PickTaskStatus.COMPLETED
            btnConfirm.setOnClickListener { onConfirm(t) }
        }
    }

    companion object {
        private val diff = object : DiffUtil.ItemCallback<PickTaskDetail>() {
            override fun areItemsTheSame(a: PickTaskDetail, b: PickTaskDetail) = a.task.id == b.task.id
            override fun areContentsTheSame(a: PickTaskDetail, b: PickTaskDetail) = a == b
        }
    }
}
