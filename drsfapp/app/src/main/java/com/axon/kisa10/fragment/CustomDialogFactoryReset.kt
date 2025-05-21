package com.axon.kisa10.fragment

import android.app.Dialog
import android.content.Context
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.os.Bundle
import android.view.Window
import android.view.WindowManager
import android.widget.Button
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.R

class CustomDialogFactoryReset(
    private val context: Context,
    private val mListener: IOnClickConfirmListener,
    private val bleService: AxonBLEService
) : Dialog(context) {
    //    private ImageView mImvDismiss;
    private lateinit var mBtnConfirm: Button
    private lateinit var mBtnCancel: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        this.requestWindowFeature(Window.FEATURE_NO_TITLE)
        setContentView(R.layout.dialog_factory_reset)
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
//        mImvDismiss = findViewById(R.id.img_close);
        mBtnCancel = findViewById(R.id.btn_cancel)
        mBtnConfirm = findViewById(R.id.btn_confirm)
    }

    private fun addListener() {
        mBtnConfirm.setOnClickListener {
            if (bleService.isConnected()) {
                bleService.readDeviceData(AppConstants.COMMAND_FACTORY_RESET)
                mListener.onConfirm()
            }
            dismiss()
        }

        mBtnCancel.setOnClickListener {
            dismiss()
            mListener.onCancel()
        }

        /*       mImvDismiss.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                dismiss();
            }
        });
 */
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
