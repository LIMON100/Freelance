package com.skyautonet.drsafe.ui

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.skyautonet.drsafe.util.AppConstants

open class BaseActivity : AppCompatActivity() {

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(this)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        localBroadcastManager.registerReceiver(bleReceiver, AppConstants.makeIntentFilter())
    }

    private val bleReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (intent.action == AppConstants.ACTION_DEVICE_CONNECTION_COMPLETE) {
                onDeviceConnected()
            }

            if (intent.action == AppConstants.ACTION_DEVICE_DISCONNECTED) {
                onDeviceDisconnected()
            }

            if (intent.action == AppConstants.ACTION_CHARACTERISTIC_CHANGED) {
                val data = intent.getStringExtra("data")
                val rawData = intent.getByteArrayExtra(AppConstants.RAW_DATA)
                onDataReceived(rawData, data)
            }
        }
    }

    protected open fun onDataReceived(rawData: ByteArray?, data: String?) {

    }

    protected open fun onDeviceDisconnected() {

    }

    protected open fun onDeviceConnected() {

    }

    override fun onDestroy() {
        super.onDestroy()
        localBroadcastManager.unregisterReceiver(bleReceiver)
    }
}