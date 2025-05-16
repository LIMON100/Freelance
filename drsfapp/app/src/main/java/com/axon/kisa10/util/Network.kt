package com.axon.kisa10.util

import android.annotation.SuppressLint
import android.content.Context
import android.net.ConnectivityManager
import android.net.NetworkInfo
import android.telephony.TelephonyManager
import java.lang.reflect.InvocationTargetException

class Network private constructor() {

    companion object {

        @JvmStatic
        fun isConnectionFast(context: Context): Boolean {
            try {
                val nInfo = context
                    .getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
                nInfo.activeNetworkInfo!!.isConnectedOrConnecting

                val cm = context
                    .getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
                val netInfo = cm.activeNetworkInfo
                return netInfo != null && netInfo.isConnectedOrConnecting
            } catch (e: Exception) {
                return false
            }
        }

    }
}