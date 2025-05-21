package com.axon.kisa10.util

import android.Manifest
import android.app.Activity
import android.app.AlertDialog
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.location.LocationManager
import android.net.Uri
import android.os.Build
import android.provider.Settings
import androidx.annotation.RequiresApi
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.axon.kisa10.R

object CheckSelfPermission {

    private var permissiondiaog: AlertDialog? = null

    @RequiresApi(Build.VERSION_CODES.S)
    private val blePermissions = arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)

    private val locationPermission = arrayOf(Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION)

    @RequiresApi(Build.VERSION_CODES.Q)
    private val backgroundLocationPermission = arrayOf( Manifest.permission.ACCESS_BACKGROUND_LOCATION)

    fun checkLocationBluetoothPermission(context: Activity): Boolean {
        if (!hasLocationPermission(context) && !hasBluetoothPermission(context)) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                ActivityCompat.requestPermissions(
                    context,
                    blePermissions + locationPermission,
                    AppConstants.REQUEST_LOCATION_PERMISSION
                )
            } else {
                ActivityCompat.requestPermissions(
                    context,
                    locationPermission,
                    AppConstants.REQUEST_LOCATION_PERMISSION
                )
            }
            return false
        }
        return true
    }

    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    fun checkNotificationPermission(context: Activity) : Boolean {
        if (!hasNotificationPermission(context)) {
            ActivityCompat.requestPermissions(
                context,
                arrayOf(Manifest.permission.POST_NOTIFICATIONS),
                AppConstants.REQUEST_NOTIFICATION_PERMISSION
            )
            return false
        }
        return true
    }

    private fun hasNotificationPermission(activity: Activity): Boolean {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            activity.checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) == PackageManager.PERMISSION_GRANTED
        } else {
            true
        }
    }

    @RequiresApi(Build.VERSION_CODES.Q)
    fun checkBackgroundLocation(context : Activity) : Boolean {
        if (!hasBackgroundLocationPermission(context)) {
            ActivityCompat.requestPermissions(
                context,
                backgroundLocationPermission,
                AppConstants.REQUEST_BG_LOCATION_PERMISSION
            )
            return false
        }
        return true
    }

    fun checkLocationBluetoothPermissionRational(context: Context): Boolean {
        try {
            permissiondiaog?.dismiss()
        } catch (e: Exception) {
            e.printStackTrace()
        }
        if (!hasLocationPermission(context) && !hasBluetoothPermission(context)) {
            if (!ActivityCompat.shouldShowRequestPermissionRationale((context as Activity), Manifest.permission.ACCESS_FINE_LOCATION)) {
                showPermissionAlert(context, context.getString(R.string.enable_location_permission))
                return false
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                ActivityCompat.requestPermissions(
                    context,
                    blePermissions + locationPermission + backgroundLocationPermission,
                    AppConstants.REQUEST_LOCATION_PERMISSION
                )
            } else {
                ActivityCompat.requestPermissions(
                    context,
                    locationPermission,
                    AppConstants.REQUEST_LOCATION_PERMISSION
                )
            }
            return false
        }
        return true
    }

    private fun hasBluetoothPermission(context: Context): Boolean {
        val bleScanPermission = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            ContextCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED
        } else {
            true
        }

        val bleConnectPermission = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            ContextCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED
        } else {
            true
        }

        return bleConnectPermission && bleScanPermission
    }

    fun hasBackgroundLocationPermission(context: Activity) : Boolean {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            ContextCompat.checkSelfPermission(context, Manifest.permission.ACCESS_BACKGROUND_LOCATION) == PackageManager.PERMISSION_GRANTED
        } else {
            true
        }
    }
    private fun hasLocationPermission(context: Context) : Boolean {
        return ContextCompat.checkSelfPermission(context, Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED
    }

    fun isLocationOn(context: Context): Boolean {
        val lm = context.getSystemService(Context.LOCATION_SERVICE) as LocationManager
        var gpsEnabled = false

        try {
            gpsEnabled = lm.isProviderEnabled(LocationManager.GPS_PROVIDER)
        } catch (ex: Exception) {
            ex.printStackTrace()
        }

        if (!gpsEnabled) {
            AlertDialog.Builder(context)
                .setMessage(R.string.enable_location_setting)
                .setPositiveButton(R.string.ok) { dialog, _ ->
                    (context as Activity).startActivityForResult(
                        Intent(
                            Settings.ACTION_LOCATION_SOURCE_SETTINGS
                        ), AppConstants.REQUEST_ENABLE_LOCATION
                    )
                    dialog.dismiss()
                }
                .setNegativeButton(R.string.cancel, null)
                .setCancelable(false)
                .show()

            return false
        }
        return true
    }


    fun isBluetoothOn(context: Context): Boolean {
        try {
            val bluetoothManager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
            val bluetoothAdapter = bluetoothManager.adapter
            if (bluetoothAdapter == null) {
                return false
            } else if (!bluetoothAdapter.isEnabled) {
                val enableBtIntent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
                (context as Activity).startActivityForResult(
                    enableBtIntent,
                    AppConstants.REQUEST_ENABLE_BLUETOOTH
                )
                return false
            } else {
                return true
            }
        } catch (e: SecurityException) {
            return false
        } catch (e: RuntimeException) {
            return false
        }
    }


    private fun showPermissionAlert(context: Context, msg: String?) {
        try {
            permissiondiaog?.dismiss()
        } catch (e: Exception) {
            e.printStackTrace()
        }
        permissiondiaog = AlertDialog.Builder(context)
            .setTitle(context.getString(R.string.app_name))
            .setMessage(msg)
            .setPositiveButton(R.string.ok) { dialog, _ ->
                dialog.dismiss()
                openPermissionsSettings(context.packageName, context)
            }.show()
    }

    fun openPermissionsSettings(packageName: String, context: Context?) {
        try {
            val intent = Intent()
            intent.setAction(Settings.ACTION_APPLICATION_DETAILS_SETTINGS)
            intent.addCategory(Intent.CATEGORY_DEFAULT)
            intent.setData(Uri.parse("package:$packageName"))
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY)
            intent.addFlags(Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS)
            ActivityCompat.startActivityForResult(
                (context as Activity?)!!,
                intent,
                AppConstants.MY_MARSHMELLO_PERMISSION,
                null
            )
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    @JvmStatic
    fun checkPermissionForReadExtertalStorage(context: Context): Boolean {
        val result = context.checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE)
        return result == PackageManager.PERMISSION_GRANTED
    }

    @JvmStatic
    fun checkPermissionForWriteExtertalStorage(context: Context): Boolean {
        val result = context.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)
        return result == PackageManager.PERMISSION_GRANTED
    }

    @JvmStatic
    fun checkStoragePermission(context: Context?) {
        if (ContextCompat.checkSelfPermission(
                context!!,
                Manifest.permission.READ_EXTERNAL_STORAGE
            ) != PackageManager.PERMISSION_GRANTED
        ) {
            ActivityCompat.requestPermissions(
                (context as Activity?)!!,
                arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE),
                1
            )
        }

        if (ContextCompat.checkSelfPermission(
                context,
                Manifest.permission.WRITE_EXTERNAL_STORAGE
            ) != PackageManager.PERMISSION_GRANTED
        ) {
            ActivityCompat.requestPermissions(
                (context as Activity?)!!,
                arrayOf(Manifest.permission.WRITE_EXTERNAL_STORAGE),
                1
            )
        }
    }
}
