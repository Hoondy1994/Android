package com.example.myapplication.wms.ui.inbound

import android.os.Bundle
import android.widget.ArrayAdapter
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.example.myapplication.R
import com.example.myapplication.databinding.ActivityInboundDetailBinding
import com.example.myapplication.databinding.DialogReceiveBinding
import com.example.myapplication.wms.data.model.InboundLineWithSku
import com.example.myapplication.wms.ui.wmsRepository
import com.google.android.material.snackbar.Snackbar
import kotlinx.coroutines.launch

class InboundDetailActivity : AppCompatActivity() {
    private lateinit var binding: ActivityInboundDetailBinding
    private val detailVm = InboundDetailViewModel(wmsRepository())
    private var lines: List<InboundLineWithSku> = emptyList()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityInboundDetailBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val orderId = intent.getLongExtra(EXTRA_ORDER_ID, -1)
        val orderNo = intent.getStringExtra(EXTRA_ORDER_NO).orEmpty()
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        title = orderNo

        lifecycleScope.launch {
            lines = detailVm.loadLines(orderId)
            renderLines()
        }

        lifecycleScope.launch {
            detailVm.message.collect { msg ->
                msg?.let {
                    Snackbar.make(binding.root, it, Snackbar.LENGTH_SHORT).show()
                }
            }
        }
    }

    private fun renderLines() {
        binding.linesContainer.removeAllViews()
        val inflater = layoutInflater
        lines.forEach { row ->
            val line = row.line
            val remain = line.expectedQty - line.receivedQty
            val view = inflater.inflate(R.layout.item_inbound_line, binding.linesContainer, false)
            view.findViewById<android.widget.TextView>(R.id.text_sku).text =
                "${row.sku.skuCode} · ${row.sku.name}"
            view.findViewById<android.widget.TextView>(R.id.text_qty).text =
                "应收 ${line.expectedQty} / 已收 ${line.receivedQty}"
            view.findViewById<com.google.android.material.button.MaterialButton>(R.id.btn_receive).apply {
                isEnabled = remain > 0
                setOnClickListener { showReceiveDialog(line.id, remain, row.sku.name) }
            }
            binding.linesContainer.addView(view)
        }
    }

    private fun showReceiveDialog(lineId: Long, maxQty: Int, skuName: String) {
        lifecycleScope.launch {
            val locations = wmsRepository().getStorageLocations()
            val dialogBinding = DialogReceiveBinding.inflate(layoutInflater)
            dialogBinding.textHint.text = "$skuName · 最多可收 $maxQty"
            dialogBinding.spinnerLocation.adapter = ArrayAdapter(
                this@InboundDetailActivity,
                android.R.layout.simple_spinner_dropdown_item,
                locations.map { it.locationCode }
            )
            AlertDialog.Builder(this@InboundDetailActivity)
                .setTitle("收货上架")
                .setView(dialogBinding.root)
                .setPositiveButton("确认") { _, _ ->
                    val qty = dialogBinding.inputQty.text.toString().toIntOrNull() ?: 0
                    val locId = locations[dialogBinding.spinnerLocation.selectedItemPosition].id
                    detailVm.receive(lineId, qty, locId)
                    lifecycleScope.launch {
                        kotlinx.coroutines.delay(400)
                        lines = detailVm.loadLines(intent.getLongExtra(EXTRA_ORDER_ID, -1))
                        renderLines()
                    }
                }
                .show()
        }
    }

    override fun onSupportNavigateUp(): Boolean {
        finish()
        return true
    }

    companion object {
        const val EXTRA_ORDER_ID = "order_id"
        const val EXTRA_ORDER_NO = "order_no"
    }

    private fun wmsRepository() =
        (application as com.example.myapplication.WmsApplication).repository
}
