//package com.axon.kisa10.activity
//
//import android.annotation.SuppressLint
//import android.content.Context
//import android.content.DialogInterface
//import android.content.Intent
//import android.content.pm.PackageManager
//import android.location.LocationManager
//import android.net.Uri
//import android.os.Build
//import android.os.Bundle
//import android.provider.Settings
//import android.widget.Toast
//import androidx.activity.result.contract.ActivityResultContracts
//import androidx.annotation.RequiresApi
//import androidx.appcompat.app.AlertDialog
//import androidx.work.Constraints
//import androidx.work.ExistingPeriodicWorkPolicy
//import androidx.work.NetworkType
//import androidx.work.PeriodicWorkRequestBuilder
//import androidx.work.WorkManager
//import com.axon.kisa10.ble.AxonBLEService
//import com.axon.kisa10.ble.ServiceConnector
//import com.axon.kisa10.distributor.LoginDistributorActivity
//import com.axon.kisa10.distributor.MainDistributorActivity
//import com.axon.kisa10.util.AppConstants
//import com.axon.kisa10.util.CheckSelfPermission
//import com.axon.kisa10.util.SharedPrefKeys
//import com.axon.kisa10.util.SharedPrefManager
//import com.axon.kisa10.workmanager.UpdateCheckWorker
//import com.google.firebase.auth.FirebaseAuth
//import com.axon.kisa10.BuildConfig
//import com.axon.kisa10.R
//import java.util.concurrent.TimeUnit
//
//import com.axon.kisa10.databinding.ActivitySplashBinding
//import android.os.Handler // <-- Import Handler
//import android.os.Looper // <-- Import Looper
//import android.view.animation.LinearInterpolator
//import android.animation.ObjectAnimator
//
//@SuppressLint("CustomSplashScreen")
//class SplashActivity : BaseActivity() {
//    private lateinit var sharedPrefManager: SharedPrefManager
//    private var isFirstPermission = true
//    private var bleService : AxonBLEService? = null
//
//    private lateinit var binding : ActivitySplashBinding // Make sure this matches your layout file name
//    private val handler by lazy { Handler(Looper.getMainLooper()) } // Correct way to initialize Handler
//
//    private val serviceConnector = ServiceConnector(
//        { s ->
//            this.bleService = s
//            if (BuildConfig.IS_DISTRIBUTOR) {
//                // goto distributor login
//                gotoDistributorLogin()
//            } else {
//                gotoLoginActivity()
//            }
//        },
//        { msg ->
//            Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
//            if (BuildConfig.IS_DISTRIBUTOR) {
//                // goto distributor login
//                gotoDistributorLogin()
//            } else {
//                gotoLoginActivity()
//            }
//        }
//    )
//
//    private val locationResult = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
//        if(isGpsEnabled()) {
//            if (BuildConfig.IS_DISTRIBUTOR) {
//                // goto distributor login
//                gotoDistributorLogin()
//            } else {
//                gotoLoginActivity()
//            }
//        } else {
//            finish()
//        }
//    }
//
////    override fun onCreate(savedInstanceState: Bundle?) {
////        super.onCreate(savedInstanceState)
////        setContentView(R.layout.activity_splash)
////        startService(Intent(this,AxonBLEService::class.java))
////        sharedPrefManager = SharedPrefManager.getInstance(this)
////    }
//
//
//    override fun onCreate(savedInstanceState: Bundle?) {
//        super.onCreate(savedInstanceState)
//        binding = ActivitySplashBinding.inflate(layoutInflater)
//        setContentView(binding.root)
//
//        // Initialize sharedPrefManager HERE:
//        sharedPrefManager = SharedPrefManager.getInstance(this) // <--- FIX
//
//        // Example: Animate progress from 0 to 100 over 2 seconds
//        val progressBar = binding.progressIndicatorSplash
//        val animator = ObjectAnimator.ofInt(progressBar, "progress", 0, 100)
//        animator.duration = 2000
//        animator.interpolator = LinearInterpolator()
//        animator.start()
//
//        handler.postDelayed({
//            // Now sharedPrefManager is initialized and can be used safely
//            val termsAgreed = sharedPrefManager.getBool(SharedPrefKeys.TERMS_CONDITION_AGREE_KEY)
//            if (!termsAgreed) {
//                val intent1 = Intent(this, TermsAndConditionActivity::class.java)
//                startActivity(intent1)
//                finish()
//            } else {
//                // Check for Kakao token or Firebase Auth user for social login status
//                val isSociallyLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.KAKAO_TOKEN_KEY).isNullOrBlank() ||
//                        FirebaseAuth.getInstance().currentUser != null
//
//                // Check for your backend's registration token
//                val isRegisteredWithBackend = !sharedPrefManager.getString(SharedPrefKeys.TOKEN).isNullOrBlank()
//
//                if (BuildConfig.IS_DISTRIBUTOR) { // Check if it's the distributor build
//                    val isDistributorLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.DISTRIBUTOR_TOKEN).isNullOrBlank()
//                    if (!isDistributorLoggedIn) {
//                        val intent = Intent(this, LoginDistributorActivity::class.java)
//                        startActivity(intent)
//                        finish()
//                    } else {
//                        val isBleConnected = bleService?.isConnected() ?: false
//                        val intent = if (isBleConnected) {
//                            Intent(this, MainActivity::class.java)
//                        } else {
//                            Intent(this, SearchDeviceActivity::class.java)
//                        }
//                        startActivity(intent)
//                        finish()
//                    }
//                } else { // Regular End-User flow
//                    if (!isSociallyLoggedIn) {
//                        val intent = Intent(this, LoginActivity::class.java)
//                        startActivity(intent)
//                        finish()
//                    } else {
//                        if (!isRegisteredWithBackend) {
//                            val intent = Intent(this, SearchDeviceActivity::class.java)
//                            startActivity(intent)
//                            finish()
//                        } else {
//                            val isBleConnected = bleService?.isConnected() ?: false
//                            val intent = if (isBleConnected) {
//                                Intent(this, MainActivity::class.java)
//                            } else {
//                                Intent(this, SearchDeviceActivity::class.java)
//                            }
//                            startActivity(intent)
//                            finish()
//                        }
//                    }
//                }
//            }
//        }, 2000)
//    }
//
//
//    override fun onResume() {
//        super.onResume()
//        checkSelfPermission()
//        scheduleUpdateWorker()
//    }
//
//    @SuppressLint("NewApi")
//    private fun checkSelfPermission() {
//        if (isFirstPermission) {
//            var accepted = CheckSelfPermission.checkNotificationPermission(this)
//            if (accepted) {
//               accepted = CheckSelfPermission.checkLocationBluetoothPermission(this)
//            }
//
//            if (accepted) {
//                accepted = CheckSelfPermission.checkBackgroundLocation(this)
//            }
//
//            if (accepted) {
//                accepted = isLocationOn()
//            }
//
//            if (accepted) {
//                bindService(Intent(this, AxonBLEService::class.java),serviceConnector,Context.BIND_AUTO_CREATE)
//            }
//            isFirstPermission = false
//        }
//    }
//
//    @RequiresApi(Build.VERSION_CODES.Q)
//    override fun onRequestPermissionsResult(
//        requestCode: Int,
//        permissions: Array<String>,
//        grantResults: IntArray
//    ) {
//        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
//        if (requestCode == AppConstants.REQUEST_LOCATION_PERMISSION) {
//            val bluetoothGranted = grantResults[0] == PackageManager.PERMISSION_GRANTED
//            val locationGranted = grantResults[1] == PackageManager.PERMISSION_GRANTED
//
//            if (!bluetoothGranted && !locationGranted) {
//                AlertDialog.Builder(this)
//                    .setTitle(getString(R.string.permission_required))
//                    .setMessage(getString(R.string.bluetooth_location_permission_required))
//                    .setPositiveButton(getString(android.R.string.ok)) { dialogInterface: DialogInterface, _ ->
//                        dialogInterface.dismiss()
//                        finish()
//                    }
//                    .show()
//            } else {
//                if (CheckSelfPermission.checkBackgroundLocation(this)) {
//                    val accepted = isLocationOn()
//                    if (accepted) {
//                        if (BuildConfig.IS_DISTRIBUTOR) {
//                            // goto distributor login
//                            gotoDistributorLogin()
//                        } else {
//                            gotoLoginActivity()
//                        }
//                    }
//                }
//            }
//        }
//        if (requestCode == AppConstants.REQUEST_NOTIFICATION_PERMISSION) {
//            if (grantResults[0] != PackageManager.PERMISSION_GRANTED) {
//                AlertDialog.Builder(this)
//                    .setTitle(getString(R.string.permission_required))
//                    .setMessage(getString(R.string.notification_permission_required))
//                    .setPositiveButton(getString(R.string.ok)) { dialog, _ ->
//                        dialog.dismiss()
//                        finish()
//                    }
//                    .show()
//            } else {
//                CheckSelfPermission.checkLocationBluetoothPermission(this)
//            }
//        }
//
//        if (requestCode == AppConstants.REQUEST_BG_LOCATION_PERMISSION) {
//            if (grantResults[0] != PackageManager.PERMISSION_GRANTED) {
//                showBackgroundLocationRequiredDialog()
//            } else {
//                val accepted = isLocationOn()
//                if (accepted) {
//                    if (BuildConfig.IS_DISTRIBUTOR) {
//                        // goto distributor login
//                        gotoDistributorLogin()
//                    } else {
//                        gotoLoginActivity()
//                    }
//                }
//            }
//        } else {
//
//        }
//    }
//
//    private fun showBackgroundLocationRequiredDialog() {
//        AlertDialog.Builder(this)
//            .setTitle(getString(R.string.permission_required))
//            .setMessage(getString(R.string.background_location_rationale))
//            .setPositiveButton(getString(R.string.ok)) { dialog, _ ->
//                dialog.dismiss()
//                openPermissionsSettings()
//            }
//            .setNegativeButton(getString(R.string.cancel)) { dialog, _ ->
//                dialog.dismiss()
//                finish()
//            }
//            .show()
//    }
//
//    private fun gotoDistributorLogin() {
//        val termsAgreed = sharedPrefManager.getBool(SharedPrefKeys.TERMS_CONDITION_AGREE_KEY)
//        if (!termsAgreed) {
//            val intent1 = Intent(this, TermsAndConditionActivity::class.java)
//            startActivity(intent1)
//            finish()
//        } else {
//            // goto distributor login screen
//            val isConnected = bleService?.isConnected() ?: false
//            val isLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.DISTRIBUTOR_TOKEN).isNullOrBlank()
//            val intent = if (!isLoggedIn){
//                // goto to login
//                Intent(this, LoginDistributorActivity::class.java)
//            } else if (!isConnected){
//                // goto search screen
//                Intent(this, SearchDeviceActivity::class.java)
//            } else {
//                Intent(this, MainActivity::class.java)
//            }
//            startActivity(intent)
//            finish()
//        }
//    }
//
//    @SuppressLint("MissingPermission")
//    private fun gotoLoginActivity() {
//        val termsAgreed = sharedPrefManager.getBool(SharedPrefKeys.TERMS_CONDITION_AGREE_KEY)
//        if (!termsAgreed) {
//            val intent1 = Intent(this, TermsAndConditionActivity::class.java)
//            startActivity(intent1)
//            finish()
//        } else {
//            val isLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.KAKAO_TOKEN_KEY).isNullOrBlank() || FirebaseAuth.getInstance().currentUser != null
//            val isRegistered = !sharedPrefManager.getString(SharedPrefKeys.TOKEN).isNullOrBlank()
//            if (!isLoggedIn) {
//                val intent = Intent(this, LoginActivity::class.java)
//                startActivity(intent)
//                finish()
//            } else {
//                val isConnected = bleService?.isConnected() ?: false
//                val intent = if(isConnected && isRegistered) {
//                    Intent(this, MainActivity::class.java)
//                } else if (!isRegistered && isConnected){
//                    Intent(this, RegisterUserActivity::class.java)
//                } else {
//                    Intent(this, SearchDeviceActivity::class.java)
//                }
//                startActivity(intent)
//                finish()
//            }
//        }
//    }
//
//    private fun isLocationOn(): Boolean {
//        val gpsEnabled = isGpsEnabled()
//
//        if (!gpsEnabled) {
//            android.app.AlertDialog.Builder(this)
//                .setMessage(R.string.enable_location_setting)
//                .setPositiveButton(R.string.ok) { dialog, _ ->
//                    locationResult.launch(Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS))
//                    dialog.dismiss()
//                }
//                .setNegativeButton(R.string.cancel, null)
//                .setCancelable(false)
//                .show()
//
//            return false
//        }
//        return true
//    }
//
//    private fun isGpsEnabled() : Boolean {
//        val lm = getSystemService(Context.LOCATION_SERVICE) as LocationManager
//        try {
//            return lm.isProviderEnabled(LocationManager.GPS_PROVIDER)
//        } catch (ex: Exception) {
//            ex.printStackTrace()
//            return false
//        }
//    }
//
//    private fun openPermissionsSettings() {
//        try {
//            val intent = Intent()
//            intent.setAction(Settings.ACTION_APPLICATION_DETAILS_SETTINGS)
//            intent.addCategory(Intent.CATEGORY_DEFAULT)
//            intent.setData(Uri.parse("package:$packageName"))
//            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
//            intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY)
//            intent.addFlags(Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS)
//            startActivity(intent)
//            isFirstPermission = true
//        } catch (e: Exception) {
//            e.printStackTrace()
//        }
//    }
//
//    override fun onDestroy() {
//        super.onDestroy()
//        try {
//            unbindService(serviceConnector)
//        } catch (e : Exception) {
//            e.printStackTrace()
//        }
//    }
//
//    private fun scheduleUpdateWorker() {
//        val workManager = WorkManager.getInstance(this)
//        val workRequest = PeriodicWorkRequestBuilder<UpdateCheckWorker>(1, TimeUnit.DAYS)
//            .setConstraints(Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build())
//            .build()
//        workManager.enqueueUniquePeriodicWork("Update",ExistingPeriodicWorkPolicy.UPDATE, workRequest)
//    }
//
//}


















//package com.axon.kisa10.activity
//import android.annotation.SuppressLint
//import android.content.Context
//import android.content.DialogInterface
//import android.content.Intent
//import android.content.pm.PackageManager
//import android.location.LocationManager
//import android.net.Uri
//import android.os.Build
//import android.os.Bundle
//import android.provider.Settings
//import android.widget.Toast
//import androidx.activity.result.contract.ActivityResultContracts
//import androidx.annotation.RequiresApi
//import androidx.appcompat.app.AlertDialog
//import androidx.work.Constraints
//import androidx.work.ExistingPeriodicWorkPolicy
//import androidx.work.NetworkType
//import androidx.work.PeriodicWorkRequestBuilder
//import androidx.work.WorkManager
//import com.axon.kisa10.ble.AxonBLEService
//import com.axon.kisa10.ble.ServiceConnector
//import com.axon.kisa10.distributor.LoginDistributorActivity
//import com.axon.kisa10.distributor.MainDistributorActivity
//import com.axon.kisa10.util.AppConstants
//import com.axon.kisa10.util.CheckSelfPermission
//import com.axon.kisa10.util.SharedPrefKeys
//import com.axon.kisa10.util.SharedPrefManager
//import com.axon.kisa10.workmanager.UpdateCheckWorker
//import com.google.firebase.auth.FirebaseAuth
//import com.axon.kisa10.BuildConfig
//import com.axon.kisa10.R
//import java.util.concurrent.TimeUnit
//
//import com.axon.kisa10.databinding.ActivitySplashBinding
//import android.os.Handler // <-- Import Handler
//import android.os.Looper // <-- Import Looper
//import android.view.animation.LinearInterpolator
//import android.animation.ObjectAnimator
//
//
//import android.util.Log // Import Log
//
//
//@SuppressLint("CustomSplashScreen")
//class SplashActivity : BaseActivity() {
//    private lateinit var sharedPrefManager: SharedPrefManager
//    private var isFirstPermission = true
//    private var bleService : AxonBLEService? = null
//
//    private lateinit var binding : ActivitySplashBinding
//    private val handler by lazy { Handler(Looper.getMainLooper()) }
//
//    private val serviceConnector = ServiceConnector(
//        { s ->
//            this.bleService = s
//            // IMPORTANT: Binding the service here is good for having the service instance.
//            // However, the actual decision to NAVIGATE to Login/DistributorLogin
//            // should ideally happen AFTER the splash screen logic and permission checks.
//            // The original gotoDistributorLogin() and gotoLoginActivity() calls here
//            // might be too early if permissions haven't been sorted yet.
//            // For now, we'll let the postDelayed block handle the main navigation.
//            Log.d("SplashDebug", "AxonBLEService bound.")
//        },
//        { msg ->
//            Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
//            // Similar to above, the main navigation logic should be in postDelayed.
//            // If service binding fails, the app might proceed without BLE initially,
//            // and SearchDeviceActivity will handle prompting for BLE enable/connect.
//            Log.e("SplashDebug", "AxonBLEService binding failed: $msg")
//        }
//    )
//
//    private val locationResult = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
//        if(isGpsEnabled()) {
//            // After returning from location settings, re-evaluate the main navigation flow
//            // by calling a method that contains the logic from the postDelayed block.
//            // Or, simply let onResume() handle it if checkSelfPermission() is called there.
//            // For simplicity, we'll let onResume -> checkSelfPermission trigger the next steps.
//            Log.d("SplashDebug", "Location enabled by user.")
//        } else {
//            Toast.makeText(this, "Location is required to proceed.", Toast.LENGTH_LONG).show()
//            finish()
//        }
//    }
//
//    override fun onCreate(savedInstanceState: Bundle?) {
//        super.onCreate(savedInstanceState)
//        binding = ActivitySplashBinding.inflate(layoutInflater)
//        setContentView(binding.root)
//
//        sharedPrefManager = SharedPrefManager.getInstance(this)
//
//        val progressBar = binding.progressIndicatorSplash
//        val animator = ObjectAnimator.ofInt(progressBar, "progress", 0, 100)
//        animator.duration = 2000
//        animator.interpolator = LinearInterpolator()
//        animator.start()
//
//        // The primary navigation logic after the splash delay
////        handler.postDelayed({
////            Log.d("SplashDebug", "Splash delay finished. Starting navigation logic.")
////            navigateToAppropriateScreen()
////        }, 2000) // Keep your 2-second delay
//        handler.postDelayed({
//            Log.d("SplashDebug", "Splash delay finished. Deciding next screen.")
//            val termsAgreed = sharedPrefManager.getBool(SharedPrefKeys.TERMS_CONDITION_AGREE_KEY)
//            Log.d("SplashDebug", "Terms agreed: $termsAgreed")
//
//            if (!termsAgreed) {
//                Log.d("SplashDebug", "Terms NOT agreed. Navigating to TermsAndConditionActivity.")
//                val intent1 = Intent(this, TermsAndConditionActivity::class.java)
//                startActivity(intent1)
//            } else {
//                // --- TERMS ARE AGREED ---
//                if (BuildConfig.IS_DISTRIBUTOR) {
//                    Log.d("SplashDebug", "Distributor build.")
//                    val isDistributorLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.DISTRIBUTOR_TOKEN).isNullOrBlank()
//                    Log.d("SplashDebug", "Distributor logged in: $isDistributorLoggedIn")
//                    val intent = if (!isDistributorLoggedIn) {
//                        Log.d("SplashDebug", "Distributor not logged in. Navigating to LoginDistributorActivity.")
//                        Intent(this, SearchDeviceActivity::class.java)
//                    } else {
//                        // Distributor is logged in. Next step is usually connecting to a device.
//                        Log.d("SplashDebug", "Distributor logged in. Navigating to SearchDeviceActivity.")
//                        Intent(this, LoginDistributorActivity::class.java)
//                    }
//                    startActivity(intent)
//                } else {
//                    // --- REGULAR USER FLOW (TERMS AGREED) ---
//                    Log.d("SplashDebug", "Regular user build, terms agreed.")
//
//                    val isFullyRegisteredWithBackend = !sharedPrefManager.getString(SharedPrefKeys.TOKEN).isNullOrBlank()
//                    Log.d("SplashDebug", "User fully registered with backend: $isFullyRegisteredWithBackend")
//
//                    // Regardless of social login status at this point, if not fully registered with backend,
//                    // or even if fully registered but BLE might not be auto-connected,
//                    // the flow after terms (for a first-time setup or re-setup) should go to BLE connection.
//                    // SearchDeviceActivity can handle attempting auto-connection to a known device first.
//
//                    Log.d("SplashDebug", "Navigating to SearchDeviceActivity for BLE setup/connection.")
//                    val intent = Intent(this, SearchDeviceActivity::class.java)
//                    startActivity(intent)
//                }
//            }
//            finish() // Finish SplashActivity
//        }, 4000)
//    }
//
//    private fun navigateToAppropriateScreen() {
//        val termsAgreed = sharedPrefManager.getBool(SharedPrefKeys.TERMS_CONDITION_AGREE_KEY)
//        Log.d("SplashDebug", "Terms agreed: $termsAgreed")
//
//        if (!termsAgreed) {
//            Log.d("SplashDebug", "Navigating to TermsAndConditionActivity.")
//            val intent1 = Intent(this, TermsAndConditionActivity::class.java)
//            startActivity(intent1)
//            finish()
//            return // Exit after starting TermsAndConditionActivity
//        }
//
//        // --- Terms are agreed, proceed to next logic ---
//
//        if (BuildConfig.IS_DISTRIBUTOR) {
//            Log.d("SplashDebug", "Distributor build.")
//            val isDistributorLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.DISTRIBUTOR_TOKEN).isNullOrBlank()
//            Log.d("SplashDebug", "Distributor logged in: $isDistributorLoggedIn")
//            val intent = if (!isDistributorLoggedIn) {
//                Log.d("SplashDebug", "Navigating to LoginDistributorActivity.")
//                Intent(this, LoginDistributorActivity::class.java)
//            } else {
//                // Distributor is logged in. They should go to connect/manage a device.
//                Log.d("SplashDebug", "Distributor logged in. Navigating to SearchDeviceActivity (or MainActivity if auto-connect).")
//                Intent(this, SearchDeviceActivity::class.java) // Or MainActivity if auto-connect is handled and successful
//            }
//            startActivity(intent)
//        } else {
//            // --- Regular End-User Flow ---
//            Log.d("SplashDebug", "Regular user build.")
//
//            // NEW FLOW: After terms, ALWAYS go to SearchDeviceActivity first,
//            // UNLESS the user is already fully registered AND their device auto-connects.
//
//            val isFullyRegisteredWithBackend = !sharedPrefManager.getString(SharedPrefKeys.TOKEN).isNullOrBlank()
//            Log.d("SplashDebug", "User fully registered with backend: $isFullyRegisteredWithBackend")
//
//            if (isFullyRegisteredWithBackend) {
//                // User is fully registered.
//                // Here, you might attempt an auto-connect if bleService is bound and ready.
//                // For now, to align with the "BLE page -> Login page" for *new* full registrations,
//                // and to handle cases where auto-connect might fail or isn't instant,
//                // routing to SearchDeviceActivity is a safe default.
//                // SearchDeviceActivity can handle attempting an auto-connect to the last known device.
//                // If auto-connect is successful there, it can then go to MainActivity.
//                // If not, it shows the scan list.
//                Log.d("SplashDebug", "User fully registered. Navigating to SearchDeviceActivity (to handle auto-connect or scan).")
//                startActivity(Intent(this, SearchDeviceActivity::class.java))
//            } else {
//                // User terms agreed, but NOT fully registered with backend.
//                // This means they might have socially logged in before but didn't complete vehicle reg,
//                // OR they are a brand new user post-terms.
//                // The new flow requires BLE setup before social login for new full registrations.
//                Log.d("SplashDebug", "User not fully registered. Navigating to SearchDeviceActivity for BLE setup.")
//                startActivity(Intent(this, SearchDeviceActivity::class.java))
//            }
//        }
//        finish() // Finish SplashActivity after deciding where to go
//    }
//
//
//    override fun onResume() {
//        super.onResume()
//        // checkSelfPermission() is important if the user returns to the app
//        // after changing permissions in settings.
//        // It will also attempt to bind the service if permissions are now granted.
//        Log.d("SplashDebug", "onResume called. Calling checkSelfPermission.")
//        checkSelfPermission() // This will handle permission checks and service binding
//        scheduleUpdateWorker()
//    }
//
//    @SuppressLint("NewApi")
//    private fun checkSelfPermission() {
//        // This function now primarily focuses on ensuring permissions are granted
//        // and then binding the service. The navigation logic is moved to navigateToAppropriateScreen().
//        if (isFirstPermission) {
//            Log.d("SplashDebug", "checkSelfPermission: First time permission check.")
//            var accepted = CheckSelfPermission.checkNotificationPermission(this)
//            if (accepted) {
//                Log.d("SplashDebug", "Notification permission OK.")
//                accepted = CheckSelfPermission.checkLocationBluetoothPermission(this)
//            }
//
//            if (accepted) {
//                Log.d("SplashDebug", "Location/Bluetooth permission OK.")
//                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) { // Only check for Q and above
//                    accepted = CheckSelfPermission.checkBackgroundLocation(this)
//                }
//            }
//
//            if (accepted) {
//                Log.d("SplashDebug", "Background location permission OK (or not needed).")
//                accepted = isLocationOn()
//            }
//
//            if (accepted) {
//                Log.d("SplashDebug", "All permissions and settings OK. Binding AxonBLEService.")
//                // Only bind the service if all permissions are okay.
//                // The serviceConnector's onServiceConnected will be called,
//                // but the primary navigation is handled by navigateToAppropriateScreen after the splash delay.
//                startService(Intent(this, AxonBLEService::class.java)) // Ensure service is started
//                bindService(Intent(this, AxonBLEService::class.java), serviceConnector, Context.BIND_AUTO_CREATE)
//            }
//            isFirstPermission = false
//        } else {
//            Log.d("SplashDebug", "checkSelfPermission: Not the first time, permissions likely already handled or user is returning.")
//            // If not the first time, and permissions were previously denied and now granted (e.g., user returns from settings),
//            // you might need to re-trigger the service binding if it failed earlier.
//            // Or, if the service is already bound, navigateToAppropriateScreen will proceed correctly.
//            // A simple check: if bleService is null and permissions are now okay, try binding again.
//            if (bleService == null && CheckSelfPermission.checkLocationBluetoothPermission(this) && isLocationOn()) {
//                Log.d("SplashDebug", "Re-attempting service bind in onResume/checkSelfPermission.")
//                startService(Intent(this, AxonBLEService::class.java))
//                bindService(Intent(this, AxonBLEService::class.java), serviceConnector, Context.BIND_AUTO_CREATE)
//            }
//        }
//    }
//
//    @RequiresApi(Build.VERSION_CODES.Q)
//    override fun onRequestPermissionsResult(
//        requestCode: Int,
//        permissions: Array<String>,
//        grantResults: IntArray
//    ) {
//        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
//        Log.d("SplashDebug", "onRequestPermissionsResult: requestCode=$requestCode")
//        // After permissions are granted/denied, re-evaluate the flow.
//        // isFirstPermission will be false here, so checkSelfPermission will run its "else" path.
//        // The checkSelfPermission() call in onResume() will handle re-trying to bind the service if needed.
//        // No immediate navigation here; let onResume() and its call to checkSelfPermission() handle it.
//        // If permissions are granted, checkSelfPermission will now successfully bind the service.
//        // The navigateToAppropriateScreen() is called after a delay anyway.
//
//        // Simplified handling based on your existing structure
//        if (requestCode == AppConstants.REQUEST_LOCATION_PERMISSION) {
//            val allGranted = grantResults.all { it == PackageManager.PERMISSION_GRANTED }
//            if (!allGranted) {
//                AlertDialog.Builder(this)
//                    .setTitle(getString(R.string.permission_required))
//                    .setMessage(getString(R.string.bluetooth_location_permission_required))
//                    .setPositiveButton(getString(android.R.string.ok)) { dialogInterface: DialogInterface, _ ->
//                        dialogInterface.dismiss()
//                        finish()
//                    }
//                    .show()
//            } else {
//                // Permissions granted, proceed to check other settings in onResume->checkSelfPermission
//                // which will eventually lead to navigateToAppropriateScreen if service binds.
//            }
//        }
//        // ... (handle other permission requestCodes similarly if needed)
//    }
//
//    // gotoDistributorLogin and gotoLoginActivity are primarily for the ServiceConnector callback,
//    // which might be too early. The main navigation is now in navigateToAppropriateScreen.
//    // These can be simplified or removed if ServiceConnector doesn't need to trigger full navigation.
//    private fun gotoDistributorLogin() {
//        Log.d("SplashDebug", "Service connected (Distributor). Redirecting from ServiceConnector (might be too early).")
//        // This will be re-evaluated by navigateToAppropriateScreen after the delay
//        // For now, let's comment out the direct navigation from here to avoid conflict
//        /*
//        val termsAgreed = sharedPrefManager.getBool(SharedPrefKeys.TERMS_CONDITION_AGREE_KEY)
//        if (!termsAgreed) {
//            val intent1 = Intent(this, TermsAndConditionActivity::class.java)
//            startActivity(intent1)
//        } else {
//            val isConnected = bleService?.isConnected() ?: false
//            val isLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.DISTRIBUTOR_TOKEN).isNullOrBlank()
//            val intent = if (!isLoggedIn){
//                Intent(this, LoginDistributorActivity::class.java)
//            } else if (!isConnected){
//                Intent(this, SearchDeviceActivity::class.java)
//            } else {
//                Intent(this, MainActivity::class.java)
//            }
//            startActivity(intent)
//        }
//        finish()
//        */
//    }
//
//    @SuppressLint("MissingPermission")
//    private fun gotoLoginActivity() {
//        Log.d("SplashDebug", "Service connected (User). Redirecting from ServiceConnector (might be too early).")
//        // This will be re-evaluated by navigateToAppropriateScreen after the delay
//        /*
//        val termsAgreed = sharedPrefManager.getBool(SharedPrefKeys.TERMS_CONDITION_AGREE_KEY)
//        if (!termsAgreed) {
//            val intent1 = Intent(this, TermsAndConditionActivity::class.java)
//            startActivity(intent1)
//        } else {
//            val isLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.KAKAO_TOKEN_KEY).isNullOrBlank() || FirebaseAuth.getInstance().currentUser != null
//            val isRegistered = !sharedPrefManager.getString(SharedPrefKeys.TOKEN).isNullOrBlank()
//            if (!isLoggedIn) {
//                val intent = Intent(this, LoginActivity::class.java)
//                startActivity(intent)
//            } else {
//                val isConnected = bleService?.isConnected() ?: false
//                val intent = if(isConnected && isRegistered) {
//                    Intent(this, MainActivity::class.java)
//                } else if (!isRegistered && isConnected){ // Socially logged in, BLE connected, but not backend registered
//                    Intent(this, RegisterUserActivity::class.java)
//                } else { // Needs BLE connection or full registration
//                    Intent(this, SearchDeviceActivity::class.java)
//                }
//                startActivity(intent)
//            }
//        }
//        finish()
//        */
//    }
//
//    // isLocationOn, isGpsEnabled, openPermissionsSettings, onDestroy, scheduleUpdateWorker remain the same
//    // ... (rest of your SplashActivity code) ...
//    private fun isLocationOn(): Boolean {
//        val gpsEnabled = isGpsEnabled()
//        if (!gpsEnabled) {
//            Log.d("SplashDebug", "Location is OFF. Prompting user.")
//            android.app.AlertDialog.Builder(this)
//                .setMessage(R.string.enable_location_setting)
//                .setPositiveButton(R.string.ok) { dialog, _ ->
//                    locationResult.launch(Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS))
//                    dialog.dismiss()
//                }
//                .setNegativeButton(R.string.cancel) { dialog, _ ->
//                    dialog.dismiss()
//                    Toast.makeText(this, "Location is required to use the app.", Toast.LENGTH_LONG).show()
//                    finish() // Exit if location is denied
//                }
//                .setCancelable(false)
//                .show()
//            return false
//        }
//        Log.d("SplashDebug", "Location is ON.")
//        return true
//    }
//
//    private fun isGpsEnabled() : Boolean {
//        val lm = getSystemService(Context.LOCATION_SERVICE) as LocationManager
//        try {
//            return lm.isProviderEnabled(LocationManager.GPS_PROVIDER)
//        } catch (ex: Exception) {
//            ex.printStackTrace()
//            return false
//        }
//    }
//
//    private fun openPermissionsSettings() {
//        try {
//            val intent = Intent()
//            intent.setAction(Settings.ACTION_APPLICATION_DETAILS_SETTINGS)
//            intent.addCategory(Intent.CATEGORY_DEFAULT)
//            intent.setData(Uri.parse("package:$packageName"))
//            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
//            intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY)
//            intent.addFlags(Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS)
//            startActivity(intent)
//            isFirstPermission = true // Allow re-check when returning
//        } catch (e: Exception) {
//            e.printStackTrace()
//        }
//    }
//
//    override fun onDestroy() {
//        super.onDestroy()
//        try {
//            if (bleService != null) { // Check if service was bound before trying to unbind
//                unbindService(serviceConnector)
//            }
//        } catch (e : Exception) {
//            e.printStackTrace()
//        }
//        handler.removeCallbacksAndMessages(null) // Clean up handler
//    }
//
//    private fun scheduleUpdateWorker() {
//        val workManager = WorkManager.getInstance(this)
//        val workRequest = PeriodicWorkRequestBuilder<UpdateCheckWorker>(1, TimeUnit.DAYS)
//            .setConstraints(Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build())
//            .build()
//        workManager.enqueueUniquePeriodicWork("Update", ExistingPeriodicWorkPolicy.UPDATE, workRequest)
//    }
//}



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
import androidx.appcompat.app.AlertDialog // Use androidx.appcompat.app.AlertDialog for consistent styling
import androidx.work.Constraints
import androidx.work.ExistingPeriodicWorkPolicy
import androidx.work.NetworkType
import androidx.work.PeriodicWorkRequestBuilder
import androidx.work.WorkManager
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.ble.ServiceConnector
import com.axon.kisa10.distributor.LoginDistributorActivity
// import com.axon.kisa10.distributor.MainDistributorActivity // Not directly used here for navigation decision
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.CheckSelfPermission
import com.axon.kisa10.util.SharedPrefKeys
import com.axon.kisa10.util.SharedPrefManager
import com.axon.kisa10.workmanager.UpdateCheckWorker
// import com.google.firebase.auth.FirebaseAuth // No longer primary decider for regular user flow start
import com.axon.kisa10.BuildConfig
import com.axon.kisa10.R
import java.util.concurrent.TimeUnit

import com.axon.kisa10.databinding.ActivitySplashBinding
import android.os.Handler
import android.os.Looper
import android.view.animation.LinearInterpolator
import android.animation.ObjectAnimator
import android.util.Log

@SuppressLint("CustomSplashScreen")
class SplashActivity : BaseActivity() {
    private lateinit var sharedPrefManager: SharedPrefManager
    private var isFirstPermissionCheck = true // Renamed for clarity
    private var bleService: AxonBLEService? = null
    private var isBleServiceBound = false // Track binding state

    private lateinit var binding: ActivitySplashBinding
    private val handler by lazy { Handler(Looper.getMainLooper()) }

    private val serviceConnector = ServiceConnector(
        { service ->
            this.bleService = service
            this.isBleServiceBound = true
            Log.d("SplashDebug", "AxonBLEService bound successfully.")
            // We don't navigate immediately from here.
            // The main navigation logic is in navigateToAppropriateScreen after permissions and splash delay.
        },
        { msg ->
            this.isBleServiceBound = false
            // Toast.makeText(this, "BLE Service binding failed: $msg", Toast.LENGTH_SHORT).show() // Inform user
            Log.e("SplashDebug", "AxonBLEService binding failed: $msg")
            // If binding fails, navigateToAppropriateScreen will still run,
            // and subsequent activities (like SearchDeviceActivity) will have to handle BLE not being ready.
        }
    )

    private val locationSettingsResultLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { _ ->
        // This callback is triggered when the user returns from the location settings screen.
        // We should re-evaluate the permissions and settings.
        // onResume() will be called, which in turn calls checkPermissionsAndProceed().
        Log.d("SplashDebug", "Returned from location settings.")
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivitySplashBinding.inflate(layoutInflater)
        setContentView(binding.root)

        Log.d("SplashDebug", "onCreate: Initializing SplashActivity.")
        sharedPrefManager = SharedPrefManager.getInstance(this)

        val progressBar = binding.progressIndicatorSplash
        val animator = ObjectAnimator.ofInt(progressBar, "progress", 0, 100)
        animator.duration = 2000 // Splash animation duration
        animator.interpolator = LinearInterpolator()
        animator.start()

        // The primary navigation logic will be called after the splash delay.
        // Permissions and service binding attempts happen before this delay finishes (in onResume/checkPermissionsAndProceed).
        handler.postDelayed({
            Log.d("SplashDebug", "Splash delay finished. Proceeding to navigateToAppropriateScreen.")
            navigateToAppropriateScreen()
        }, 4000) // Slightly longer delay to ensure animations/checks can run
    }

    override fun onResume() {
        super.onResume()
        Log.d("SplashDebug", "onResume: Calling checkPermissionsAndProceed.")
        // This is a good place to check permissions, especially if the user
        // comes back to the app after changing them in settings.
        checkPermissionsAndProceed()
        scheduleUpdateWorker()
    }

    @SuppressLint("NewApi") // For Build.VERSION_CODES.Q check
    private fun checkPermissionsAndProceed() {
        Log.d("SplashDebug", "checkPermissionsAndProceed: isFirstPermissionCheck = $isFirstPermissionCheck")
        if (isFirstPermissionCheck) {
            var allPermissionsGranted = true

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                if (!CheckSelfPermission.checkNotificationPermission(this)) {
                    Log.d("SplashDebug", "Requesting Notification permission.")
                    allPermissionsGranted = false
                    // Permission request is asynchronous. Further logic will be handled in onRequestPermissionsResult or next onResume.
                } else {
                    Log.d("SplashDebug", "Notification permission already granted.")
                }
            }

            if (allPermissionsGranted && !CheckSelfPermission.checkLocationBluetoothPermission(this)) {
                Log.d("SplashDebug", "Requesting Location/Bluetooth permissions.")
                allPermissionsGranted = false
            } else if (allPermissionsGranted) {
                Log.d("SplashDebug", "Location/Bluetooth permissions already granted.")
            }

            if (allPermissionsGranted && Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                if (!CheckSelfPermission.checkBackgroundLocation(this)) {
                    Log.d("SplashDebug", "Requesting Background Location permission.")
                    allPermissionsGranted = false
                } else {
                    Log.d("SplashDebug", "Background Location permission already granted or not applicable.")
                }
            }

            if (allPermissionsGranted && !isLocationOn()) {
                // isLocationOn() itself prompts the user and returns true/false.
                // If it returns false, it means the prompt was shown or settings need changing.
                // The flow will pause here until the user interacts with the location settings dialog/screen.
                Log.d("SplashDebug", "Location is not ON. Prompt shown.")
                allPermissionsGranted = false // Mark as not ready to proceed fully yet
            } else if (allPermissionsGranted) {
                Log.d("SplashDebug", "Location is ON.")
            }

            if (allPermissionsGranted) {
                Log.d("SplashDebug", "All permissions and settings appear OK. Attempting to bind AxonBLEService.")
                attemptToBindService()
            }
            isFirstPermissionCheck = false // Only run the full permission request sequence once per launch/creation.
        } else {
            // Not the first permission check (e.g., onResume after returning from settings or permission dialog)
            // Re-check critical ones and attempt service bind if now okay.
            Log.d("SplashDebug", "checkPermissionsAndProceed: Not the first check. Verifying critical permissions for service binding.")
            if (CheckSelfPermission.checkLocationBluetoothPermission(this) && isLocationOn() && !isBleServiceBound) {
                Log.d("SplashDebug", "Permissions OK and service not bound. Re-attempting to bind AxonBLEService.")
                attemptToBindService()
            } else if (isBleServiceBound) {
                Log.d("SplashDebug", "Service already bound or permissions still not ready.")
            }
        }
    }

    private fun attemptToBindService() {
        if (!isBleServiceBound) {
            Log.d("SplashDebug", "Attempting to start and bind AxonBLEService.")
            startService(Intent(this, AxonBLEService::class.java)) // Ensure service is started
            bindService(Intent(this, AxonBLEService::class.java), serviceConnector, Context.BIND_AUTO_CREATE)
        } else {
            Log.d("SplashDebug", "AxonBLEService already bound or binding in progress.")
        }
    }

    private fun navigateToAppropriateScreen() {
        Log.d("SplashDebug", "navigateToAppropriateScreen: Determining next screen...")
        val termsAgreed = sharedPrefManager.getBool(SharedPrefKeys.TERMS_CONDITION_AGREE_KEY)
        Log.d("SplashDebug", "Terms agreed: $termsAgreed")

        if (!termsAgreed) {
            Log.d("SplashDebug", "Terms NOT agreed. Navigating to TermsAndConditionActivity.")
            startActivity(Intent(this, TermsAndConditionActivity::class.java))
            finish()
            return
        }

        // --- TERMS ARE AGREED ---
        if (BuildConfig.IS_DISTRIBUTOR) {
            Log.d("SplashDebug", "Distributor build.")
            val isDistributorLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.DISTRIBUTOR_TOKEN).isNullOrBlank()
            Log.d("SplashDebug", "Distributor logged in: $isDistributorLoggedIn")
            val intent = if (!isDistributorLoggedIn) {
                Log.d("SplashDebug", "Distributor NOT logged in. Navigating to LoginDistributorActivity.")
                Intent(this, LoginDistributorActivity::class.java)
            } else {
                // Distributor is logged in. Next step is usually connecting to a device.
                Log.d("SplashDebug", "Distributor IS logged in. Navigating to SearchDeviceActivity (distributor might need to select a new device).")
                Intent(this, SearchDeviceActivity::class.java)
            }
            startActivity(intent)
        } else {
            // --- REGULAR END-USER FLOW (TERMS AGREED) ---
            Log.d("SplashDebug", "Regular user build.")
            val isFullyRegisteredWithBackend = !sharedPrefManager.getString(SharedPrefKeys.TOKEN).isNullOrBlank()
            Log.d("SplashDebug", "User fully registered with backend: $isFullyRegisteredWithBackend")

            // For regular users, after terms, they always go to SearchDeviceActivity.
            // SearchDeviceActivity will handle auto-connecting to a known device if possible.
            // If no known device or auto-connect fails, it shows the scan list.
            // If a new user connects, SearchDeviceActivity then navigates to RegisterUserActivity.
            // If an existing registered user connects (or auto-connects), it navigates to MainActivity.
            Log.d("SplashDebug", "Navigating to SearchDeviceActivity for BLE setup/connection/auto-login continuation.")
            startActivity(Intent(this, SearchDeviceActivity::class.java))
        }
        finish() // Finish SplashActivity after deciding where to go
    }

    @RequiresApi(Build.VERSION_CODES.Q) // Ensure API level check if using Build.VERSION_CODES.Q directly
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        Log.d("SplashDebug", "onRequestPermissionsResult: requestCode=$requestCode, results=${grantResults.joinToString()}")

        // Set isFirstPermissionCheck to true so that onResume can re-evaluate the whole sequence
        // if the user comes back from the permission dialog.
        // isFirstPermissionCheck = true; // Re-evaluate all checks on next onResume

        // It's generally better to let onResume handle the re-check rather than duplicating logic here.
        // If permissions were just granted, the subsequent onResume -> checkPermissionsAndProceed will attempt service binding.
        // If permissions were denied, show appropriate messages.

        var allRequiredPermissionsGranted = true
        grantResults.forEach {
            if (it != PackageManager.PERMISSION_GRANTED) {
                allRequiredPermissionsGranted = false
            }
        }

        if (!allRequiredPermissionsGranted) {
            // Determine which permission group failed for a more specific message or action
            if (requestCode == AppConstants.REQUEST_LOCATION_PERMISSION || requestCode == AppConstants.REQUEST_BG_LOCATION_PERMISSION) {
                AlertDialog.Builder(this)
                    .setTitle(getString(R.string.permission_required))
                    .setMessage(getString(R.string.bluetooth_location_permission_required))
                    .setPositiveButton(getString(R.string.setting)) { _, _ -> openPermissionsSettings() }
                    .setNegativeButton(getString(R.string.setting)) { _, _ -> finish() }
                    .setCancelable(false)
                    .show()
            } else if (requestCode == AppConstants.REQUEST_NOTIFICATION_PERMISSION) {
                AlertDialog.Builder(this)
                    .setTitle(getString(R.string.permission_required))
                    .setMessage(getString(R.string.notification_permission_required))
                    .setPositiveButton(getString(R.string.setting)) { _, _ -> openPermissionsSettings() }
                    .setNegativeButton(getString(R.string.setting)) { _, _ -> finish() }
                    .setCancelable(false)
                    .show()
            }
        } else {
            // All permissions for this request code were granted.
            // Let onResume() handle the next steps naturally.
            Log.d("SplashDebug", "Permissions for requestCode $requestCode granted.")
        }
    }

    private fun isLocationOn(): Boolean {
        val gpsEnabled = isGpsEnabled()
        if (!gpsEnabled) {
            Log.d("SplashDebug", "Location is OFF. Prompting user.")
            AlertDialog.Builder(this) // Use androidx.appcompat.app.AlertDialog
                .setMessage(R.string.enable_location_setting)
                .setPositiveButton(R.string.ok) { dialog, _ ->
                    locationSettingsResultLauncher.launch(Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS))
                    dialog.dismiss()
                }
                .setNegativeButton(R.string.cancel) { dialog, _ ->
                    dialog.dismiss()
                    Toast.makeText(this, "Location is required to use the app.", Toast.LENGTH_LONG).show()
                    finish() // Exit if location is denied
                }
                .setCancelable(false)
                .show()
            return false
        }
        Log.d("SplashDebug", "Location is ON.")
        return true
    }

    private fun isGpsEnabled() : Boolean {
        val lm = getSystemService(Context.LOCATION_SERVICE) as LocationManager
        return try {
            lm.isProviderEnabled(LocationManager.GPS_PROVIDER)
        } catch (ex: Exception) {
            Log.e("SplashDebug", "Error checking GPS status", ex)
            false
        }
    }

    private fun openPermissionsSettings() {
        Log.d("SplashDebug", "Opening app settings for permissions.")
        try {
            val intent = Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS)
            intent.data = Uri.fromParts("package", packageName, null)
            startActivity(intent)
            // When the user returns, onResume() will be called.
        } catch (e: Exception) {
            Log.e("SplashDebug", "Error opening app settings", e)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        Log.d("SplashDebug", "onDestroy: Cleaning up SplashActivity.")
        if (isBleServiceBound && bleService != null) {
            try {
                unbindService(serviceConnector)
                isBleServiceBound = false
                Log.d("SplashDebug", "AxonBLEService unbound.")
            } catch (e : IllegalArgumentException) {
                Log.e("SplashDebug", "Error unbinding service (already unbound?): ${e.message}")
            }
        }
        handler.removeCallbacksAndMessages(null) // Clean up handler
    }

    private fun scheduleUpdateWorker() {
        val workManager = WorkManager.getInstance(this)
        val workRequest = PeriodicWorkRequestBuilder<UpdateCheckWorker>(1, TimeUnit.DAYS)
            .setConstraints(Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build())
            .build()
        workManager.enqueueUniquePeriodicWork("Kisa10AppUpdate", ExistingPeriodicWorkPolicy.KEEP, workRequest)
        Log.d("SplashDebug", "UpdateCheckWorker scheduled.")
    }
}