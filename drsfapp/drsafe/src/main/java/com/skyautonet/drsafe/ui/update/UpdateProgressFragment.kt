package com.skyautonet.drsafe.ui.update

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.content.res.ResourcesCompat
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentUpdateProgressBinding
import com.skyautonet.drsafe.ui.BaseFragment
import com.skyautonet.drsafe.util.Constants

/**
 * Created by Hussain on 30/07/24.
 */
class UpdateProgressFragment : BaseFragment() {

    private lateinit var binding : FragmentUpdateProgressBinding
    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentUpdateProgressBinding.inflate(inflater, container, false)
        setupToolbar(binding.toolbar)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
    }

    override fun getStatusBarColor(): Int {
        return ResourcesCompat.getColor(resources, R.color.white, null)
    }

    override fun getToolbarTitle(): String {
        return arguments?.getString(Constants.EXTRA_TITLE,getString(R.string.update)) ?: getString(R.string.update)
    }

    override fun showToolbarImage(): Boolean {
        return true
    }

    override fun shouldShowBottomNavigation(): Boolean {
        return false
    }
}