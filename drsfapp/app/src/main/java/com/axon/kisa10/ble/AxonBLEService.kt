package com.axon.kisa10.ble

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
import androidx.work.Constraints
import androidx.work.ExistingPeriodicWorkPolicy
import androidx.work.NetworkType
import androidx.work.PeriodicWorkRequestBuilder
import androidx.work.WorkManager
import com.axon.kisa10.activity.MainActivity
import com.axon.kisa10.activity.SearchDeviceActivity
import com.axon.kisa10.distributor.MainDistributorActivity
import com.axon.kisa10.model.LogSdiData
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AppMethods
import com.axon.kisa10.util.LogUtil
import com.axon.kisa10.util.SharedPrefManager
import com.axon.kisa10.util.isNotifiable
import com.axon.kisa10.workmanager.LogDownloadWorker
import com.axon.kisa10.BuildConfig
import com.axon.kisa10.R
import com.skt.tmap.engine.navigation.data.SDIInfo
import java.lang.ref.WeakReference
import java.util.Locale
import java.util.Timer
import java.util.TimerTask
import java.util.UUID
import java.util.concurrent.ConcurrentLinkedQueue
import java.util.concurrent.TimeUnit

/**
 * Created by Hussain on 01/07/24.
 */
class AxonBLEService : Service(), LifecycleOwner {

    private val TAG = "AxonBLEService"

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

    private val sdiDataHandler by lazy {
        SdiDataHandler.getInstance(this)
    }

    private val backgroundDrivingBroadcastReceiver = BackgroundDrivingBroadcastReceiver()

    private val lifecycleRegistry : LifecycleRegistry by lazy {
        LifecycleRegistry(this)
    }

    private val sharedPrefManager by lazy {
        SharedPrefManager.getInstance(this)
    }

    // Ble stuff

    private val bleGattCallback = object : BluetoothGattCallback() {
        @SuppressLint("MissingPermission")
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
                checkAutoPair()
            } else {
                Log.i(TAG, "onConnectionStateChange: $status $newState")
                checkAutoPair()
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
                    sharedPrefManager.setString(AppConstants.LAST_CONNECTED_DEVICE, gatt?.device?.address?.replace(":","") ?: "")
                    localBroadcastManager.sendBroadcast(Intent(AppConstants.ACTION_DEVICE_CONNECTION_COMPLETE))
                    if (!BuildConfig.IS_DISTRIBUTOR) {
                        scheduleLogDownloadWorker()
                    }
//                startMapRequestTimer()
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
                        var response = AppMethods.convertByteToString(data)
                        if (response.startsWith(AppConstants.COMMAND_RES)
                            && !response.split(",").contains(AppConstants.COMMAND_REQUEST_FILE)
                            && !response.split(",").contains(AppConstants.COMMAND_STORED_LOG_FILE_LIST)
                            && !response.split(",").contains(AppConstants.COMMAND_STORED_EVENT_FILE_LIST)) {
                            if (response.split(";").isNotEmpty()) {
                                response = response.split(";")[0]
                            }
                            if (response.isEmpty()) return
                        }
                        Log.d(TAG, "onCharacteristicChanged: $response")
                        val intent = Intent(AppConstants.ACTION_CHARACTERISTIC_CHANGED)
                        intent.putExtra("data",response)
                        intent.putExtra(AppConstants.RAW_DATA,data)
                        localBroadcastManager.sendBroadcast(intent)
                    }
                    AppConstants.device_char_uuid -> {
                        val response = AppMethods.convertByteToString(data)
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

//        override fun onCharacteristicChanged(
//            gatt: BluetoothGatt,
//            characteristic: BluetoothGattCharacteristic,
//            value: ByteArray
//        ) {
//            pendingOperation = null
//            val data = characteristic.value
//            if (data != null) {
//                when (characteristic.uuid.toString().lowercase(Locale.getDefault())) {
//                    AppConstants.spp_char_uuid -> {
//                        var response = AppMethods.convertByteToString(data)
//                        if (response.split(";").isNotEmpty()) {
//                            response = response.split(";")[0]
//                        }
//                        if (response.isEmpty()) return
//                        Log.d(TAG, "onCharacteristicChanged_response : ${AppMethods.convertByteToString(data)}")
//                        val intent = Intent(AppConstants.ACTION_CHARACTERISTIC_CHANGED)
//                        intent.putExtra("data",response)
//                        localBroadcastManager.sendBroadcast(intent)
//                    }
//                    AppConstants.device_char_uuid -> {
//                        val response = AppMethods.convertByteToString(data)
//                        Log.d("device_status", "device_status : $response")
//                    }
//                }
//            }
//            super.onCharacteristicChanged(gatt, characteristic, value)
//        }
    }

    @SuppressLint("MissingPermission")
    private fun enableNotification(gatt: BluetoothGatt) {
        gattService = gatt.getService(UUID.fromString(AppConstants.spp_service_uuid))
        gattCharacteristic = gattService?.getCharacteristic(UUID.fromString(AppConstants.spp_char_uuid))
        deviceCharacteristic = gattService?.getCharacteristic(UUID.fromString(AppConstants.device_char_uuid))
        gattDescriptor = deviceCharacteristic?.getDescriptor(UUID.fromString(AppConstants.desc_uuid))
//        if (gattCharacteristic != null) {
//            subscribeList.add(gattCharacteristic!!)
//        }
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
            sharedPrefManager.setString(AppConstants.LAST_CONNECTED_DEVICE, bluetoothGatt?.device?.address?.replace(":","") ?: "")
            localBroadcastManager.sendBroadcast(Intent(AppConstants.ACTION_DEVICE_CONNECTION_COMPLETE))
            if (!BuildConfig.IS_DISTRIBUTOR) {
                scheduleLogDownloadWorker()
            }
        }
    }

    @SuppressLint("MissingPermission")
    fun connectGatt(bluetoothDevice: BluetoothDevice) {
        enqueueOperation(BleOperationType.Connect(bluetoothDevice,this))
        scanTimer?.cancel()
        btScanner.stopScan(ScanCallback)
    }

    fun readDeviceData(command: String) {
        val data = AppConstants.COMMAND_REQ + "," + command + ";"
        Log.d(TAG, "write command: $data")
        writeCharacteristic(data.toByteArray(), type = BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)
    }

    fun writeDeviceData(command: String, value: Int) {
        val data = AppConstants.COMMAND_REQ + "," + command + "," + value + ";"
        Log.d(TAG, "write command: $data")
        writeCharacteristic(data)
    }

    fun writeBTOData(command: String, value1: Int, value2: Int, value3: Int) {
        val data = AppConstants.COMMAND_REQ + "," + command + "," + value1 + "," + value2 + "," + value3 + ";"
        Log.d(TAG, "write command: $data")
        writeCharacteristic(data)
    }

    fun writeStringData(command: String, value: String) {
        val data = AppConstants.COMMAND_REQ + "," + command + "," + value + ";"
        Log.d(TAG, "write command: $data")
        writeCharacteristic(data)
    }

    fun writeCharacteristic(byteString: String): Boolean {
        if (bluetoothGatt == null) {
            return false
        }
        val byteValue = AppMethods.convertStringToByte(byteString!!) ?: return false
        if (gattCharacteristic != null) {
            enqueueOperation(BleOperationType.WriteCharacteristics(bluetoothGatt!!, gattCharacteristic!!,byteValue))
            return true
        } else {
            return false
        }
    }

    fun writeCharacteristic(byteArray: ByteArray, type : Int = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT): Boolean {
        Log.d(TAG, "writeCharacteristic: ${AppMethods.convertByteToString(byteArray)}")
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

    override fun onCreate() {
        super.onCreate()
        Log.i(TAG, "Service Created")
        lifecycleRegistry.currentState = Lifecycle.State.RESUMED
        setInstance(this)
        registerReceiver(bleStateReceiver, makeBleActionIntentFilter())
        localBroadcastManager.registerReceiver(backgroundDrivingBroadcastReceiver, makeBackgroundDrivingIntentFilter())
        val intentFilter = IntentFilter()
        intentFilter.addAction(ACTION_STOP_BG_SCAN)
        localBroadcastManager.registerReceiver(scanReceiver, intentFilter)
        if (!isDeviceConnected) {
            scheduleScanTimer()
        }
    }

    @SuppressLint("MissingPermission")
    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when(intent?.action) {
            AppConstants.START_FOREGROUND -> {
                val title = if (isConnected()) getString(R.string.connected, bluetoothGatt?.device?.name ?: "AXON") else getString(R.string.disconnected)
                val notification = createNotification(title)
                startForeground(serviceId, notification)
            }
        }
        return super.onStartCommand(intent, flags, startId)
    }


    inner class LocalBinder : Binder() {
        fun getService() : WeakReference<AxonBLEService> = WeakReference(this@AxonBLEService)
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
        localBroadcastManager.unregisterReceiver(backgroundDrivingBroadcastReceiver)
    }

    // End Service Methods

    // Broadcast Receivers

    private inner class BackgroundDrivingBroadcastReceiver : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            intent?.let {
                if (it.action == ACTION_START_BACKGROUND_DRIVING) {
                    startSafeDrive()
                }

                if (it.action == ACTION_STOP_BACKGROUND_DRIVING) {
                    stopSafeDrive()
                }

                if (it.action == ACTION_SDI_DATA_AVAILABLE) {
                    val data : LogSdiData? = it.extras?.getSerializable(AppConstants.EXTRA_SETTING) as? LogSdiData
                    data?.let { setSdiDataForMapRequest(data.sdiInfo,data.isBto) }
                }
            }
        }

    }

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

    private fun stopSafeDrive() {
        lifecycleRegistry.currentState = Lifecycle.State.DESTROYED
        sdiDataHandler.stopDriving()
        sdiDataHandler.stopCalculatingAzimuth()
    }

    private fun startSafeDrive() {
        lifecycleRegistry.currentState = Lifecycle.State.RESUMED
        sdiDataHandler.startDriving(createDrivingNotification())
        sdiDataHandler.startCalculatingAzimuth()
        sdiDataHandler.addSdiObserver(this)
    }

    private fun createDrivingNotification(): Notification {
       return NotificationCompat.Builder(this, AppConstants.AXON_BLE_NOTIFICATION_CHANNEL)
            .setContentTitle(getString(R.string.app_name))
            .setContentText("TMap Background Service")
            .setOngoing(true)
            .setAutoCancel(false)
            .setOnlyAlertOnce(true)
            .setSmallIcon(R.mipmap.ic_drsafe_logo)
            .build()
    }

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
        val mainActivity = if (isConnected()) Intent(this, MainActivity::class.java) else Intent(this,SearchDeviceActivity::class.java)
        val broadcast = Intent(this, OnNotificationDismissReceiver::class.java)
        val pendingIntent = PendingIntent.getActivity(this, 1, mainActivity, PendingIntent.FLAG_IMMUTABLE)
        val deleteIntent = PendingIntent.getBroadcast(this, 2, broadcast, PendingIntent.FLAG_IMMUTABLE)
        return NotificationCompat.Builder(this, AppConstants.AXON_BLE_NOTIFICATION_CHANNEL)
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
            .setSmallIcon(R.mipmap.ic_drsafe_logo)
            .build()
    }
    @SuppressLint("MissingPermission")
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



    @SuppressLint("MissingPermission")
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
        cancelLogDownloadWorker()
    }

    companion object {
        private var AxonServiceInstance : AxonBLEService? = null
        const val ACTION_START_BACKGROUND_DRIVING = "ACTION_START_BACKGROUND_DRIVING"
        const val ACTION_STOP_BACKGROUND_DRIVING = "ACTION_STOP_BACKGROUND_DRIVING"
        const val ACTION_SDI_DATA_AVAILABLE = "ACTION_SDI_DATA_AVAILABLE"
        const val ACTION_STOP_BG_SCAN = "ACTION_STOP_BG_SCAN"

        fun makeBackgroundDrivingIntentFilter() : IntentFilter {
            val intentFilter = IntentFilter()
            intentFilter.addAction(ACTION_START_BACKGROUND_DRIVING)
            intentFilter.addAction(ACTION_STOP_BACKGROUND_DRIVING)
            intentFilter.addAction(ACTION_SDI_DATA_AVAILABLE)
            return intentFilter
        }

        fun setInstance(axonBLEService: AxonBLEService?) {
            AxonServiceInstance = axonBLEService
        }

        fun getInstance() : AxonBLEService? = AxonServiceInstance


        fun makeBleActionIntentFilter() : IntentFilter {
            val intentFilter = IntentFilter()
            intentFilter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED)
            intentFilter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED)
            intentFilter.addAction(BluetoothDevice.ACTION_ACL_CONNECTED)
            return intentFilter
        }
    }

    // Map Request

    private var timer : Timer? = null
    private var scanTimer : Timer? = null

    private val SCAN_PERIOD = 30000L
    private val PERIOD_MS = 500L

    private var sdiData : SDIInfo? = null

    private var isBto = 0

    private var lock = Any()

    fun startMapRequestTimer() {
        if(timer != null) {
            timer?.cancel()
        }
        timer = Timer()
        timer?.schedule(object : TimerTask() {
            override fun run() {
                sendMapRequestCommand()
            }
        },PERIOD_MS,PERIOD_MS)
    }


    fun stopMapRequestTimer() {
        timer?.cancel()
        timer?.purge()
        timer = null
        sdiData = null
    }

    private fun sendMapRequestCommand() {
        synchronized(lock) {
            if (isDeviceConnected && bluetoothGatt != null) {
                var command = AppConstants.COMMAND_REQ + "," + AppConstants.COMMAND_MAP_INFO + ","
                val date = (System.currentTimeMillis()/1000).toString(16)
                command += "$date,"
                if (sdiData != null) {
                    val speedLimit = sdiData!!.nSdiSpeedLimit
                    val schoolSpeedLimit = if(sdiData!!.bIsInSchoolZone) speedLimit else 0
                    val cameraSpeedLimit = if (isSpeedCamera(sdiData!!.nSdiType)) speedLimit else 0
                    val distanceToSdi = if(isSpeedCamera(sdiData!!.nSdiType)) sdiData!!.nSdiDist else 0
                    command += "${speedLimit.toString(16)},"
                    command += "${schoolSpeedLimit.toString(16)},"
                    command += "${cameraSpeedLimit.toString(16)},"
                    command += "${distanceToSdi.toString(16)},"
                    command += isBto.toString(16)
                } else {
                    command += "0,"
                    command += "0,"
                    command += "0,"
                    command += "0,"
                    command += "0"
                }
                command += ";"
                Log.i(TAG, command)
                sdiData = null
                val success = enqueueOperation(
                    BleOperationType.WriteCharacteristics(
                        bluetoothGatt!!,
                        gattCharacteristic!!,
                        command.toByteArray(),
                        BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
                    )
                )
                Log.d(TAG, "write success : $success")
            }
        }
    }

    fun setSdiDataForMapRequest(sdiInfo: SDIInfo, bto: Int) {
        synchronized(lock) {
            this.sdiData = sdiInfo
            this.isBto = 0
        }
    }

    private fun isSpeedCamera(sdiType : Int): Boolean {
        return when(sdiType) {
            0,1,2,3,4,5,6,7,8,9,10,11,12,16,17,75,76 -> true
            else -> false
        }
    }

    @SuppressLint("MissingPermission")
    private fun scanDevices() {
        Log.i(TAG, "scanDevices: Scanning Devices")
        val scanSettingsBuilder = ScanSettings.Builder()
        scanSettingsBuilder.setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
        btScanner.startScan(null, scanSettingsBuilder.build(), ScanCallback)
        Handler(Looper.getMainLooper()).postDelayed( {
            btScanner.stopScan(ScanCallback)
            if (!isConnected()) {
                scheduleScanTimer()
            }
        }, SCAN_PERIOD)
    }

    @SuppressLint("MissingPermission")
    private val ScanCallback: ScanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult?) {
            super.onScanResult(callbackType, result)
            if (result?.device?.name?.contains(AppConstants.axon_appname) == true
                || result?.device?.name?.contains(AppConstants.dtg_lite) == true) {
                Log.i(TAG, "onScanResult: found ${result.device?.name} device")
                connectGatt(result.device)
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
                    scanDevices()
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
            }
        }
    }

    private fun stopScanning() {
        btScanner.stopScan(ScanCallback)
        scanTimer?.cancel()
        scanTimer = null
    }


    private fun scheduleLogDownloadWorker() {
        val workManager = WorkManager.getInstance(this)
        val workRequest = PeriodicWorkRequestBuilder<LogDownloadWorker>(15, TimeUnit.MINUTES)
            .setConstraints(Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build())
            .build()
        workManager.enqueueUniquePeriodicWork("LogDownloadWorker", ExistingPeriodicWorkPolicy.UPDATE, workRequest)
    }

    private fun cancelLogDownloadWorker() {
        val workManager = WorkManager.getInstance(this)
        workManager.cancelUniqueWork("LogDownloadWorker")
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