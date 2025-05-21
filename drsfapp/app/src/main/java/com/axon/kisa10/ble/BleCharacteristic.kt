package com.axon.kisa10.ble

import android.annotation.SuppressLint
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattDescriptor
import android.content.Context
import android.util.Log
import android.widget.Toast
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AppMethods.convertStringToByte
import com.axon.kisa10.util.AppMethods.hideProgressDialog
import com.axon.kisa10.util.AppMethods.setAlertDialog
import com.axon.kisa10.util.AppMethods.showProgressDialog
import com.axon.kisa10.util.CheckSelfPermission.isBluetoothOn
import com.axon.kisa10.R
import java.util.UUID

@SuppressLint("MissingPermission")
object BleCharacteristic {

    /*  public static boolean WriteCharacteristic(Context context, String byteString) {
        BluetoothGatt mBluetoothGatt = BleDeviceActor.getmBluetoothGatt();
        if (!canReadWrite(context, mBluetoothGatt)) {
            return false;
        }
        byte[] byteValue = AppConstants.convertStringToByte(byteString);
        BluetoothGattCharacteristic characteristic = mBluetoothGatt
                .getService(UUID.fromString(AppConstants.UART_SERVICE_UUID))
                .getCharacteristic(UUID.fromString(AppConstants.RX_RIGHT_UUID));

        if (characteristic != null) {
            characteristic.setValue(byteValue);
            boolean isWrite = mBluetoothGatt.writeCharacteristic(characteristic);
            Log.d("ble==> ", "writeCharacteristic writeWifiName: " + isWrite);
            return isWrite;
        }
        return false;
    }

    public static int crcShort=0;
    public static byte[] sendPacket(Context context, String byteString) {
        BluetoothGatt mBluetoothGatt = BleDeviceActor.getmBluetoothGatt();
        if (canReadWrite(context, mBluetoothGatt)) {
            byte[] data = AppConstants.convertStringToByte(byteString);
            ByteArrayOutputStream output = new ByteArrayOutputStream();
            try {
                output.write(0x15);
                output.write(data);
            } catch (IOException e) {
                e.printStackTrace();
            }
            data = output.toByteArray();
            Log.d("ble==>","byte array"+ Arrays.toString(data));
            int mMaxPacketLen = 512;
            if (data.length == 0 || data.length > mMaxPacketLen) {
                return new byte[0];
            }

            ByteArrayOutputStream to_send = new ByteArrayOutputStream();
            int len_tot = data.length;

            if (len_tot <= 255) {
                to_send.write((char) 2);
                to_send.write((char) len_tot);
            } else if (len_tot <= 65535) {
                to_send.write((char) 3);
                to_send.write((char) (len_tot >> 8));
                to_send.write((char) (len_tot & 0xFF));
            } else {
                to_send.write((char) 4);
                to_send.write((char) ((len_tot >> 16) & 0xFF));
                to_send.write((char) ((len_tot >> 8) & 0xFF));
                to_send.write((char) (len_tot & 0xFF));
            }

            int crc = AppConstants.crc16(data,len_tot);
            crcShort = crc;
            Log.d("ble==>","CRC"+ crc);

            try {
                to_send.write(data);
            } catch (IOException e) {
                e.printStackTrace();
            }
            to_send.write((char) (crc >> 8));
            to_send.write((char) (crc & 0xFF));
            to_send.write((char) 3);

            Log.d("ble==>","Converted byte array"+ Arrays.toString(to_send.toByteArray()));
            return to_send.toByteArray();
        } else {
            return null;
        }
    }


    public static void disableNotifyDataSyncService(Context context) {
        BluetoothGatt mBluetoothGatt = BleDeviceActor.getmBluetoothGatt();
        if (!canReadWrite(context, mBluetoothGatt)) {
            return;
        }
        BluetoothGattCharacteristic characteristic = mBluetoothGatt
                .getService(UUID.fromString(AppConstants.UART_SERVICE_UUID))
                .getCharacteristic(UUID.fromString(AppConstants.TX_READ_UUID));

        BluetoothGattDescriptor descriptor =
                characteristic.getDescriptor(UUID.fromString(AppConstants.DES_UUID));
        descriptor.setValue(BluetoothGattDescriptor.DISABLE_NOTIFICATION_VALUE);
        mBluetoothGatt.writeDescriptor(descriptor);
        mBluetoothGatt.setCharacteristicNotification(characteristic, true);
    }

    public static void readWifiChar(Context context, String characteristicUuid) {
        BluetoothGatt mBluetoothGatt = BleDeviceActor.getmBluetoothGatt();
        if (!canReadWrite(context, mBluetoothGatt)) {
            return;
        }
        BluetoothGattCharacteristic characteristic = mBluetoothGatt
                .getService(UUID.fromString(AppConstants.UART_SERVICE_UUID))
                .getCharacteristic(UUID.fromString(characteristicUuid));
        if (characteristic != null) {
            boolean isRead = mBluetoothGatt.readCharacteristic(characteristic);
            Log.d("ble==>", "readCharacteristic readWifiChar: " + characteristicUuid + "==>" + isRead);
        }
    }*/
    fun readDeviceData(context: Context, command: String) {
        val data = AppConstants.COMMAND_REQ + "," + command + ";"
        Log.d("result==>", "write command: $data")
        writeCharacteristic(context, data)
    }

    fun writeDeviceData(context: Context, command: String, value: Int) {
        showProgressDialog(context)
        val data = AppConstants.COMMAND_REQ + "," + command + "," + value + ";"
        Log.d("result==>", "write command: $data")
        writeCharacteristic(context, data)
    }

    fun writeBTOData(context: Context, command: String, value1: Int, value2: Int, value3: Int) {
        showProgressDialog(context)
        val data =
            AppConstants.COMMAND_REQ + "," + command + "," + value1 + "," + value2 + "," + value3 + ";"
        Log.d("result==>", "write command: $data")
        writeCharacteristic(context, data)
    }

    fun writeStringData(context: Context, command: String, value: String) {
        if ((command !== AppConstants.COMMAND_DATABASE_START_UPDATE) && (command !== AppConstants.COMMAND_MAP_DATABASE_REMOVE)) showProgressDialog(
            context
        )
        val data = AppConstants.COMMAND_REQ + "," + command + "," + value + ";"
        Log.d("result==>", "write command: $data")
        writeCharacteristic(context, data)
    }

    fun writeCharacteristic(context: Context, byteString: String?): Boolean {
        val mBluetoothGatt = BleDeviceActor.getmBluetoothGatt()
        if (!canReadWrite(context, mBluetoothGatt)) {
            hideProgressDialog(context)
            return false
        }
        val byteValue = convertStringToByte(byteString!!)
        val service = mBluetoothGatt?.getService(UUID.fromString(AppConstants.spp_service_uuid))
        if (service != null) {
            val characteristic =
                service.getCharacteristic(UUID.fromString(AppConstants.spp_char_uuid))
            if (characteristic != null) {
                characteristic.setValue(byteValue)
                val isWrite = mBluetoothGatt.writeCharacteristic(characteristic)
                Log.d("ble==> ", "writeCharacteristic: $isWrite")
                return isWrite
            } else {
                setAlertDialog(context, context.getString(R.string.spp_warning))
            }
        } else {
            Toast.makeText(context, context.getString(R.string.spp_warning2), Toast.LENGTH_SHORT)
                .show()
            refreshServices()
        }
        return false
    }

    private fun refreshServices() {
        val bluetoothGatt = BleDeviceActor.getmBluetoothGatt()
        if (bluetoothGatt != null && bluetoothGatt.device != null) {
            refreshDeviceCache(bluetoothGatt)
            bluetoothGatt.discoverServices()
        }
    }

    private fun refreshDeviceCache(bluetoothGatt: BluetoothGatt): Boolean {
        try {
            Log.d("refreshDevice", "Called")
            val localMethod = bluetoothGatt.javaClass.getMethod("refresh")
            val bool = (localMethod.invoke(bluetoothGatt, *arrayOfNulls(0)) as Boolean)
            Log.d("refreshDevice", "bool: $bool")
            return bool
        } catch (localException: Exception) {
            Log.e("refreshDevice", "An exception occured while refreshing device")
        }
        return false
    }

    @JvmStatic
    fun enableNotifychar(context: Context) {
        val mBluetoothGatt = BleDeviceActor.getmBluetoothGatt()
        if (!canReadWrite(context, mBluetoothGatt)) {
            return
        }
        val service = mBluetoothGatt?.getService(UUID.fromString(AppConstants.spp_service_uuid))
        if (service != null) {
            val characteristic =
                service.getCharacteristic(UUID.fromString(AppConstants.spp_char_uuid))
            if (characteristic != null) {
                val descriptor = characteristic.getDescriptor(UUID.fromString(AppConstants.desc_uuid))
                descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)
                val isWrite = mBluetoothGatt.writeDescriptor(descriptor)
                mBluetoothGatt.setCharacteristicNotification(characteristic, true)
                Log.d("ble==> ", "enableNotifyWifiStatusLevel: $isWrite")
            } else {
                setAlertDialog(context, context.getString(R.string.sppcharnotfound))
            }
        } else {
            Toast.makeText(
                context,
                "spp service not found, refreshing device cache",
                Toast.LENGTH_SHORT
            ).show()
            refreshServices()
        }
    }

    fun canReadWrite(context: Context, mBluetoothGatt: BluetoothGatt?): Boolean {
        if (!isBluetoothOn(context)) {
            Toast.makeText(
                context,
                context.getString(R.string.enable_bluetooth),
                Toast.LENGTH_SHORT
            ).show()
            return false
        } else if (!BleDeviceActor.isIsConnected()) {
            Toast.makeText(
                context,
                context.getString(R.string.device_disconnected),
                Toast.LENGTH_SHORT
            ).show()
            return false
        } else if (mBluetoothGatt == null) {
            return false
        } else {
            return true
        }
    }
}
