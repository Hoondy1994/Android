package com.example.myapplication

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.Fragment
import com.example.myapplication.databinding.ActivityMainBinding
import com.example.myapplication.wms.ui.dashboard.DashboardFragment
import com.example.myapplication.wms.ui.inbound.InboundFragment
import com.example.myapplication.wms.ui.inventory.InventoryFragment
import com.example.myapplication.wms.ui.outbound.OutboundFragment
import com.example.myapplication.wms.ui.pick.PickFragment

class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        setSupportActionBar(binding.toolbar)

        binding.bottomNav.setOnItemSelectedListener { item ->
            val fragment: Fragment = when (item.itemId) {
                R.id.nav_dashboard -> DashboardFragment()
                R.id.nav_inventory -> InventoryFragment()
                R.id.nav_inbound -> InboundFragment()
                R.id.nav_outbound -> OutboundFragment()
                R.id.nav_pick -> PickFragment()
                else -> DashboardFragment()
            }
            supportFragmentManager.beginTransaction()
                .replace(R.id.fragment_container, fragment)
                .commit()
            true
        }

        if (savedInstanceState == null) {
            binding.bottomNav.selectedItemId = R.id.nav_dashboard
        }
    }
}
