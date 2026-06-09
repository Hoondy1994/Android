package com.example.inputtest

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.example.inputtest.databinding.ActivityMainBinding
import com.google.android.material.tabs.TabLayoutMediator

/**
 * Android Input 综合实验室：触摸 / 手势 / 键盘 IME / 物理按键 / 嵌套滑动 / 事件派发链。
 */
class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    private val tabTitles by lazy {
        arrayOf(
            getString(R.string.tab_touch),
            getString(R.string.tab_gesture),
            getString(R.string.tab_keyboard),
            getString(R.string.tab_key),
            getString(R.string.tab_nested),
            getString(R.string.tab_dispatch),
        )
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setSupportActionBar(binding.toolbar)

        binding.viewPager.adapter = InputLabPagerAdapter(this)
        binding.viewPager.offscreenPageLimit = tabTitles.size

        TabLayoutMediator(binding.tabLayout, binding.viewPager) { tab, position ->
            tab.text = tabTitles[position]
        }.attach()
    }
}
