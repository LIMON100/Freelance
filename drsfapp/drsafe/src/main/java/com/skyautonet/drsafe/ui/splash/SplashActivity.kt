package com.skyautonet.drsafe.ui.splash

import android.annotation.SuppressLint
import android.content.Context
import android.content.DialogInterface
import android.content.Intent
import android.content.pm.PackageManager
import android.location.LocationManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.RequiresApi
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.databinding.DataBindingUtil
import com.skyautonet.drsafe.ui.MainActivity
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.ActivitySplashBinding
import com.skyautonet.drsafe.service.DrSafeBleService
import com.skyautonet.drsafe.service.ServiceConnector
import com.skyautonet.drsafe.ui.StartupActivity
import com.skyautonet.drsafe.ui.search.SearchDeviceActivity
import com.skyautonet.drsafe.ui.termsandcondition.TermsAndConditionActivity
import com.skyautonet.drsafe.util.AppConstants
import com.skyautonet.drsafe.util.CheckSelfPermission
import com.skyautonet.drsafe.util.SharedPreferenceUtil
import com.skyautonet.drsafe.util.start

@SuppressLint("CustomSplashScreen")
class SplashActivity : AppCompatActivity() {

    private lateinit var binding : ActivitySplashBinding

    private val sharedPreferenceUtil by lazy {
        SharedPreferenceUtil.getInstance(this)
    }

    private var isFirstPermission = true

    private val locationResult = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
        if(isGpsEnabled()) {
            gotoLoginActivity()
        } else {
            finish()
        }
    }

    private var bleService : DrSafeBleService? = null

    private val serviceConnector = ServiceConnector(
        { s ->
            bleService = s
            gotoLoginActivity()
        },
        { msg ->
            Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
        }
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = DataBindingUtil.setContentView(this, R.layout.activity_splash)
        startService(Intent(this,DrSafeBleService::class.java))
    }

    override fun onResume() {
        super.onResume()
        checkSelfPermission()
    }

    @SuppressLint("NewApi")
    private fun checkSelfPermission() {
        if (isFirstPermission) {
            var accepted = CheckSelfPermission.checkNotificationPermission(this)

            if (accepted) {
                accepted = CheckSelfPermission.checkLocationBluetoothPermission(this)
            }

            if (accepted) {
                accepted = CheckSelfPermission.checkBackgroundLocation(this)
            }

            if (accepted) {
                accepted = isLocationOn()
            }

            if (accepted) {
                bindService(Intent(this, DrSafeBleService::class.java), serviceConnector,Context.BIND_AUTO_CREATE)
            }

            isFirstPermission = false
        }
    }

    @RequiresApi(Build.VERSION_CODES.Q)
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String>,
        grantResults: IntArray
    ) {
        if (requestCode == AppConstants.REQUEST_LOCATION_PERMISSION) {
            val bluetoothGranted = grantResults.indexOf(0) == PackageManager.PERMISSION_GRANTED
            val locationGranted = grantResults.indexOf(1) == PackageManager.PERMISSION_GRANTED

            if (!bluetoothGranted && !locationGranted) {
                AlertDialog.Builder(this)
                    .setTitle(getString(R.string.permission_required))
                    .setMessage(getString(R.string.bluetooth_location_permission_required))
                    .setPositiveButton(getString(android.R.string.ok)) { dialogInterface: DialogInterface, _ ->
                        dialogInterface.dismiss()
                        finish()
                    }
                    .show()
            } else {
                if (CheckSelfPermission.checkBackgroundLocation(this)) {
                    val accepted = isLocationOn()
                    if (accepted) {
                        gotoLoginActivity()
                    }
                }
            }
        }



        if (requestCode == AppConstants.REQUEST_NOTIFICATION_PERMISSION) {
            if (grantResults[0] != PackageManager.PERMISSION_GRANTED) {
                AlertDialog.Builder(this)
                    .setTitle(getString(R.string.permission_required))
                    .setMessage(getString(R.string.notification_permission_required))
                    .setPositiveButton(getString(R.string.ok)) { dialog, _ ->
                        dialog.dismiss()
                        finish()
                    }
                    .show()
            } else {
                CheckSelfPermission.checkLocationBluetoothPermission(this)
            }
        }

        if (requestCode == AppConstants.REQUEST_BG_LOCATION_PERMISSION) {
            if (grantResults[0] != PackageManager.PERMISSION_GRANTED) {
                showBackgroundLocationRequiredDialog()
            } else {
                val accepted = isLocationOn()
                if (accepted) {
                    gotoLoginActivity()
                }
            }
        } else {

        }

        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
    }

    private fun showBackgroundLocationRequiredDialog() {
        AlertDialog.Builder(this)
            .setTitle(getString(R.string.permission_required))
            .setMessage(getString(R.string.background_location_rationale))
            .setPositiveButton(getString(R.string.ok)) { dialog, _ ->
                dialog.dismiss()
                openPermissionsSettings()
            }
            .setNegativeButton(getString(R.string.cancel)) { dialog, _ ->
                dialog.dismiss()
                finish()
            }
            .show()
    }

    private fun gotoLoginActivity() {
        binding.progress.progress = 100
        val isConnected = bleService?.isConnected() == true
        val isLoggedIn = sharedPreferenceUtil.getString(AppConstants.TOKEN) != null
        if (isLoggedIn && isConnected) {
            start(MainActivity::class.java)
        } else {
            start(SearchDeviceActivity::class.java)
        }
    }

    private fun isLocationOn(): Boolean {
        val gpsEnabled = isGpsEnabled()

        if (!gpsEnabled) {
            android.app.AlertDialog.Builder(this)
                .setMessage(R.string.enable_location_setting)
                .setPositiveButton(R.string.ok) { dialog, _ ->
                    locationResult.launch(Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS))
                    dialog.dismiss()
                }
                .setNegativeButton(R.string.cancel, null)
                .setCancelable(false)
                .show()

            return false
        }
        return true
    }

    private fun isGpsEnabled() : Boolean {
        val lm = getSystemService(Context.LOCATION_SERVICE) as LocationManager
        try {
            return lm.isProviderEnabled(LocationManager.GPS_PROVIDER)
        } catch (ex: Exception) {
            ex.printStackTrace()
            return false
        }
    }

    private fun openPermissionsSettings() {
        try {
            val intent = Intent()
            intent.setAction(Settings.ACTION_APPLICATION_DETAILS_SETTINGS)
            intent.addCategory(Intent.CATEGORY_DEFAULT)
            intent.setData(Uri.parse("package:$packageName"))
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY)
            intent.addFlags(Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS)
            startActivity(intent)
            isFirstPermission = true
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
}