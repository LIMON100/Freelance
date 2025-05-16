package com.axon.kisa10.util

import android.app.Activity
import android.app.AlertDialog
import android.app.Dialog
import android.app.ProgressDialog
import android.content.Context
import android.util.Log
import android.view.View
import android.view.Window
import android.view.WindowManager
import android.widget.ImageView
import android.widget.TextView
import com.kisa10.R
import java.io.File
import java.io.UnsupportedEncodingException

object AppMethods {

    private const val TAG = "AppMethods"

    @JvmStatic
    fun deleteFile(context: Context, fileName: String) {
        try {
            val filePath = context.cacheDir.toString() + AppConstants.AXON_DIR + "/" + fileName
            Log.d("result", "AppMethods deleteFile  -- filePath   $filePath")

            val file: File

            if (filePath.isNotEmpty()) {
                file = File(filePath)
                if (file.exists()) {
                    file.delete()
                }
            } else {
                Log.i(TAG, "File : $fileName does not exist.")
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    @JvmStatic
    fun deleteFile(file: File) {
        try {
            if (file.exists()) {
                file.delete()
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    @JvmStatic
    fun convertStringToByte(text: String?): ByteArray? {
        return try {
            text?.toByteArray(charset("UTF-8"))
        } catch (e: UnsupportedEncodingException) {
            null
        }
    }

    @JvmStatic
    fun convertByteToString(data: ByteArray?): String {
        try {
            return String(data!!, charset("UTF-8"))
        } catch (e: UnsupportedEncodingException) {
            e.printStackTrace()
            return ""
        }
    }


    private var pDialog: ProgressDialog? = null
    @JvmStatic
    fun showProgressDialog(context: Context?) {
        hideProgressDialog(context)
        try {
            if (context != null) {
                pDialog = ProgressDialog(context)
                pDialog!!.setMessage(context.getString(R.string.please_wait))
                pDialog!!.setCancelable(false)
                pDialog!!.show()
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    @JvmStatic
    fun hideProgressDialog(context: Context?) {
        try {
            pDialog?.dismiss()
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    @JvmStatic
    fun setAlertDialog(context: Context, msg: String?, onClick : (() -> Unit)? = null) {
        (context as Activity).runOnUiThread {
            val alertDialogBuilder = AlertDialog.Builder(context)
            alertDialogBuilder.setMessage(msg)
            alertDialogBuilder.setPositiveButton(
                R.string.ok
            ) { dialog , _ ->
                onClick?.invoke()
                dialog.dismiss()
            }

            val alertDialog = alertDialogBuilder.create()
            alertDialog.show()
            alertDialog.setCancelable(true)
            alertDialog.setCanceledOnTouchOutside(false)
        }
    }


    fun showLatestVersionDialog(activity: Activity?) {
        val dialog =
            Dialog(activity!! /*, android.R.style.Theme_Translucent_NoTitleBar_Fullscreen*/)
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE)
        dialog.setCancelable(false)
        dialog.window!!.setBackgroundDrawableResource(android.R.color.transparent)
        dialog.setContentView(R.layout.dialog_latest_version)
        val tv_popupmsg = dialog.findViewById<View>(R.id.tv_popupmsg) as TextView
        tv_popupmsg.setText(R.string.this_is_latest_version)
        val tv_ok = dialog.findViewById<View>(R.id.tv_ok) as TextView
        val iv_close = dialog.findViewById<View>(R.id.iv_close) as ImageView
        tv_ok.setOnClickListener { dialog.dismiss() }
        iv_close.setOnClickListener { dialog.dismiss() }
        dialog.setCancelable(false)
        dialog.show()
        val window = dialog.window
        window!!.setLayout(
            WindowManager.LayoutParams.MATCH_PARENT,
            WindowManager.LayoutParams.MATCH_PARENT
        )
    }

    private val HEX_ARRAY = "0123456789ABCDEF".toCharArray()

    @JvmStatic
    fun bytesToHexString(bytes: ByteArray): String {
        val hexChars = CharArray(bytes.size * 2)
        for (j in bytes.indices) {
            val v = bytes[j].toInt() and 0xFF
            hexChars[j * 2] = HEX_ARRAY[v ushr 4]
            hexChars[j * 2 + 1] = HEX_ARRAY[v and 0x0F]
        }
        return String(hexChars)
    }

    @JvmField
    var previous_bto_speed: Int = 0 // 1: PUA    //2: Speed //3:Camera
    @JvmField
    var previous_bto_camera: Int = 0 // 1: PUA    //2: Speed //3:Camera
    @JvmField
    var previous_bto_pua: Int = 0 // 1: PUA    //2: Speed //3:Camera
}
