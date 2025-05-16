package com.axon.kisa10.model

import com.axon.kisa10.model.neartoroad.NearRoadHeader
import com.axon.kisa10.util.AlarmSoundHandler
import com.skt.tmap.engine.navigation.data.SDIInfo
import java.io.Serializable
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * Created by Hussain on 04/07/24.
 */
data class LogSdiData(
    val latitude : Double,
    val longitude : Double,
    val azimuth : Double,
    val accelerometerData : List<Float>,
    val magnetometerData : List<Float>,
    val gyroscopeData : List<Float>,
    val sdiInfo: SDIInfo,
    val currentSpeed : Int,
    val averageSpeed : Int,
    val limitSpeed : Int,
    val isBto : Int,
    val soundId : AlarmSoundHandler.Companion.SoundType = AlarmSoundHandler.Companion.SoundType.NONE,
    val nearRoadHeader: NearRoadHeader? = null
) : Serializable {

    override fun toString(): String {
        val simpleFormat = SimpleDateFormat("yyyy/MM/dd HH:mm:ss", Locale.getDefault())
        val date = simpleFormat.format(Date())
        var logString = "{"
        logString += "timestamp : $date,"
        logString += "currentSpeed:$currentSpeed,"
        logString += "latitude:${latitude},longitude:${longitude},Azimuth:${azimuth}"
        logString += "accelerometer:x-${accelerometerData[0]} y-${accelerometerData[1]} z-${accelerometerData[2]},"
        logString += "gyroScope:x-${gyroscopeData[0]} y-${gyroscopeData[1]} z-${gyroscopeData[2]},"
        logString += "magnetometer:x-${magnetometerData[0]} y-${magnetometerData[1]} z-${magnetometerData[2]},"
        val sdiType = sdiInfo.nSdiType
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
        logString += "sdiData : "
        logString += "sdiType-$sdiType,"
        logString += "sdiSection-$sdiSection,"
        logString += "sdiDist-$sdiDist,"
        logString += "sdiSpeedLimit-$sdiSpeedLimit,"
        logString += "sdiBlockSection-$sdiBlockSection,"
        logString += "sdiBlockDisk-$sdiBlockDisk,"
        logString += "sdiBlockSpeed-$sdiBlockSpeed,"
        logString += "sdiBlockAvgSpeed-$sdiBlockAvgSpeed,"
        logString += "sdiBlockTime-$sdiBlockTime,"
        logString += "sdiBisChangeAbleSpeedType-$sdiBisChangeAbleSpeedType,"
        logString += "sdiBIsLimitSignChanged-$sdiBIsLimitSignChanged,"
        logString += "sdiTruckLimit-$sdiTruckLimit"
        logString += "}"
        return logString
    }
}
