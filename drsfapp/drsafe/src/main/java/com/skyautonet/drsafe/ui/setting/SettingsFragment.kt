package com.skyautonet.drsafe.ui.setting

import android.graphics.drawable.Drawable
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.content.res.ResourcesCompat
import androidx.navigation.fragment.findNavController
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentSettingsBinding
import com.skyautonet.drsafe.ui.BaseFragment

/**
 * Created by Hussain on 31/07/24.
 */
class SettingsFragment : BaseFragment() {


    private lateinit var binding : FragmentSettingsBinding

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentSettingsBinding.inflate(inflater, container, false)
        setupToolbar(binding.toolbar)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        binding.llVehicle.setOnClickListener {
//            findNavController().navigate(R.id.action_settingsFragment_to_registerVehicleFragment)
        }

        binding.llDvrOptions.setOnClickListener {
//            findNavController().navigate(R.id.action_settingsFragment_to_dvrSettingsFragment)
        }

//        binding.llMasterDevice.setOnClickListener {
//            findNavController().navigate(R.id.action_settingsFragment_to_masterDeviceFragment)
//        }
//
//        binding.llCameraDevice.setOnClickListener {
//            findNavController().navigate(R.id.action_settingsFragment_to_cameraDeviceFragment)
//        }
//
//        binding.llAudioSettings.setOnClickListener {
//            findNavController().navigate(R.id.action_settingsFragment_to_audioSettingsFragment)
//        }
    }

    override fun getStatusBarColor(): Int {
        return ResourcesCompat.getColor(resources, R.color.white, null)
    }

    override fun getToolbarTitle(): String {
        return getString(R.string.setting)
    }

    override fun showToolbarImage(): Boolean {
        return false
    }
}