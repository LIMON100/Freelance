package com.axon.kisa10.activity

import android.annotation.SuppressLint
import android.content.Context
import android.content.DialogInterface
import android.content.Intent
import android.content.pm.PackageManager
import android.location.LocationManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.provider.Settings
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.RequiresApi
import androidx.appcompat.app.AlertDialog
import androidx.work.Constraints
import androidx.work.ExistingPeriodicWorkPolicy
import androidx.work.NetworkType
import androidx.work.PeriodicWorkRequestBuilder
import androidx.work.WorkManager
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.ble.ServiceConnector
import com.axon.kisa10.distributor.LoginDistributorActivity
import com.axon.kisa10.distributor.MainDistributorActivity
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.CheckSelfPermission
import com.axon.kisa10.util.SharedPrefKeys
import com.axon.kisa10.util.SharedPrefManager
import com.axon.kisa10.workmanager.UpdateCheckWorker
import com.google.firebase.auth.FirebaseAuth
import com.kisa10.BuildConfig
import com.kisa10.R
import java.util.concurrent.TimeUnit

@SuppressLint("CustomSplashScreen")
class SplashActivity : BaseActivity() {
    private lateinit var sharedPrefManager: SharedPrefManager
    private var isFirstPermission = true
    private var bleService : AxonBLEService? = null
    private val serviceConnector = ServiceConnector(
        { s ->
            this.bleService = s
            if (BuildConfig.IS_DISTRIBUTOR) {
                // goto distributor login
                gotoDistributorLogin()
            } else {
                gotoLoginActivity()
            }
        },
        { msg ->
            Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
            if (BuildConfig.IS_DISTRIBUTOR) {
                // goto distributor login
                gotoDistributorLogin()
            } else {
                gotoLoginActivity()
            }
        }
    )

    private val locationResult = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
        if(isGpsEnabled()) {
            if (BuildConfig.IS_DISTRIBUTOR) {
                // goto distributor login
                gotoDistributorLogin()
            } else {
                gotoLoginActivity()
            }
        } else {
            finish()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_splash)
        startService(Intent(this,AxonBLEService::class.java))
        sharedPrefManager = SharedPrefManager.getInstance(this)
    }

    override fun onResume() {
        super.onResume()
        checkSelfPermission()
        scheduleUpdateWorker()
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
                bindService(Intent(this, AxonBLEService::class.java),serviceConnector,Context.BIND_AUTO_CREATE)
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
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == AppConstants.REQUEST_LOCATION_PERMISSION) {
            val bluetoothGranted = grantResults[0] == PackageManager.PERMISSION_GRANTED
            val locationGranted = grantResults[1] == PackageManager.PERMISSION_GRANTED

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
                        if (BuildConfig.IS_DISTRIBUTOR) {
                            // goto distributor login
                            gotoDistributorLogin()
                        } else {
                            gotoLoginActivity()
                        }
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
                    if (BuildConfig.IS_DISTRIBUTOR) {
                        // goto distributor login
                        gotoDistributorLogin()
                    } else {
                        gotoLoginActivity()
                    }
                }
            }
        } else {

        }
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

    private fun gotoDistributorLogin() {
        val termsAgreed = sharedPrefManager.getBool(SharedPrefKeys.TERMS_CONDITION_AGREE_KEY)
        if (!termsAgreed) {
            val intent1 = Intent(this, TermsAndConditionActivity::class.java)
            startActivity(intent1)
            finish()
        } else {
            // goto distributor login screen
            val isConnected = bleService?.isConnected() ?: false
            val isLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.DISTRIBUTOR_TOKEN).isNullOrBlank()
            val intent = if (!isLoggedIn){
                // goto to login
                Intent(this, LoginDistributorActivity::class.java)
            } else if (!isConnected){
                // goto search screen
                Intent(this, SearchDeviceActivity::class.java)
            } else {
                Intent(this, MainActivity::class.java)
            }
            startActivity(intent)
            finish()
        }
    }

    @SuppressLint("MissingPermission")
    private fun gotoLoginActivity() {
        val termsAgreed = sharedPrefManager.getBool(SharedPrefKeys.TERMS_CONDITION_AGREE_KEY)
        if (!termsAgreed) {
            val intent1 = Intent(this, TermsAndConditionActivity::class.java)
            startActivity(intent1)
            finish()
        } else {
            val isLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.KAKAO_TOKEN_KEY).isNullOrBlank() || FirebaseAuth.getInstance().currentUser != null
            val isRegistered = !sharedPrefManager.getString(SharedPrefKeys.TOKEN).isNullOrBlank()
            if (!isLoggedIn) {
                val intent = Intent(this, LoginActivity::class.java)
                startActivity(intent)
                finish()
            } else {
                val isConnected = bleService?.isConnected() ?: false
                val intent = if(isConnected && isRegistered) {
                    Intent(this, MainActivity::class.java)
                } else if (!isRegistered && isConnected){
                    Intent(this, RegisterUserActivity::class.java)
                } else {
                    Intent(this, SearchDeviceActivity::class.java)
                }
                startActivity(intent)
                finish()
            }
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

    override fun onDestroy() {
        super.onDestroy()
        try {
            unbindService(serviceConnector)
        } catch (e : Exception) {
            e.printStackTrace()
        }
    }

    private fun scheduleUpdateWorker() {
        val workManager = WorkManager.getInstance(this)
        val workRequest = PeriodicWorkRequestBuilder<UpdateCheckWorker>(1, TimeUnit.DAYS)
            .setConstraints(Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build())
            .build()
        workManager.enqueueUniquePeriodicWork("Update",ExistingPeriodicWorkPolicy.UPDATE, workRequest)
    }

}