package com.axon.kisa10.fragment

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.activity.MainActivity
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.R
import com.axon.kisa10.databinding.FragmentDrawerBinding

class DrawerFragment : Fragment() {
    private lateinit var binding: FragmentDrawerBinding

    private var localBroadcastManager: LocalBroadcastManager? = null

    private val bleStatusReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            when(intent.action) {
                AppConstants.ACTION_DEVICE_CONNECTED -> {
                    requireActivity().runOnUiThread {
                        binding.drawer.ivConnectStatus.setImageResource(R.drawable.ic_ovel_connected)
                    }
                }

                AppConstants.ACTION_DEVICE_DISCONNECTED -> {
                    requireActivity().runOnUiThread {
                        binding.drawer.ivConnectStatus.setImageResource(R.drawable.ic_ovel_disconnected)
                    }
                }
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        localBroadcastManager = LocalBroadcastManager.getInstance(requireActivity())
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentDrawerBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

//        binding.drawer.llBleFirmwareUpadte.setOnClickListener {
//            val intent = Intent(MainActivity.ACTION_BLE_FIRMWARE_UPDATE_FRAGMENT)
//            localBroadcastManager?.sendBroadcast(intent)
//        }
        binding.drawer.llCalibration.setOnClickListener {
            val intent = Intent(MainActivity.ACTION_CALIBRATION_FRAGMENT)
            localBroadcastManager?.sendBroadcast(intent)
        }
//        binding.drawer.llDbUpdate.setOnClickListener {
//            val intent = Intent(MainActivity.ACTION_DB_UPDATE_FRAGMENT)
//            localBroadcastManager?.sendBroadcast(intent)
//        }
        binding.drawer.llDevice.setOnClickListener {
            val intent = Intent(MainActivity.ACTION_DEVICE_FRAGMENT)
            localBroadcastManager?.sendBroadcast(intent)
        }
        binding.drawer.llFacoryReset.setOnClickListener {
            val intent = Intent(MainActivity.ACTION_FACTORY_RESET_FRAGMENT)
            localBroadcastManager?.sendBroadcast(intent)
        }
        binding.drawer.llObdSetting.setOnClickListener {
            val intent = Intent(MainActivity.ACTION_OBD_SETTINGS_FRAGMENT)
            localBroadcastManager?.sendBroadcast(intent)
        }
//        binding.drawer.llUseridReset.setOnClickListener {
//            val intent = Intent(MainActivity.ACTION_USERID_RESET_FRAGMENT)
//            localBroadcastManager?.sendBroadcast(intent)
//        }
        binding.drawer.llFirmwareUpadte.setOnClickListener {
            val intent = Intent(MainActivity.ACTION_FIRMWARE_UPDATE_FRAGMENT)
            localBroadcastManager?.sendBroadcast(intent)
        }

        binding.drawer.llDataUpdateMenu.setOnClickListener {
            val intent = Intent(MainActivity.ACTION_DATA_UPDATE_FRAGMENT)
            localBroadcastManager?.sendBroadcast(intent)
        }

        binding.drawer.llDbUpdate.setOnClickListener {
            val intent = Intent(MainActivity.ACTION_DB_UPDATE_MAIN_FRAGMENT)
            localBroadcastManager?.sendBroadcast(intent)
        }

        binding.drawer.llLogOut.setOnClickListener {
            val intent = Intent(MainActivity.ACTION_LOGOUT)
            localBroadcastManager?.sendBroadcast(intent)
        }

        binding.drawer.llLogCalibration.setOnClickListener {
            val intent = Intent(MainActivity.ACTION_CALIBRATION_HISTORY)
            localBroadcastManager?.sendBroadcast(intent)
        }

//        binding.drawer.llLanguage.setOnClickListener {
//            val intent = Intent(MainActivity.ACTION_CHANGE_LANGUAGE)
//            localBroadcastManager?.sendBroadcast(intent)
//        }
    }

    override fun onResume() {
        super.onResume()
        val intentFilter = IntentFilter()
        intentFilter.addAction(AppConstants.ACTION_DEVICE_DISCONNECTED)
        intentFilter.addAction(AppConstants.ACTION_DEVICE_CONNECTED)
        localBroadcastManager?.registerReceiver(bleStatusReceiver, intentFilter)
    }

    override fun onPause() {
        super.onPause()
        localBroadcastManager?.unregisterReceiver(bleStatusReceiver)
    }

    companion object {
        fun newInstance(): DrawerFragment {
            return DrawerFragment()
        }
    }
}
