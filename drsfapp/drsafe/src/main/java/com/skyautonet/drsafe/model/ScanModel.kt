package com.skyautonet.drsafe.model

import android.bluetooth.BluetoothDevice
import java.io.Serializable

data class ScanModel(
    var deviceName: String,
    var deviceMacAddress: String,
    var bluetoothDevice: BluetoothDevice,
    var deviceRssi: Int
) : Serializable
