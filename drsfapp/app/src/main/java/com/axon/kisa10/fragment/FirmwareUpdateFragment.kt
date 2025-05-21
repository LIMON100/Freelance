package com.axon.kisa10.fragment

import android.annotation.SuppressLint
import android.bluetooth.BluetoothGattCharacteristic
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.view.KeyEvent
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.activity.MainActivity
import com.axon.kisa10.activity.SearchDeviceActivity
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AppConstants.makeIntentFilter
import com.axon.kisa10.util.AppMethods.hideProgressDialog
import com.axon.kisa10.util.AppMethods.setAlertDialog
import com.axon.kisa10.R
import io.feeeei.circleseekbar.CircleSeekBar
import java.io.File
import java.io.FileInputStream

class FirmwareUpdateFragment(private val bleService: AxonBLEService) : Fragment() {
    private lateinit var tv_ok: TextView
    private lateinit var tv_update_status: TextView
    private lateinit var tv_progress: TextView
    private lateinit var iv_success: ImageView
    private lateinit var iv_percent: ImageView
    private lateinit var seek_bar_progress: CircleSeekBar

    //private RelativeLayout rl_progress;
    private var delayNoResponse = 1
    private var MAX_LENGHTH = 20
    private var isResponseC = true
    private var otaCurrentbyte = 0

    private var localBroadcastManager: LocalBroadcastManager? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        localBroadcastManager = LocalBroadcastManager.getInstance(requireActivity())
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        return inflater.inflate(R.layout.fragment_firmware_update, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        initUi(view)
        localBroadcastManager?.registerReceiver(mGattUpdateReceiver, makeIntentFilter())
        MAX_LENGHTH = bleService.getMtuSize()
//        bleService.stopMapRequestTimer()
        dfuMode("OTABEGIN")

        view.isFocusableInTouchMode = true
        view.requestFocus()
        view.setOnKeyListener(View.OnKeyListener { _, keyCode, event ->
            if (event.action == KeyEvent.ACTION_DOWN) {
                if (keyCode == KeyEvent.KEYCODE_BACK) {
                    return@OnKeyListener true
                }
            }
            false
        })
    }

    @SuppressLint("SetTextI18n")
    private fun initUi(view: View) {
        tv_ok = view.findViewById(R.id.tv_ok)
        tv_ok.visibility = View.GONE
        tv_update_status = view.findViewById(R.id.tv_update_status)
        iv_success = view.findViewById(R.id.iv_success)
        iv_percent = view.findViewById(R.id.iv_percent)
        seek_bar_progress = view.findViewById(R.id.seek_bar_progress)
        //rl_progress = view.findViewById(R.id.rl_progress);
        tv_progress = view.findViewById(R.id.tv_progress)

        val intent = Intent()
        intent.setAction(AppConstants.BROADCAST_CHILD_FRAGMENT)
        intent.putExtra(AppConstants.EXTRA_SETTING, false)
        localBroadcastManager?.sendBroadcast(intent)

        seek_bar_progress.setOnSeekBarChangeListener { _ , curValue: Int ->
            tv_progress.text = curValue.toString() + ""
            if (curValue < 100) {
                iv_success.visibility = View.GONE
                iv_percent.visibility = View.VISIBLE
                tv_progress.visibility = View.VISIBLE
            } else {
                iv_success.visibility = View.VISIBLE
                iv_percent.visibility = View.GONE
                tv_progress.visibility = View.GONE
            }
        }

        tv_ok.setOnClickListener {
            localBroadcastManager?.sendBroadcast(Intent(MainActivity.FIRMWARE_UPDATE_OK))
        }
    }

    override fun onDestroyView() {
        unRegisterReceiver()
        super.onDestroyView()
    }

    private var datathread: ByteArray? = null

    @Synchronized
    fun dfuMode(step: String?) {
        when (step) {
            "OTABEGIN" -> try {
                datathread = readBinary()
            } catch (e: Exception) {
                Log.d("ota==>", "Couldn't open file : $e")
            }

            "OTAUPLOAD" -> {
                val sendByte = datathread
                val otaUpload = Thread { whiteOtaData(sendByte) }
                otaUpload.start()
            }

            else -> {}
        }
    }

    @Synchronized
    private fun readBinary(): ByteArray? {
        val ebl: ByteArray?
        try {
            val filePath = requireContext().cacheDir.toString() + "/Axon/" + AppConstants.FIRMWARE_FILE_NAME
            val file: File

            if (filePath.isNotEmpty()) {
                file = File(filePath)
                val fileInputStream = FileInputStream(file)
                val size = fileInputStream.available()
                Log.d("size", "" + size)
                val temp = ByteArray(size)
                fileInputStream.read(temp)
                fileInputStream.close()
                ebl = temp
                bleService.writeStringData(AppConstants.COMMAND_FIRMWARE_WRITE, ebl.size.toString() + "," + AppConstants.FIRMWARE_FILE_NAME)
                Log.d("ota==>", """Binary data : ${ebl.contentToString()} len=${ebl.size} NAME=${AppConstants.FIRMWARE_FILE_NAME}""")
                return ebl
            } else {
                setAlertDialog(requireActivity(), getString(R.string.filenotfound))
                return null
            }
        } catch (e: Exception) {
            setAlertDialog(requireActivity(), getString(R.string.filenotfound))
            Log.e("InputStream", "Couldn't open file$e")
            return null
        }
    }

    @SuppressLint("MissingPermission")
    @Synchronized
    fun whiteOtaData(datathread: ByteArray?) {
        if (!bleService.isConnected()) {
            return
        }
        try {
            datathread?.let { data ->
                val value = ByteArray(bleService.getMtuSize())
                val start = System.nanoTime()
                var j = 0
                var lastLength = datathread.size
                if (otaCurrentbyte + MAX_LENGHTH < lastLength) {
                    lastLength = otaCurrentbyte + MAX_LENGHTH
                }
                for (i in otaCurrentbyte until lastLength) {
                    value[j] = datathread[i]
                    j++
                    if (j >= bleService.getMtuSize() || i >= (lastLength - 1)) {
                        var wait = System.nanoTime()
                        val progress = ((i + 1).toFloat() / datathread.size) * 100
                        val otaData = if (j < bleService.getMtuSize()) {
                            val end = ByteArray(j)
                            System.arraycopy(value, 0, end, 0, j)
                            requireActivity().runOnUiThread {
                                seek_bar_progress.curProcess = progress.toInt()
                            }
                            otaCurrentbyte = i + 1
                            end
                        } else {
                            j = 0
                            requireActivity().runOnUiThread {
                                seek_bar_progress.curProcess = progress.toInt()
                            }
                            value
                        }
                        delayNoResponse = Math.round(progress)

                        if (bleService.writeCharacteristic(otaData, type = BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)) {
                            while ((System.nanoTime() - wait) / 1000000.0 < delayNoResponse);
                        } else {
                            do {
                                while ((System.nanoTime() - wait) / 1000000.0 < delayNoResponse);
                                wait = System.nanoTime()
                            } while (!bleService.writeCharacteristic(otaData, type = BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE))
                        }
                    }
                }
                val end = System.currentTimeMillis()
                val time = ((end - start) / 1000L).toFloat()
                Log.d("OTA Time - ", "" + time + "s")
                dfuMode("OTAEND")
            }
        } catch (e: NullPointerException) {
            e.printStackTrace()
            setAlertDialog(requireActivity(), getString(R.string.Exception) + e.message)
        }
    }

    private val mGattUpdateReceiver: BroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val action = intent.action
            Log.d("result", " action$action")
            when (action) {
                AppConstants.ACTION_DEVICE_DISCONNECTED -> {
                    unRegisterReceiver()
                    hideProgressDialog(activity)
                    setAlertDialog(requireActivity(), getString(R.string.device_disconnected))
                }

                AppConstants.ACTION_CHARACTERISTIC_CHANGED -> {
                    val response: String?
                    try {
                        response = intent.getStringExtra("data")
                        Log.d("result", "FirmwareUpdateFragment  response  $response")
                        parseResponse(response)
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                }
            }
        }
    }

    private fun parseResponse(response: String?) {
        val responseArray = response!!.split(",")
        if (responseArray[0].trim() == AppConstants.COMMAND_RES) {
            val command = responseArray[1].trim()
            Log.d("result", " firmware_ - " + responseArray[2].trim() + "  cmd " + command.trim())
            when (command) {
                AppConstants.COMMAND_FIRMWARE_WRITE -> {
                    if (responseArray.size == 4) {
                        hideProgressDialog(activity)
                        if (responseArray[2].trim() == AppConstants.RESPONSE_S) {
                            MAX_LENGHTH = responseArray[3].trim().toInt()
                            dfuMode("OTAUPLOAD")
                        } else if (responseArray[2].trim() == AppConstants.RESPONSE_F) {
                            setAlertDialog(requireContext(),getString(R.string.firmware_update_fail)) {
                                startActivity(Intent(requireContext(), SearchDeviceActivity::class.java))
                                bleService.disconnect()
                                requireActivity().finish()
                            }
                            requireActivity().runOnUiThread {
                                seek_bar_progress.pointerColor = resources.getColor(R.color.red,null)
                                seek_bar_progress.reachedColor = resources.getColor(R.color.red,null)
                                tv_ok.visibility = View.VISIBLE
                            }
                        }
                    } else if (responseArray.size < 4) {
                        if (responseArray[2].trim() == AppConstants.RESPONSE_F) {
                            unRegisterReceiver()
                            Log.d("result", "firmware_update_fail -2 ")
                            setAlertDialog(requireActivity(), getString(R.string.firmware_update_fail))
                            requireActivity().onBackPressed()
                        } else if (responseArray[2].trim() == AppConstants.RESPONSE_C) {
                            isResponseC = true
                            val otaUpload = Thread { whiteOtaData(datathread) }
                            otaUpload.start()
                        } else if (responseArray[2].trim() == AppConstants.RESPONSE_A) {
                            isResponseC = false
                            requireActivity().runOnUiThread {
                                seek_bar_progress.curProcess = 100
                                //rl_progress.setVisibility(View.GONE);
                                iv_success.visibility = View.VISIBLE
                                iv_percent.visibility = View.GONE
                                tv_ok.visibility = View.VISIBLE
                                tv_update_status.text = getString(R.string.successfully_updated)
                            }
                        }
                    }
                }
            }
        }
    }

    private fun unRegisterReceiver() {
        try {
            localBroadcastManager?.unregisterReceiver(mGattUpdateReceiver)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
}
