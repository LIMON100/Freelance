package com.axon.kisa10.model

import android.bluetooth.BluetoothDevice
import java.io.Serializable

data class ScanModel(
    var deviceName: String,
    var deviceMacAddress: String,
    var bluetoothDevice: BluetoothDevice,
    var deviceRssi: Int
) : Serializable
