package com.example.myapplication.wms.ui.pick

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.appcompat.app.AlertDialog
import androidx.fragment.app.Fragment
import androidx.fragment.app.viewModels
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import com.example.myapplication.databinding.DialogPickQtyBinding
import androidx.recyclerview.widget.LinearLayoutManager
import com.example.myapplication.databinding.FragmentPickBinding
import com.example.myapplication.wms.data.model.PickTaskDetail
import com.example.myapplication.wms.ui.adapter.PickTaskAdapter
import com.example.myapplication.wms.ui.wmsViewModelFactory
import com.google.android.material.snackbar.Snackbar
import kotlinx.coroutines.launch

class PickFragment : Fragment() {
    private var _binding: FragmentPickBinding? = null
    private val binding get() = _binding!!
    private val viewModel: PickViewModel by viewModels { wmsViewModelFactory() }
    private val adapter = PickTaskAdapter { showPickDialog(it) }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        _binding = FragmentPickBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        binding.recycler.layoutManager = LinearLayoutManager(requireContext())
        binding.recycler.adapter = adapter
        viewLifecycleOwner.lifecycleScope.launch {
            viewLifecycleOwner.repeatOnLifecycle(Lifecycle.State.STARTED) {
                viewModel.tasks.collect { adapter.submitList(it) }
            }
        }
        viewLifecycleOwner.lifecycleScope.launch {
            viewLifecycleOwner.repeatOnLifecycle(Lifecycle.State.STARTED) {
                viewModel.message.collect { msg ->
                    msg?.let {
                        Snackbar.make(binding.root, it, Snackbar.LENGTH_SHORT).show()
                        viewModel.clearMessage()
                    }
                }
            }
        }
    }

    private fun showPickDialog(detail: PickTaskDetail) {
        val remain = detail.task.qty - detail.task.pickedQty
        val dialogBinding = DialogPickQtyBinding.inflate(layoutInflater)
        dialogBinding.textHint.text = "本次最多拣 $remain"
        dialogBinding.inputQty.setText(remain.toString())
        AlertDialog.Builder(requireContext())
            .setTitle("确认拣货")
            .setView(dialogBinding.root)
            .setPositiveButton("确认") { _, _ ->
                val qty = dialogBinding.inputQty.text.toString().toIntOrNull() ?: 0
                viewModel.confirmPick(detail.task.id, qty)
            }
            .show()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
