package com.skyautonet.drsafe.ui.update

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.content.res.ResourcesCompat
import androidx.navigation.fragment.findNavController
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentCameraUpdateBinding
import com.skyautonet.drsafe.databinding.FragmentFirmwareUpdateBinding
import com.skyautonet.drsafe.ui.BaseFragment
import com.skyautonet.drsafe.util.Constants

/**
 * Created by Hussain on 30/07/24.
 */
class UpdateCameraFragment : BaseFragment() {

    private lateinit var binding: FragmentCameraUpdateBinding


    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentCameraUpdateBinding.inflate(inflater, container, false)
        setupToolbar(binding.toolbar)
        binding.updateLayout.btnUpdate.setOnClickListener {
            val bundle = Bundle()
            bundle.putString(Constants.EXTRA_TITLE, getString(R.string.camera_firmware_update))
//            findNavController().navigate(R.id.action_updateCameraFragment_to_updateProgressFragment,bundle)
        }
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.updateLayout.swUpdateNotification.setOnClickListener {
            binding.updateLayout.swUpdateNotification.toggle()
        }

        binding.updateLayout.swAutoUpdate.setOnClickListener {
            binding.updateLayout.swAutoUpdate.toggle()
        }
    }

    override fun shouldShowBottomNavigation(): Boolean {
        return false
    }

    override fun getStatusBarColor(): Int {
        return ResourcesCompat.getColor(resources, R.color.white, null)
    }

    override fun getToolbarTitle(): String {
        return getString(R.string.camera_firmware_update)
    }

    override fun showToolbarImage(): Boolean {
        return true
    }
}