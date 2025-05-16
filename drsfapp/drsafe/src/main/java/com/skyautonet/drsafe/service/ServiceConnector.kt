package com.skyautonet.drsafe.service

import android.content.ComponentName
import android.content.ServiceConnection
import android.os.IBinder

/**
 * Created by Hussain on 05/05/25.
 */
class ServiceConnector(
    private val onSuccess : (DrSafeBleService) -> Unit,
    private val onFailure : (String) -> Unit
) : ServiceConnection {
    override fun onServiceConnected(className: ComponentName?, binder: IBinder?) {
        val localBinder = binder as? DrSafeBleService.LocalBinder
        if (localBinder != null) {
            val service = binder.getService().get()
            if (service != null) {
                onSuccess(service)
            } else {
                onFailure("Failed to get service")
            }
        } else {
            onFailure("Failed to get service.")
        }
    }

    override fun onServiceDisconnected(p0: ComponentName?) {
        onFailure("Service Disconnected")
    }
}