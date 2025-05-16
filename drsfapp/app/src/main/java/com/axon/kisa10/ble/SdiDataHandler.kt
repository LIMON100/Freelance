package com.axon.kisa10.ble

import android.annotation.SuppressLint
import android.app.Notification
import android.app.NotificationManager
import android.content.Context
import android.content.Intent
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.location.Location
import android.location.LocationManager
import android.media.AudioManager
import android.media.SoundPool
import android.util.Log
import androidx.lifecycle.LifecycleOwner
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.data.SdiDataLogger
import com.axon.kisa10.model.LogSdiData
import com.axon.kisa10.model.neartoroad.NearRoadHeader
import com.axon.kisa10.model.neartoroad.NearToRoadResponse
import com.axon.kisa10.util.AlarmSoundHandler
import com.axon.kisa10.util.AppConstants
import com.google.gson.Gson
import com.kisa10.R
import com.skt.tmap.engine.navigation.NavigationManager
import com.skt.tmap.engine.navigation.data.DriveMode
import com.skt.tmap.engine.navigation.data.SDIInfo
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import okhttp3.OkHttpClient
import okhttp3.Request
import java.util.Timer
import java.util.TimerTask


/**
 * Created by Hussain on 04/07/24.
 */
class SdiDataHandler private constructor(private val context: Context) : SensorEventListener {


    private var sdiInfo : SDIInfo? = null
    private var accSensorData = listOf(0.0f,0.0f,0.0f)
    private var magSensorData = listOf(0.0f,0.0f,0.0f)
    private var gyroSensorData = listOf(0.0f,0.0f,0.0f)
    private var locationData : Location? = null
    private var azimuth = 0.0
    private val sdiDataLogger = SdiDataLogger.getInstance(context)
    private val ioScope = CoroutineScope(Dispatchers.IO)
    private val localBroadcastManager = LocalBroadcastManager.getInstance(context)
    private var timer : Timer? = null
    private var shouldCallApi = false
    private var nearRoadHeader : NearRoadHeader? = null

    companion object {
        private lateinit var sdiDataHandler: SdiDataHandler

        fun getInstance(context: Context) : SdiDataHandler {
            if(!this::sdiDataHandler.isInitialized) {
                sdiDataHandler = SdiDataHandler(context)
            }
            return sdiDataHandler
        }
    }

    private val alarm1 by lazy {
        SoundPool(1, AudioManager.STREAM_MUSIC, 0)
    }
    private val alarm2 by lazy {
        SoundPool(1, AudioManager.STREAM_MUSIC, 0)
    }
    private val alarm3 by lazy {
        SoundPool(1, AudioManager.STREAM_MUSIC, 0)
    }

    private val alarm4 by lazy {
        SoundPool(1, AudioManager.STREAM_MUSIC, 0)
    }

    private val alarm5 by lazy {
        SoundPool(1, AudioManager.STREAM_MUSIC, 0)
    }

    private val TAG = "SdiDataHandler"
    private val navigationManager = NavigationManager.getInstance()
    private val locationManager = context.getSystemService(Context.LOCATION_SERVICE) as LocationManager
    private val sensorManager = context.getSystemService(Context.SENSOR_SERVICE) as SensorManager

    private fun registerSensors() {
        val accelerometer = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)
        if (accelerometer != null) {
            sensorManager.registerListener(this, accelerometer, SensorManager.SENSOR_DELAY_NORMAL)
        } else {
            Log.i(TAG, "initSensors:No Accelerometer Found.")
        }

//        val gyroscope = sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE_UNCALIBRATED)
//        if (gyroscope != null) {
//            sensorManager.registerListener(this, gyroscope, SensorManager.SENSOR_DELAY_NORMAL)
//        } else {
//            Log.i(TAG, "initSensors: No Gyroscope Found.")
//        }

        val magnetometer = sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD)
        if (magnetometer != null) {
            sensorManager.registerListener(this, magnetometer, SensorManager.SENSOR_DELAY_NORMAL)
        } else {
            Log.i(TAG, "initSensors: No Magnetometer Found.")
        }
    }

    private fun unRegisterSensors() {
        sensorManager.unregisterListener(this)
    }
    override fun onSensorChanged(event: SensorEvent?) {
        event?.let { e ->
            when (e.sensor.type) {
                Sensor.TYPE_GYROSCOPE_UNCALIBRATED -> {
                    gyroSensorData = listOf(event.values[0],event.values[1],event.values[2])
                }

                Sensor.TYPE_ACCELEROMETER -> {
                    accSensorData = listOf(event.values[0],event.values[1],event.values[2])
                }

                Sensor.TYPE_MAGNETIC_FIELD -> {
                    magSensorData = listOf(event.values[0],event.values[1],event.values[2])
                }

                else -> {
                }
            }

            if (accSensorData.all { it != 0.0f } && magSensorData.all { it != 0.0f }) {
                val R = FloatArray(9)
                val I = FloatArray(9)
                val success = SensorManager.getRotationMatrix(R, I, accSensorData.toFloatArray(), magSensorData.toFloatArray())
                if (success) {
                    val orientation = FloatArray(3)
                    SensorManager.getOrientation(R, orientation)
                    val azimuth = orientation[0] // Azimuth in radians
                    var azimuthDegrees = Math.toDegrees(azimuth.toDouble()).toFloat() // Convert to degrees
                    if (azimuthDegrees < 0) {
                        azimuthDegrees += 360f
                    }
                    this.azimuth = azimuthDegrees.toDouble()
                }
            }
        }

    }

    override fun onAccuracyChanged(p0: Sensor?, p1: Int) {
    }

    @SuppressLint("MissingPermission")
    fun startDriving(notification : Notification) {
        loadSounds()
        Log.i(TAG, "startDriving")
        navigationManager.startDriving(context, DriveMode.SAFE_DRIVE, notification, 101, null, 0, false)
        val notificationManager = context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.cancel(101)
        navigationManager.removeLocationUpdate()
//        registerSensors()
    }

    @SuppressLint("MissingPermission")
    fun addSdiObserver(lifecycleOwner: LifecycleOwner) = navigationManager.observableSDIData.observe(lifecycleOwner) { observableSDIData ->
        observableSDIData?.let {
            if (timer == null) {
                timer = Timer()
                timer?.schedule(object : TimerTask() {
                    override fun run() {
                        shouldCallApi = true
                        getNearRoad()
                    }
                }, 1000, 1000)
            }
            if (observableSDIData.currentSpeed > 0) {
                var sdiInfo: SDIInfo? = null
                if (observableSDIData.firstSDIInfo != null) {
                    sdiInfo = observableSDIData.firstSDIInfo
                } else if (observableSDIData.secondSdiInfo != null) {
                    sdiInfo = observableSDIData.secondSdiInfo
                }

                if (sdiInfo != null) {
                    this.sdiInfo = sdiInfo
                    Log.d(TAG, "SDI Data: " + "speed : " + observableSDIData.currentSpeed + " averageSpeed " + observableSDIData.averageSpeed
                            + " isCaution : " + observableSDIData.isCaution + " limit : " + observableSDIData.limitSpeed)
                    Log.d(TAG, "SDI Info Available")
                    Log.d(TAG, "SDI : " + "SdiType : " + sdiInfo.nSdiType + " blockSpeed:" + sdiInfo.nSdiBlockSpeed + " speed limit: " + sdiInfo.nSdiSpeedLimit + " distance: " + sdiInfo.nSdiDist + " schoolZone : " + sdiInfo.bIsInSchoolZone)
                    locationData = locationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER)
                    Log.d(TAG, "Location : $locationData")
                    val currentSpeed = observableSDIData.currentSpeed
                    val sdiSpeedLimit = sdiInfo.nSdiSpeedLimit
                    val sdiDist = sdiInfo.nSdiDist
                    var isBto = 0
                    var soundId = AlarmSoundHandler.Companion.SoundType.NONE
                    val type = sdiInfo.nSdiType
                    val offset = 10
                    var shouldPlaySound = false
                    if (type in 0..4) {
                        var checkBto = false
                        if (sdiSpeedLimit in 0..49 && sdiDist < 150) {
                            shouldPlaySound = true
                            checkBto = true
                        } else if(sdiSpeedLimit in 50..60 && sdiDist < 300) {
                            shouldPlaySound = true
                            checkBto = true
                        } else if(sdiSpeedLimit in 61..89 && sdiDist < 400) {
                            shouldPlaySound = true
                            checkBto = true
                        } else if(sdiSpeedLimit in 90..99 && sdiDist < 500) {
                            shouldPlaySound = true
                            checkBto = true
                        } else if(sdiSpeedLimit in 100..140 && sdiDist < 600) {
                            shouldPlaySound = true
                            checkBto = true
                        }

                        if (shouldPlaySound) {
                            soundId = AlarmSoundHandler.Companion.SoundType.SPEED_LIMIT
                        }
                        if (checkBto) {
                            if((sdiSpeedLimit+offset) < currentSpeed) {
                                isBto = 1
                            } else if ((sdiSpeedLimit-(sdiSpeedLimit*0.95) > currentSpeed)) {
                                isBto = 0
                            } else if((sdiSpeedLimit+40) > currentSpeed) {
                                isBto = 0
                            } else {
                                isBto = 0
                            }
                        } else {
                            isBto = 0
                        }
                    }

                    if (type in 0..4) {
                        if (sdiSpeedLimit in 1..49 && sdiDist == 0) {
                            shouldPlaySound = true
                        } else if(sdiSpeedLimit in 50..60 && sdiDist == 0) {
                            shouldPlaySound = true
                        } else if(sdiSpeedLimit in 61..89 && sdiDist == 0) {
                            shouldPlaySound = true
                        } else if(sdiSpeedLimit in 90..99 && sdiDist == 0) {
                            shouldPlaySound = true
                        } else if(sdiSpeedLimit in 100..140 && sdiDist == 0) {
                            shouldPlaySound = true
                        }
                        if (shouldPlaySound) {
                            soundId = AlarmSoundHandler.Companion.SoundType.CURSOR
                        }
                    }

                    if (type == 20 || type == 21 || type in 66..70 && (sdiSpeedLimit+offset) > currentSpeed) {
                        soundId = AlarmSoundHandler.Companion.SoundType.SPEED_LIMIT
                    }
                    val logSdiData = LogSdiData(locationData?.latitude ?: 0.0, locationData?.longitude ?: 0.0, azimuth, accSensorData, magSensorData, gyroSensorData, sdiInfo, observableSDIData.currentSpeed,observableSDIData.averageSpeed, observableSDIData.limitSpeed, isBto, soundId, nearRoadHeader)
                    val sdiIntent = Intent(AxonBLEService.ACTION_SDI_DATA_AVAILABLE)
                    sdiIntent.putExtra(AppConstants.EXTRA_SETTING, logSdiData)
                    localBroadcastManager.sendBroadcast(sdiIntent)
                    ioScope.launch {
                        val logString = logSdiData.toString()
                        Log.i("sdiLogging", "writing log --> $logString")
                        sdiDataLogger.writeLogLine(logString)
                    }
                }
            }
        }
    }

    fun loadSounds() {
        alarm1.load(context, R.raw.alarm1,1)
        alarm2.load(context, R.raw.alarm2,1)
        alarm3.load(context, R.raw.alarm3,1)
        alarm4.load(context, R.raw.alarm4,1)
        alarm5.load(context, R.raw.alarm5,1)
    }

    fun unLoadSounds() {
        alarm1.release()
        alarm2.release()
        alarm3.release()
        alarm4.release()
        alarm5.release()
    }

    fun stopDriving() {
        Log.i(TAG, "stopDriving")
        navigationManager.stopDriving(false)
        unLoadSounds()
        timer?.cancel()
        timer?.purge()
        timer = null
    }

    fun startCalculatingAzimuth() {
        registerSensors()
    }

    fun stopCalculatingAzimuth() {
        unRegisterSensors()
    }

    fun getNearRoad() {
//        https://apis.openapi.sk.com/tmap/road/nearToRoad?appKey=GwLMAY6cBg9EoOI1G9v6Q5AaDKF4oPw06tVU490B&lat=37.568791&lon=126.982754
        if (locationData != null && shouldCallApi) {
            val lat = locationData?.latitude
            val long = locationData?.longitude
            val url = "https://apis.openapi.sk.com/tmap/road/nearToRoad?appKey=GwLMAY6cBg9EoOI1G9v6Q5AaDKF4oPw06tVU490B&lat=$lat&lon=$long"
            val client = OkHttpClient()
            val request = Request.Builder()
                .url(url)
                .build()
            try {
                val response = client.newCall(request).execute()
                if (response.isSuccessful) {
                    val responseBody = response.body
                    val responseBodyString = responseBody?.string() ?: return
                    Log.i(TAG, "getNearRoad: $responseBodyString")
                    val gson = Gson()
                    val nearRoadResponse = gson.fromJson(responseBodyString, NearToRoadResponse::class.java)
                    nearRoadHeader = nearRoadResponse.resultData.header
                }
            } catch (e : Exception) {
                e.printStackTrace()
            } finally {
                shouldCallApi = false
            }
        }
    }
}