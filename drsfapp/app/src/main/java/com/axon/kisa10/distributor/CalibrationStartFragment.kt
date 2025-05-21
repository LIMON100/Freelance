package com.axon.kisa10.distributor

import android.bluetooth.BluetoothGattCharacteristic
import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.databinding.FragmentCalibrationStartBinding

class CalibrationStartFragment(private val bleService: AxonBLEService) : Fragment() {

    private lateinit var binding : FragmentCalibrationStartBinding

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(requireContext())
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentCalibrationStartBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        binding.btnConfirm.setOnClickListener {
            if (bleService.isConnected()) {
                bleService.flush()
                val cmd = "AT$\$DS_CALIB=1\r\n".toByteArray(charset("euc-kr"))
                bleService.writeCharacteristic(cmd, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)
                val calibIntent = Intent(MainDistributorActivity.ACTION_CALIBRATION_FRAGMENT)
                calibIntent.putExtras(requireArguments())
                localBroadcastManager.sendBroadcast(calibIntent)
            }
        }

        binding.btnCancel.setOnClickListener {
            requireActivity().onBackPressed()
        }
    }
}