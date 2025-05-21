package com.axon.kisa10.fragment

import android.annotation.SuppressLint
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.graphics.Color
import android.icu.lang.UCharacter.SentenceBreak.SP
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AppConstants.makeIntentFilter
import com.axon.kisa10.util.AppMethods.hideProgressDialog
import com.axon.kisa10.util.AppMethods.setAlertDialog
import com.axon.kisa10.R
import io.feeeei.circleseekbar.CircleSeekBar

class FactoryResetFragment(private val bleService: AxonBLEService) : Fragment() {
    private lateinit var tv_ok: TextView
    private lateinit var tv_update_status: TextView
    private lateinit var tv_progress: TextView
    private lateinit var iv_success: ImageView
    private lateinit var iv_not_success: ImageView
    private lateinit var iv_percent: ImageView
    private lateinit var seek_bar_progress: CircleSeekBar
    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(requireContext())
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        return inflater.inflate(R.layout.fragment_firmware_update, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        localBroadcastManager.registerReceiver(mGattUpdateReceiver, makeIntentFilter())
        initUi(view)
        setHandler()
    }

    @SuppressLint("SetTextI18n")
    private fun initUi(view: View) {
        tv_ok = view.findViewById(R.id.tv_ok)
        tv_update_status = view.findViewById(R.id.tv_update_status)
        iv_success = view.findViewById(R.id.iv_success)
        iv_not_success = view.findViewById(R.id.iv_not_success)
        iv_percent = view.findViewById(R.id.iv_percent)
        seek_bar_progress = view.findViewById(R.id.seek_bar_progress)
        //rl_progress = view.findViewById(R.id.rl_progress);
        tv_progress = view.findViewById(R.id.tv_progress)
        tv_update_status.setText(R.string.Memoryformat_progress)
        seek_bar_progress.setOnSeekBarChangeListener { _, curValue: Int ->
            tv_progress.text = "$curValue "
            if (curValue <100) {
                tv_update_status.setText(R.string.Memoryformat_progress)
            }
        }
        tv_ok.setOnClickListener {
//            (requireActivity() as MainActivity).OnFragmentLayoutClick(AppConstants.factory_reset_click)
            requireActivity().onBackPressed()
        }
    }


    private fun setHandler() {
        Handler(Looper.getMainLooper()).postDelayed({
            try {
                if (seek_bar_progress.curProcess < 100) {
                    seek_bar_progress.curProcess = 30
                    if (bleService.isConnected()) {
                        bleService.readDeviceData(AppConstants.COMMAND_FACTORY_RESET)
                    }
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }, 1000)

        Handler(Looper.getMainLooper()).postDelayed({
            try {
                if (seek_bar_progress.curProcess < 100) {
                    seek_bar_progress.curProcess = 55
                    if (bleService.isConnected()) {
                        bleService.readDeviceData(AppConstants.COMMAND_FACTORY_RESET)
                    }
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }, 3000)

        Handler(Looper.getMainLooper()).postDelayed({
            try {
                if (seek_bar_progress.curProcess < 100) {
                    seek_bar_progress.curProcess = 85
                    if (bleService.isConnected()) {
                        bleService.readDeviceData(AppConstants.COMMAND_FACTORY_RESET)
                    }
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }, 5000)


        Handler(Looper.getMainLooper()).postDelayed({
            try {
                if (seek_bar_progress.curProcess < 100) {
                    seek_bar_progress.curProcess = 100
                    //rl_progress.setVisibility(View.GONE);
                    tv_progress.visibility = View.GONE
                    iv_success.visibility = View.GONE
                    iv_percent.visibility = View.GONE
                    iv_not_success.visibility = View.VISIBLE
                    tv_ok.visibility = View.VISIBLE
                    tv_update_status.setTextSize(SP, 25f)
                    tv_update_status.setTextColor((Color.rgb(200, 0, 0)))
                    tv_update_status.setText(R.string.Memoryformat_failure)
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }, 10000)
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
        val responseArray = response?.split(",") ?: return
        if (responseArray[0].trim() == AppConstants.COMMAND_RES) {
            val command = responseArray[1].trim()
            when (command) {
                AppConstants.COMMAND_FACTORY_RESET -> if (responseArray[2] == AppConstants.RESPONSE_S) {
                    seek_bar_progress.curProcess = 100
                    //rl_progress.setVisibility(View.GONE);
                    iv_not_success.visibility = View.GONE
                    tv_progress.visibility = View.GONE
                    iv_success.visibility = View.VISIBLE
                    iv_percent.visibility = View.GONE
                    tv_ok.visibility = View.VISIBLE
                    tv_update_status.setText(R.string.Memoryformat_success)
                } else if (responseArray[2].trim() == AppConstants.RESPONSE_F) {
                    seek_bar_progress.curProcess = 100
                    //rl_progress.setVisibility(View.GONE);
                    tv_progress.visibility = View.GONE
                    iv_success.visibility = View.GONE
                    iv_percent.visibility = View.GONE
                    iv_not_success.visibility = View.VISIBLE
                    tv_ok.visibility = View.VISIBLE
                    tv_update_status.setText(R.string.Memoryformat_failure)
                    tv_update_status.setTextSize(SP, 25f)
                    tv_update_status.setTextColor((Color.rgb(200, 0, 0)))
                }
            }
        }
    }

    private fun unRegisterReceiver() {
        try {
            localBroadcastManager.unregisterReceiver(mGattUpdateReceiver)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    override fun onDestroyView() {
        unRegisterReceiver()
        super.onDestroyView()
    }
}
