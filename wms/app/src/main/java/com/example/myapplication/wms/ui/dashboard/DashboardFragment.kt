package com.example.myapplication.wms.ui.dashboard

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.fragment.app.viewModels
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import com.example.myapplication.databinding.FragmentDashboardBinding
import com.example.myapplication.wms.ui.wmsViewModelFactory
import kotlinx.coroutines.launch

class DashboardFragment : Fragment() {
    private var _binding: FragmentDashboardBinding? = null
    private val binding get() = _binding!!
    private val viewModel: DashboardViewModel by viewModels { wmsViewModelFactory() }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        _binding = FragmentDashboardBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        viewLifecycleOwner.lifecycleScope.launch {
            viewLifecycleOwner.repeatOnLifecycle(Lifecycle.State.STARTED) {
                viewModel.stats.collect { stats ->
                    if (stats == null) return@collect
                    binding.textSkuCount.text = stats.skuCount.toString()
                    binding.textLocationCount.text = stats.locationCount.toString()
                    binding.textOnHand.text = stats.totalOnHand.toString()
                    binding.textOpenInbound.text = stats.openInbound.toString()
                    binding.textOpenOutbound.text = stats.openOutbound.toString()
                    binding.textOpenPick.text = stats.openPicks.toString()
                }
            }
        }
    }

    override fun onResume() {
        super.onResume()
        viewModel.refresh()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
