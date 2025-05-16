package com.skyautonet.drsafe.service

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent

/**
 * Created by Hussain on 05/05/25.
 */
class OnNotificationDismissReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context?, intent: Intent?) {
        DrSafeBleService.getInstance()?.restartNotification()
    }
}