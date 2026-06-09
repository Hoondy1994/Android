package com.example.inputtest

import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentActivity
import androidx.viewpager2.adapter.FragmentStateAdapter
import com.example.inputtest.ui.DispatchFragment
import com.example.inputtest.ui.GestureFragment
import com.example.inputtest.ui.KeyEventFragment
import com.example.inputtest.ui.KeyboardFragment
import com.example.inputtest.ui.NestedScrollFragment
import com.example.inputtest.ui.TouchInspectorFragment

class InputLabPagerAdapter(activity: FragmentActivity) : FragmentStateAdapter(activity) {

    private val factories: List<() -> Fragment> = listOf(
        { TouchInspectorFragment() },
        { GestureFragment() },
        { KeyboardFragment() },
        { KeyEventFragment() },
        { NestedScrollFragment() },
        { DispatchFragment() },
    )

    override fun getItemCount(): Int = factories.size

    override fun createFragment(position: Int): Fragment = factories[position]()
}
