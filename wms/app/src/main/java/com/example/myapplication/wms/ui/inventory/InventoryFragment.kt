package com.example.myapplication.wms.ui.inventory

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ArrayAdapter
import androidx.appcompat.app.AlertDialog
import androidx.core.widget.addTextChangedListener
import androidx.fragment.app.Fragment
import androidx.fragment.app.viewModels
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import com.example.myapplication.databinding.DialogTransferBinding
import androidx.recyclerview.widget.LinearLayoutManager
import com.example.myapplication.databinding.FragmentInventoryBinding
import com.example.myapplication.wms.data.entity.LocationEntity
import com.example.myapplication.wms.data.model.InventoryDetail
import com.example.myapplication.wms.ui.adapter.InventoryAdapter
import com.example.myapplication.wms.ui.wmsRepository
import com.example.myapplication.wms.ui.wmsViewModelFactory
import com.google.android.material.snackbar.Snackbar
import kotlinx.coroutines.launch

class InventoryFragment : Fragment() {
    private var _binding: FragmentInventoryBinding? = null
    private val binding get() = _binding!!
    private val viewModel: InventoryViewModel by viewModels { wmsViewModelFactory() }
    private val adapter = InventoryAdapter { showTransferDialog(it) }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        _binding = FragmentInventoryBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        binding.recycler.layoutManager = LinearLayoutManager(requireContext())
        binding.recycler.adapter = adapter
        binding.searchInput.addTextChangedListener { viewModel.setQuery(it?.toString().orEmpty()) }
        viewLifecycleOwner.lifecycleScope.launch {
            viewLifecycleOwner.repeatOnLifecycle(Lifecycle.State.STARTED) {
                viewModel.inventory.collect { adapter.submitList(it) }
            }
        }
        viewLifecycleOwner.lifecycleScope.launch {
            viewLifecycleOwner.repeatOnLifecycle(Lifecycle.State.STARTED) {
                viewModel.uiMessage.collect { msg ->
                    msg?.let {
                        Snackbar.make(binding.root, it, Snackbar.LENGTH_SHORT).show()
                        viewModel.clearMessage()
                    }
                }
            }
        }
    }

    private fun showTransferDialog(detail: InventoryDetail) {
        viewLifecycleOwner.lifecycleScope.launch {
            val locations = wmsRepository().getStorageLocations()
            val dialogBinding = DialogTransferBinding.inflate(layoutInflater)
            val labels = locations.map { "${it.locationCode} (${it.zone})" }
            val adapter = ArrayAdapter(requireContext(), android.R.layout.simple_spinner_dropdown_item, labels)
            dialogBinding.spinnerTarget.adapter = adapter
            val defaultIndex = locations.indexOfFirst { it.id != detail.location.id }.coerceAtLeast(0)

            AlertDialog.Builder(requireContext())
                .setTitle("库内移库")
                .setView(dialogBinding.root)
                .setPositiveButton("确认") { _, _ ->
                    val qty = dialogBinding.inputQty.text.toString().toIntOrNull() ?: 0
                    val target: LocationEntity = locations[dialogBinding.spinnerTarget.selectedItemPosition]
                    viewModel.transfer(detail.sku.id, detail.location.id, target.id, qty)
                }
                .setNegativeButton(android.R.string.cancel, null)
                .show()
            dialogBinding.spinnerTarget.setSelection(defaultIndex)
            dialogBinding.textFrom.text = "从 ${detail.location.locationCode} 移出 · 可用 ${detail.availableQty}"
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
