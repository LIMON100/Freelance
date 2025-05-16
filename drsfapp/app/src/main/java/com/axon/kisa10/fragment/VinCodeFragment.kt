package com.axon.kisa10.fragment

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.EditText
import android.widget.TextView
import androidx.fragment.app.Fragment
import com.axon.kisa10.ble.BleCharacteristic
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AppConstants.makeIntentFilter
import com.axon.kisa10.util.AppMethods.hideProgressDialog
import com.axon.kisa10.util.AppMethods.setAlertDialog
import com.axon.kisa10.util.AppMethods.showProgressDialog
import com.kisa10.R

class VinCodeFragment : Fragment() {
    private lateinit var etVinCode: EditText
    private lateinit var tvVinCodeSave: TextView

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        return inflater.inflate(R.layout.fragment_vin_code, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        initUi(view)
        requireActivity().registerReceiver(mGattUpdateReceiver, makeIntentFilter())
        showProgressDialog(activity)
        BleCharacteristic.readDeviceData(requireContext(), AppConstants.COMMAND_VINCODE_READ)
    }

    private fun initUi(view: View) {
        etVinCode = view.findViewById(R.id.et_vincode)
        tvVinCodeSave = view.findViewById(R.id.tv_vincode_save)

        tvVinCodeSave.setOnClickListener {
            val vinCode = etVinCode.text.toString().trim { it <= ' ' }
            if (vinCode == "") {
                setAlertDialog(requireActivity(), getString(R.string.enter_vincode))
            } else if (vinCode.length < 17) {
                setAlertDialog(requireActivity(), getString(R.string.vincode_validation))
            } else {
                BleCharacteristic.writeStringData(
                    requireContext(),
                    AppConstants.COMMAND_VINCODE_WRITE,
                    vinCode
                )
            }
        }
    }

    private fun unRegisterReceiver() {
        try {
            requireActivity().unregisterReceiver(mGattUpdateReceiver)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    override fun onDestroyView() {
        unRegisterReceiver()
        super.onDestroyView()
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
                    val response: String?
                    try {
                        response = intent.getStringExtra("data")
                        parseResponse(response)
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                }
            }
        }
    }

    private fun parseResponse(response: String?) {
        val responseArray =
            response!!.split(",".toRegex()).dropLastWhile { it.isEmpty() }.toTypedArray()
        if (responseArray[0].trim { it <= ' ' } == AppConstants.COMMAND_RES) {
            val command = responseArray[1].trim { it <= ' ' }
            when (command) {
                AppConstants.COMMAND_VINCODE_READ -> if (responseArray.size >= 4) {
                    hideProgressDialog(activity)
                    if (responseArray[2].trim { it <= ' ' } == AppConstants.RESPONSE_S) {
                        val vinCode = responseArray[3].trim { it <= ' ' }
                        etVinCode.setText(vinCode)
                    }
                }

                AppConstants.COMMAND_VINCODE_WRITE -> if (responseArray.size >= 3) {
                    hideProgressDialog(activity)
                    if (responseArray[2].trim { it <= ' ' } == AppConstants.RESPONSE_S) {
                        setAlertDialog(requireActivity(), getString(R.string.write_success))
                    } else {
                        setAlertDialog(requireActivity(), getString(R.string.write_fail))
                    }
                }
            }
        }
    }

    companion object {
        fun newInstance(): VinCodeFragment {
            return VinCodeFragment()
        }
    }
}
