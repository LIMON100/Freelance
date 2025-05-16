package com.axon.kisa10.distributor

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Bundle
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.databinding.DataBindingUtil
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentManager
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.activity.SearchDeviceActivity
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.ble.ServiceConnector
import com.axon.kisa10.fragment.CalibrationFragment
import com.axon.kisa10.model.distributor.calibration.DistributorCalibrationUploadRequest
import com.axon.kisa10.util.AppConstants
import com.kisa10.R
import com.kisa10.databinding.ActivityMainDistributorBinding

class MainDistributorActivity : AppCompatActivity() {

    companion object {
        const val ACTION_CALIBRATION_FRAGMENT = "ACTION_CALIBRATION_FRAGMENT"
        const val ACTION_CALIBRATION_START_FRAGMENT = "ACTION_CALIBRATION_START_FRAGMENT"
        const val ACTION_CALIBRATION_SUCCESS = "ACTION_CALIBRATION_SUCCESS"
        const val CALIBRATION_EXTRA_DATA = "CALIBRATION_EXTRA_DATA"
        const val ACTION_LOCATION_PROVINCE = "ACTION_LOCATION_PROVINCE"
        const val ACTION_LOCATION_COUNTY = "ACTION_LOCATION_COUNTY"
        const val EXTRA_DATA_PROVINCE = "EXTRA_DATA_PROVINCE"
        const val EXTRA_DATA_COUNTY = "EXTRA_DATA_COUNTY"
        const val LOCATION_SELECTION_COMPLETE = "LOCATION_SELECTION_COMPLETE"
    }

    private lateinit var binding : ActivityMainDistributorBinding

    private lateinit var localBroadcastManager: LocalBroadcastManager

    private lateinit var fragmentManager: FragmentManager

    private var bleService : AxonBLEService? = null

    private val serviceConnector = ServiceConnector(
        { service ->
            bleService = service
            binding.ivConnectStatus.setImageResource(R.drawable.ic_ovel_connected)
        },
        { msg ->
            Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
            binding.ivConnectStatus.setImageResource(R.drawable.ic_ovel_disconnected)
            finish()
        }
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = DataBindingUtil.setContentView(this, R.layout.activity_main_distributor)

        binding.imgBack.setOnClickListener {
            onBackPressed()
        }

        localBroadcastManager = LocalBroadcastManager.getInstance(this)
        val intentFilter = IntentFilter()
        intentFilter.addAction(ACTION_CALIBRATION_FRAGMENT)
        intentFilter.addAction(ACTION_CALIBRATION_START_FRAGMENT)
        intentFilter.addAction(ACTION_CALIBRATION_SUCCESS)
        intentFilter.addAction(AppConstants.ACTION_DEVICE_DISCONNECTED)
        intentFilter.addAction(ACTION_LOCATION_PROVINCE)
        intentFilter.addAction(ACTION_LOCATION_COUNTY)
        intentFilter.addAction(LOCATION_SELECTION_COMPLETE)

        localBroadcastManager.registerReceiver(navigationListener, intentFilter)

        val serviceIntent = Intent(this, AxonBLEService::class.java)
        bindService(serviceIntent, serviceConnector, Context.BIND_AUTO_CREATE)

        fragmentManager = supportFragmentManager

        addFragment(VehicleInfoFragment(),VehicleInfoFragment::class.java.name, AppConstants.Anim.NONE)
    }

    private val navigationListener = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (intent.action == ACTION_LOCATION_PROVINCE) {
                val locations = intent.extras?.getStringArray(EXTRA_DATA_PROVINCE) ?: emptyArray()
                addFragment(LocationFragment(locations.toList(), LocationFragment.Companion.LOCATION_TYPE.PROVINCE), ACTION_LOCATION_PROVINCE, AppConstants.Anim.NONE)
            }

            if (intent.action == ACTION_LOCATION_COUNTY) {
                val selectedProvince = intent.extras?.getString(EXTRA_DATA_PROVINCE) ?: ""
                val cities = when (selectedProvince) {
                    getString(R.string.Seoul_special_city) -> {
                        resources.getStringArray(R.array.Seoul_special_city)
                    }

                    getString(R.string.Busan_Metropolitan_City) -> {
                        resources.getStringArray(R.array.Busan_Metropolitan_City)
                    }

                    getString(R.string.Daegu_Metropolitan_City) -> {
                        resources.getStringArray(R.array.Daegu_Metropolitan_City)
                    }

                    getString(R.string.Incheon_Metropolitan_City) -> {
                        resources.getStringArray(R.array.Incheon_Metropolitan_City)
                    }

                    getString(R.string.Gwangju_Metropolitan_City) -> {
                        resources.getStringArray(R.array.Gwangju_Metropolitan_City)
                    }

                    getString(R.string.Ulsan_Metropolitan_City) -> {
                        resources.getStringArray(R.array.Ulsan_Metropolitan_City)
                    }

                    getString(R.string.gyeonggi_do) -> {
                        resources.getStringArray(R.array.gyeonggi_do)
                    }

                    getString(R.string.Gangwon_do) -> {
                        resources.getStringArray(R.array.Gangwon_do)
                    }

                    getString(R.string.Chungcheongbuk_do) -> {
                        resources.getStringArray(R.array.Chungcheongbuk_do)
                    }

                    getString(R.string.Chungcheongnam_do) -> {
                        resources.getStringArray(R.array.Chungcheongnam_do)
                    }

                    getString(R.string.Jeollabuk_do) -> {
                        resources.getStringArray(R.array.Jeollabuk_do)
                    }

                    getString(R.string.Jeollanam_do) -> {
                        resources.getStringArray(R.array.Jeollanam_do)
                    }

                    getString(R.string.Gyeongsangbuk_do) -> {
                        resources.getStringArray(R.array.Gyeongsangbuk_do)
                    }

                    getString(R.string.Gyeongsangnam_do) -> {
                        resources.getStringArray(R.array.Gyeongsangnam_do)
                    }

                    getString(R.string.Jeju_Special_Self_Governing_Province) -> {
                        resources.getStringArray(R.array.Jeju_Special_Self_Governing_Province)
                    }

                    else -> {
                        emptyArray<String>()
                    }
                }
                val args = Bundle()
                args.putString(EXTRA_DATA_PROVINCE, intent.extras?.getString(EXTRA_DATA_PROVINCE))
                addFragment(LocationFragment(cities.toList(), LocationFragment.Companion.LOCATION_TYPE.CITY), ACTION_LOCATION_COUNTY, AppConstants.Anim.NONE, args)
            }

            if (intent.action == LOCATION_SELECTION_COMPLETE) {
                popFragmentUntil(VehicleInfoFragment::class.java.name)
                val vehicleInfoFragment = supportFragmentManager.findFragmentByTag(VehicleInfoFragment::class.java.name) as? VehicleInfoFragment
                val province = intent.getStringExtra(EXTRA_DATA_PROVINCE)
                val city = intent.getStringExtra(EXTRA_DATA_COUNTY)
                vehicleInfoFragment?.setLocation("$city, $province")
            }

            if (intent.action == ACTION_CALIBRATION_START_FRAGMENT) {
                if (bleService == null) {
                    Toast.makeText(this@MainDistributorActivity, getString(R.string.device_disconnected), Toast.LENGTH_SHORT).show()
                    finish()
                } else {
                    val distributorCalibrationUploadRequest = intent.getSerializableExtra(CALIBRATION_EXTRA_DATA) as? DistributorCalibrationUploadRequest
                    val bundle = Bundle()
                    bundle.putSerializable(CALIBRATION_EXTRA_DATA, distributorCalibrationUploadRequest)
                    val calibrationStartFragment = CalibrationStartFragment(bleService!!)
                    addFragment(calibrationStartFragment, ACTION_CALIBRATION_START_FRAGMENT, AppConstants.Anim.NONE, bundle)
                }
            }

            if (intent.action == ACTION_CALIBRATION_FRAGMENT) {
                popBackStack()
                // start calibration process
                if (bleService == null) {
                    Toast.makeText(this@MainDistributorActivity, getString(R.string.device_disconnected), Toast.LENGTH_SHORT).show()
                    finish()
                } else {
                    val distributorCalibrationUploadRequest = intent.getSerializableExtra(CALIBRATION_EXTRA_DATA) as? DistributorCalibrationUploadRequest
                    val bundle = Bundle()
                    bundle.putSerializable(CALIBRATION_EXTRA_DATA, distributorCalibrationUploadRequest)
                    addFragment(
                        CalibrationFragment(bleService),
                        CalibrationFragment::class.java.name,
                        AppConstants.Anim.NONE,
                        bundle
                    )
                }
            }

            if (intent.action == AppConstants.ACTION_DEVICE_DISCONNECTED) {
                binding.ivConnectStatus.setImageResource(R.drawable.ic_ovel_disconnected)
                Toast.makeText(this@MainDistributorActivity, getString(R.string.device_disconnected), Toast.LENGTH_SHORT).show()
                val newIntent = Intent(this@MainDistributorActivity, SearchDeviceActivity::class.java)
                this@MainDistributorActivity.startActivity(newIntent)
                finish()
            }

            if (intent.action == ACTION_CALIBRATION_SUCCESS) {
                finish()
            }
        }

    }

    private fun popFragmentUntil(tag: String) {
        var backStackEntryCount = supportFragmentManager.backStackEntryCount
        while (backStackEntryCount > 1) {
            val entry = supportFragmentManager.getBackStackEntryAt(backStackEntryCount-1)
            if (entry.name != tag) {
                supportFragmentManager.popBackStackImmediate()
            } else {
                break
            }
            backStackEntryCount -= 1
        }
    }

    fun addFragment(fragment: Fragment, tag: String?, animation: Int, args : Bundle? = null) {
        if (supportFragmentManager.isDestroyed) {
            return
        }

        if (args != null) {
            fragment.arguments = args
        }

        val transaction = supportFragmentManager.beginTransaction()
        when (animation) {
            AppConstants.Anim.SLIDING -> transaction.setCustomAnimations(
                R.anim.sliding_left,
                R.anim.sliding_right,
                R.anim.sliding_left,
                R.anim.sliding_right
            )

            AppConstants.Anim.FADE -> transaction.setCustomAnimations(
                R.anim.fade_in,
                R.anim.fade_out,
                R.anim.fade_in,
                R.anim.fade_out
            )
        }
        transaction.replace(R.id.frame_Layout, fragment, tag)
        transaction.addToBackStack(tag)
        transaction.commit()
    }

    override fun onBackPressed() {
        val numOfFragments = fragmentManager.backStackEntryCount
        if (numOfFragments > 1) {
            fragmentManager.popBackStackImmediate()
        } else {
            finish()
        }
    }

    private fun popBackStack() {
        val numOfFragments = fragmentManager.backStackEntryCount
        if (numOfFragments > 1) {
            fragmentManager.popBackStackImmediate()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        unbindService(serviceConnector)
        localBroadcastManager.unregisterReceiver(navigationListener)
    }
}