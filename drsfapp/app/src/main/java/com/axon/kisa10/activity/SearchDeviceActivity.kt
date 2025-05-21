package com.axon.kisa10.activity

import android.annotation.SuppressLint
import android.app.ProgressDialog
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothManager
import android.bluetooth.le.BluetoothLeScanner
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.View
import android.widget.ImageView
import android.widget.Toast
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.axon.kisa10.adapter.SearchDeviceAdapter
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.ble.ServiceConnector
import com.axon.kisa10.distributor.LoginDistributorActivity
import com.axon.kisa10.model.ScanModel
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.CheckSelfPermission
import com.axon.kisa10.util.SharedPref
import com.axon.kisa10.util.SharedPrefKeys
import com.axon.kisa10.util.SharedPrefManager
import com.axon.kisa10.BuildConfig
import com.axon.kisa10.R
import java.util.Timer
import java.util.TimerTask

class SearchDeviceActivity : BaseActivity(), View.OnClickListener {
    private lateinit var rvDeviceList: RecyclerView
    private lateinit var ivRefresh: ImageView
    private lateinit var ivBack: ImageView
    private lateinit var searchDeviceAdapter: SearchDeviceAdapter
    private val scanList = ArrayList<ScanModel>()
    private val btManager: BluetoothManager by lazy {
        getSystemService(BLUETOOTH_SERVICE) as BluetoothManager
    }
    private val btAdapter: BluetoothAdapter by lazy {
        btManager.adapter
    }

    private val btScanner: BluetoothLeScanner by lazy {
        btAdapter.getBluetoothLeScanner()
    }

    private var isFirstPermission = true

    private var bluetoothDevice: BluetoothDevice? = null
    private lateinit var sharedPrefManager : SharedPrefManager
    private var bleService : AxonBLEService? = null
    private val serviceConnector = ServiceConnector(
        { service ->
            this.bleService = service
        },{
            msg -> Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
        })
    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(this)
    }
    private var timer : Timer? = null
    private val BLE_CONNECTION_TIMEOUT = 15000L
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_search_device)
        localBroadcastManager.sendBroadcast(Intent(AxonBLEService.ACTION_STOP_BG_SCAN))
        val serviceIntent = Intent(this, AxonBLEService::class.java).apply {
            action = AppConstants.START_FOREGROUND
        }
        startService(serviceIntent)
        bindService(serviceIntent, serviceConnector, Context.BIND_AUTO_CREATE)
        sharedPrefManager = SharedPrefManager.getInstance(this)
        initUi()
        localBroadcastManager.registerReceiver(mGattUpdateReceiver, AppConstants.makeIntentFilter())
    }

    @SuppressLint("MissingPermission", "NotifyDataSetChanged")
    override fun onResume() {
        super.onResume()
        if (isFirstPermission) {
            Log.d(
                "TEST",
                "isFirstPermission == true onResume --chechSelfPermission  "
            ) //this is a bug of Dexter
            chechSelfPermission()
        } else {
            Log.d("TEST", "isFirstPermission == false onResume BleDeviceActor.getmBluetoothGatt() " + bleService?.getBluetoothGatt()) //this is a bug of Dexter
            Log.d("OTA", "BleDeviceActor.getmBluetoothGatt() " + bleService?.getBluetoothGatt())
            if (bleService?.getBluetoothGatt() != null) {
                val bluetoothGatt = bleService?.getBluetoothGatt()
                if (bluetoothGatt?.device != null) {
                    bluetoothDevice = bluetoothGatt.device
                    bluetoothGatt.disconnect()
                    Handler(Looper.getMainLooper()).postDelayed({ bluetoothDevice?.fetchUuidsWithSdp() }, 7000)
                }
            }
            chechSelfPermissionRetional()
        }
        scanList.clear()
        searchDeviceAdapter.notifyDataSetChanged()
    }

    private fun initUi() {
        ivRefresh = findViewById(R.id.iv_refresh)
        ivBack = findViewById(R.id.iv_back)
        rvDeviceList = findViewById(R.id.rv_deviceList)
        ivRefresh.setOnClickListener(this)
        ivBack.setOnClickListener(this)
        searchDeviceAdapter = SearchDeviceAdapter(this@SearchDeviceActivity, scanList) { device ->
            bleService?.disconnect()
            val macAddress = device.address
            SharedPref.setValue(this, SharedPref.DEVICE_MAC_ADDRESS_PREFS, macAddress)
            SharedPref.setValue(this, SharedPref.DEVICE_NAME_PREFS, device.name)
            bleService?.connectGatt(device)
            openConnectDialog()
            timer = Timer()
            timer?.schedule(object : TimerTask() {
                override fun run() {
                    bleService?.forceClose()
                    runOnUiThread {
                        Toast.makeText(this@SearchDeviceActivity, getString(R.string.connection_timeout), Toast.LENGTH_SHORT).show()
                        hideProgressDialog()
                    }
                }
            }, BLE_CONNECTION_TIMEOUT)
        }
        rvDeviceList.setLayoutManager(LinearLayoutManager(this))
        rvDeviceList.setAdapter(searchDeviceAdapter)
    }

    override fun onClick(view: View) {
        when (view.id) {
            R.id.iv_refresh -> chechSelfPermissionRetional()
            R.id.iv_back -> finish()
        }
    }

    private fun chechSelfPermissionRetional() {
        if (CheckSelfPermission.checkLocationBluetoothPermissionRational(this@SearchDeviceActivity)) {
            if (CheckSelfPermission.isBluetoothOn(this@SearchDeviceActivity)) {
                if (CheckSelfPermission.isLocationOn(this@SearchDeviceActivity)) {
                    try {
                        for (model in scanList) {
//                            Log.d("result", " chechSelfPermissionRetional deviceName " +model.deviceName);
                        }
                        scanList.clear()
                        searchDeviceAdapter.notifyDataSetChanged()
                        startScan()
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                }
            }
        }
    }

    private fun chechSelfPermission() {
        if (CheckSelfPermission.checkLocationBluetoothPermission(this@SearchDeviceActivity)) {
            if (CheckSelfPermission.isBluetoothOn(this@SearchDeviceActivity)) {
                if (CheckSelfPermission.isLocationOn(this@SearchDeviceActivity)) {
                    try {
                        scanList.clear()
                        searchDeviceAdapter.notifyDataSetChanged()
                        startScan()
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                }
            }
        }
        isFirstPermission = false
    }

    @SuppressLint("MissingPermission")
    private fun startScan() {
        localBroadcastManager.sendBroadcast(Intent(AxonBLEService.ACTION_STOP_BG_SCAN))
        val scanSettingsBuilder = ScanSettings.Builder()
        scanSettingsBuilder.setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
        // Stops scanning after a pre-defined scan period.
        Handler(Looper.getMainLooper()).postDelayed({ btScanner.stopScan(ScanCallback) }, SCAN_PERIOD)
        btScanner.startScan(null, scanSettingsBuilder.build(), ScanCallback)
    }


    @SuppressLint("MissingPermission")
    private fun stopScan() {
        if (btAdapter.isEnabled) {
            btScanner.stopScan(ScanCallback)
        }
    }


    private val ScanCallback: ScanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            super.onScanResult(callbackType, result)
            val scannedDevice = result.device
            if (scannedDevice != null) {
                if (!isContainScanList(scannedDevice.address)) {
                    result.scanRecord?.let { scanRecord ->
                        if (scanRecord.deviceName != null) {
                            if (scanRecord.deviceName?.contains(AppConstants.axon_appname) == true ||
                                scanRecord.deviceName?.contains(AppConstants.dtg_lite) == true ) {
                                scanList.add(
                                    ScanModel(
                                        scanRecord.deviceName ?: "Unknown",
                                        scannedDevice.address,
                                        scannedDevice,
                                        result.rssi
                                    )
                                )
                                searchDeviceAdapter.notifyDataSetChanged()
                            }
                        }
                    }
                }
            }
        }

        override fun onBatchScanResults(results: List<ScanResult>) {
            for (sr in results) {
//                Log.d("result", " onBatchScanResults " +sr.toString());
            }
        }

        override fun onScanFailed(errorCode: Int) {
//            Log.d("result", "Discovery onScanFailed: " + errorCode);
            super.onScanFailed(errorCode)
        }
    }


    private fun isContainScanList(mac: String): Boolean {
        for (model in scanList) {
            if (model.deviceMacAddress == mac) {
                return true
            }
        }
        return false
    }

    private var pDialog: ProgressDialog? = null

    fun openConnectDialog() {
        pDialog = ProgressDialog(this)
        pDialog?.setMessage(getString(R.string.connecting))
        pDialog?.setCancelable(false)
        pDialog?.setCanceledOnTouchOutside(false)
        pDialog?.show()
    }

    private fun hideProgressDialog() {
        if (pDialog != null) {
            if (pDialog!!.isShowing) {
                pDialog!!.dismiss()
            }
        }
    }

    private val mGattUpdateReceiver: BroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val action = intent.action
            when (action) {
                AppConstants.ACTION_DEVICE_CONNECTION_COMPLETE -> {
                    timer?.cancel()
                    hideProgressDialog()
                    isFirstPermission = false
                    if (BuildConfig.IS_DISTRIBUTOR) {
                        val isLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.DISTRIBUTOR_TOKEN).isNullOrBlank()
                        val newIntent = if (!isLoggedIn) {
                            // goto login
                            Intent(this@SearchDeviceActivity, LoginDistributorActivity::class.java)
                        } else {
                            // goto calibration screen
                            Intent(this@SearchDeviceActivity, MainActivity::class.java)
                        }
                        startActivity(newIntent)
                        finish()
                    } else {
                        val termsAgreed = sharedPrefManager.getBool(SharedPrefKeys.TERMS_CONDITION_AGREE_KEY)
                        val isRegistered = !sharedPrefManager.getString(SharedPrefKeys.TOKEN).isNullOrBlank()
                        if (termsAgreed && !isRegistered) {
                            val intent1 = Intent(this@SearchDeviceActivity, RegisterUserActivity::class.java)
                            startActivity(intent1)
                            finish()
                        } else {
                            val intent1 = Intent(this@SearchDeviceActivity, MainActivity::class.java)
                            startActivity(intent1)
                            finish()
                        }
                    }

                }

                AppConstants.ACTION_DEVICE_DISCONNECTED -> {
                    hideProgressDialog()
                    timer?.cancel()
                }
            }
        }
    }

    override fun onDestroy() {
        try {
            stopScan()
            localBroadcastManager.unregisterReceiver(mGattUpdateReceiver)
            bleService?.checkAutoPair()
            unbindService(serviceConnector)
        } catch (e: Exception) {
            e.printStackTrace()
        }
        super.onDestroy()
    }

    override fun onPause() {
        try {
            stopScan()
        } catch (e: Exception) {
            e.printStackTrace()
        }
        super.onPause()
    }

    companion object {
        private const val SCAN_PERIOD: Long = 10000
        @SuppressLint("MissingPermission")
        fun setBluetooth(enable: Boolean): Boolean {
            val bluetoothAdapter = BluetoothAdapter.getDefaultAdapter()
            val isEnabled = bluetoothAdapter.isEnabled
            if (enable && !isEnabled) {
                return bluetoothAdapter.enable()
            } else if (!enable && isEnabled) {
                return bluetoothAdapter.disable()
            }
            // No need to change bluetooth state
            return true
        }
    }
}
