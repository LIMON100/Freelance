package com.axon.kisa10.fragment

import android.annotation.SuppressLint
import android.app.AlertDialog
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.Editable
import android.text.TextWatcher
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.inputmethod.InputMethodManager
import android.widget.CompoundButton
import android.widget.EditText
import android.widget.ImageView
import android.widget.RelativeLayout
import android.widget.TextView
import android.widget.Toast
import android.widget.ToggleButton
import androidx.fragment.app.Fragment
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AppConstants.makeIntentFilter
import com.axon.kisa10.util.AppMethods
import com.axon.kisa10.util.AppMethods.hideProgressDialog
import com.axon.kisa10.util.AppMethods.setAlertDialog
import com.axon.kisa10.util.AppMethods.showProgressDialog
import com.axon.kisa10.util.CheckAutoUpdate.isNewUpdate
import com.kisa10.R

class DeviceFragment(private val bleService: AxonBLEService) : Fragment(), View.OnClickListener, CompoundButton.OnCheckedChangeListener {
    private val TAG = "DeviceFragment"
    private lateinit var iv_volume_decrese: ImageView
    private lateinit var iv_volume_increse: ImageView

    //    private ImageView iv_speed_decrease, iv_speed_increase;
    private lateinit var iv_speed_control_decrese: ImageView
    private lateinit var iv_speed_control_increse: ImageView
    private lateinit var iv_camera_range_control_decrese: ImageView
    private lateinit var iv_camera_range_control_increse: ImageView

    private lateinit var toggle_auto_speed_control: ToggleButton
    private lateinit var toggle_schoolzone_control: ToggleButton
    private lateinit var toggle_speedlimit_control: ToggleButton
    private lateinit var toggle_pua_control: ToggleButton //, mode_onoff;
    private lateinit var tv_volume: TextView
    private lateinit var tv_speedcontrol: TextView
    private lateinit var tv_speed_control: TextView
    private lateinit var tv_camera_range_autocontrol: TextView
    private lateinit var et_rpm: EditText
    private lateinit var et_speed: EditText
    private lateinit var tv_rpm_apply: TextView
    private lateinit var tv_speed_apply: TextView
    private lateinit var rv_speed: RelativeLayout
    private lateinit var rv_rpm: RelativeLayout
    private lateinit var rv_schoolzone_ctrl: RelativeLayout
    private lateinit var rv_speedlimit_camera_ctrl: RelativeLayout
    private lateinit var rv_pua_ctrl: RelativeLayout

    private lateinit var controlspeed_sensitivityArry : Array<String>
    private lateinit var controldistance_sensitivityArry : Array<String>


    private var previousSpeedLimit = ""
    private var previousRPMLimit = ""
    private var isBackStacked = false

    private var realspeed = 1

    private var alert_rpm_status = true
    private var alert_speed_status = true

    private var firmwareVersion = "0"

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(requireContext())
    }

    //    private static boolean flag_toggle_auto_speed_control = false;
    private val handler = Handler(Looper.getMainLooper())
    private val runnable: Runnable = object : Runnable {
        override fun run() {
            if (!isBackStacked && bleService.isConnected()) {
                bleService.readDeviceData(AppConstants.COMMAND_REAL_TIME_V_I_READ) //lsh_test
            }
            handler.postDelayed(this, 3000)
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        return inflater.inflate(R.layout.fragment_device, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        initUi(view)
        localBroadcastManager.registerReceiver(mGattUpdateReceiver, makeIntentFilter())
        showProgressDialog(activity)
        bleService.readDeviceData(AppConstants.COMMAND_FIRMWARE_READ)
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun initUi(view: View) {
        rv_rpm = view.findViewById(R.id.rv_rpm)
        rv_speed = view.findViewById(R.id.rv_speed)

        rv_schoolzone_ctrl = view.findViewById(R.id.rv_schoolzone_ctrl)
        rv_speedlimit_camera_ctrl = view.findViewById(R.id.rv_speedlimit_camera_ctrl)
        rv_pua_ctrl = view.findViewById(R.id.rv_pua_ctrl)

        et_rpm = view.findViewById(R.id.et_rpm)
        et_speed = view.findViewById(R.id.et_speed)

        tv_rpm_apply = view.findViewById(R.id.tv_rpm_apply)
        tv_speed_apply = view.findViewById(R.id.tv_speed_apply)

        iv_speed_control_decrese = view.findViewById(R.id.iv_speed_control_decrese)
        iv_speed_control_increse = view.findViewById(R.id.iv_speed_control_increse)

        iv_camera_range_control_decrese = view.findViewById(R.id.iv_camera_range_control_decrese)
        iv_camera_range_control_increse = view.findViewById(R.id.iv_camera_range_control_increse)


        iv_volume_decrese = view.findViewById(R.id.iv_volume_decrese)
        iv_volume_increse = view.findViewById(R.id.iv_volume_increse)

        //        toggle_auto_speed_control = view.findViewById(R.id.toggle_auto_speed_control);
//        iv_speed_decrease = view.findViewById(R.id.iv_speed_decrease);
//        iv_speed_increase = view.findViewById(R.id.iv_speed_increase);

//        iv_sensitivity_increase = view.findViewById(R.id.iv_senstivity_increase);
//        tv_sensitivity = view.findViewById(R.id.tv_sensitivity);
//        iv_sensitivity_decrease = view.findViewById(R.id.iv_sensitivity_decrease);
        toggle_schoolzone_control = view.findViewById(R.id.toggle_schoolzone_control)
        toggle_speedlimit_control = view.findViewById(R.id.toggle_speedlimit_control)
        toggle_pua_control = view.findViewById(R.id.toggle_pua_control)

        tv_volume = view.findViewById(R.id.tv_volume)
        tv_speedcontrol = view.findViewById(R.id.tv_speedcontrol)

        iv_volume_decrese.setOnClickListener(this)
        iv_volume_increse.setOnClickListener(this)

        iv_speed_control_decrese.setOnClickListener(this)
        iv_speed_control_increse.setOnClickListener(this)

        iv_camera_range_control_decrese.setOnClickListener(this)
        iv_camera_range_control_increse.setOnClickListener(this)

        tv_speed_control = view.findViewById(R.id.tv_speed_control)

        tv_camera_range_autocontrol = view.findViewById(R.id.tv_camera_range_autocontrol)
        controlspeed_sensitivityArry = arrayOf(getString(R.string.sensitive), getString(R.string.stolidity))
        controldistance_sensitivityArry = arrayOf(getString(R.string.sensitive), getString(R.string.stolidity))


        setVolumeValue()

        setControl_Speed_Sensitivity()
        setControl_Distance_Sensitivity()

        setMaxRpmControl()
        setMaxSpeedControl()

        rv_rpm.setOnClickListener(this)
        rv_speed.setOnClickListener(this)

        rv_schoolzone_ctrl.setOnClickListener(this)
        rv_speedlimit_camera_ctrl.setOnClickListener(this)


        //        AppMethods.showProgressDialog(getActivity());
        isBackStacked = false

        if (!bleService.isConnected()) {
            bto_pua = AppMethods.previous_bto_pua
            bto_speed = AppMethods.previous_bto_speed
            bto_camera = AppMethods.previous_bto_camera
            getBtoSpeedCameraPua()
        }

        setBTOSpeedCheckedChnageListener()
        setBTOCameraCheckedChnageListener()
        setBTOPUASCheckedChnageListener()


        et_rpm.setOnTouchListener { _, _ -> //                Toast.makeText(getActivity(), getString(R.string.please_set_rpm_value), Toast.LENGTH_SHORT).show();
            Log.d(TAG, " RPM alert_rpm_status:::  $alert_rpm_status")
            // + " flag_toggle_auto_speed_control:::   " +flag_toggle_auto_speed_control);
            //                if(!flag_toggle_auto_speed_control){
//                    AppMethods.setAlertDialog(getActivity(), getString(R.string.enable_autospeed_control_alert));
//                    return true;
//                }else
            if (!BTO_PUA) {
                setAlertDialog(requireActivity(), getString(R.string.enable_pua_control_alert))
                true
            } else if (alert_rpm_status) {
                alert_rpm_status = false
                showMessageAlertDiaog(getString(R.string.rpm_alert))
                true
            } else {
                false
            }
        }

        et_rpm.addTextChangedListener(object : TextWatcher {
            override fun afterTextChanged(s: Editable) {
                val data = et_rpm.text.toString().trim { it <= ' ' }
                et_rpm.setSelection(et_rpm.text.length)
                if (data != previousRPMLimit) {
                    if (data.isNotEmpty()) {
                        tv_rpm_apply.alpha = 1f
                        tv_rpm_apply.isEnabled = true
                    }
                }
            }

            override fun beforeTextChanged(
                s: CharSequence, start: Int,
                count: Int, after: Int
            ) {
            }

            override fun onTextChanged(
                s: CharSequence, start: Int,
                before: Int, count: Int
            ) {
            }
        })

        et_speed.setOnTouchListener { _, _ ->
            Log.d(TAG, " SPPED  alert_rpm_status:::  $alert_rpm_status")
            //+"  flag_toggle_auto_speed_control:::   " +flag_toggle_auto_speed_control);

            //                if(!flag_toggle_auto_speed_control) {
//                    AppMethods.setAlertDialog(getActivity(), getString(R.string.enable_autospeed_control_alert));
//                    return true;
//                }
//                else
            if (!BTO_PUA) {
                setAlertDialog(requireActivity(), getString(R.string.enable_pua_control_alert))
                true
            } else if (alert_speed_status) {
                alert_speed_status = false
                showMessageAlertDiaog(getString(R.string.speed_alert))
                true
            } else {
                false
            }
        }

        et_speed.addTextChangedListener(object : TextWatcher {
            override fun afterTextChanged(s: Editable) {
                val data = et_speed.text.toString().trim()
                et_speed.setSelection(et_speed.text.length)
                if (data != previousSpeedLimit) {
                    if (data.isNotEmpty()) {
                        tv_speed_apply.alpha = 1f
                        tv_speed_apply.isEnabled = true
                    }
                }
            }

            override fun beforeTextChanged(
                s: CharSequence, start: Int,
                count: Int, after: Int
            ) {
            }

            override fun onTextChanged(
                s: CharSequence, start: Int,
                before: Int, count: Int
            ) {
            }
        })

        tv_rpm_apply.setOnClickListener(View.OnClickListener {
            if (realspeed <= 5) {
                val newRpm = et_rpm.text.toString().trim { it <= ' ' }
                val strrpm: String
                if (newRpm == "") {
                    Toast.makeText(
                        activity,
                        getString(R.string.please_set_rpm_value),
                        Toast.LENGTH_SHORT
                    ).show()
                } else if (newRpm != previousRPMLimit) {
                    try {
                        var intrpm = newRpm.toInt()
                        if (intrpm > 100) {
                            intrpm -= (intrpm % 100)
                        }
                        strrpm = intrpm.toString()
                        et_rpm.setText(strrpm)
                        if (intrpm < 0 || intrpm > 999999) {
                            setAlertDialog(requireActivity(), getString(R.string.rpm_range_wrong))
                            return@OnClickListener
                        }
                    } catch (e: NumberFormatException) {
                        setAlertDialog(requireActivity(), getString(R.string.rpm_range_wrong))
                        return@OnClickListener
                    }
                    if (previousRPMLimit != strrpm) {
                        bleService.writeStringData(AppConstants.COMMAND_SET_RPM, strrpm)
                    }
                }
                tv_rpm_apply.alpha = 0.5f
                tv_rpm_apply.isEnabled = false
            } else {
                setAlertDialog(requireActivity(), getString(R.string.speed_is_greater_then_5))
            }
        })

        tv_speed_apply.setOnClickListener(View.OnClickListener {
            if (realspeed <= 5) {
                val newSpeed = et_speed.text.toString().trim()
                val strSpeed: String
                if (newSpeed == "") {
                    Toast.makeText(requireActivity(), getString(R.string.please_set_speed_value), Toast.LENGTH_SHORT).show()
                } else if (newSpeed != previousSpeedLimit) {
                    try {
                        var speedLimit = newSpeed.toInt()
                        if (speedLimit > 10) {
                            speedLimit -= (speedLimit % 10)
                        }
                        strSpeed = speedLimit.toString()
                        et_speed.setText(strSpeed)
                        if (speedLimit != 0 && (speedLimit < 10 || speedLimit > 255)) {
                            setAlertDialog(requireActivity(), "오류: 속도가 잘못되었습니다 (10~255)")
                            return@OnClickListener
                        }
                    } catch (e: NumberFormatException) {
                        setAlertDialog(requireActivity(), getString(R.string.speed_range_wrong))
                        return@OnClickListener
                    }
                    if (previousSpeedLimit != strSpeed) {
                        bleService.writeStringData(AppConstants.COMMAND_SET_SPEED, strSpeed)
                    }
                }
                tv_speed_apply.alpha = 0.5f
                tv_speed_apply.isEnabled = false
            } else {
                setAlertDialog(requireActivity(), getString(R.string.speed_is_greater_then_5))
            }
        })
    }

    //    private void setCheckedChangeListener() {
    //        toggle_auto_speed_control.setOnCheckedChangeListener(this);
    //    }
    private fun setBTOSpeedCheckedChnageListener() {
        toggle_schoolzone_control.setOnCheckedChangeListener(this)
    }

    private fun setBTOCameraCheckedChnageListener() {
        toggle_speedlimit_control.setOnCheckedChangeListener(this)
    }

    private fun setBTOPUASCheckedChnageListener() {
        toggle_pua_control.setOnCheckedChangeListener(this)
    }


    override fun onClick(view: View) {
        when (view.id) {
            R.id.iv_volume_decrese -> if (volume > 0) {
                volume -= 1
                setVolumeValue()
                bleService.writeDeviceData(AppConstants.COMMAND_DEVICE_VOLUME_WRITE, volume)
            }

            R.id.iv_volume_increse -> if (volume < 5) {
                volume += 1
                setVolumeValue()
                bleService.writeDeviceData(AppConstants.COMMAND_DEVICE_VOLUME_WRITE, volume)
            }

            R.id.rv_rpm -> //                if (!flag_toggle_auto_speed_control) {
//                    AppMethods.setAlertDialog(getActivity(), getString(R.string.enable_autospeed_control_alert));
//                    return;
//                }
                if (!BTO_PUA) {
                    setAlertDialog(requireActivity(), getString(R.string.enable_pua_control_alert))
                    return
                } else if (alert_rpm_status) {
                    alert_rpm_status = false
                    showMessageAlertDiaog(getString(R.string.rpm_alert))
                }

            R.id.rv_speed -> //                if (!flag_toggle_auto_speed_control) {
//                    AppMethods.setAlertDialog(getActivity(), getString(R.string.enable_autospeed_control_alert));
//                    return;
//                }
                if (alert_speed_status) {
                    alert_speed_status = false
                    showMessageAlertDiaog(getString(R.string.speed_alert))
                }

            R.id.iv_speed_control_decrese -> //                if (!flag_toggle_auto_speed_control){
                if (realspeed <= 5) {
                    if (speed_control_sensitivity > 0) {
                        speed_control_sensitivity -= 1
                        showMessageAlertDiaog(getString(R.string.speed_control_sensitivity_alert))
                    }
                } else {
                    setAlertDialog(requireActivity(), getString(R.string.speed_is_greater_then_5))
                }

            R.id.iv_speed_control_increse -> //                if (!flag_toggle_auto_speed_control){
                if (realspeed <= 5) {
                    if (speed_control_sensitivity < 1) {
                        speed_control_sensitivity += 1
                        showMessageAlertDiaog(getString(R.string.speed_control_sensitivity_alert))
                    }
                } else {
                    setAlertDialog(requireActivity(), getString(R.string.speed_is_greater_then_5))
                }

            R.id.iv_camera_range_control_decrese -> //                if (!flag_toggle_auto_speed_control){
                if (realspeed <= 5) {
                    if (overspeed_cam_dist_control_sensitivity > 0) {
                        overspeed_cam_dist_control_sensitivity -= 1
                        showMessageAlertDiaog(getString(R.string.overspeed_cam_dist_control_sensitivity_alert))
                    }
                } else {
                    setAlertDialog(requireActivity(), getString(R.string.speed_is_greater_then_5))
                }

            R.id.iv_camera_range_control_increse -> //                if (!flag_toggle_auto_speed_control){
                if (realspeed <= 5) {
                    if (overspeed_cam_dist_control_sensitivity < 1) {
                        overspeed_cam_dist_control_sensitivity += 1
                        showMessageAlertDiaog(getString(R.string.overspeed_cam_dist_control_sensitivity_alert))
                    }
                } else {
                    setAlertDialog(requireActivity(), getString(R.string.speed_is_greater_then_5))
                }
        }
    }

    override fun onCheckedChanged(compoundButton: CompoundButton, isChecked: Boolean) {
        when (compoundButton.id) {
            R.id.toggle_pua_control -> //                if(flag_toggle_auto_speed_control) {
                if (realspeed <= 5) {
                    AppMethods.previous_bto_pua = bto_pua
                    if (bleService.isConnected()) {
                        if (isChecked) {
                            BTO_PUA = true
                            bto_pua = 1
                        } else {
                            BTO_PUA = false
                            bto_pua = 0
                        }
                    }
                    if (toggle_pua_control.isPressed) {
                        set_BTO_Speed_Camera_Pua()
                    }
                } else {
                    setAlertDialog(requireActivity(), getString(R.string.speed_is_greater_then_5))
                }

            R.id.toggle_schoolzone_control -> //                if(flag_toggle_auto_speed_control) {
                if (realspeed <= 5) {
                    AppMethods.previous_bto_speed = bto_speed
                    if (bleService.isConnected()) {
                        if (isChecked) {
                            BTO_SPEED = true
                            bto_speed = 1
                        } else {
                            BTO_SPEED = false
                            bto_speed = 0
                        }
                    }
                    if (toggle_schoolzone_control.isPressed) {
                        set_BTO_Speed_Camera_Pua()
                    }
                } else {
                    setAlertDialog(requireActivity(), getString(R.string.speed_is_greater_then_5))
                }

            R.id.toggle_speedlimit_control -> //                if(flag_toggle_auto_speed_control) {
                if (realspeed <= 5) {
                    AppMethods.previous_bto_camera = bto_camera
                    if (bleService.isConnected()) {
                        if (isChecked) {
                            BTO_CAMERA = true
                            bto_camera = 1
                        } else {
                            BTO_CAMERA = false
                            bto_camera = 0
                        }
                    }
                    if (toggle_speedlimit_control.isPressed) {
                        set_BTO_Speed_Camera_Pua()
                    }
                } else {
                    setAlertDialog(requireActivity(), getString(R.string.speed_is_greater_then_5))
                }
        }
    }


    private fun set_BTO_Speed_Camera_Pua() {
        var value = 0

        if (BTO_PUA && BTO_SPEED && BTO_CAMERA) {
            value = 7
        }
        if (BTO_PUA && BTO_SPEED && !BTO_CAMERA) {
            value = 6
        }
        if (BTO_PUA && !BTO_SPEED && BTO_CAMERA) {
            value = 5
        }
        if (BTO_PUA && !BTO_SPEED && !BTO_CAMERA) {
            value = 4
        }
        if (!BTO_PUA && BTO_SPEED && BTO_CAMERA) {
            value = 3
        }
        if (!BTO_PUA && BTO_SPEED && !BTO_CAMERA) {
            value = 2
        }
        if (!BTO_PUA && !BTO_SPEED && BTO_CAMERA) {
            value = 1
        }
        if (!BTO_PUA && !BTO_SPEED && !BTO_CAMERA) {
            value = 0
        }
        Log.d(TAG, "value :: $value")

        if (!bleService.isConnected()) {
            if (bto_pua == 1) {
                toggle_pua_control.isChecked = true
            } else {
                toggle_pua_control.isChecked = false
            }

            if (bto_speed == 1) {
                toggle_schoolzone_control.isChecked = true
            } else {
                toggle_schoolzone_control.isChecked = false
            }

            if (bto_camera == 1) {
                toggle_speedlimit_control.isChecked = true
            } else {
                toggle_speedlimit_control.isChecked = false
            }
        }
        when (value) {
            0 -> bleService.writeBTOData(AppConstants.COMMAND_BTO_CONTROL_WRITE, 0, 0, 0)

            1 -> bleService.writeBTOData(AppConstants.COMMAND_BTO_CONTROL_WRITE, 0, 0, 1)

            2 -> bleService.writeBTOData(AppConstants.COMMAND_BTO_CONTROL_WRITE, 0, 1, 0)

            3 -> bleService.writeBTOData(AppConstants.COMMAND_BTO_CONTROL_WRITE, 0, 1, 1)

            4 -> bleService.writeBTOData(AppConstants.COMMAND_BTO_CONTROL_WRITE, 1, 0, 0)

            5 -> bleService.writeBTOData(AppConstants.COMMAND_BTO_CONTROL_WRITE, 1, 0, 1)

            6 -> bleService.writeBTOData(AppConstants.COMMAND_BTO_CONTROL_WRITE, 1, 1, 0)

            7 -> bleService.writeBTOData(AppConstants.COMMAND_BTO_CONTROL_WRITE, 1, 1, 1)

            else -> bleService.writeBTOData(AppConstants.COMMAND_BTO_CONTROL_WRITE, 0, 0, 0)
        }
    }

    @SuppressLint("SetTextI18n")
    private fun setVolumeValue() {
        tv_volume.text = volume.toString() + ""
    }

    private fun getBtoSpeedCameraPua() {
        Log.d(TAG, "bto_speed:: " + bto_speed + " bto_camera::  " + bto_camera + "  bto_pua :: " + bto_pua)
        if (bto_speed == 1) {
            toggle_schoolzone_control.isChecked = true
        } else {
            toggle_schoolzone_control.isChecked = false
        }
        if (bto_camera == 1) {
            toggle_speedlimit_control.isChecked = true
        } else {
            toggle_speedlimit_control.isChecked = false
        }

        if (bto_pua == 1) {
            toggle_pua_control.isChecked = true
        } else {
            toggle_pua_control.isChecked = false
        }
        hideProgressDialog(activity)
    }


    private fun setControl_Speed_Sensitivity() {
        tv_speed_control.text =
            controlspeed_sensitivityArry[speed_control_sensitivity]
    }

    private fun setControl_Distance_Sensitivity() {
        tv_camera_range_autocontrol.text =
            controldistance_sensitivityArry[overspeed_cam_dist_control_sensitivity]
    }

    private fun setMaxRpmControl() {
        et_rpm.setText(previousRPMLimit + "")
    }

    private fun setMaxSpeedControl() {
        et_speed.setText(previousSpeedLimit + "")
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

    private val mGattUpdateReceiver: BroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val action = intent.action
            when (action) {
                AppConstants.ACTION_DEVICE_DISCONNECTED -> try {
                    hideProgressDialog(activity)
                    setAlertDialog(requireActivity(), getString(R.string.device_disconnected))
                    bto_speed = AppMethods.previous_bto_speed
                    bto_camera = AppMethods.previous_bto_camera
                    bto_pua = AppMethods.previous_bto_pua
                    Log.d(TAG, " ACTION_DEVICE_DISCONNECTED bto_speed  ::   " + bto_speed)
                    getBtoSpeedCameraPua()
                } catch (e: Exception) {
                    e.printStackTrace()
                }

                AppConstants.ACTION_CHARACTERISTIC_CHANGED -> {
                    var response: String?
                    try {
                        response = intent.getStringExtra("data") ?: ""
                        Log.d(TAG, "::   $response")
                        response = response.replace("SRES".toRegex(), "S")
                        parseResponse(response)
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                }
            }
        }
    }

    private fun parseResponse(response: String) {
        val responseArray = response.split(",")
        if (responseArray[0].trim() == AppConstants.COMMAND_RES) {
            val command = responseArray[1].trim()
            Log.d(TAG, "response :: $response")
            when (command) {
                AppConstants.COMMAND_FIRMWARE_READ -> {
                    if (responseArray.size == 4) {
                        if (responseArray[2].trim() == AppConstants.RESPONSE_S) {
                            val version = responseArray[3].trim()
                            firmwareVersion = version
                            val check_version = "2.2.0"
                            if (isNewUpdate(
                                    check_version,
                                    firmwareVersion
                                )
                            ) { //단말기 최신버전이면 false CheckAutoU
                                hideProgressDialog(activity)
                                setAlertDialog(
                                    requireActivity(),
                                    getString(R.string.firmware_update_check)
                                )
                            }
                        }
                    }
                    bleService.readDeviceData(AppConstants.COMMAND_SPEED_RPM)
                }

                AppConstants.COMMAND_SPEED_RPM -> {
                    if (responseArray.size >= 5) {
                        if (responseArray[2].trim() == AppConstants.RESPONSE_S) {
                            previousRPMLimit = responseArray[3]
                            previousSpeedLimit = responseArray[4]
                            setMaxRpmControl()
                            setMaxSpeedControl()
                        }
                    }
                    bleService.readDeviceData(AppConstants.COMMAND_DEVICE_VOLUME_READ)
                }

                AppConstants.COMMAND_DEVICE_VOLUME_READ -> {
                    if (responseArray.size >= 4) {
                        if (responseArray[2].trim() == AppConstants.RESPONSE_S) {
                            volume = responseArray[3].trim().toInt()
                            setVolumeValue()
                        }
                    }
                    bleService.readDeviceData(AppConstants.COMMAND_SPEED_CONTROL_READ)
                }

                AppConstants.COMMAND_SPEED_CONTROL_READ -> {
                    if (responseArray.size >= 4) {
                        if (responseArray[2].trim() == AppConstants.RESPONSE_S) {
                            speed_control_sensitivity = responseArray[3].trim().toInt()
                            speed_control_sensitivity = if (speed_control_sensitivity == NORMAL_SPEED) {
                                0
                            } else {
                                1
                            }
                            try {
                                setControl_Speed_Sensitivity()
                            } catch (e: Exception) {
                                e.printStackTrace()
                            }
                        }
                    }
                    bleService.readDeviceData(AppConstants.COMMAND_CAMERA_RANGE_CONTROL_READ)
                }

                AppConstants.COMMAND_CAMERA_RANGE_CONTROL_READ -> {
                    if (responseArray.size >= 4) {
                        if (responseArray[2].trim() == AppConstants.RESPONSE_S) {
                            overspeed_cam_dist_control_sensitivity = responseArray[4].trim().toInt()
                            overspeed_cam_dist_control_sensitivity = if (overspeed_cam_dist_control_sensitivity == NORMAL_DISTANCE) {
                                0
                            } else {
                                1
                            }
                            try {
                                setControl_Distance_Sensitivity()
                            } catch (e: Exception) {
                                e.printStackTrace()
                            }
                        }
                    }
                    bleService.readDeviceData(AppConstants.COMMAND_BTO_CONTROL_READ)
                }

                AppConstants.COMMAND_BTO_CONTROL_READ -> if (responseArray.size >= 5) {
                    if (responseArray[2].trim() == AppConstants.RESPONSE_S) {
                        bto_pua = responseArray[3].trim().toInt()
                        bto_speed = responseArray[4].trim().toInt()
                        bto_camera = responseArray[5].trim().toInt()
                        try {
                            getBtoSpeedCameraPua()
                            handler.post(runnable)
                        } catch (e: Exception) {
                            e.printStackTrace()
                        }
                    }
                }

                AppConstants.COMMAND_DEVICE_VOLUME_WRITE -> setWriteResponseAlert(responseArray.toTypedArray())
                AppConstants.COMMAND_DEVICE_THROTTLE_WRITE -> setWriteResponseAlert(responseArray.toTypedArray())
                AppConstants.COMMAND_DEVICE_SENSITIVITY_WRITE -> setWriteResponseAlert(responseArray.toTypedArray())
                AppConstants.COMMAND_SET_SPEED -> {
                    bleService.readDeviceData(AppConstants.COMMAND_SPEED_RPM)
                    setWriteResponseAlert(responseArray.toTypedArray())
                }

                AppConstants.COMMAND_SET_RPM -> {
                    bleService.readDeviceData(AppConstants.COMMAND_SPEED_RPM)
                    setWriteResponseAlert(responseArray.toTypedArray())
                }

                AppConstants.COMMAND_BTO_CONTROL_WRITE -> {
                    if (responseArray.size >= 3) {
                        if (responseArray[2].trim() == AppConstants.RESPONSE_F) {
                            bto_pua = AppMethods.previous_bto_pua
                            bto_speed = AppMethods.previous_bto_speed
                            bto_camera = AppMethods.previous_bto_camera
                        }
                        getBtoSpeedCameraPua()
                    }
                    setWriteResponseAlert(responseArray.toTypedArray())
                }

                AppConstants.COMMAND_SPEED_CONTROL_WRITE -> if (responseArray.size >= 3) {
                    if (responseArray[2].trim() == AppConstants.RESPONSE_S) {
                        try {
                            hideProgressDialog(activity)
                            setControl_Speed_Sensitivity()
                            setWriteResponseAlert(responseArray.toTypedArray())
                        } catch (e: Exception) {
                            e.printStackTrace()
                        }
                    }
                }

                AppConstants.COMMAND_CAMERA_RANGE_CONTROL_WRITE -> if (responseArray.size >= 3) {
                    if (responseArray[2].trim() == AppConstants.RESPONSE_S) {
                        try {
                            hideProgressDialog(activity)
                            setControl_Distance_Sensitivity()
                            setWriteResponseAlert(responseArray.toTypedArray())
                        } catch (e: Exception) {
                            e.printStackTrace()
                        }
                    }
                }

                AppConstants.COMMAND_REAL_TIME_V_I_READ -> if (responseArray[2] == AppConstants.RESPONSE_S) {
                    if (responseArray.size >= 9) {
                        try {
                            realspeed = responseArray[4].trim().toInt() //012 //3
                        } catch (e: Exception) {
                            e.printStackTrace()
                        }
                    }
                } else {
                    Toast.makeText(activity, getString(R.string.failure), Toast.LENGTH_SHORT).show()
                }
            }
        }
    }

    private fun setWriteResponseAlert(responseArry: Array<String>) {
        if (responseArry.size >= 3) {
            hideProgressDialog(activity)
            Log.d(TAG, "responseArry[2].trim():: " + responseArry[2].trim { it <= ' ' })
            if (responseArry[2].trim { it <= ' ' } == AppConstants.RESPONSE_S) {
                setAlertDialog(requireActivity(), getString(R.string.write_success))
            } else {
                setAlertDialog(requireActivity(), getString(R.string.write_fail))
            }
        }
    }

    override fun onStop() {
        super.onStop()
        isBackStacked = true
    }

    override fun onResume() {
        super.onResume()
        isBackStacked = false
    }

    private fun setWritesensitivityData(strvalue: Int) {
        val deactiv1_sensitivity = "95"
        val activ1_sensitivity = "100"
        val deactiv2_sensitivity = "100"
        val activ2_sensitivity = "105"

        when (strvalue) {
            0 -> bleService.writeStringData(AppConstants.COMMAND_SPEED_CONTROL_WRITE, ("$deactiv1_sensitivity,$activ1_sensitivity"))

            1 -> bleService.writeStringData(AppConstants.COMMAND_SPEED_CONTROL_WRITE, ("$deactiv2_sensitivity,$activ2_sensitivity"))

            else -> bleService.writeStringData(AppConstants.COMMAND_SPEED_CONTROL_WRITE, ("$deactiv1_sensitivity,$activ1_sensitivity"))
        }
    }

    private fun setWriteRangesettingData(strvalue: Int) {
        val Level1_range1 = "0,49,150"
        val Level1_range2 = "50,60,300"
        val Level1_range3 = "61,89,400"
        val Level1_range4 = "90,99,500"
        val Level1_range5 = "100,255,600"
        val Level2_range1 = "0,79,150"
        val Level2_range2 = "80,89,200"
        val Level2_range3 = "90,99,300"
        val Level2_range4 = "100,140,400"

        when (strvalue) {
            0 -> bleService.writeStringData(
                AppConstants.COMMAND_CAMERA_RANGE_CONTROL_WRITE,
                ("$Level1_range1,$Level1_range2,$Level1_range3,$Level1_range4,$Level1_range5")
            )

            1 -> bleService.writeStringData(
                AppConstants.COMMAND_CAMERA_RANGE_CONTROL_WRITE,
                ("$Level2_range1,$Level2_range2,$Level2_range3,$Level2_range4")
            )

            else -> bleService.writeStringData(
                AppConstants.COMMAND_CAMERA_RANGE_CONTROL_WRITE,
                ("$Level1_range1,$Level1_range2,$Level1_range3,$Level1_range4,$Level1_range5")
            )
        }
    }


    private lateinit var messageAlertDialog: AlertDialog

    private fun showMessageAlertDiaog(msg: String) {
        hideKeyboard()
        hideMessageAlertDialog()
        val alertDialogBuilder = AlertDialog.Builder(activity)
        alertDialogBuilder.setMessage(msg)
        alertDialogBuilder.setPositiveButton(
            R.string.ok
        ) { dialog, which ->
            dialog.dismiss()
            if (msg == getString(R.string.speed_control_sensitivity_alert)) {
                setWritesensitivityData(speed_control_sensitivity)
            } else if (msg == getString(R.string.overspeed_cam_dist_control_sensitivity_alert)) {
                setWriteRangesettingData(overspeed_cam_dist_control_sensitivity)
            } else {
                if (msg == getString(R.string.speed_alert)) {
                    et_speed.requestFocus()
                } else {
                    et_rpm.requestFocus()
                }
                Handler(Looper.getMainLooper()).postDelayed({
                    val imm =
                        requireActivity().getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
                    if (msg == getString(R.string.speed_alert)) {
                        imm.showSoftInput(et_speed, InputMethodManager.SHOW_IMPLICIT)
                    } else {
                        imm.showSoftInput(et_rpm, InputMethodManager.SHOW_IMPLICIT)
                    }
                }, 100)
            }
        }
        alertDialogBuilder.setNegativeButton(
            R.string.cancel
        ) { dialog, which -> dialog.dismiss() }
        messageAlertDialog = alertDialogBuilder.create()
        messageAlertDialog.show()
        messageAlertDialog.setCancelable(true)
        messageAlertDialog.setCanceledOnTouchOutside(false)
    }

    private fun hideMessageAlertDialog() {
        if (this::messageAlertDialog.isInitialized && messageAlertDialog.isShowing) {
            messageAlertDialog.dismiss()
        }
    }

    private fun hideKeyboard() {
        try {
            val view = requireActivity().currentFocus
            if (view != null) {
                val imm =
                    requireActivity().getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
                imm.hideSoftInputFromWindow(view.windowToken, 0)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    companion object {
        private const val NORMAL_DISTANCE =
            49 // [49] REQ,608,0,49,150,50,60,300,61,89,400,90,99,500,100,255,600;
        private const val NORMAL_SPEED = 95 // [95]REQ,606,95,100;;

        private var volume = 1
        private var speed_control_sensitivity = 1
        private var overspeed_cam_dist_control_sensitivity = 1

        private var bto_speed = 1 // 1: Speed  //2:Camera //3:PUA
        private var bto_camera = 1 // 1: Speed  //2:Camera //3:PUA
        private var bto_pua = 1 // 1: Speed  //2:Camera //3:PUA

        private var BTO_SPEED = false // 1: Speed  //2:Camera //3:PUA
        private var BTO_CAMERA = false // 1: Speed  //2:Camera //3:PUA
        private var BTO_PUA = false // 1: Speed  //2:Camera //3:PUA
    }
}
