package com.skyautonet.drsafe.ui.setting.wifi

import android.graphics.drawable.Drawable
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentCameraDeviceBinding
import com.skyautonet.drsafe.ui.BaseFragment

/**
 * Created by Hussain on 01/08/24.
 */
class CameraDeviceFragment : BaseFragment() {

    private lateinit var binding : FragmentCameraDeviceBinding

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentCameraDeviceBinding.inflate(inflater, container, false)
        setupToolbar(binding.toolbar)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        binding.cameraSettings.tvStatusLabel.text = getString(R.string.camera_device)
    }

    override fun getStatusBarColor(): Int {
        return getWhiteStatusBarColor()
    }

    override fun getToolbarTitle(): String {
        return getString(R.string.camera_device)
    }

    override fun showToolbarImage(): Boolean {
        return true
    }

    override fun shouldShowBottomNavigation(): Boolean {
        return false
    }
}