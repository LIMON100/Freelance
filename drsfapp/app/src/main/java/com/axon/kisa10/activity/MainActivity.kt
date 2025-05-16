package com.axon.kisa10.activity

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Bundle
import android.preference.PreferenceManager
import android.util.Log
import android.view.View
import android.widget.FrameLayout
import android.widget.ImageView
import android.widget.RelativeLayout
import android.widget.TextView
import android.widget.Toast
import androidx.activity.viewModels
import androidx.appcompat.app.AlertDialog
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentManager
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import androidx.work.Constraints
import androidx.work.ExistingPeriodicWorkPolicy
import androidx.work.NetworkType
import androidx.work.PeriodicWorkRequestBuilder
import androidx.work.WorkManager
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.ble.ServiceConnector
import com.axon.kisa10.data.CalibrationHistoryDb
import com.axon.kisa10.data.SdiDataLogger
import com.axon.kisa10.distributor.CalibrationHistoryFragment
import com.axon.kisa10.distributor.DeviceSettingFragment
import com.axon.kisa10.distributor.LoginDistributorActivity
import com.axon.kisa10.distributor.MainDistributorActivity
import com.axon.kisa10.distributor.OBDFragment
import com.axon.kisa10.fragment.CalibrationFragment
import com.axon.kisa10.fragment.ChangeLanguageFragment
import com.axon.kisa10.fragment.CustomDialogCalibration
import com.axon.kisa10.fragment.CustomDialogFactoryReset
import com.axon.kisa10.fragment.DBUpdateFragment
import com.axon.kisa10.fragment.DBUpdateMainFragment
import com.axon.kisa10.fragment.DeviceFragment
import com.axon.kisa10.fragment.DrawerFragment
import com.axon.kisa10.fragment.FactoryResetFragment
import com.axon.kisa10.fragment.FirmwareUpdateFragment
import com.axon.kisa10.fragment.FirmwareUpdateMainFragment
import com.axon.kisa10.fragment.MapFragment
import com.axon.kisa10.fragment.MapFragmentProd
import com.axon.kisa10.fragment.OBDSettingFragment
import com.axon.kisa10.fragment.log.DataMenuFragment
import com.axon.kisa10.fragment.log.EventListFragment
import com.axon.kisa10.fragment.log.EventViewFragment
import com.axon.kisa10.fragment.log.LogListFragment
import com.axon.kisa10.fragment.log.LogMapViewFragment
import com.axon.kisa10.fragment.log.LogViewFragment
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.IBackPressed
import com.axon.kisa10.util.SharedPrefKeys
import com.axon.kisa10.util.SharedPrefManager
import com.axon.kisa10.viewmodel.CalibrationViewModel
import com.axon.kisa10.viewmodel.RegisterViewModel
import com.axon.kisa10.workmanager.FileUploadWorker
import com.google.gson.Gson
import com.kisa10.BuildConfig
import com.kisa10.R
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import org.osmdroid.config.Configuration
import java.util.concurrent.TimeUnit

class MainActivity : BaseActivity(), View.OnClickListener {
    private lateinit var rlMain: RelativeLayout
    private lateinit var tvTitle: TextView
    private lateinit var ivMainTitle: ImageView
    private lateinit var ivBack: ImageView
    private val FIRMWAREUPDATE_FRAGMENT_TAG: String = "FIRMWAREUPDATE_FRAGMENT_TAG"
    private val FACTORY_RESET_FRAGMENT_TAG: String = "FACTORY_RESET_FRAGMENT_TAG"
    private val DEVICE_FRAGMENT_TAG: String = "DEVICE_FRAGMENT_TAG"
    private val OBDSET_FRAGMENT_TAG: String = "OBDSET_FRAGMENT_TAG"
    private val CALIBRATION_FRAGMENT_TAG: String = "CALIBRATION_FRAGMENT_TAG"
    private val MAPVIEW_FRAGMENT_TAG: String = "MAPVIEW_FRAGMENT_TAG"
    private val DRAWER_FRAGMENT_TAG: String = "DRAWER_FRAGMENT_TAG"
    private val ChangeLanguage_Fragment_TAG: String = "ChangeLanguage_Fragment_TAG"
    private val DATA_UPDATE_MENU_TAG = "DATA_UPDATE_MENU_TAG"

    private val MENU_TAG = "MENU_TAG"
    private val BACK_TAG = "BACK_TAG"

    private var isGroup = true

    private lateinit var localBroadcastManager: LocalBroadcastManager

    private lateinit var fragmentManager: FragmentManager

    private var stateChanged = false

    private var bleService : AxonBLEService? = null

    private var isSafeDrivingStarted = false

    private val serviceConnector = ServiceConnector(
        { service ->
            this.bleService = service
            upDeviceConnectedIcon()
            if (!service.isConnected()) {
                gotoSearchActivity()
            }
        },{ msg ->
            Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
            finish()
        })

    private val registerViewmodel by viewModels<RegisterViewModel>()

    private val calibrationViewModel by viewModels<CalibrationViewModel>()

    private val sharedPrefManager by lazy { SharedPrefManager.getInstance(this) }

    private var shouldUploadCalibrationValues = false

    private val calibrationDb by lazy {
        CalibrationHistoryDb.getDb(applicationContext).calibrationDao()
    }

    private val ioScope = CoroutineScope(Dispatchers.IO)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (!BuildConfig.IS_DISTRIBUTOR) {
            SdiDataLogger.getInstance(this) // init
        }
        stateChanged = savedInstanceState?.getBoolean(CHANGE_CONFIGURATION) ?: false

        // load osm android map cache files
        Configuration.getInstance().load(applicationContext, PreferenceManager.getDefaultSharedPreferences(applicationContext))

        setContentView(R.layout.activity_main)
        val intent = Intent(this, AxonBLEService::class.java)
        bindService(intent, serviceConnector, Context.BIND_AUTO_CREATE)

        localBroadcastManager = LocalBroadcastManager.getInstance(this)
        fragmentManager = supportFragmentManager
        localBroadcastManager.registerReceiver(
            mGattUpdateReceiver,
            AppConstants.makeIntentFilter()
        )
        if (stateChanged) {
            popAllFragments()
        }
        initUi()
        scheduleWorker()
        registerViewmodel.getUserInfo()
        registerViewmodel.userInfo.observe(this) { response ->
            if (response != null) {
                val gson = Gson()
                val responseString = gson.toJson(response)
                sharedPrefManager.setString(SharedPrefKeys.USER_INFO,responseString)
            }
        }

        calibrationViewModel.success.observe(this) { event ->
            event?.getContentIfNotHandled()?.let { response ->
                Log.i("Calibration", "success : $response")
            }
        }

        calibrationViewModel.error.observe(this) { event ->
            event?.getContentIfNotHandled()?.let { response ->
                Toast.makeText(this, response, Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun scheduleWorker() {
        val workManager = WorkManager.getInstance(this)
        val workRequest = PeriodicWorkRequestBuilder<FileUploadWorker>(1, TimeUnit.DAYS)
            .setConstraints(Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build())
            .build()
        workManager.enqueueUniquePeriodicWork("FileUploadWork",ExistingPeriodicWorkPolicy.KEEP, workRequest)
    }

    override fun onPause() {
        super.onPause()
        if (bleService?.isConnected() == true && isSafeDrivingStarted) {
            localBroadcastManager.sendBroadcast(Intent(AxonBLEService.ACTION_START_BACKGROUND_DRIVING))
        }
    }

    override fun onResume() {
        super.onResume()
        if (BuildConfig.IS_TESTING) {
            localBroadcastManager.sendBroadcast(Intent(AxonBLEService.ACTION_STOP_BACKGROUND_DRIVING))
        }
    }

    private fun initUi() {
        //ll_vinCode = findViewById(R.id.ll_vinCode);
        val sharedPref = this.getPreferences(MODE_PRIVATE)
        val editor = sharedPref.edit()
        editor.putString("MODE_ONOFF", "MODE_OFF")
        editor.apply()
        rlMain = findViewById(R.id.rl_main)
        tvTitle = findViewById(R.id.tv_title)
        ivMainTitle = findViewById(R.id.iv_main_title)
        ivBack = findViewById(R.id.iv_back)
        ivBack.tag = MENU_TAG
//        ivConnectStatus = findViewById(R.id.iv_connectStatus)

        tvTitle.text = "Home"

        //        tv_version_info = findViewById(R.id.tv_version_info);
//        tv_version_info.setText(getString(R.string.version_info)+" : "+BuildConfig.VERSION_NAME);

        //ll_vinCode.setOnClickListener(this);
        rlMain.setOnClickListener(this)
        ivBack.setOnClickListener(this)
        val argsIntentFilter = IntentFilter()
        argsIntentFilter.addAction(AppConstants.BROADCAST_CHILD_FRAGMENT)
        argsIntentFilter.addAction(SAFE_DRIVING_STARTED)
        localBroadcastManager.registerReceiver(mBroadcastReceiver,argsIntentFilter)

        val intentFilter = IntentFilter()
        intentFilter.addAction(ACTION_BLE_FIRMWARE_UPDATE_FRAGMENT)
        intentFilter.addAction(ACTION_CALIBRATION_FRAGMENT)
        intentFilter.addAction(ACTION_DB_UPDATE_FRAGMENT)
        intentFilter.addAction(ACTION_DB_UPDATE_MAIN_FRAGMENT)
        intentFilter.addAction(ACTION_DEVICE_FRAGMENT)
        intentFilter.addAction(ACTION_CALIBRATION_HISTORY)
        intentFilter.addAction(ACTION_FIRMWARE_UPDATE_FRAGMENT)
        intentFilter.addAction(ACTION_FACTORY_RESET_FRAGMENT)
        intentFilter.addAction(ACTION_USERID_RESET_FRAGMENT)
        intentFilter.addAction(ACTION_OBD_SETTINGS_FRAGMENT)
        intentFilter.addAction(ACTION_CHANGE_LANGUAGE)
        intentFilter.addAction(FIRMWARE_UPDATE_CLICK)
        intentFilter.addAction(FIRMWARE_UPDATE_OK)
        intentFilter.addAction(ACTION_DATA_UPDATE_FRAGMENT)
        intentFilter.addAction(ACTION_LOG_UPDATE_FRAGMENT)
        intentFilter.addAction(ACTION_EVENT_UPDATE_FRAGMENT)
        intentFilter.addAction(ACTION_LOG_VIEW_FRAGMENT)
        intentFilter.addAction(ACTION_EVENT_VIEW_FRAGMENT)
        intentFilter.addAction(ACTION_LOG_MAP_VIEW_FRAGMENT)
        intentFilter.addAction(ACTION_CALIBRATION_SUCCESS)
        intentFilter.addAction(ACTION_LOGOUT)

        localBroadcastManager.registerReceiver(drawerReceiver, intentFilter)

        val isFirmwareUpdate = intent?.extras?.getBoolean(AppConstants.EVENT_UPDATE_FIRMWARE) ?: false
        if (isFirmwareUpdate) {
            localBroadcastManager.sendBroadcast(Intent(ACTION_FIRMWARE_UPDATE_FRAGMENT))
            setBackIcon()
        } else {
//            setMapView()
            setDrawerFragment()
        }
    }

    private fun setDrawerFragment() {
        addFragment(
            DrawerFragment.newInstance(),
            DRAWER_FRAGMENT_TAG,
            AppConstants.Anim.NONE
        )
        hideBackIcon()
    }

    private fun hideBackIcon() {
        ivBack.visibility = View.INVISIBLE
    }

    private val drawerReceiver: BroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {

            if (intent.action == ACTION_CALIBRATION_HISTORY) {
                addFragment(
                    CalibrationHistoryFragment(),
                    ACTION_CALIBRATION_HISTORY,
                    AppConstants.Anim.NONE
                )
                updateToolbar("Calibration Logs")
            }

            if (intent.action == ACTION_LOGOUT) {
                AlertDialog.Builder(this@MainActivity)
                    .setTitle(getString(R.string.logout))
                    .setMessage(R.string.logout_msg)
                    .setPositiveButton(getString(R.string.ok), { dialog, _ ->
                        sharedPrefManager.setString(SharedPrefKeys.DISTRIBUTOR_TOKEN, null)
                        sharedPrefManager.setString(SharedPrefKeys.DISTRIBUTOR_USER_INFO, null)
                        ioScope.launch {
                            calibrationDb.deleteAll()
                        }
                        bleService?.disconnect()
                        dialog.dismiss()
                    })
                    .setNegativeButton(getString(R.string.cancel), { dialog, _ ->
                        dialog.dismiss()
                    })
                    .show()
            }

            if (intent.action == ACTION_CALIBRATION_FRAGMENT) {
                if (BuildConfig.IS_DISTRIBUTOR) {
                    val mainDistributorActivity = Intent(this@MainActivity, MainDistributorActivity::class.java)
                    startActivity(mainDistributorActivity)
                } else {
                    shouldUploadCalibrationValues = true
                    if (bleService == null) {
                        Toast.makeText(this@MainActivity, getString(R.string.device_disconnected), Toast.LENGTH_SHORT).show()
                        finish()
                    } else {
                        isGroup = true
                        ivMainTitle.visibility = View.INVISIBLE
                        updateToolbar(getString(R.string.calibration))
                        rlMain.setBackgroundColor(resources.getColor(R.color.bg_main))
                        val calibrationDialog = CustomDialogCalibration(
                            this@MainActivity,
                            object : CustomDialogCalibration.IOnClickConfirmListener {
                                override fun onConfirm() {
                                    addFragment(
                                        CalibrationFragment.newInstance(),
                                        CALIBRATION_FRAGMENT_TAG,
                                        AppConstants.Anim.NONE
                                    )
                                }

                                override fun onCancel() {
                                }
                            }, bleService!!
                        )
                        if (this@MainActivity.window.isActive) {
                            calibrationDialog.show()
                        }
                    }
                }
            } else {
                shouldUploadCalibrationValues = false
            }

            if (intent.action == ACTION_CALIBRATION_SUCCESS) {
                bleService?.readDeviceData(AppConstants.COMMAND_DEVICE_CALIBRATION_READ + ",2;")
            }

            if (intent.action == ACTION_DEVICE_FRAGMENT) {
                if (bleService == null) {
                    Toast.makeText(this@MainActivity, getString(R.string.device_disconnected), Toast.LENGTH_SHORT).show()
                    finish()
                } else {
                    isGroup = true
                    addFragment(
                        DeviceSettingFragment(bleService!!),
                        DEVICE_FRAGMENT_TAG,
                        AppConstants.Anim.NONE
                    )
                    ivMainTitle.visibility = View.INVISIBLE
                    updateToolbar(getString(R.string.device))
                }
            }

            if (intent.action == ACTION_FIRMWARE_UPDATE_FRAGMENT) {
                isGroup = true
                addFragment(
                    FirmwareUpdateMainFragment(),
                    FIRMWAREUPDATE_FRAGMENT_TAG,
                    AppConstants.Anim.NONE
                )
                ivMainTitle.visibility = View.INVISIBLE
                updateToolbar(getString(R.string.firmware_update))
            }

            if (intent.action == ACTION_FACTORY_RESET_FRAGMENT) {
                if (bleService == null) {
                    Toast.makeText(this@MainActivity, getString(R.string.device_disconnected), Toast.LENGTH_SHORT).show()
                    finish()
                } else {
                    isGroup = true
                    ivMainTitle.visibility = View.INVISIBLE
                    updateToolbar(getString(R.string.factory_reset))
                    rlMain.setBackgroundColor(resources.getColor(R.color.bg_main))
                    val dialog = CustomDialogFactoryReset(
                        this@MainActivity,
                        object : CustomDialogFactoryReset.IOnClickConfirmListener {
                            override fun onConfirm() {
                                addFragment(
                                    FactoryResetFragment(bleService!!),
                                    FACTORY_RESET_FRAGMENT_TAG,
                                    AppConstants.Anim.NONE
                                )
                            }

                            override fun onCancel() {
                            }
                        },bleService!!)
                    dialog.show()
                }
            }

            if (intent.action == ACTION_OBD_SETTINGS_FRAGMENT) {
                if (bleService == null) {
                    Toast.makeText(this@MainActivity, getString(R.string.device_disconnected), Toast.LENGTH_SHORT).show()
                    finish()
                } else {
                    isGroup = true
                    addFragment(
                        OBDFragment(bleService!!),
                        OBDSET_FRAGMENT_TAG,
                        AppConstants.Anim.NONE
                    )
                    ivMainTitle.visibility = View.INVISIBLE
                    updateToolbar(getString(R.string.obd_setting))
                }
            }

            if (intent.action == ACTION_CHANGE_LANGUAGE) {
                addFragment(
                    ChangeLanguageFragment(),
                    ChangeLanguage_Fragment_TAG,
                    AppConstants.Anim.NONE
                )
                ivMainTitle.visibility = View.INVISIBLE
                updateToolbar(getString(R.string.change_language))
            }

            if (intent.action == FIRMWARE_UPDATE_CLICK) {
                if (bleService == null) {
                    Toast.makeText(this@MainActivity, getString(R.string.device_disconnected), Toast.LENGTH_SHORT).show()
                    finish()
                } else {
                    if (!fragmentManager.isDestroyed) {
                        addfragment2(FirmwareUpdateFragment(bleService!!), R.id.frame_Layout)
                    }
                }
            }

            if (intent.action == FIRMWARE_UPDATE_OK) {
                bleService?.disconnect()
                startActivity(Intent(this@MainActivity, SearchDeviceActivity::class.java))
                finish()
            }

            if (intent.action == ACTION_DATA_UPDATE_FRAGMENT) {
                isGroup = true
                addFragment(
                    DataMenuFragment(),
                    ACTION_DATA_UPDATE_FRAGMENT,
                    AppConstants.Anim.NONE
                )
                ivMainTitle.visibility = View.INVISIBLE
                updateToolbar(getString(R.string.data_update))
            }

            if (intent.action == ACTION_LOG_UPDATE_FRAGMENT) {
                if (bleService == null) {
                    Toast.makeText(this@MainActivity, getString(R.string.device_disconnected), Toast.LENGTH_SHORT).show()
                    finish()
                } else {
                    isGroup = true
                    addFragment(
                        LogListFragment(bleService!!),
                        ACTION_LOG_UPDATE_FRAGMENT,
                        AppConstants.Anim.NONE
                    )
                    ivMainTitle.visibility = View.INVISIBLE
                    updateToolbar(getString(R.string.log_update))
                }
            }

            if (intent.action == ACTION_EVENT_UPDATE_FRAGMENT) {
                if (bleService == null) {
                    Toast.makeText(this@MainActivity, getString(R.string.device_disconnected), Toast.LENGTH_SHORT).show()
                    finish()
                } else {
                    isGroup = true
                    addFragment(
                        EventListFragment(bleService!!),
                        ACTION_EVENT_UPDATE_FRAGMENT,
                        AppConstants.Anim.NONE
                    )
                    ivMainTitle.visibility = View.INVISIBLE
                    updateToolbar(getString(R.string.event_update))
                }
            }

            if (intent.action == ACTION_LOG_VIEW_FRAGMENT) {
                isGroup = true
                addFragment(
                    LogViewFragment(bleService!!),
                    ACTION_LOG_VIEW_FRAGMENT,
                    AppConstants.Anim.NONE
                )
                ivMainTitle.visibility = View.INVISIBLE
                updateToolbar(getString(R.string.log_view))
            }

            if (intent.action == ACTION_EVENT_VIEW_FRAGMENT) {
                isGroup = true
                addFragment(
                    EventViewFragment(),
                    ACTION_EVENT_VIEW_FRAGMENT,
                    AppConstants.Anim.NONE
                )
                ivMainTitle.visibility = View.INVISIBLE
                updateToolbar(getString(R.string.event_view))
            }

            if (intent.action == ACTION_LOG_MAP_VIEW_FRAGMENT) {
                isGroup = true
                val fileName = intent.getStringExtra(AppConstants.KEY_LOG_FILE) ?: ""
                addFragment(
                    LogMapViewFragment().apply {
                        arguments = Bundle().apply {
                            putString(AppConstants.KEY_LOG_FILE, fileName)
                        }
                    },
                    ACTION_LOG_MAP_VIEW_FRAGMENT,
                    AppConstants.Anim.NONE
                )
                updateToolbar(getString(R.string.log_map_view))
            }

            if (intent.action == ACTION_DB_UPDATE_FRAGMENT) {
                isGroup = true
                val bundle = Bundle().apply {
                    putString(AppConstants.DB_VERSION, intent.getStringExtra(AppConstants.DB_VERSION))
                    putString(AppConstants.FIRMWARE_FILE_NAME, intent.getStringExtra(AppConstants.FIRMWARE_FILE_NAME))
                }
                addFragment(
                    DBUpdateFragment(bleService!!).apply {
                        arguments = bundle
                    },
                    ACTION_DB_UPDATE_FRAGMENT,
                    AppConstants.Anim.NONE
                )
                updateToolbar(getString(R.string.db_update))
            }

            if (intent.action == ACTION_DB_UPDATE_MAIN_FRAGMENT) {
                isGroup = true
                addFragment(
                    DBUpdateMainFragment(bleService!!),
                    ACTION_DB_UPDATE_MAIN_FRAGMENT,
                    AppConstants.Anim.NONE
                )
                updateToolbar(getString(R.string.db_update))
            }
        }
    }

    private val mBroadcastReceiver: BroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (intent.action == AppConstants.BROADCAST_CHILD_FRAGMENT) {
                isGroup = intent.getBooleanExtra(AppConstants.EXTRA_SETTING, true)
            }

            if (intent.action == SAFE_DRIVING_STARTED) {
                isSafeDrivingStarted = intent.getBooleanExtra(AppConstants.EXTRA_SETTING, true)
            }
        }
    }

    override fun onClick(view: View) {
        when (view.id) {
            R.id.iv_back -> {
                onBackPressed()
//                val isDrawer = ivBack.tag === MENU_TAG
//                if (isDrawer) {
//                    addFragment(
//                        DrawerFragment.newInstance(),
//                        DRAWER_FRAGMENT_TAG,
//                        AppConstants.Anim.NONE
//                    )
//                    setBackIcon()
//                } else {
//                    //업데이트 중 뒤로가기 < 버튼 동작 막기
//                    ivConnectStatus.visibility = View.VISIBLE
//                    if (isGroup) {
//                        onBackPressed()
//                        //                    iv_main_title.setVisibility(View.INVISIBLE);
////                    closeDrawerText(getString(R.string.setting));
//                    } else {
//                        Toast.makeText(this, "업데이트 중에는 중지할 수 없습니다.", Toast.LENGTH_SHORT).show()
//                    }
//                }
            }
        }
    }

    private fun setBackIcon() {
        ivBack.setImageResource(R.drawable.ic_back)
        ivBack.tag = BACK_TAG
    }

    fun addFragment(fragment: Fragment, tag: String?, animation: Int, args : Bundle? = null) {
        if (supportFragmentManager.isDestroyed) {
            return
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

    private fun popFragmentFromStack(fragmentTag: String) {
        val numFragments = fragmentManager.backStackEntryCount
        if (numFragments >= 1) {
            val topFragmentEntry = fragmentManager.getBackStackEntryAt(numFragments - 1)
            val topFragmentTag = topFragmentEntry.name
            if (topFragmentTag == fragmentTag) {
                fragmentManager.popBackStackImmediate()
            }
        }
    }

    private fun popAllFragments() {
        while (fragmentManager.backStackEntryCount > 0) {
            fragmentManager.popBackStackImmediate()
        }
    }

    private fun addfragment2(fragment: Fragment, id: Int) {
        //하위 뷰 중복 터치 방지
        clearBackStack()
        val lay = findViewById<FrameLayout>(R.id.frame_Layout)
        lay.removeAllViews() //뒷 뷰 다 날리기
        supportFragmentManager.beginTransaction()
            .replace(R.id.frame_Layout, fragment)
            .addToBackStack(null)
            .commit()
    }

    private fun updateToolbar(string: String) {
        ivMainTitle.visibility = View.INVISIBLE
        tvTitle.visibility = View.VISIBLE
        ivBack.visibility = View.VISIBLE
        setBackIcon()
        val normalBgFrags = listOf(getString(R.string.data_update), getString(R.string.log_update), getString(R.string.event_update), getString(R.string.log_view), getString(R.string.event_view),getString(R.string.log_map_view))
        if (string in normalBgFrags) {
            rlMain.setBackgroundResource(R.drawable.bg_top_normal)
        } else {
            rlMain.setBackgroundResource(R.drawable.bg_top_nav)
        }
        tvTitle.text = string
//        if (string == getString(R.string.ble_firmware_update)) {
////            ivConnectStatus.visibility = View.GONE
//        } else {
////            ivConnectStatus.visibility = View.VISIBLE
//        }
        upDeviceConnectedIcon()
    }

    private fun clearBackStack() {
        val fragmentManager = supportFragmentManager
        for (i in 0 until fragmentManager.backStackEntryCount) {
            fragmentManager.popBackStackImmediate(null, FragmentManager.POP_BACK_STACK_INCLUSIVE)
        }
    }

//    fun OnFragmentLayoutClick(layout: String?) {
//        /*LayoutInflater inflater = (LayoutInflater)getSystemService(Context.LAYOUT_INFLATER_SERVICE);
//        View view = inflater.inflate(R.layout.activity_main, null);
//        FrameLayout f = view.findViewById(R.id.frame_Layout);*/
//        when (layout) {
//            AppConstants.firmware_update_click -> addfragment2(
//                FirmwareUpdateFragment.newInstance(),
//                R.id.frame_Layout
//            )
//
//            AppConstants.database_update_click -> addfragment2(
//                DatabaseUpdateFragment.newInstance(),
//                R.id.frame_Layout
//            )
//
//            AppConstants.ble_upgrade_click -> addfragment2(
//                BleUpgradeFragment.newInstance(),
//                R.id.frame_Layout
//            )
//
//            AppConstants.firmware_update_ok -> finish()
//            AppConstants.factory_reset_click, AppConstants.calibartion_click -> {}
//            else -> {}
//        }
//    }

    interface onKeyBackPressedListener {
        fun onBackKey()
    }

    private var mOnKeyBackPressedListener: onKeyBackPressedListener? = null

    fun setOnKeyBackPressedListener(listener: onKeyBackPressedListener?) {
        mOnKeyBackPressedListener = listener
    }


    override fun onBackPressed() {
        val numOfFragments = fragmentManager.backStackEntryCount
        val isFirmwareUpdatePushNotification = intent.extras?.getBoolean(AppConstants.EVENT_UPDATE_FIRMWARE) ?: false
        if (numOfFragments > 1) {
            val topFragmentEntry = fragmentManager.getBackStackEntryAt(numOfFragments - 1)
            val topFragmentTag = topFragmentEntry.name
            val topFragment = fragmentManager.findFragmentByTag(topFragmentTag)
            var shouldPopFragment = true
            if (topFragment is IBackPressed) {
                shouldPopFragment = (topFragment as IBackPressed).onBackPressed()
            }

            if (!shouldPopFragment) {
                return
            }

            fragmentManager.popBackStackImmediate()

            val newBackStack = fragmentManager.getBackStackEntryAt(
                fragmentManager.backStackEntryCount - 1
            )
            val newTopTag = newBackStack.name
            if (newTopTag == DRAWER_FRAGMENT_TAG) {
                restoreToolbar()
            } else if (newTopTag == ACTION_DATA_UPDATE_FRAGMENT) {
                ivMainTitle.visibility = View.INVISIBLE
                updateToolbar(getString(R.string.data_update))
            }
        } else if (isFirmwareUpdatePushNotification) {
            clearBackStack()
            setMapView()
            restoreToolbar()
            setMenuIcon()
        } else {
            finish()
        }


        //        FragmentManager.BackStackEntry backStackEntry = getSupportFragmentManager().getBackStackEntryAt(0);
//        Fragment topFragment = getSupportFragmentManager().findFragmentById(backStackEntry.getId());


//        rl_main.setBackgroundColor(getResources().getColor(R.color.bg_main));
//        if (updating) {
//            if (mOnKeyBackPressedListener != null) {
//                mOnKeyBackPressedListener.onBackKey();
//            } else { //콜백이벤트
//                //updating = true;
//                Toast.makeText(this, getResources().getString(R.string.tv_cannot_stop), Toast.LENGTH_SHORT).show();
//                //super.onBackPressed();
//            }
//        } else {
//            //          tv_title.setText(getString(R.string.setting));
//            tv_title.setVisibility(View.INVISIBLE);
//            setMenuIcon();
//            iv_main_title.setVisibility(View.VISIBLE);
//            super.onBackPressed();
//        }
    }

    private fun restoreToolbar() {
        ivMainTitle.visibility = View.INVISIBLE
        tvTitle.visibility = View.VISIBLE
        rlMain.setBackgroundResource(R.drawable.bg_top_normal)
        tvTitle.text = "Home"
//        ivConnectStatus.visibility = View.VISIBLE
        ivBack.visibility = View.INVISIBLE
        upDeviceConnectedIcon()
        //        setMenuIcon();
    }

    private fun setMapView() {
        if (BuildConfig.IS_TESTING) {
            // set fragment with map visible
            addFragment(MapFragment.newInstance(), MAPVIEW_FRAGMENT_TAG, AppConstants.Anim.NONE)
        } else {
            // set fragment with map removed.
            addFragment(MapFragmentProd.newInstance(), MAPVIEW_FRAGMENT_TAG, AppConstants.Anim.NONE)
        }
    }

    private fun setMenuIcon() {
        ivBack.setImageResource(R.drawable.menu)
        ivBack.tag = MENU_TAG
    }

    private val mGattUpdateReceiver: BroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val action = intent.action
            when (action) {
                AppConstants.ACTION_DEVICE_DISCONNECTED -> {
//                    ivConnectStatus.setImageResource(R.drawable.ic_ovel_disconnected)
                    val isLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.DISTRIBUTOR_TOKEN).isNullOrBlank()
                    if (isLoggedIn) {
                        gotoSearchActivity()
                    } else {
                        startActivity(Intent(this@MainActivity, LoginDistributorActivity::class.java))
                        finish()
                    }
                }
                AppConstants.ACTION_DEVICE_CONNECTED -> {
//                    ivConnectStatus.setImageResource(R.drawable.ic_ovel_connected)
                }
                AppConstants.ACTION_CHARACTERISTIC_CHANGED -> {
                    val data = intent.getStringExtra("data") ?: return
                    if (data.startsWith(AppConstants.COMMAND_RES)) {
                        val responseArray = data.split(",")
                        if (responseArray[1] == AppConstants.COMMAND_DEVICE_CALIBRATION_READ && shouldUploadCalibrationValues) {
                            if (responseArray.size == 10) {
                                val calibrationData = responseArray.subList(4,responseArray.size)
                                Log.i("Calibration", "$calibrationData")
                                val vinCode = sharedPrefManager.getString(SharedPrefKeys.VIN_CODE) ?: ""
                                calibrationViewModel.sendCalibrationData(vinCode, calibrationData)
                            }
                        }
                    }
                }
            }
        }
    }

    private fun unRegisterReceiver() {
        try {
            localBroadcastManager.unregisterReceiver(mBroadcastReceiver)
            localBroadcastManager.unregisterReceiver(mGattUpdateReceiver)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    override fun onDestroy() {
        unRegisterReceiver()
        bleService?.checkAutoPair()
        unbindService(serviceConnector)
        super.onDestroy()
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        if (isChangingConfigurations) {
            outState.putBoolean(CHANGE_CONFIGURATION, true)
        } else {
            outState.remove(CHANGE_CONFIGURATION)
        }
    }

    private fun upDeviceConnectedIcon() {
//        if (bleService?.isConnected() == true) {
//            ivConnectStatus.setImageResource(R.drawable.ic_ovel_connected)
//        } else {
//            ivConnectStatus.setImageResource(R.drawable.ic_ovel_disconnected)
//        }
    }

    private fun gotoSearchActivity() {
        val intent = Intent(this, SearchDeviceActivity::class.java)
        startActivity(intent)
        startActivity(intent)
        finish()
    }


    companion object {
        const val ACTION_LOGOUT = "ACTION_LOGOUT"
        const val CHANGE_CONFIGURATION = "CHANGE_CONFIGURATION"
        const val ACTION_BLE_FIRMWARE_UPDATE_FRAGMENT = "ACTION_BLE_FIRMWARE_UPDATE_FRAGMENT"
        const val ACTION_CALIBRATION_FRAGMENT = "ACTION_CALIBRATION_FRAGMENT"
        const val ACTION_CALIBRATION_SUCCESS = "ACTION_CALIBRATION_SUCCESS"
        const val ACTION_DB_UPDATE_FRAGMENT = "ACTION_DB_UPDATE_FRAGMENT"
        const val ACTION_DB_UPDATE_MAIN_FRAGMENT = "ACTION_DB_UPDATE_MAIN_FRAGMENT"
        const val ACTION_DEVICE_FRAGMENT = "ACTION_DEVICE_FRAGMENT"
        const val ACTION_CALIBRATION_HISTORY = "ACTION_CALIBRATION_HISTORY"
        const val ACTION_FACTORY_RESET_FRAGMENT = "ACTION_FACTORY_RESET_FRAGMENT"
        const val ACTION_OBD_SETTINGS_FRAGMENT= "ACTION_OBD_SETTINGS_FRAGMENT"
        const val ACTION_USERID_RESET_FRAGMENT = "ACTION_USERID_RESET_FRAGMENT"
        const val ACTION_FIRMWARE_UPDATE_FRAGMENT = "ACTION_FIRMWARE_UPDATE_FRAGMENT"
        const val ACTION_DATA_UPDATE_FRAGMENT = "ACTION_DATA_UPDATE_FRAGMENT"
        const val ACTION_LOG_UPDATE_FRAGMENT = "ACTION_LOG_UPDATE_FRAGMENT"
        const val ACTION_LOG_VIEW_FRAGMENT = "ACTION_LOG_VIEW_FRAGMENT"
        const val ACTION_EVENT_VIEW_FRAGMENT = "ACTION_EVENT_VIEW_FRAGMENT"
        const val ACTION_EVENT_UPDATE_FRAGMENT = "ACTION_EVENT_UPDATE_FRAGMENT"
        const val ACTION_LOG_MAP_VIEW_FRAGMENT = "ACTION_LOG_MAP_VIEW_FRAGMENT"
        const val ACTION_CHANGE_LANGUAGE = "ACTION_CHANGE_LANGUAGE"
        const val FIRMWARE_UPDATE_CLICK = "FIRMWARE_UPDATE_CLICK"
        const val FIRMWARE_UPDATE_OK = "FIRMWARE_UPDATE_OK"
        const val SAFE_DRIVING_STARTED = "SAFE_DRIVING_STARTED"

        private var updating = false

        @JvmStatic
        fun setUpdating(isUpdate: Boolean) {
            updating = isUpdate
        }
    }
}
