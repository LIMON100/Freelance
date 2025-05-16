package com.skyautonet.drsafe.service

import android.annotation.SuppressLint
import android.app.Notification
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothGattService
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothStatusCodes
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.ServiceInfo
import android.os.Binder
import android.os.Build
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.util.Log
import androidx.core.app.NotificationCompat
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.LifecycleRegistry
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.ble.BleOperationType
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.ui.MainActivity
import com.skyautonet.drsafe.ui.search.SearchDeviceActivity
import com.skyautonet.drsafe.util.AppConstants
import com.skyautonet.drsafe.util.SharedPreferenceUtil
import com.skyautonet.drsafe.util.convertByteToString
import com.skyautonet.drsafe.util.convertStringToByte
import com.skyautonet.drsafe.util.isNotifiable
import java.lang.ref.WeakReference
import java.util.Locale
import java.util.Timer
import java.util.TimerTask
import java.util.UUID
import java.util.concurrent.ConcurrentLinkedQueue

/**
 * Created by Hussain on 05/05/25.
 */
class DrSafeBleService : Service(), LifecycleOwner {

    private val TAG = "DrSafeBleService"

    private val localBinder = LocalBinder()

    private var bluetoothGatt : BluetoothGatt? = null

    private val operationQueue = ConcurrentLinkedQueue<BleOperationType>()
    private var pendingOperation: BleOperationType? = null
    private val localBroadcastManager : LocalBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(this)
    }
    private var gattCharacteristic: BluetoothGattCharacteristic? = null
    private var deviceCharacteristic: BluetoothGattCharacteristic? = null
    private var subscribeList = mutableListOf<BluetoothGattCharacteristic>()
    private var gattService: BluetoothGattService? = null
    private var gattDescriptor: BluetoothGattDescriptor? = null
    private val MTU = 247
    private var isDeviceConnected: Boolean = false
    private val bluetoothManager by lazy {
        getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    }

    private val btScanner by lazy {
        bluetoothManager.adapter.bluetoothLeScanner
    }

    private val notificationManager by lazy {
        getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
    }
    private val serviceId = 111

    private val bleStateReceiver = BleStateChangeListener()

    private val lifecycleRegistry : LifecycleRegistry by lazy {
        LifecycleRegistry(this)
    }

    private val sharedPrefManager by lazy {
        SharedPreferenceUtil.getInstance(this)
    }

    // Ble stuff

    private val bleGattCallback = object : BluetoothGattCallback() {
        @SuppressLint("MissingPermission", "NotificationPermission")
        override fun onConnectionStateChange(gatt: BluetoothGatt?, status: Int, newState: Int) {
            pendingOperation = null
            if (status == BluetoothGatt.GATT_SUCCESS && gatt != null) {
                if (newState == BluetoothGatt.STATE_CONNECTED) {
                    bluetoothGatt = gatt
                    localBroadcastManager.sendBroadcast(Intent(AppConstants.ACTION_DEVICE_CONNECTED))
                    Log.i(TAG, "onConnectionStateChange: Connected ${gatt.device}")
                    enqueueOperation(BleOperationType.DiscoverServices(gatt))
                    isDeviceConnected = true
                    val notification = createNotification(getString(R.string.connected,gatt.device?.name ?: "AXON"))
                    notificationManager.notify(serviceId, notification)
                } else if (newState == BluetoothGatt.STATE_DISCONNECTED) {
                    Log.i(TAG, "onConnectionStateChange: Disconnected ${gatt.device}")
                    deviceDisconnected()
//                    stopMapRequestTimer()
                }
            } else if (status == 8) {
                Log.i(TAG, "onConnectionStateChange: $status $newState")
            } else {
                Log.i(TAG, "onConnectionStateChange: $status $newState")
            }
        }

        @SuppressLint("MissingPermission")
        override fun onServicesDiscovered(gatt: BluetoothGatt?, status: Int) {
            pendingOperation = null
            if (status == BluetoothGatt.GATT_SUCCESS && gatt != null) {
                Log.i(TAG, "onServicesDiscovered: Success")
                enqueueOperation(BleOperationType.RequestMtu(gatt))
            } else {
                Log.e(TAG, "onServicesDiscovered: failed with status $status")
            }
            super.onServicesDiscovered(gatt, status)
        }

        override fun onMtuChanged(gatt: BluetoothGatt?, mtu: Int, status: Int) {
            pendingOperation = null
            if (status == BluetoothGatt.GATT_SUCCESS && gatt != null) {
                Log.i(TAG, "onMtuChanged: Success $mtu")
                enableNotification(gatt)
            } else {
                Log.e(TAG, "onMtuChanged: failed with status $status")
            }
            super.onMtuChanged(gatt, mtu, status)
        }

        override fun onCharacteristicWrite(
            gatt: BluetoothGatt?,
            characteristic: BluetoothGattCharacteristic?,
            status: Int
        ) {
            pendingOperation = null
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.i(TAG, "onCharacteristicWrite: Success")
            } else {
                Log.i(TAG, "onCharacteristicWrite: Failure")
            }
            super.onCharacteristicWrite(gatt, characteristic, status)
        }

        override fun onDescriptorWrite(
            gatt: BluetoothGatt?,
            descriptor: BluetoothGattDescriptor?,
            status: Int
        ) {
            pendingOperation = null
            if (status == BluetoothGatt.GATT_SUCCESS) {
                if(subscribeList.isNotEmpty()) {
                    subscribeCharacteristics()
                } else {
                    sharedPrefManager.setString(AppConstants.LAST_CONNECTED_DEVICE, gatt?.device?.address)
                    localBroadcastManager.sendBroadcast(Intent(AppConstants.ACTION_DEVICE_CONNECTION_COMPLETE))
                }
                Log.i(TAG, "onDescriptorWrite: Success")
            } else {
                Log.i(TAG, "onDescriptorWrite: Failure")
            }
            super.onDescriptorWrite(gatt, descriptor, status)
        }

        @Deprecated("Deprecated in Java")
        override fun onCharacteristicChanged(
            gatt: BluetoothGatt?,
            characteristic: BluetoothGattCharacteristic?
        ) {
            pendingOperation = null
            val data = characteristic?.value
            if (data != null) {
                when (characteristic.uuid.toString().lowercase(Locale.getDefault())) {
                    AppConstants.spp_char_uuid -> {
                        Log.d(TAG, "onCharacteristicChanged: ${data.size} received")
                        var response = data.convertByteToString()
                        Log.d(TAG, "onCharacteristicChanged: $response")
                        val intent = Intent(AppConstants.ACTION_CHARACTERISTIC_CHANGED)
                        intent.putExtra("data",response)
                        intent.putExtra(AppConstants.RAW_DATA,data)
                        localBroadcastManager.sendBroadcast(intent)
                    }
                    AppConstants.device_char_uuid -> {
                        val response = data.convertByteToString()
                        Log.i(TAG, "onCharacteristicChanged: $response")
                        val intent = Intent(AppConstants.ACTION_CHARACTERISTIC_CHANGED)
                        intent.putExtra("data",response)
                        intent.putExtra(AppConstants.RAW_DATA,data)
                        localBroadcastManager.sendBroadcast(intent)
                        Log.d("device_status", "device_status : $response")
                    }
                }
            }
            super.onCharacteristicChanged(gatt, characteristic)
        }
    }

    @SuppressLint("MissingPermission")
    private fun enableNotification(gatt: BluetoothGatt) {
        gattService = gatt.getService(UUID.fromString(AppConstants.spp_service_uuid))
        gattCharacteristic = gattService?.getCharacteristic(UUID.fromString(AppConstants.spp_char_uuid))
        deviceCharacteristic = gattService?.getCharacteristic(UUID.fromString(AppConstants.device_char_uuid))
        gattDescriptor = deviceCharacteristic?.getDescriptor(UUID.fromString(AppConstants.desc_uuid))

        if (deviceCharacteristic != null) {
            subscribeList.add(deviceCharacteristic!!)
        }
        subscribeCharacteristics()
    }

    @SuppressLint("MissingPermission")
    fun subscribeCharacteristics() {
        if (subscribeList.isEmpty()) return
        val characteristic = subscribeList.removeFirst()
        var success = false
        if (characteristic.isNotifiable()) {
            success = enqueueOperation(BleOperationType.EnableCharacteristicNotification(bluetoothGatt!!,characteristic))
        }

        val descriptor = characteristic.getDescriptor(UUID.fromString(AppConstants.desc_uuid))
        if (descriptor != null && success) {
            enqueueOperation(BleOperationType.WriteDescriptor(bluetoothGatt!!,descriptor, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE))
        } else {
            Log.d(TAG, "subscribeCharacteristics: descriptor null can't subscribe to characteristics.")
            sharedPrefManager.setString(AppConstants.LAST_CONNECTED_DEVICE, bluetoothGatt?.device?.address)
            localBroadcastManager.sendBroadcast(Intent(AppConstants.ACTION_DEVICE_CONNECTION_COMPLETE))
        }
    }

    @SuppressLint("MissingPermission")
    fun connectGatt(bluetoothDevice: BluetoothDevice) {
        enqueueOperation(BleOperationType.Connect(bluetoothDevice,this))
        btScanner.stopScan(ScanCallback)
        scanTimer?.cancel()
    }

    fun writeCharacteristic(byteString: String): Boolean {
        if (bluetoothGatt == null) {
            return false
        }
        val byteValue = byteString.convertStringToByte() ?: return false
        if (gattCharacteristic != null) {
            enqueueOperation(BleOperationType.WriteCharacteristics(bluetoothGatt!!, gattCharacteristic!!,byteValue))
            return true
        } else {
            return false
        }
    }

    fun writeCharacteristic(byteArray: ByteArray, type : Int = BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE): Boolean {
        Log.d(TAG, "writeCharacteristic: ${byteArray.convertByteToString()}")
        if (bluetoothGatt == null) {
            return false
        }
        return if (gattCharacteristic != null) {
            enqueueOperation(BleOperationType.WriteCharacteristics(bluetoothGatt!!, gattCharacteristic!!,byteArray,type))
        } else {
            false
        }
    }

    fun disconnect() {
        if (pendingOperation != null) {
            pendingOperation = null
        }
        bluetoothGatt?.let { gatt ->
            enqueueOperation(BleOperationType.Disconnect(gatt))
        }
    }

    @Synchronized
    private fun enqueueOperation(operation: BleOperationType) : Boolean {
        operationQueue.add(operation)
        if (pendingOperation == null) {
            return doNextOperation()
        }
        return false
    }

    @SuppressLint("MissingPermission")
    @Synchronized
    private fun doNextOperation() : Boolean {
        if (pendingOperation != null) {
            Log.e(TAG, "doNextOperation() called when an operation is pending! Aborting.")
            return false
        }

        val operation = operationQueue.poll() ?: run {
            Log.v(TAG,"Operation queue empty, returning")
            return false
        }
        pendingOperation = operation

        when (operation) {
            is BleOperationType.Connect -> {
                operation.device.connectGatt(operation.context,false,bleGattCallback)
                return true
            }

            is BleOperationType.Disconnect -> {
                operation.bluetoothGatt.disconnect()
                return true
            }

            is BleOperationType.DiscoverServices -> {
                val success = operation.bluetoothGatt.discoverServices()
                Log.i(TAG, "BleOperationType->DiscoverServices: $success")
                return true
            }

            is BleOperationType.RequestMtu -> {
                operation.bluetoothGatt.requestMtu(MTU)
                return true
            }

            is BleOperationType.WriteCharacteristics -> {
                val success = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    operation.bluetoothGatt.writeCharacteristic(operation.characteristic, operation.data, operation.type) == BluetoothStatusCodes.SUCCESS
                } else {
                    operation.characteristic.setValue(operation.data)
                    operation.characteristic.writeType = operation.type
                    operation.bluetoothGatt.writeCharacteristic(operation.characteristic)
                }
                Log.i(TAG, "BleOperationType->WriteCharacteristics: $success")
                if (operation.type == BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE) {
                    pendingOperation = null
                }
                return success
            }
            is BleOperationType.WriteDescriptor -> {
                val isWrite = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    operation.bluetoothGatt.writeDescriptor(operation.descriptor, operation.payload)
                } else {
                    // Fall back to deprecated version of writeDescriptor for Android <13
                    operation.descriptor.setValue(operation.payload)
                    operation.bluetoothGatt.writeDescriptor(operation.descriptor)
                }
                Log.i(TAG, "BleOperationType->WriteDescriptor: $isWrite")
            }

            is BleOperationType.EnableCharacteristicNotification -> {
                val success = operation.bluetoothGatt.setCharacteristicNotification(operation.characteristic, true)
                Log.i(TAG, "BleOperationType->EnableCharacteristicNotification: $success")
                pendingOperation = null
                return true
            }

            is BleOperationType.DisableCharacteristicNotification -> {
                val success = operation.bluetoothGatt.setCharacteristicNotification(operation.characteristic, false)
                Log.i(TAG, "BleOperationType.DisableCharacteristicNotification: $success")
                pendingOperation = null
                return true
            }
        }
        return false
    }

    // End Ble Stuff

    // Service Methods

    private val serviceUUID = UUID.randomUUID().toString()

    override fun onCreate() {
        super.onCreate()
        Log.i(TAG, "Service Created -- $serviceUUID")
        lifecycleRegistry.currentState = Lifecycle.State.RESUMED
        setInstance(this)
        registerReceiver(bleStateReceiver, makeBleActionIntentFilter())
        val intentFilter = IntentFilter()
        intentFilter.addAction(ACTION_STOP_BG_SCAN)
        intentFilter.addAction(ACTION_START_BG_SCAN)
        localBroadcastManager.registerReceiver(scanReceiver, intentFilter)
    }

    @SuppressLint("MissingPermission")
    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when(intent?.action) {
            AppConstants.START_FOREGROUND -> {
                val title = if (isConnected()) getString(R.string.connected, bluetoothGatt?.device?.name ?: "AXON") else getString(R.string.disconnected)
                val notification = createNotification(title)
//                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
//                    startForeground(serviceId, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE or ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION)
//                } else {
                    startForeground(serviceId, notification)
//                }
            }
        }
        return super.onStartCommand(intent, flags, startId)
    }


    inner class LocalBinder : Binder() {
        fun getService() : WeakReference<DrSafeBleService> = WeakReference(this@DrSafeBleService)
    }

    override fun onBind(p0: Intent?): IBinder {
        return localBinder
    }

    @SuppressLint("MissingPermission")
    override fun onDestroy() {
        super.onDestroy()
        lifecycleRegistry.currentState = Lifecycle.State.DESTROYED
        bluetoothGatt?.close()
        setInstance(null)
        unregisterReceiver(bleStateReceiver)
    }

    // End Service Methods

    // Broadcast Receivers

    private inner class BleStateChangeListener : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            intent?.let {
                if (it.action == BluetoothDevice.ACTION_ACL_CONNECTED && bluetoothGatt != null) {
                    isDeviceConnected = true
                }

                if (it.action == BluetoothDevice.ACTION_ACL_DISCONNECTED) {
                    deviceDisconnected()
                }

                if (it.action == BluetoothAdapter.ACTION_STATE_CHANGED) {
                    val state = it.extras?.get(BluetoothAdapter.EXTRA_STATE)
                    if (state == BluetoothAdapter.STATE_DISCONNECTED || state == BluetoothAdapter.STATE_OFF) {
                        deviceDisconnected()
                    }
                    Log.d(TAG, "onReceive: $state")
                }
            }
        }

    }

    // End Broadcast Receivers

    override val lifecycle: Lifecycle
        get() = lifecycleRegistry

    fun isConnected(): Boolean {
        return isDeviceConnected
    }

    @SuppressLint("MissingPermission")
    fun forceClose() {
        bluetoothGatt?.close()
        bluetoothGatt = null
    }

    fun getMtuSize(): Int {
        return MTU - 3
    }

    fun flush() {
        if (pendingOperation != null) {
            doNextOperation()
        }
    }

    private fun createNotification(contentText : String) : Notification {
        val mainActivity = if (isConnected()) Intent(this, MainActivity::class.java) else Intent(this, SearchDeviceActivity::class.java)
        val broadcast = Intent(this, OnNotificationDismissReceiver::class.java)
        val pendingIntent = PendingIntent.getActivity(this, 1, mainActivity, PendingIntent.FLAG_IMMUTABLE)
        val deleteIntent = PendingIntent.getBroadcast(this, 2, broadcast, PendingIntent.FLAG_IMMUTABLE)
        return NotificationCompat.Builder(this, AppConstants.DR_SAFE_BLE_NOTIFICATION_CHANNEL)
            .setContentTitle(getString(R.string.app_name))
            .setContentText(contentText)
            .setOngoing(true)
            .setContentIntent(pendingIntent)
            .setDeleteIntent(deleteIntent)
            .setAutoCancel(false)
            .setShowWhen(false)
            .setNumber(0)
            .setSilent(true)
            .setBadgeIconType(NotificationCompat.BADGE_ICON_NONE)
            .setOnlyAlertOnce(true)
            .setSmallIcon(R.mipmap.ic_app_logo)
            .build()
    }

    @SuppressLint("MissingPermission", "NotificationPermission")
    fun restartNotification() {
        val notification = if (isConnected()) {
            createNotification(getString(R.string.connected, bluetoothGatt?.device?.name ?: "AXON"))
        } else {
            createNotification(getString(R.string.disconnected))
        }
        notificationManager.notify(serviceId,notification)
    }

    fun getBluetoothGatt(): BluetoothGatt? {
        return bluetoothGatt
    }



    @SuppressLint("MissingPermission", "NotificationPermission")
    private fun deviceDisconnected() {
        isDeviceConnected = false
        val disconnect = Intent(AppConstants.ACTION_DEVICE_DISCONNECTED)
        localBroadcastManager.sendBroadcast(disconnect)
        val notification = createNotification(getString(R.string.disconnected))
        notificationManager.notify(serviceId, notification)
//        stopMapRequestTimer()
        bluetoothGatt?.close()
        bluetoothGatt = null
        scheduleScanTimer()
    }

    companion object {
        private var DrSafeBleServiceInstance : DrSafeBleService? = null
        const val ACTION_STOP_BG_SCAN = "ACTION_STOP_BG_SCAN"
        const val ACTION_START_BG_SCAN = "ACTION_START_BG_SCAN"


        fun setInstance(axonBLEService: DrSafeBleService?) {
            DrSafeBleServiceInstance = axonBLEService
        }

        fun getInstance() : DrSafeBleService? = DrSafeBleServiceInstance


        fun makeBleActionIntentFilter() : IntentFilter {
            val intentFilter = IntentFilter()
            intentFilter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED)
            intentFilter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED)
            intentFilter.addAction(BluetoothDevice.ACTION_ACL_CONNECTED)
            return intentFilter
        }
    }

    private var scanTimer : Timer? = null

    private val SCAN_PERIOD = 30000L
    private val PERIOD_MS = 500L

    @SuppressLint("MissingPermission")
    private fun scanDevices() {
        Log.i(TAG, "scanDevices: Scanning Devices")
        val scanSettingsBuilder = ScanSettings.Builder()
        scanSettingsBuilder.setScanMode(ScanSettings.SCAN_MODE_BALANCED)
        btScanner.stopScan(ScanCallback)
        btScanner.startScan(null, scanSettingsBuilder.build(), ScanCallback)
        btScanner.flushPendingScanResults(ScanCallback)
        Handler(Looper.getMainLooper()).postDelayed( {
            btScanner.stopScan(ScanCallback)
            if (!isConnected()) {
                Log.i(TAG, "scanDevices: restarting timer")
                scheduleScanTimer()
            }
        }, SCAN_PERIOD)
    }

    @SuppressLint("MissingPermission")
    private val ScanCallback: ScanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult?) {
            super.onScanResult(callbackType, result)
            val deviceName = result?.device?.name
            val deviceMac = result?.device?.address
            val lastConnectedMac = sharedPrefManager.getString(AppConstants.LAST_CONNECTED_DEVICE)
            if (deviceName?.contains(AppConstants.dtg_lite) == true && lastConnectedMac == deviceMac) {
                Log.i(TAG, "onScanResult: found ${result.device?.name} device")
                connectGatt(result.device)
                stopScanning()
                return
            }
        }

        override fun onScanFailed(errorCode: Int) {
            super.onScanFailed(errorCode)
            Log.i(TAG, "onScanFailed: $errorCode")
        }
    }

    private fun scheduleScanTimer() {
        if (scanTimer != null) {
            scanTimer?.cancel()
        }
        Log.i(TAG, "scheduleScanTimer: Starting timer with duration 30s")
        scanTimer = Timer()
        scanTimer?.schedule(object : TimerTask() {
            override fun run() {
                if (!isConnected()) {
                    try {
                        val adapter = bluetoothManager.adapter
                        val device = adapter.getRemoteDevice(sharedPrefManager.getString(AppConstants.LAST_CONNECTED_DEVICE))
                        if (device != null) {
                            connectGatt(device)
                        } else {
                            scanDevices()
                        }
                    } catch (e : Exception) {
                        e.printStackTrace()
                        Log.e(TAG, e.localizedMessage, e)
                        stopScanning()
                        checkAutoPair()
                    }
                } else {
                    scanTimer?.cancel()
                }
            }
        }, SCAN_PERIOD)
    }

    private val scanReceiver = object : BroadcastReceiver() {
        @SuppressLint("MissingPermission")
        override fun onReceive(context: Context?, intent: Intent?) {
            intent?.let {
                if (it.action == ACTION_STOP_BG_SCAN) {
                    Log.i(TAG, "stop background scanning")
                    stopScanning()
                }

                if (it.action == ACTION_START_BG_SCAN) {
                    Log.i(TAG, "start background scanning")
                    checkAutoPair()
                }
            }
        }
    }

    @SuppressLint("MissingPermission")
    private fun stopScanning() {
        btScanner.stopScan(ScanCallback)
        scanTimer?.cancel()
        scanTimer = null
    }

    fun checkAutoPair() {
        if (isConnected()) {
            Log.i(TAG, "checkAutoPair: Already Connected")
            return
        } else {
            Log.i(TAG, "checkAutoPair: Scheduling Scan in background")
            scheduleScanTimer()
        }
    }
}