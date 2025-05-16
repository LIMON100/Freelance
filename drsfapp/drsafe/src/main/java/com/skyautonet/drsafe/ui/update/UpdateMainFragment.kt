package com.skyautonet.drsafe.ui.update

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.content.res.ResourcesCompat
import androidx.navigation.fragment.findNavController
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentUpdateMainBinding
import com.skyautonet.drsafe.ui.BaseFragment

/**
 * Created by Hussain on 30/07/24.
 */
class UpdateMainFragment : BaseFragment() {

    private lateinit var binding : FragmentUpdateMainBinding


    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentUpdateMainBinding.inflate(inflater, container, false)
        setupToolbar(binding.toolbar)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
//        binding.llMapDbUpdate.setOnClickListener {
//            findNavController().navigate(R.id.action_updateMainFragment_to_updateDbFragment)
//        }
//
//        binding.llMasterUpdate.setOnClickListener {
//            findNavController().navigate(R.id.action_updateMainFragment_to_updateMasterFragment)
//
//        }
//
//        binding.llCameraUpdate.setOnClickListener {
//            findNavController().navigate(R.id.action_updateMainFragment_to_updateCameraFragment)
//        }
    }

    override fun getStatusBarColor(): Int {
        return ResourcesCompat.getColor(resources, R.color.white, null)
    }

    override fun getToolbarTitle(): String {
        return getString(R.string.update)
    }

    override fun showToolbarImage(): Boolean {
        return false
    }
}