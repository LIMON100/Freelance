package com.axon.kisa10.model.distributor.calibration

import androidx.room.Entity
import androidx.room.PrimaryKey
import com.google.gson.annotations.Expose
import java.io.Serializable

data class DistributorCalibrationUploadRequest(
    val caliMaxHigh: Int,
    val caliMaxLow: Int,
    val caliMinHigh: Int,
    val caliMinLow: Int,
    val highBTO : Int,
    val lowBTO : Int,
    val deviceId: String,
    val licensePlate: String,
    val location: String,
    val projectId: String,
    val vehicleType: String,
    val vincode: String,
    val deviceMacAddress : String
) : Serializable {

    fun toDto(date : String, time : String) : CalibrationHistory {
        return CalibrationHistory(
            caliMaxHigh = caliMaxHigh,
            caliMaxLow = caliMaxLow,
            caliMinHigh = caliMinHigh,
            caliMinLow = caliMinLow,
            highBTO = highBTO,
            lowBTO = lowBTO,
            deviceId = deviceId,
            location = location,
            vehicleType = vehicleType,
            licensePlate = licensePlate,
            vincode = vincode,
            deviceMacAddress = deviceMacAddress,
            projectId = projectId,
            date = date,
            time = time
        )
    }
}