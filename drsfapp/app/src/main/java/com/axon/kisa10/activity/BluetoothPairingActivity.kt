package com.axon.kisa10.activity

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.content.Intent
import android.os.Bundle
import android.provider.Settings
import androidx.appcompat.app.AppCompatActivity
import androidx.databinding.DataBindingUtil
import com.kisa10.R
import com.kisa10.databinding.ActivityBluetoothPairingBinding
import com.axon.kisa10.util.AppConstants

class BluetoothPairingActivity : AppCompatActivity() {
    private lateinit var binding: ActivityBluetoothPairingBinding
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = DataBindingUtil.setContentView(this, R.layout.activity_bluetooth_pairing)
        binding.btnBluetoothSetting.setOnClickListener { _ ->
            val intentOpenBluetoothSettings = Intent()
            intentOpenBluetoothSettings.setAction(Settings.ACTION_BLUETOOTH_SETTINGS)
            startActivity(intentOpenBluetoothSettings)
        }
    }

    @SuppressLint("MissingPermission")
    override fun onResume() {
        super.onResume()
        val adapter = BluetoothAdapter.getDefaultAdapter()
        if (adapter != null) {
            for (device in adapter.bondedDevices) {
                if (device.name != null && device.name == AppConstants.axon_appname) {
                    // device already paired
                    val intent = Intent(this, SearchDeviceActivity::class.java)
                    startActivity(intent)
                    finish()
                }
            }
        }
    }
}