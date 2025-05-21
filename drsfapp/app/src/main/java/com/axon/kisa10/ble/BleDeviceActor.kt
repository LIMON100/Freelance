package com.axon.kisa10.ble

import android.annotation.SuppressLint
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothProfile
import android.content.Context
import android.content.Intent
import android.util.Log
import com.axon.kisa10.ble.BleCharacteristic.enableNotifychar
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AppMethods.convertByteToString
import com.axon.kisa10.util.AppMethods.setAlertDialog
import com.axon.kisa10.R
import java.util.Locale

@SuppressLint("MissingPermission")
class BleDeviceActor : Runnable {

    constructor(context: Context) {
        mContext = context
    }

    private var mBluetoothDevice: BluetoothDevice? = null
    override fun run() {
        if (mBluetoothDevice == null) {
            return
        }
        try {
            if (mBluetoothGatt != null) {
                mBluetoothGatt?.close()
                mBluetoothGatt = null
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }

        mBluetoothGatt = mBluetoothDevice?.connectGatt(
            mContext,
            false,
            mGattCallback,
            BluetoothDevice.TRANSPORT_LE
        )
    }

    fun connectToDevice(bluetoothDevice: BluetoothDevice?) {
        this.mBluetoothDevice = bluetoothDevice
        stopThread()
        startThread()
    }

    fun startThread() {
        thread = Thread(this)
        thread?.start()
    }

    companion object {
        @SuppressLint("StaticFieldLeak")
        private lateinit var mContext: Context
        private var mBluetoothGatt: BluetoothGatt? = null
        private var thread: Thread? = null
        private var isConnected: Boolean = false
        var MTU: Int = 20

        fun isIsConnected() : Boolean {
            return isConnected
        }
        fun getmBluetoothGatt(): BluetoothGatt? {
            return mBluetoothGatt
        }

        fun disconnectDevice() {
            try {
                if (mBluetoothGatt != null && isConnected) {
                    mBluetoothGatt?.disconnect()
                    mBluetoothGatt?.close()
                    isConnected = false
                    mBluetoothGatt = null
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }

        @Deprecated("Deprecated in Java")
        var mGattCallback: BluetoothGattCallback = object : BluetoothGattCallback() {
            override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
                super.onConnectionStateChange(gatt, status, newState)

                Log.d("ble==> ", "onConnectionStateChange: $newState")
                if (newState == BluetoothProfile.STATE_CONNECTED) {
                    gatt.discoverServices()
                } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                    isConnected = false
                    stopThread()
                    broadcastUpdate(AppConstants.ACTION_DEVICE_DISCONNECTED)
                } else {
                    isConnected = false
                    stopThread()
                    broadcastUpdate(AppConstants.ACTION_DEVICE_DISCONNECTED)
                }
            }

            override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
                super.onServicesDiscovered(gatt, status)
                Log.d("ble==> ", "onServicesDiscovered")
                if (status == BluetoothGatt.GATT_SUCCESS) {
                    isConnected = true
                    mBluetoothGatt = gatt
                    gatt.requestMtu(247)
                    Log.d("ble==> ", "onServicesDiscovered: success")
                }
            }

            @Deprecated("Deprecated in Java")
            override fun onCharacteristicRead(
                gatt: BluetoothGatt,
                characteristic: BluetoothGattCharacteristic,
                status: Int
            ) {
                super.onCharacteristicRead(gatt, characteristic, status)
                Log.d("ble==> ", "onCharacteristicRead: " + characteristic.value.contentToString())
//                if (characteristic != null) {
//                    val data = characteristic.value
//                    if (data != null) {
//                        when (characteristic.uuid.toString().lowercase(Locale.getDefault())) {
//                        }
//                    }
//                }
            }


            override fun onCharacteristicWrite(
                gatt: BluetoothGatt,
                characteristic: BluetoothGattCharacteristic,
                status: Int
            ) {
                super.onCharacteristicWrite(gatt, characteristic, status)
                Log.d(
                    "ble==> ",
                    "onCharacteristicWrite: " + characteristic.value.contentToString() + " statu s" + status
                )
            }

            @Deprecated("Deprecated in Java")
            override fun onCharacteristicChanged(
                gatt: BluetoothGatt,
                characteristic: BluetoothGattCharacteristic
            ) {
                super.onCharacteristicChanged(gatt, characteristic)
                Log.d(
                    "ble==> ",
                    "onCharacteristicChanged: " + convertByteToString(characteristic.value)
                )
                val data = characteristic.value
                if (data != null) {
                    when (characteristic.uuid.toString().lowercase(Locale.getDefault())) {
                        AppConstants.spp_char_uuid -> {
                            var response = convertByteToString(data)
                            if (response.split(";").isNotEmpty()) {
                                response = response.split(";")[0]
                            }
                            Log.d("ble==> ", "onCharacteristicChanged_response : $response")
                            broadcastUpdate(
                                AppConstants.ACTION_CHARACTERISTIC_CHANGED,
                                response
                            )
                        }
                    }
                }
            }

            @Deprecated("Deprecated in Java")
            override fun onDescriptorRead(
                gatt: BluetoothGatt,
                descriptor: BluetoothGattDescriptor,
                status: Int
            ) {
                super.onDescriptorRead(gatt, descriptor, status)
            }

            override fun onDescriptorWrite(
                gatt: BluetoothGatt,
                descriptor: BluetoothGattDescriptor,
                status: Int
            ) {
                super.onDescriptorWrite(gatt, descriptor, status)
                if (status == BluetoothGatt.GATT_SUCCESS) {
                    Log.d("ble==> ", "onDescriptorWrite: success " + descriptor.characteristic.uuid.toString() + "   value: " + descriptor.value[0])
                    broadcastUpdate(AppConstants.ACTION_DEVICE_CONNECTED)
                } else {
                    setAlertDialog(mContext, mContext.getString(R.string.Enablenotifyfail))
                    Log.e("ble==> ", "onDescriptorWrite: false status: $status")
                }
            }

            override fun onMtuChanged(gatt: BluetoothGatt, mtu: Int, status: Int) {
                super.onMtuChanged(gatt, mtu, status)
                enableNotifychar(mContext)
                if (status == BluetoothGatt.GATT_SUCCESS) {
                    MTU = mtu - 3
                    Log.e("ble==> ", "onMtuChanged: $mtu")
                }
            }
        }

        fun stopThread() {
            if (thread != null) {
                val tempThread = thread
                thread = null
                tempThread?.interrupt()
            }
        }

        fun broadcastUpdate(action: String) {
            val intent = Intent(action)
            Log.d("ble==> ", "broadcastUpdate: $action")
            mContext.sendBroadcast(intent)
        }

        private fun broadcastUpdate(action: String, data: String) {
            val intent = Intent(action)
            Log.d("ble==> ", "broadcastUpdate: $action STR data :$data")
            intent.putExtra("data", data)
            mContext.sendBroadcast(intent)
        }

        private fun broadcastUpdate(action: String, data: Int) {
            val intent = Intent(action)
            Log.d("ble==> ", "broadcastUpdate: $action INT data :$data")
            intent.putExtra("data", data)
            mContext.sendBroadcast(intent)
        }

        private fun broadcastUpdate(action: String, data: ByteArray) {
            val intent = Intent(action)
            Log.d("ble==> ", "broadcastUpdate: $action BYTE data :$data")
            intent.putExtra("data", data)
            mContext.sendBroadcast(intent)
        }

        private val SyncObj = Any()

        private fun WaitForSync() {
            try {
                Thread.sleep(40)
            } catch (e: InterruptedException) {
                e.printStackTrace()
            }
        }
    }
}
