package com.axon.kisa10.model.distributor.calibration

import androidx.room.Entity
import androidx.room.PrimaryKey
import com.google.gson.annotations.Expose

@Entity(tableName = "Calibration")
data class CalibrationHistory(
    @PrimaryKey(autoGenerate = true)
    @Expose(serialize = false, deserialize = false)
    val id : Long = 0,
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
    val deviceMacAddress : String,
    val date : String,
    val time : String
)
