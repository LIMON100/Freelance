package com.skyautonet.drsafe.ui.search

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
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.res.ResourcesCompat
import androidx.databinding.DataBindingUtil
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.ActivitySearchDeviceBinding
import com.skyautonet.drsafe.model.ScanModel
import com.skyautonet.drsafe.service.DrSafeBleService
import com.skyautonet.drsafe.service.ServiceConnector
import com.skyautonet.drsafe.ui.BaseActivity
import com.skyautonet.drsafe.ui.MainActivity
import com.skyautonet.drsafe.ui.StartupActivity
import com.skyautonet.drsafe.util.AppConstants
import com.skyautonet.drsafe.util.CheckSelfPermission
import com.skyautonet.drsafe.util.SharedPreferenceUtil
import com.skyautonet.drsafe.util.start
import java.util.Timer
import java.util.TimerTask

/**
 * Created by Hussain on 05/05/25.
 */
class SearchDeviceActivity : BaseActivity() {

    private lateinit var binding : ActivitySearchDeviceBinding

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
    private lateinit var sharedPrefManager : SharedPreferenceUtil

    private var bleService : DrSafeBleService? = null

    private val serviceConnector = ServiceConnector(
        { service ->
            this.bleService = service
        },{
            msg -> Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
        }
    )

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(this)
    }

    private var timer : Timer? = null
    private val BLE_CONNECTION_TIMEOUT = 10000L

    private val SCAN_PERIOD = 30000L

    private lateinit var searchDeviceAdapter: SearchDeviceAdapter

    private val scanList = mutableListOf<ScanModel>()

    override fun onCreate(savedInstanceState: Bundle?) {
        window.statusBarColor = getWhiteStatusBarColor()
        super.onCreate(savedInstanceState)
        binding = DataBindingUtil.setContentView(this, R.layout.activity_search_device)
        localBroadcastManager.sendBroadcast(Intent(DrSafeBleService.ACTION_STOP_BG_SCAN))
        val serviceIntent = Intent(this, DrSafeBleService::class.java).apply {
            action = AppConstants.START_FOREGROUND
        }
        startService(serviceIntent)
        bindService(serviceIntent, serviceConnector, Context.BIND_AUTO_CREATE)
        sharedPrefManager = SharedPreferenceUtil.getInstance(this)
        setupToolbar()

        searchDeviceAdapter = SearchDeviceAdapter { device ->
            bleService?.disconnect()
            val macAddress = device.bluetoothDevice.address
//            SharedPref.setValue(this, SharedPref.DEVICE_MAC_ADDRESS_PREFS, macAddress)
//            SharedPref.setValue(this, SharedPref.DEVICE_NAME_PREFS, device.name)
            bleService?.connectGatt(device.bluetoothDevice)
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

        binding.rvDeviceList.adapter = searchDeviceAdapter

        binding.refresh.setOnRefreshListener {
            binding.refresh.isRefreshing = false
            stopScan()
            startScan()
        }
    }

    override fun onResume() {
        super.onResume()
        checkSelfPermissionRetional()
    }

    private fun checkSelfPermissionRetional() {
        if (CheckSelfPermission.checkLocationBluetoothPermissionRational(this@SearchDeviceActivity)) {
            if (CheckSelfPermission.isBluetoothOn(this@SearchDeviceActivity)) {
                if (CheckSelfPermission.isLocationOn(this@SearchDeviceActivity)) {
                    try {
                        startScan()
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                }
            }
        }
    }


    private fun setupToolbar() {
        binding.toolbar.apply {
            if (showToolbarImage()) {
                imgToolbar.visibility = View.VISIBLE
            } else {
                imgToolbar.visibility = View.INVISIBLE
            }

            toolbarTitle.text = getString(R.string.devices)

            imgToolbar.setOnClickListener {
                finish()
            }

            if (showHeaderLogo()) {
                imgHeaderLogo.visibility = View.VISIBLE
            } else {
                imgHeaderLogo.visibility = View.INVISIBLE
            }
        }
    }

    fun showToolbarImage() : Boolean = true

    private fun getWhiteStatusBarColor() = ResourcesCompat.getColor(resources, R.color.white,null)

    private fun showHeaderLogo() = false

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

    override fun onDeviceDisconnected() {
        super.onDeviceDisconnected()
        hideProgressDialog()
        timer?.cancel()
    }

    override fun onDeviceConnected() {
        super.onDeviceConnected()
        timer?.cancel()
        hideProgressDialog()
        isFirstPermission = false
        stopScan()
        if (sharedPrefManager.getString(AppConstants.TOKEN) != null) {
            start(MainActivity::class.java)
        } else {
            start(StartupActivity::class.java)
        }
    }


    @SuppressLint("MissingPermission")
    private fun startScan() {
        val scanSettingsBuilder = ScanSettings.Builder()
        scanSettingsBuilder.setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
        Handler(Looper.getMainLooper()).postDelayed({ stopScan() }, SCAN_PERIOD)
        btScanner.startScan(null, scanSettingsBuilder.build(), bleScanCallback)
        btScanner.flushPendingScanResults(bleScanCallback)
    }


    @SuppressLint("MissingPermission")
    private fun stopScan() {
        if (btAdapter.isEnabled) {
            btScanner.stopScan(bleScanCallback)
        }
    }


    private val bleScanCallback: ScanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            super.onScanResult(callbackType, result)
            val scannedDevice = result.device
            if (scannedDevice != null) {
                result.scanRecord?.let { scanRecord ->
                    if (scanRecord.deviceName != null) {
                        if (scanRecord.deviceName?.contains(AppConstants.dtg_lite) == true ) {
                            scanList.clear()
                            scanList.add(
                                ScanModel(
                                    scanRecord.deviceName ?: "Unknown",
                                    scannedDevice.address,
                                    scannedDevice,
                                    result.rssi
                                )
                            )
                            searchDeviceAdapter.addDevices(scanList)
                            stopScan()
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

    override fun onDestroy() {
        super.onDestroy()
        if (bleService?.isConnected() == false) {
            localBroadcastManager.sendBroadcast(Intent(DrSafeBleService.ACTION_START_BG_SCAN))
        }
        stopScan()
    }
}