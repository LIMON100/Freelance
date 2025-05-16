package com.skyautonet.drsafe.util

import android.content.IntentFilter

object AppConstants {

    const val default_web_client_id = "161674994276-rurmpj3hq9efopst8bmgrlokejl1rboe.apps.googleusercontent.com"
    const val RAW_DATA = "RAW_DATA"
    const val START_FOREGROUND = "START_FOREGROUND"
    const val LAST_CONNECTED_DEVICE = "LAST_CONNECTED_DEVICE"

    const val REQUEST_LOCATION_PERMISSION: Int = 1
    const val REQUEST_ENABLE_BLUETOOTH: Int = 3
    const val REQUEST_ENABLE_LOCATION: Int = 4
    const val REQUEST_BG_LOCATION_PERMISSION = 5
    const val REQUEST_NOTIFICATION_PERMISSION = 6
    const val ACTION_SETTINGS = 7

    const val spp_service_uuid: String = "4880c12c-fdcb-4077-8920-a450d7f9b907"
    const val spp_char_uuid: String = "b273bfed-974f-43a7-9f77-d28e9f7c2d41"
    const val device_char_uuid: String = "fec26ec4-6d71-4442-9f81-55bc21d658d6"
    const val desc_uuid: String = "00002902-0000-1000-8000-00805f9b34fb"

    const val ACTION_DEVICE_DISCONNECTED: String = "com.axon.ACTION_DEVICE_DISCONNECTED"
    const val ACTION_DEVICE_CONNECTED: String = "com.axon.ACTION_DEVICE_CONNECTED"
    const val ACTION_DEVICE_CONNECTION_COMPLETE: String = "com.axon.ACTION_CONNECTION_COMPLETE"
    const val ACTION_CHARACTERISTIC_CHANGED: String = "com.axon.ACTION_CHARACTERISTIC_CHANGED"

    const val DR_SAFE_BLE_NOTIFICATION_CHANNEL = "DR_SAFE_BLE_NOTIFICATION_CHANNEL"

    const val dtg_lite = "DTGLITE"

    fun makeIntentFilter(): IntentFilter {
        val filter = IntentFilter()
        filter.addAction(ACTION_DEVICE_CONNECTED)
        filter.addAction(ACTION_DEVICE_DISCONNECTED)
        filter.addAction(ACTION_CHARACTERISTIC_CHANGED)
        filter.addAction(ACTION_DEVICE_CONNECTION_COMPLETE)
        return filter
    }


    const val VIN_CODE = "VIN_CODE"
    const val USER_INFO = "USER_INFO"
    const val TOKEN = "TOKEN"
    const val DISTRIBUTOR_TOKEN = "DISTRIBUTOR_TOKEN"
    const val DISTRIBUTOR_USER_INFO = "DISTRIBUTOR_USER_INFO"
    const val FCM_TOKEN = "FCM_TOKEN"
    const val VEHICLE_ID = "VEHICLE_ID"
    const val APP_LOCALE: String = "APPLICATION_LOCALE"
    const val TERMS_CONDITION_AGREE_KEY: String = "TERMS_CONDITION_AGREE_KEY"
    const val KAKAO_TOKEN_KEY: String = "KAKAO_TOKEN_KEY"
    const val GOOGLE_TOKEN_KEY: String = "GOOGLE_TOKEN_KEY"

    val NATIVE_APP_KEY = "5bdfd212ac417e0b79f8a9bc67f5796d"


}