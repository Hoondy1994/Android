package com.example.myapplication.wms.ui.outbound

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.fragment.app.viewModels
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import androidx.recyclerview.widget.LinearLayoutManager
import com.example.myapplication.databinding.FragmentOutboundBinding
import com.example.myapplication.wms.ui.adapter.OutboundOrderAdapter
import com.example.myapplication.wms.ui.wmsViewModelFactory
import com.google.android.material.snackbar.Snackbar
import kotlinx.coroutines.launch

class OutboundFragment : Fragment() {
    private var _binding: FragmentOutboundBinding? = null
    private val binding get() = _binding!!
    private val viewModel: OutboundViewModel by viewModels { wmsViewModelFactory() }
    private val adapter = OutboundOrderAdapter { viewModel.allocate(it.id) }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        _binding = FragmentOutboundBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        binding.recycler.layoutManager = LinearLayoutManager(requireContext())
        binding.recycler.adapter = adapter
        binding.textHint.text = getString(com.example.myapplication.R.string.outbound_hint)
        viewLifecycleOwner.lifecycleScope.launch {
            viewLifecycleOwner.repeatOnLifecycle(Lifecycle.State.STARTED) {
                viewModel.orders.collect { adapter.submitList(it) }
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

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
