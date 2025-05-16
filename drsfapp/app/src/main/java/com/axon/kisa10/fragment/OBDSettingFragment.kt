
package com.axon.kisa10.fragment

import android.app.ProgressDialog
import android.bluetooth.BluetoothGattCharacteristic
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.Gravity
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.WindowManager
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.RadioButton
import android.widget.RadioGroup
import android.widget.RadioGroup.OnCheckedChangeListener
import android.widget.TableLayout
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.fragment.app.Fragment
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.axon.kisa10.adapter.ListItemDialogAdapter
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.model.ListItemDialog
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AppConstants.makeIntentFilter
import com.axon.kisa10.util.AppMethods.hideProgressDialog
import com.axon.kisa10.util.AppMethods.setAlertDialog
import com.axon.kisa10.util.AppMethods.showProgressDialog
import com.kisa10.R
import com.kisa10.databinding.FragmentObdSettingBinding
import java.util.Timer
import java.util.TimerTask

@Suppress("DEPRECATION")
class OBDSettingFragment(private val bleService: AxonBLEService) : Fragment() {


    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(requireContext())
    }

    private lateinit var binding : FragmentObdSettingBinding

    private var canMode = "0"
    private var baudRate = "500_000"
    private var speedSigType = "0"
    private var speedFactor = "0"
    private var rpmSigType = "0"
    private var rpmFactor = "0"
    private var brakeSigType = "0"
    private var brakeActLvl = "0"

    private var progressDialog : ProgressDialog? = null

    override fun onAttach(context: Context) {
        super.onAttach(context)
        Log.i("OBD", "onAttach: ")
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        localBroadcastManager.registerReceiver(mGattUpdateReceiver, makeIntentFilter())
        val cmd = "AT$\$DS_OBD_CFG?\r\n".toByteArray(charset("euc-kr"))
        bleService.writeCharacteristic(cmd, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)
        progressDialog = ProgressDialog.show(requireContext(), getString(R.string.please_wait),"")
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentObdSettingBinding.inflate(inflater, container, false)

        binding.rgBaudrate.clearCheck()
        binding.rgRpm.clearCheck()
        binding.rgBrake.clearCheck()
        binding.rgSignal.clearCheck()
        binding.rgCanmode.clearCheck()
        binding.rgBrake.clearCheck()
        binding.rgSpeed.clearCheck()
        binding.btnSaveChange.isFocusable = true
        binding.btnSaveChange.isClickable = true


//        binding.rgBaudrate.setOnCheckedChangeListener(this)
//        binding.rgRpm.setOnCheckedChangeListener(this)
//        binding.rgBrake.setOnCheckedChangeListener(this)
//        binding.rgSignal.setOnCheckedChangeListener(this)
//        binding.rgCanmode.setOnCheckedChangeListener(this)
//        binding.rgBrake.setOnCheckedChangeListener(this)
//        binding.rgSpeed.setOnCheckedChangeListener(this)
//        binding.btnSaveChange.setOnClickListener(this)

        return inflater.inflate(R.layout.fragment_obd_setting, container, false)
    }

    private val mGattUpdateReceiver: BroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val action = intent.action
            when (action) {
                AppConstants.ACTION_DEVICE_DISCONNECTED -> {
                    hideProgressDialog(activity)
                    setAlertDialog(requireActivity(), getString(R.string.device_disconnected))
                }

                AppConstants.ACTION_CHARACTERISTIC_CHANGED -> {
                    progressDialog?.dismiss()
                    progressDialog = null
                    val response: String?
                    try {
                        response = intent.getStringExtra("data") ?: return
                        parseResponse(response)
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                }
            }
        }
    }

    private fun parseResponse(response: String) {
        if (response.contains("OBD_CFG")) {
            val responseArray = response.split(":")[1].trim()
            val data = responseArray.split(",")
            if (data.size > 2) { // read response
                canMode = data[1].trim()
                baudRate = data[2].trim()
                speedSigType = data[3].trim()
                speedFactor = data[4].trim()
                rpmSigType = data[5].trim()
                rpmFactor = data[6].trim()
                brakeSigType = data[7].trim()
                brakeActLvl = data[8].trim().split(" ")[0].trim()
                Log.i("AxonBle", "parseResponse: $canMode $baudRate $speedSigType $speedFactor $rpmSigType $rpmFactor $brakeSigType $brakeActLvl")


                when(canMode) {
                    "0" -> binding.canModeOBD2.isChecked = true
                    "1" -> binding.canModeBroadcast.isChecked = true
                    "2" -> binding.canModeUDS.isChecked = true
                }

                when(baudRate) {
                    "125000" -> binding.rb125kb.isChecked = true
                    "250000" -> binding.rb250kb.isChecked = true
                    "500000" -> binding.rb500kb.isChecked = true
                    "1000000" -> binding.rb1000kb.isChecked = true
                }

                when(speedSigType) {
                    "0" -> binding.speedCanType.isChecked = true
                    "1" -> binding.speedLineType.isChecked = true
                }

                binding.etSpped.setText(speedFactor)

                when(rpmSigType) {
                    "0" -> binding.rpmCanType.isChecked = true
                    "1" -> binding.rpmLineType.isChecked = true
                }

                binding.etRpm.setText(rpmFactor)

                when(brakeSigType) {
                    "0" -> binding.speedCanType.isChecked = true
                    "1" -> binding.speedLineType.isChecked = true
                }

                when(brakeActLvl) {
                    "0" -> binding.signalHigh.isChecked = true
                    "1" -> binding.signalLow.isChecked = true
                }
            } else { // write response
                val responseCode = data[0].trim()
                if (responseCode.contains("0")) {
                    Toast.makeText(requireContext(), R.string.write_success, Toast.LENGTH_SHORT).show()
                } else {
                    Toast.makeText(requireContext(), R.string.write_fail, Toast.LENGTH_SHORT).show()
                }
            }
        }
    }



}
