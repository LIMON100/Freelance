package com.skyautonet.drsafe.ui.setting.wifi

import android.content.Intent
import android.graphics.drawable.Drawable
import android.os.Bundle
import android.provider.Settings
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentMasterDeviceBinding
import com.skyautonet.drsafe.ui.BaseFragment
import com.skyautonet.drsafe.ui.dialog.ConfirmationDialog
import com.skyautonet.drsafe.util.DialogType

/**
 * Created by Hussain on 01/08/24.
 */
class MasterDeviceFragment : BaseFragment() {

    private lateinit var binding : FragmentMasterDeviceBinding

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentMasterDeviceBinding.inflate(inflater, container, false)
        setupToolbar(binding.toolbar)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

//        binding.wifiSettings.btnWifiReset.setOnClickListener {
//            showWifiResetDialog()
//        }

        binding.wifiList.tvWifiSettings.setOnClickListener {
            val intent = Intent(Settings.ACTION_WIFI_SETTINGS)
            startActivity(intent)
        }
    }

    private fun showWifiResetDialog() {
        val dialog = ConfirmationDialog(dialogType = DialogType.RESET_WIFI)
        dialog.show(requireActivity().supportFragmentManager, null)
    }

    override fun getStatusBarColor(): Int {
        return getWhiteStatusBarColor()
    }

    override fun getToolbarTitle(): String {
        return getString(R.string.master_device)
    }

    override fun showToolbarImage(): Boolean {
        return true
    }

    override fun shouldShowBottomNavigation(): Boolean {
        return false
    }
}