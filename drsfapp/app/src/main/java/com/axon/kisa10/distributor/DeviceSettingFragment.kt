package com.axon.kisa10.distributor

import android.bluetooth.BluetoothGattCharacteristic
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AppMethods.hideProgressDialog
import com.axon.kisa10.util.AppMethods.setAlertDialog
import com.axon.kisa10.R
import com.axon.kisa10.databinding.FragmentDeviceSettingBinding

class DeviceSettingFragment(private val bleService: AxonBLEService?) : Fragment() {

    private lateinit var binding : FragmentDeviceSettingBinding

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(requireContext())
    }

    private var pauEnabled = 0
    private var speedEnabled = 0

    private var isInitialCall = true

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val intentFilter = IntentFilter()
        intentFilter.addAction(AppConstants.ACTION_DEVICE_DISCONNECTED)
        intentFilter.addAction(AppConstants.ACTION_CHARACTERISTIC_CHANGED)
        localBroadcastManager.registerReceiver(receiver, intentFilter)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentDeviceSettingBinding.inflate(inflater, container, false)
        getDeviceSettings()
        return binding.root
    }

    private fun getDeviceSettings() {
        val cmdBTO = "AT$\$DS_BTO_EN?\r\n".toByteArray(charset("euc-kr"))
        bleService?.writeCharacteristic(cmdBTO, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)

        Thread.sleep(100)

        val MaxSpeedCmd = "AT$\$DS_BTO_SPEED?\r\n".toByteArray(charset("euc-kr"))
        bleService?.writeCharacteristic(MaxSpeedCmd, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)

        Thread.sleep(100)


        val MaxRPMCmd = "AT$\$DS_BTO_RPM?\r\n".toByteArray(charset("euc-kr"))
        bleService?.writeCharacteristic(MaxRPMCmd, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)

        Thread.sleep(100)

        val VolumeCmd = "AT$\$DS_VOLUME?\r\n".toByteArray(charset("euc-kr"))
        bleService?.writeCharacteristic(VolumeCmd, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.btnChangeRpm.setOnClickListener {
            val maxRpm = binding.etRpm.text.toString().toIntOrNull() ?: return@setOnClickListener
            if (maxRpm in 1..9999) {
                val cmd = "AT$\$DS_BTO_RPM=1,$maxRpm\r\n".toByteArray(charset("euc-kr"))
                bleService?.writeCharacteristic(cmd, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)
            }
        }

        binding.btnChangeSpeed.setOnClickListener {
            val maxSpeed = binding.etSpeed.text.toString().toIntOrNull() ?: return@setOnClickListener
            if (maxSpeed in 1..255) {
                val cmd = "AT$\$DS_BTO_SPEED=1,$maxSpeed\r\n".toByteArray(charset("euc-kr"))
                bleService?.writeCharacteristic(cmd, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)
            }
        }

        binding.tbPUA.setOnCheckedChangeListener { _, checked ->
            if (isInitialCall) return@setOnCheckedChangeListener
            val pauEnabled = if(binding.tbPUA.isChecked) 1 else 0
            val cmd = "AT$\$DS_BTO_EN=$pauEnabled,$speedEnabled\r\n".toByteArray(charset("euc-kr"))
            bleService?.writeCharacteristic(cmd, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)
        }

        binding.tbSpeedcamera.setOnCheckedChangeListener { _, checked ->
            if (isInitialCall) return@setOnCheckedChangeListener
            val cameraEnabled = if(binding.tbSpeedcamera.isChecked) 1 else  0
            val cmd = "AT$\$DS_BTO_EN=$pauEnabled,$cameraEnabled\r\n".toByteArray(charset("euc-kr"))
            bleService?.writeCharacteristic(cmd, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)
        }

        binding.ivVolumeDecrease.setOnClickListener {
            val currentVolume = binding.tvVolumeSetting.text.toString().toInt()
            if (currentVolume > 0) {
                val newVolume = currentVolume - 1
                val cmd = "AT$\$DS_VOLUME=$newVolume\r\n".toByteArray(charset("euc-kr"))
                bleService?.writeCharacteristic(cmd, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)
            }
        }

        binding.ivVolumeIncrease.setOnClickListener {
            val currentVolume = binding.tvVolumeSetting.text.toString().toInt()
            val newVolume = currentVolume + 1
            val cmd = "AT$\$DS_VOLUME=$newVolume\r\n".toByteArray(charset("euc-kr"))
            bleService?.writeCharacteristic(cmd, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)
        }

    }

    private val receiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val action = intent.action
            when (action) {
                AppConstants.ACTION_DEVICE_DISCONNECTED -> {
                    hideProgressDialog(activity)
                    setAlertDialog(requireActivity(), getString(R.string.device_disconnected))
                }

                AppConstants.ACTION_CHARACTERISTIC_CHANGED -> {
                    val response: String?
                    try {
                        response = intent.getStringExtra("data") ?: return
                        Log.i("AxonBle", "onReceive: $response")
                        if (response.contains("DS_BTO_EN")) { // bto control
                            val data = response.split(":")[1].trim().split(",")
                            pauEnabled = data[1].trim().toInt()
                            speedEnabled = data[2].trim().split(" ")[0].toInt()
                            binding.tbPUA.isChecked = pauEnabled == 1
                            binding.tbSpeedcamera.isChecked = speedEnabled == 1
                            isInitialCall = false
                        }

                        if (response.contains("DS_BTO_SPEED")) { // max speed control
                            val data = response.split(":")[1].trim().split(",")
                            val isEnabled = data[1].trim()
                            val maxAllowedSpeed = data[2].trim().split(" ")[0]
                            binding.etSpeed.setText(maxAllowedSpeed)

                        }

                        if (response.contains("DS_BTO_RPM")) { // max rpm control
                            val data = response.split(":")[1].trim().split(",")
                            val rpmEnabled = data[1].trim()
                            val maxAllowedRpm = data[2].trim().split(" ")[0].trim()
                            binding.etRpm.setText(maxAllowedRpm)
                        }

                        if (response.contains("DS_VOLUME")) {
                            val volume = response.split(":")[1].split(",").last().trim().split(" ").first()
                            binding.tvVolumeSetting.text = volume
                        }
                    } catch (e : Exception) {
                        e.printStackTrace()
                        requireActivity().onBackPressed()
                    }
                }
            }
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        localBroadcastManager.unregisterReceiver(receiver)
    }

}