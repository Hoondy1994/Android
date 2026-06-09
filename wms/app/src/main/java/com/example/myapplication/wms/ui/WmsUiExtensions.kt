package com.example.myapplication.wms.ui

import androidx.fragment.app.Fragment
import com.example.myapplication.WmsApplication
import com.example.myapplication.wms.data.repository.WmsRepository
import com.example.myapplication.wms.ui.WmsViewModelFactory

fun Fragment.wmsRepository(): WmsRepository =
    (requireActivity().application as WmsApplication).repository

fun Fragment.wmsViewModelFactory(): WmsViewModelFactory =
    WmsViewModelFactory(wmsRepository())
