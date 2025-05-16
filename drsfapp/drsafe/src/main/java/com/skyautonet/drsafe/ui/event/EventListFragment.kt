package com.skyautonet.drsafe.ui.event

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.content.res.ResourcesCompat
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentTripListBinding
import com.skyautonet.drsafe.ui.BaseFragment

class EventListFragment : BaseFragment() {

    private lateinit var binding : FragmentTripListBinding

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentTripListBinding.inflate(inflater, container, false)
        setupToolbar(binding.toolbar)
        return binding.root
    }

    override fun getToolbarTitle(): String {
        return "Events"
    }

    override fun showToolbarImage(): Boolean {
        return false
    }

    override fun getStatusBarColor(): Int {
        return ResourcesCompat.getColor(resources, R.color.white, null)
    }
}