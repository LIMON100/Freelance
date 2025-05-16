package com.axon.kisa10.fragment

import android.app.Dialog
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCharacteristic
import android.content.Context
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.os.Bundle
import android.util.Log
import android.view.Window
import android.view.WindowManager
import android.widget.Button
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.util.AppConstants
import com.kisa10.R

class CustomDialogCalibration(private val context: Context, private val mListener: IOnClickConfirmListener,private val bleService: AxonBLEService) : Dialog(context) {
    private lateinit var mBtnConfirm: Button
    private lateinit var mBtnCancel: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        this.requestWindowFeature(Window.FEATURE_NO_TITLE)
        setContentView(R.layout.dialog_calibration)
        val window = this.window
        val wlp = window!!.attributes
        //        wlp.gravity = Gravity.CENTER;
        wlp.width = WindowManager.LayoutParams.MATCH_PARENT
        wlp.flags = wlp.flags and WindowManager.LayoutParams.MATCH_PARENT
        window.attributes = wlp
        window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
        window.attributes = wlp
        initComponents()
        addListener()
    }

    private fun initComponents() {
        mBtnCancel = findViewById(R.id.btn_cancel)
        mBtnConfirm = findViewById(R.id.btn_confirm)
    }

    private fun addListener() {
        mBtnConfirm.setOnClickListener {
            if (bleService.isConnected()) {
                bleService.flush()
                val cmd = "AT$\$DS_CALIB=1\r\n".toByteArray(charset("euc-kr"))
                bleService.writeCharacteristic(cmd, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)
//                Log.i("TEST",cmd)
//                bleService.readDeviceData(AppConstants.COMMAND_DEVICE_CALIBRATION_WRITE)
                mListener.onConfirm()
            }
            dismiss()
        }

        mBtnCancel.setOnClickListener {
            dismiss()
            mListener.onCancel()
        }
    }

    override fun onBackPressed() {
        cancel()
        mListener.onCancel()
    }

    interface IOnClickConfirmListener {
        fun onConfirm()
        fun onCancel()
    }
}
