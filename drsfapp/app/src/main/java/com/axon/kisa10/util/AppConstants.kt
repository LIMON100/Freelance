package com.axon.kisa10.util

import android.content.IntentFilter

object AppConstants {
    const val LAST_CONNECTED_DEVICE = "LAST_CONNECTED_DEVICE"
    const val DB_VERSION: String = "DB_VERSION"
    const val KEY_LOG_FILE = "KEY_LOG_FILE"
    const val RAW_DATA = "RAW_DATA_BYTES"
    const val AXON_DIR: String = "/Axon"
    const val LOG_DIR: String = "/Axon/Logs/"
    const val DB_DIR : String = "/Axon/DBUpdate/"
    const val EVENT_DIR: String = "/Axon/Events/"


    const val REQUEST_LOCATION_PERMISSION: Int = 1
    const val REQUEST_BG_LOCATION_PERMISSION: Int = 5
    const val MY_MARSHMELLO_PERMISSION: Int = 2
    const val REQUEST_ENABLE_BLUETOOTH: Int = 3
    const val REQUEST_ENABLE_LOCATION: Int = 4
    const val REQUEST_NOTIFICATION_PERMISSION = 6

    const val COMMAND_SYSTEM_RESET: String = "553"
    const val BROADCAST_CHILD_FRAGMENT: String = "BROADCAST_CHILD_FRAGMENT"
    const val BROADCAST_DATE_DIALOG: String = "BROADCAST_DATE_DIALOG ="
    const val EXTRA_SETTING: String = "EXTRA_SETTING"
    const val BASE_URL: String = "http://axonfms.com/userservices/"
    const val GPS_Update: String = BASE_URL + "gps_location"
    const val FirmWare_Update: String = BASE_URL + "firmware_location"

    //    public static final String FirmWare_Update=BASE_URL+"firmware_location_test"; //Firmware_test_type
    const val ble_Update: String = BASE_URL + "blefirmware_location"
    const val version_url: String = BASE_URL + "firmware_version"
    const val get_map_db_version: String = "http://axonfms.com/userservices/gps_version"
    const val get_map_db_update_url_remove_version: String =
        "http://axonfms.com/userservices/updated_gps"
    const val reset_userid_url: String = "http://103.55.190.192/userservices/user_reset_device"

    //    public static final String firmware_version_params = "test_stm32";  //Firmware_test_type
    const val firmware_version_params: String = "stm32"
    const val databse_version_params: String = "gps"
    const val ble_version_params: String = "ble"

    const val ble_gbl_file_name: String = "application.gbl"
    const val spp_service_uuid: String = "4880c12c-fdcb-4077-8920-a450d7f9b907"
    const val spp_char_uuid: String = "b273bfed-974f-43a7-9f77-d28e9f7c2d41"
    const val device_char_uuid: String = "fec26ec4-6d71-4442-9f81-55bc21d658d6"
    const val desc_uuid: String = "00002902-0000-1000-8000-00805f9b34fb"

    const val generic_access_service_uuid: String = "00001800-0000-1000-8000-00805f9b34fb"
    const val device_name_char_uuid: String = "00002a00-0000-1000-8000-00805f9b34fb"
    const val apperence_char_uuid: String = "00002a01-0000-1000-8000-00805f9b34fb"

    const val device_service_uuid: String = "0000180a-0000-1000-8000-00805f9b34fb"
    const val manufacture_name_char_uuid: String = "00002a29-0000-1000-8000-00805f9b34fb"
    const val model_number_char_uuid: String = "00002a24-0000-1000-8000-00805f9b34fb"
    const val system_id_char_uuid: String = "00002a23-0000-1000-8000-00805f9b34fb"

    const val ota_service_uuid: String = "1d14d6ee-fd63-4fa1-bfa4-8f47b42119f0"
    const val ota_char_uuid: String = "f7bf3564-fb6d-4e53-88a4-5e37e0326063"
    const val pref_deviceName: String = "DeviceName"
    const val pref_deviceAddress: String = "DeviceAddress"

    const val COMMAND_REQ: String = "REQ"
    const val COMMAND_RES: String = "RES"
    const val RESPONSE_S: String = "S"
    const val RESPONSE_F: String = "F"
    const val RESPONSE_C: String = "C"
    const val RESPONSE_A: String = "A"
    const val RESPONSE_D = "D"
    const val RESPONSE_E = "E"

    const val COMMAND_STORED_LOG_FILE_LIST: String = "101"
    const val COMMAND_REQUEST_FILE = "102"
    const val COMMAND_STORED_EVENT_FILE_LIST = "103"
    const val COMMAND_REAL_TIME_V_I_READ: String = "111"
    const val COMMAND_MAP_INFO = "112"
    const val COMMAND_VEHICLE_MANUFACTURER_READ: String = "131"
    const val COMMAND_VEHICLE_MANUFACTURER_WRITE: String = "132"
    const val COMMAND_CUSTOMER_CODE_READ: String = "141"
    const val COMMAND_CUSTOMER_CODE_WRITE: String = "142"

    const val COMMAND_VINCODE_READ: String = "201"
    const val COMMAND_VINCODE_WRITE: String = "202"

    const val COMMAND_FIRMWARE_READ: String = "301"
    const val COMMAND_FIRMWARE_WRITE: String = "302"

    const val COMMAND_DATABASE_VERSION: String = "401"
    const val COMMAND_DATABASE_TRANSFER: String = "402"
    const val COMMAND_DATABASE_START_UPDATE: String = "403"
    const val COMMAND_MAP_DATABASE_REMOVE: String = "405"
    const val COMMAND_DATABASE_END: String = "404"

    const val COMMAND_BLE_UPGRADE_READ: String = "501"
    const val COMMAND_BLE_UPGRADE_WRITE: String = "502"
    const val COMMAND_DEVICE_VOLUME_READ: String = "513"
    const val COMMAND_DEVICE_VOLUME_WRITE: String = "514"
    const val COMMAND_DEVICE_THROTTLE_READ: String = "511"
    const val COMMAND_DEVICE_THROTTLE_WRITE: String = "512"
    const val COMMAND_DEVICE_SPEED_READ: String = "521"
    const val COMMAND_DEVICE_SPEED_WRITE: String = "522"
    const val COMMAND_DEVICE_CALIBRATION_READ: String = "531"
    const val COMMAND_DEVICE_CALIBRATION_WRITE: String = "532"
    const val COMMAND_DEVICE_SENSITIVITY_READ: String = "541"
    const val COMMAND_DEVICE_SENSITIVITY_WRITE: String = "542"
    const val COMMAND_DEVICE_INITIALIZATION_READ: String = "551"
    const val COMMAND_FACTORY_RESET: String = "552"

    const val COMMAND_SPEED_RPM: String = "601"
    const val COMMAND_SET_SPEED: String = "602"
    const val COMMAND_SET_RPM: String = "603"
    const val COMMAND_BTO_CONTROL_WRITE: String = "604"
    const val COMMAND_BTO_CONTROL_READ: String = "605"
    const val COMMAND_SPEED_CONTROL_WRITE: String = "606"
    const val COMMAND_SPEED_CONTROL_READ: String = "607"
    const val COMMAND_CAMERA_RANGE_CONTROL_WRITE: String = "608"
    const val COMMAND_CAMERA_RANGE_CONTROL_READ: String = "609"

    const val COMMAND_OBD_READ: String = "701"
    const val COMMAND_OBD_WRITE: String = "702"
    const val COMMAND_OBD_VALUE_READ: String = "703"
    const val COMMAND_GPS_VALUE_READ: String = "704"

    const val ACTION_DEVICE_DISCONNECTED: String = "com.axon.ACTION_DEVICE_DISCONNECTED"
    const val ACTION_DEVICE_CONNECTED: String = "com.axon.ACTION_DEVICE_CONNECTED"
    const val ACTION_DEVICE_CONNECTION_COMPLETE: String = "com.axon.ACTION_CONNECTION_COMPLETE"
    const val ACTION_CHARACTERISTIC_CHANGED: String = "com.axon.ACTION_CHARACTERISTIC_CHANGED"


    const val EVENT_START_SERVICE = "startService"
    const val EVENT_UPLOAD_LOG = "uploadLog"
    const val EVENT_UPLOAD_EVENT = "uploadEvent"
    const val EVENT_UPDATE_STATE = "updateState"
    const val EVENT_UPDATE_FIRMWARE = "updateFirmware"

    const val START_FOREGROUND = "START_FOREGROUND"

    fun makeIntentFilter(): IntentFilter {
        val filter = IntentFilter()
        filter.priority = IntentFilter.SYSTEM_HIGH_PRIORITY - 1
        filter.addAction(ACTION_DEVICE_CONNECTED)
        filter.addAction(ACTION_DEVICE_DISCONNECTED)
        filter.addAction(ACTION_CHARACTERISTIC_CHANGED)
        filter.addAction(ACTION_DEVICE_CONNECTION_COMPLETE)
        return filter
    }

    const val FIRMWARE_FILE_NAME: String = "FIRMWARE.bin"
    const val BLE_FILE_NAME: String = "application.gbl"

    const val No_data_value: Int = -5000
    @JvmField
    var dbUpdateFileNameList: ArrayList<String> = ArrayList()
    @JvmField
    var dbRemoveFileNameList: ArrayList<String> = ArrayList()
    @JvmField
    var latestDbVesrion: String = ""
    @JvmField
    var rededVersion: String = ""
    var samebrand_dtglite: String = "DTGLITE"
    var axon_appname: String = "AXON"
    const val dtg_lite = "DTGLITE"

    val GOOGLE_WEB_CLIENT_ID = "173243809391-9ugqecg2bgtudmn34mcs8uii70e6bo30.apps.googleusercontent.com"

    val NATIVE_APP_KEY = "5bdfd212ac417e0b79f8a9bc67f5796d"

    const val AXON_BLE_NOTIFICATION_CHANNEL = "AXON_BLE_NOTIFICATION_CHANNEL"



    object Anim {
        const val NONE: Int = 0
        const val SLIDING: Int = 1
        const val FADE: Int = 2
    }
}
