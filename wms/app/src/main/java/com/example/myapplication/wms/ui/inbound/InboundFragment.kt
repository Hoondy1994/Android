package com.example.myapplication.wms.ui.inbound

import android.content.Intent
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
import com.example.myapplication.databinding.FragmentInboundBinding
import com.example.myapplication.wms.ui.adapter.InboundOrderAdapter
import com.example.myapplication.wms.ui.wmsViewModelFactory
import kotlinx.coroutines.launch

class InboundFragment : Fragment() {
    private var _binding: FragmentInboundBinding? = null
    private val binding get() = _binding!!
    private val viewModel: InboundViewModel by viewModels { wmsViewModelFactory() }
    private val adapter = InboundOrderAdapter { order ->
        startActivity(
            Intent(requireContext(), InboundDetailActivity::class.java)
                .putExtra(InboundDetailActivity.EXTRA_ORDER_ID, order.id)
                .putExtra(InboundDetailActivity.EXTRA_ORDER_NO, order.orderNo)
        )
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        _binding = FragmentInboundBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        binding.recycler.layoutManager = LinearLayoutManager(requireContext())
        binding.recycler.adapter = adapter
        viewLifecycleOwner.lifecycleScope.launch {
            viewLifecycleOwner.repeatOnLifecycle(Lifecycle.State.STARTED) {
                viewModel.orders.collect { adapter.submitList(it) }
            }
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
