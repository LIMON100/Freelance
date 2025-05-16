package com.axon.kisa10.ble

import android.content.ComponentName
import android.content.ServiceConnection
import android.os.IBinder

/**
 * Created by Hussain on 01/07/24.
 */
class ServiceConnector(
    private val onSuccess : (AxonBLEService) -> Unit,
    private val onFailure : (String) -> Unit
) : ServiceConnection {
    override fun onServiceConnected(className: ComponentName?, binder: IBinder?) {
        val localBinder = binder as? AxonBLEService.LocalBinder
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