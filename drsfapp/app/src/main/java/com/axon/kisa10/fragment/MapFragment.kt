package com.axon.kisa10.fragment

import android.annotation.SuppressLint
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.location.Location
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentManager
import androidx.fragment.app.FragmentTransaction
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.activity.MainActivity
import com.axon.kisa10.activity.MainActivity.onKeyBackPressedListener
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.ble.SdiDataHandler
import com.axon.kisa10.model.LogSdiData
import com.axon.kisa10.util.AlarmSoundHandler
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AppMethods
import com.axon.kisa10.util.getSdiNameFromType
import com.kisa10.R
import com.kisa10.databinding.FragmentMapBinding
import com.skt.tmap.engine.navigation.SDKManager
import com.skt.tmap.engine.navigation.data.SDIInfo
import com.skt.tmap.engine.navigation.network.ndds.NddsDataType.DestSearchFlag
import com.skt.tmap.engine.navigation.route.RoutePlanType
import com.tmapmobility.tmap.tmapsdk.ui.data.MapSetting
import com.tmapmobility.tmap.tmapsdk.ui.fragment.NavigationFragment
import com.tmapmobility.tmap.tmapsdk.ui.util.TmapUISDK
import com.tmapmobility.tmap.tmapsdk.ui.util.TmapUISDK.Companion.initialize

/**
 * Created by Hussain on 18/06/24.
 */
class MapFragment internal constructor() : Fragment(), onKeyBackPressedListener, SensorEventListener {
    private lateinit var binding: FragmentMapBinding

    private lateinit var navigationFragment: NavigationFragment
    private var fragmentManager: FragmentManager? = null
    private var transaction: FragmentTransaction? = null
    private lateinit var sensorManager: SensorManager
    private val localBroadcastManager by lazy { LocalBroadcastManager.getInstance(requireContext()) }
    private var isSdkInitialized = false
    private var sdiInfo : SDIInfo? = null
    private var accSensorData = listOf(0.0f,0.0f,0.0f)
    private var magSensorData = listOf(0.0f,0.0f,0.0f)
    private var gyroSensorData = listOf(0.0f,0.0f,0.0f)
    private var locationData : Location? = null
    private val sdiDataHandler by lazy { SdiDataHandler.getInstance(requireContext()) }

    private val alarmSoundHandler by lazy { AlarmSoundHandler(requireContext()) }

    private var currentSdiType = -1

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        binding = FragmentMapBinding.inflate(inflater, container, false)
        localBroadcastManager.registerReceiver(sdiBroadcastReceiver, AxonBLEService.makeBackgroundDrivingIntentFilter())
        return binding.root
    }

    @SuppressLint("SetTextI18n", "DefaultLocale")
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        initUI()
        AppMethods.showProgressDialog(requireContext())
        initUISDK()
        alarmSoundHandler.loadSounds()
        sdiDataHandler.startCalculatingAzimuth()
        sdiDataHandler.addSdiObserver(viewLifecycleOwner)
//        SDKManager.getInstance().observableSDIData.observe(viewLifecycleOwner) { observableSDIData ->
//            if (observableSDIData != null && observableSDIData.currentSpeed > 0) {
//                val speed = "" + observableSDIData.currentSpeed
//                binding.speed.subtitle.text = "$speed Km/h"
//                val sdiInfo = if (observableSDIData.firstSDIInfo != null) {
//                    observableSDIData.firstSDIInfo
//                } else {
//                    observableSDIData.secondSdiInfo
//                }
//
//                if (sdiInfo != null) {
//                    val sdiType = getSdiTypeString(sdiInfo.nSdiType) + " -> (${sdiInfo.nSdiType})"
//                    binding.sdiType.subtitle.text = sdiType
//                    val sdiSection = sdiInfo.nSdiSection
//                    val sdiDist = sdiInfo.nSdiDist
//                    val sdiSpeedLimit = sdiInfo.nSdiSpeedLimit
//                    val sdiBlockSection = sdiInfo.bSdiBlockSection
//                    val sdiBlockDisk = sdiInfo.nSdiBlockDist
//                    val sdiBlockSpeed = sdiInfo.nSdiBlockSpeed
//                    val sdiBlockAvgSpeed = sdiInfo.nSdiBlockAverageSpeed
//                    val sdiBlockTime = sdiInfo.nSdiBlockTime
//                    val sdiBisChangeAbleSpeedType = sdiInfo.bIsChangeableSpeedType
//                    val sdiBIsLimitSignChanged = sdiInfo.bIsLimitSpeedSignChanged
//                    val sdiTruckLimit = sdiInfo.nSDITruckLimit
//                    this.sdiInfo = sdiInfo
//                    val currentSpeed = observableSDIData.currentSpeed
//                    var isBto = 0
//                    if ((sdiBlockSpeed + 10) < currentSpeed) {
//                        // alarm 1
//                        alarm1.play(1, 1f, 1f, 0, 0, 1f)
//                    }
//
//                    if ((sdiBlockSpeed + 5 ) < currentSpeed  ||  (sdiBlockSpeed + 10 ) > currentSpeed ) {
//                        // alarm 2
//                        alarm2.play(2,1f,1f,0,0,1f)
//                    }
//
//                    if (sdiBlockSpeed < 80 && sdiDist < 200  ) {
//                        // alarm 3
//                        alarm3.play(3,1f,1f,0,0,1f)
//                    }
//
//                    if (sdiBlockSpeed < 80 && sdiDist < 200 &&  (sdiBlockSpeed+10) < currentSpeed ) {
//                        //  BTO COMMAND
//                        isBto = 1
//                    }
//
//                    if (sdiBlockSpeed > 80 && sdiDist  < 300) {
//                        // alarm 4
//                        alarm4.play(4,1f,1f,0,0,1f)
//                    }
//
//                    if (sdiBlockSpeed > 80 && sdiDist  < 300  &&  (sdiBlockSpeed+10) < currentSpeed ) {
//                        // BTO COMMAND
//                        isBto = 1
//                    }
//
//                    val type = sdiInfo.nSdiType
//                    if ((type == 20 || type == 21 || type == 66 || type == 67 || type == 68 || type == 69 || type == 70) &&
//                        currentSpeed in (sdiBlockSpeed+5)..(sdiBlockSpeed+10)) {
//                        // alarm 5
//                        alarm5.play(5,1f,1f,0,0,1f)
//                    }
//
//                    val logSdiData = LogSdiData(
//                        locationData?.latitude?: 0.0,
//                        locationData?.longitude?: 0.0,
//                        accSensorData,
//                        magSensorData,
//                        gyroSensorData,
//                        sdiInfo,
//                        observableSDIData.currentSpeed,
//                        isBto
//                    )
//                    val sdiIntent = Intent(AxonBLEService.ACTION_SDI_DATA_AVAILABLE)
//                    sdiIntent.putExtra(AppConstants.EXTRA_SETTING, logSdiData)
//                    localBroadcastManager.sendBroadcast(sdiIntent)
//                    ioScope.launch {
//                        val logString = logSdiData.toString()
//                        Log.i("sdiLogging", "writing log --> $logString")
//                        sdiDataLogger.writeLogLine(logString)
//                    }
//                    binding.sdiSection.subtitle.text = sdiSection.toString()
//                    binding.sdiDist.subtitle.text = sdiDist.toString()
//                    binding.sdiSpeedLimit.subtitle.text = sdiSpeedLimit.toString()
//                    binding.sdiBlockSection.subtitle.text = sdiBlockSection.toString()
//                    binding.sdiBlockDist.subtitle.text = sdiBlockDisk.toString()
//                    binding.sdiBlockSpeed.subtitle.text = sdiBlockSpeed.toString()
//                    binding.sdiBlockAverageSpeed.subtitle.text = sdiBlockAvgSpeed.toString()
//                    binding.sdiBlockTime.subtitle.text = sdiBlockTime.toString()
//                    binding.bIsChangeableSpeedType.subtitle.text = sdiBisChangeAbleSpeedType.toString()
//                    binding.bIsLimitSpeedSignChanged.subtitle.text = sdiBIsLimitSignChanged.toString()
//                    binding.nTruckLimit.subtitle.text = sdiTruckLimit.toString()
//                    Log.d(TAG, "SDI Data: " + "speed : " + observableSDIData.currentSpeed + " averageSpeed " + observableSDIData.averageSpeed
//                            + " isCaution : " + observableSDIData.isCaution + " limit : " + observableSDIData.limitSpeed)
//                    Log.d(TAG, "SDI Info Available")
//                    Log.d(TAG, "SDI : " + "SdiType : " + sdiInfo.nSdiType + " blockSpeed:" + sdiInfo.nSdiBlockSpeed + " speed limit: " + sdiInfo.nSdiSpeedLimit + " distance: " + sdiInfo.nSdiDist + " schoolZone : " + sdiInfo.bIsInSchoolZone)
//                    Log.d(TAG, "Location : $locationData")
//                } else {
//                    binding.sdiType.subtitle.text = "-"
//                    binding.sdiSection.subtitle.text = ""
//                    binding.sdiDist.subtitle.text = ""
//                    binding.sdiSpeedLimit.subtitle.text = ""
//                    binding.sdiBlockSection.subtitle.text = ""
//                    binding.sdiBlockDist.subtitle.text = ""
//                    binding.sdiBlockSpeed.subtitle.text = ""
//                    binding.sdiBlockAverageSpeed.subtitle.text = ""
//                    binding.sdiBlockTime.subtitle.text = ""
//                    binding.bIsChangeableSpeedType.subtitle.text = ""
//                    binding.bIsLimitSpeedSignChanged.subtitle.text = ""
//                    binding.nTruckLimit.subtitle.text = ""
//                }
//
////                val format = SimpleDateFormat("yyyy/MM/dd hh:mm", Locale.getDefault())
////                val currentDate = format.format(Date())
////                binding.date.subtitle.text = currentDate
//            }
//        }
        SDKManager.getInstance().observableLocationData.observe(viewLifecycleOwner) { locationData ->
            locationData?.location?.let { location ->
                val lat = String.format("%.2f",location.latitude)
                val lon = String.format("%.2f",location.longitude)
                binding.gps.subtitle.text = "lat : ${lat} lon : ${lon}"
                this.locationData = location
            }
        }
    }

    @SuppressLint("SetTextI18n")
    private fun initSensors() {
//
//        binding.acc.title.setTextColor(Color.BLACK)
//        binding.acc.subtitle.setTextColor(Color.BLACK)
//        binding.gyro.title.setTextColor(Color.BLACK)
//        binding.gyro.subtitle.setTextColor(Color.BLACK)
//        binding.mag.title.setTextColor(Color.BLACK)
//        binding.mag.subtitle.setTextColor(Color.BLACK)
        sensorManager = requireContext().getSystemService(Context.SENSOR_SERVICE) as SensorManager

        val accelerometer = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)
        if (accelerometer != null) {
            sensorManager.registerListener(this, accelerometer, SensorManager.SENSOR_DELAY_NORMAL)
        } else {
//            binding.acc.subtitle.text = "No Accelerometer Found."
        }

        val gyroscope = sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE_UNCALIBRATED)
        if (gyroscope != null) {
            sensorManager.registerListener(this, gyroscope, SensorManager.SENSOR_DELAY_NORMAL)
        } else {
//            binding.gyro.subtitle.text = "No Gyroscope Found."
        }

        val magnetometer = sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD)
        if (magnetometer != null) {
            sensorManager.registerListener(this, magnetometer, SensorManager.SENSOR_DELAY_NORMAL)
        } else {
//            binding.mag.subtitle.text = "No Magnetometer Found."
        }
    }

    private fun initUI() {
        fragmentManager = childFragmentManager
        val d = MapSetting()
        d.isShowClosedPopup = true
        d.isShowTrafficInfo = false

        navigationFragment = TmapUISDK.getFragment()
        transaction = fragmentManager?.beginTransaction()

        if (navigationFragment.isAdded) {
            transaction?.remove(navigationFragment)
        }
        transaction?.add(R.id.tmapUILayout, navigationFragment)
        transaction?.commitAllowingStateLoss()
        binding.apply {
            speed.title.text = "Speed : "
            sdiType.title.text = "Type : "
            date.title.text = "Date/Time : "
            gps.title.text = "GPS : "
            azimuth.title.text = "Azimuth : "
//            acc.title.text = "Accelerometer"
//            gyro.title.text = "GyroScope"
//            mag.title.text = "Magnetometer"
            sdiSection.title.text = "sdiSection : "
            sdiDist.title.text = "sdiDist : "
            sdiSpeedLimit.title.text = "sdiSpeedLimit : "
            sdiBlockSection.title.text = "sdiBlockSection : "
            sdiBlockDist.title.text = "sdiBlockDist : "
            sdiBlockSpeed.title.text = "sdiBlockSpeed : "
            sdiBlockAverageSpeed.title.text = "sdiBlockAverageSpeed : "
            sdiBlockTime.title.text = "sdiBlockTime : "
            bIsChangeableSpeedType.title.text = "ChangeableSpeedType : "
            bIsLimitSpeedSignChanged.title.text = "LimitSpeedSignChanged : "
            linkId.title.text = "linkId : "
            idxName.title.text = "idxName : "
            roadSpeed.title.text = "Road speed : "
            roadName.title.text = "roadName : "
            lane.title.text = "lane : "
            laneType.title.text = "laneType : "
            roadCategory.title.text = "roadCategory : "
            averageSpeed.title.text = "AverageSpeed : "
            limitSpeed.title.text = "LimitSpeed : "
        }
//        binding.acc.title.text = "Accelerometer"
//        binding.gyro.title.text = "GyroScope"
//        binding.mag.title.text = "Magnetometer"

        binding.btnStartNavigation.setOnClickListener {
            navigationFragment.startSafeDrive()
            sendSafeDrivingStartedSignal()
            binding.btnStartNavigation.visibility = View.GONE
        }

        navigationFragment.drivingStatusCallback = object : TmapUISDK.DrivingStatusCallback {
            override fun onStartNavigation() {
                Log.i(TAG, "onStartNavigation")
                binding.btnStartNavigation.visibility = View.GONE
            }

            override fun onStartNavigationInfo(totalDistanceInMeter: Int, totalTimeInSec: Int, tollFee: Int) {
            }

            override fun onStopNavigation() {
                Log.i(TAG, "onStopNavigation")
                binding.btnStartNavigation.visibility = View.VISIBLE
                sendSafeDrivingStoppedSignal()
            }

            override fun onPermissionDenied(errorCode: Int, errorMsg: String?) {
            }

            override fun onArrivedDestination(destination: String, drivingTime: Int, drivingDistance: Int) {
            }

            override fun onBreakawayFromRouteEvent() {
            }

            override fun onApproachingAlternativeRoute() {
            }

            override fun onPassedAlternativeRouteJunction() {
            }

            override fun onPeriodicReroute() {
            }

            override fun onRouteChanged(index: Int) {
            }

            override fun onForceReroute(periodicType: DestSearchFlag) {
            }

            override fun onNoLocationSignal(noLocationSignal: Boolean) {
            }

            override fun onApproachingViaPoint() {
            }

            override fun onPassedViaPoint() {
            }

            override fun onChangeRouteOptionComplete(routePlanType: RoutePlanType) {
            }

            override fun onBreakAwayRequestComplete() {
            }

            override fun onPeriodicRerouteComplete() {
            }

            override fun onUserRerouteComplete() {
            }

            override fun onDestinationDirResearchComplete() {
            }

            override fun onDoNotRerouteToDestinationComplete() {
            }

            override fun onFailRouteRequest(errorCode: String, errorMessage: String) {
            }

            override fun onPassedTollgate(fee: Int) {
            }

            override fun onLocationChanged() {
            }
        }
    }

    private fun initUISDK() {
        initialize(
            requireActivity(),
            CLIENT_ID,
            API_KEY,
            USER_KEY,
            DEVICE_KEY,
            object : TmapUISDK.InitializeListener {
                override fun onSuccess() {
                    Log.e(TAG, "success initialize")
                    Handler(Looper.getMainLooper()).postDelayed({
                        binding.btnStartNavigation.performClick()
                        isSdkInitialized = true
                        AppMethods.hideProgressDialog(requireContext())
                    },2000)
                }

                override fun onFail(errorCode: Int, errorMsg: String?) {
                    Toast.makeText(requireContext(), "$errorCode::$errorMsg", Toast.LENGTH_SHORT).show()
                    Log.e(TAG, "onFail $errorCode :: $errorMsg")
                    isSdkInitialized = false
                }

                override fun savedRouteInfoExists(destinationName: String?) {
                }
            })
    }

    override fun onPause() {
        super.onPause()
        navigationFragment.stopDrive()
        sendSafeDrivingStoppedSignal()
    }

    private fun sendSafeDrivingStartedSignal() {
        val intent = Intent(MainActivity.SAFE_DRIVING_STARTED)
        intent.putExtra(AppConstants.EXTRA_SETTING, true)
        localBroadcastManager.sendBroadcast(intent)
    }

    private fun sendSafeDrivingStoppedSignal() {
        val intent = Intent(MainActivity.SAFE_DRIVING_STARTED)
        intent.putExtra(AppConstants.EXTRA_SETTING, false)
        localBroadcastManager.sendBroadcast(intent)
    }

    override fun onDestroyView() {
        super.onDestroyView()
        binding.tmapUILayout.removeAllViews()
        alarmSoundHandler.unLoadSounds()
        sdiDataHandler.stopCalculatingAzimuth()
    }

    override fun onBackKey() {
    }

    companion object {
        private const val TAG = "MapFragment"
        private const val CLIENT_ID = ""
        private const val API_KEY = "GwLMAY6cBg9EoOI1G9v6Q5AaDKF4oPw06tVU490B" //발급받은 KEY
        private const val USER_KEY = ""
        private const val DEVICE_KEY = ""

        fun newInstance(): MapFragment {
            return MapFragment()
        }
    }

    @SuppressLint("DefaultLocale")
    override fun onSensorChanged(event: SensorEvent?) {
        event?.let { e ->
            when (e.sensor.type) {
                Sensor.TYPE_GYROSCOPE_UNCALIBRATED -> {
                    // String.format("%.2f", number3digits).toDouble()
                    val x = String.format("%.2f",event.values[0])
                    val y = String.format("%.2f",event.values[1])
                    val z = String.format("%.2f",event.values[2])
                    val text = "x : $x rad/s, y : $y rad/s, z : $z rads/s"
                    gyroSensorData = listOf(event.values[0],event.values[1],event.values[2])
//                    binding.gyro.subtitle.text = text
                    Log.i("sensor", "Gyroscope :  $text")
                }

                Sensor.TYPE_ACCELEROMETER -> {
                    val x = String.format("%.2f",event.values[0])
                    val y = String.format("%.2f",event.values[1])
                    val z = String.format("%.2f",event.values[2])
                    val text = "x : $x m/s^2, y : $y m/s^2, z : $z m/s^2"
                    accSensorData = listOf(event.values[0],event.values[1],event.values[2])
//                    binding.acc.subtitle.text = text
                    Log.i("sensor", "Acceleration : ")
                }

                Sensor.TYPE_MAGNETIC_FIELD -> {
                    val x = String.format("%.2f",event.values[0])
                    val y = String.format("%.2f",event.values[1])
                    val z = String.format("%.2f",event.values[2])
                    val text = "$x μT, y : $y μT, z : $z μT"
//                    binding.mag.subtitle.text = text
                    magSensorData = listOf(event.values[0],event.values[1],event.values[2])
                    Log.i("sensor", "Magnetometer : $text")
                }

                else -> {
                    Log.i(TAG,"Unknown sensor type : ${e.sensor.name}" )
                }
            }
        }
    }

    override fun onAccuracyChanged(sensor: Sensor?, p1: Int) {
    }

    private val sdiBroadcastReceiver = object : BroadcastReceiver() {

        override fun onReceive(context: Context, intent: Intent) {
            if (intent.action == AxonBLEService.ACTION_SDI_DATA_AVAILABLE) {
                val data : LogSdiData? = intent.extras?.getSerializable(AppConstants.EXTRA_SETTING) as? LogSdiData
                data?.let { sdi -> setSdiDataUI(sdi) }
            }
        }

    }

    private fun setSdiDataUI(data: LogSdiData) {
        if (data.currentSpeed > 0) {
            val speed = "" + data.currentSpeed
            binding.speed.subtitle.text = "$speed Km/h"
            val sdiInfo = data.sdiInfo
            val sdiType = resources.getSdiNameFromType(sdiInfo.nSdiType) + " -> (${sdiInfo.nSdiType})"
            binding.sdiType.subtitle.text = sdiType
            val sdiSection = sdiInfo.nSdiSection
            val sdiDist = sdiInfo.nSdiDist
            val sdiSpeedLimit = sdiInfo.nSdiSpeedLimit
            val sdiBlockSection = sdiInfo.bSdiBlockSection
            val sdiBlockDisk = sdiInfo.nSdiBlockDist
            val sdiBlockSpeed = sdiInfo.nSdiBlockSpeed
            val sdiBlockAvgSpeed = sdiInfo.nSdiBlockAverageSpeed
            val sdiBlockTime = sdiInfo.nSdiBlockTime
            val sdiBisChangeAbleSpeedType = sdiInfo.bIsChangeableSpeedType
            val sdiBIsLimitSignChanged = sdiInfo.bIsLimitSpeedSignChanged
            val sdiTruckLimit = sdiInfo.nSDITruckLimit

            binding.sdiSection.subtitle.text = sdiSection.toString()
            binding.sdiDist.subtitle.text = sdiDist.toString()
            binding.sdiSpeedLimit.subtitle.text = sdiSpeedLimit.toString()
            binding.sdiBlockSection.subtitle.text = sdiBlockSection.toString()
            binding.sdiBlockDist.subtitle.text = sdiBlockDisk.toString()
            binding.sdiBlockSpeed.subtitle.text = sdiBlockSpeed.toString()
            binding.sdiBlockAverageSpeed.subtitle.text = sdiBlockAvgSpeed.toString()
            binding.sdiBlockTime.subtitle.text = sdiBlockTime.toString()
            binding.bIsChangeableSpeedType.subtitle.text = sdiBisChangeAbleSpeedType.toString()
            binding.bIsLimitSpeedSignChanged.subtitle.text = sdiBIsLimitSignChanged.toString()
            binding.azimuth.subtitle.text = String.format("%.2f",data.azimuth) + " Deg"
            if (data.nearRoadHeader != null) {
                binding.linkId.subtitle.text = data.nearRoadHeader.linkId
                binding.idxName.subtitle.text = data.nearRoadHeader.idxName
                binding.roadSpeed.subtitle.text = data.nearRoadHeader.speed.toString()
                binding.roadName.subtitle.text = data.nearRoadHeader.roadName
                binding.lane.subtitle.text = data.nearRoadHeader.lane.toString()
                binding.laneType.subtitle.text = data.nearRoadHeader.laneType.toString()
                binding.roadCategory.subtitle.text = data.nearRoadHeader.roadCategory.toString()
            }
            binding.averageSpeed.subtitle.text = data.averageSpeed.toString()
            binding.limitSpeed.subtitle.text = data.limitSpeed.toString()
            Log.d(TAG, "SDI Info Available")
            Log.d(TAG, "SDI : " + "SdiType : " + sdiInfo.nSdiType + " blockSpeed:" + sdiInfo.nSdiBlockSpeed + " speed limit: " + sdiInfo.nSdiSpeedLimit + " distance: " + sdiInfo.nSdiDist + " schoolZone : " + sdiInfo.bIsInSchoolZone)
            Log.d(TAG, "Location : ${data.latitude} ${data.longitude}")
            val lat = String.format("%.4f",data.latitude)
            val lon = String.format("%.4f",data.longitude)
            binding.gps.subtitle.text = "lat : ${lat} lon : ${lon}"
            binding.gps.subtitle.text = "$lat | $lon"
            if (data.soundId != AlarmSoundHandler.Companion.SoundType.NONE) {
                if (currentSdiType == -1) {
                    currentSdiType = sdiInfo.nSdiType
                    alarmSoundHandler.playSound(data.soundId)
                } else if(currentSdiType != sdiInfo.nSdiType) {
                    currentSdiType = sdiInfo.nSdiType
                    alarmSoundHandler.playSound(data.soundId)
                }
            }
        }
    }
}
