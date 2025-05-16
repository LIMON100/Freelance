package com.axon.kisa10.ble

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent

/**
 * Created by Hussain on 03/07/24.
 */
class OnNotificationDismissReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context?, intent: Intent?) {
        AxonBLEService.getInstance()?.restartNotification()
    }
}