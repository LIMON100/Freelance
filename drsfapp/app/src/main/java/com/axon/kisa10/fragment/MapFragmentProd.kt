package com.axon.kisa10.fragment

import android.annotation.SuppressLint
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.fragment.app.Fragment
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.model.LogSdiData
import com.axon.kisa10.util.AlarmSoundHandler
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AppMethods
import com.axon.kisa10.util.getSdiNameFromType
import com.axon.kisa10.databinding.FragmentMapProdBinding
import com.tmapmobility.tmap.tmapsdk.ui.util.TmapUISDK

/**
 * Created by Hussain on 11/07/24.
 */
class MapFragmentProd : Fragment() {

    companion object {
        private const val TAG = "MapFragmentProd"
        private const val CLIENT_ID = ""
        private const val API_KEY = "GwLMAY6cBg9EoOI1G9v6Q5AaDKF4oPw06tVU490B" //발급받은 KEY
        private const val USER_KEY = ""
        private const val DEVICE_KEY = ""

        fun newInstance() : MapFragmentProd {
            return MapFragmentProd()
        }
    }

    private lateinit var binding: FragmentMapProdBinding

    private var isSdkInitialized = false

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(requireContext())
    }

    private val alarmSoundHandler by lazy { AlarmSoundHandler(requireContext()) }

    private var currentSdiType = -1

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentMapProdBinding.inflate(layoutInflater, container, false)
        val filter = IntentFilter().apply {
            addAction(AxonBLEService.ACTION_SDI_DATA_AVAILABLE)
        }
        localBroadcastManager.registerReceiver(sdiBroadcastReceiver, filter)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        initUI()
        initUISDK()
    }

    private fun initUISDK() {
        TmapUISDK.initialize(
            requireActivity(),
            CLIENT_ID,
            API_KEY,
            USER_KEY,
            DEVICE_KEY,
            object : TmapUISDK.InitializeListener {
                override fun onSuccess() {
                    Log.e(TAG, "success initialize")
                    Handler(Looper.getMainLooper()).postDelayed({
                        startSafeDrive()
                        isSdkInitialized = true
                        AppMethods.hideProgressDialog(requireContext())
                    }, 2000)
                }

                override fun onFail(errorCode: Int, errorMsg: String?) {
                    Toast.makeText(requireContext(), "$errorCode::$errorMsg", Toast.LENGTH_SHORT)
                        .show()
                    Log.e(TAG, "onFail $errorCode :: $errorMsg")
                    isSdkInitialized = false
                }

                override fun savedRouteInfoExists(destinationName: String?) {
                }
            })
    }

    private fun initUI() {
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
    }

    private fun startSafeDrive() {
        alarmSoundHandler.loadSounds()
        localBroadcastManager.sendBroadcast(Intent(AxonBLEService.ACTION_START_BACKGROUND_DRIVING))
    }

    private val sdiBroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (intent.action == AxonBLEService.ACTION_SDI_DATA_AVAILABLE) {
                val data: LogSdiData? = intent.extras?.getSerializable(AppConstants.EXTRA_SETTING) as? LogSdiData
                data?.let { setSdiDataUI(data) }
            }
        }
    }

    @SuppressLint("SetTextI18n")
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

    override fun onDestroyView() {
        super.onDestroyView()
        localBroadcastManager.unregisterReceiver(sdiBroadcastReceiver)
        alarmSoundHandler.unLoadSounds()
    }
}
