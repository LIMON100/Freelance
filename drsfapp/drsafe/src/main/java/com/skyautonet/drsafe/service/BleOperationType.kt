package com.axon.kisa10.ble

import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.content.Context

/**
 * Created by Hussain on 05/05/25.
 */
sealed class BleOperationType {
    data class Connect(val device: BluetoothDevice, val context: Context) : BleOperationType()
    data class Disconnect(val bluetoothGatt : BluetoothGatt) : BleOperationType()
    data class DiscoverServices(val bluetoothGatt : BluetoothGatt) : BleOperationType()
    data class RequestMtu(val bluetoothGatt: BluetoothGatt) : BleOperationType()
    data class WriteDescriptor(val bluetoothGatt: BluetoothGatt, val descriptor: BluetoothGattDescriptor,val payload : ByteArray) : BleOperationType()
    data class WriteCharacteristics(val bluetoothGatt: BluetoothGatt, val characteristic: BluetoothGattCharacteristic, val data : ByteArray, val type : Int = BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE) : BleOperationType()
    data class EnableCharacteristicNotification(val bluetoothGatt: BluetoothGatt, val characteristic: BluetoothGattCharacteristic) : BleOperationType()
    data class DisableCharacteristicNotification(val bluetoothGatt: BluetoothGatt, val characteristic: BluetoothGattCharacteristic) : BleOperationType()

}