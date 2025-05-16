package com.axon.kisa10.model.calibration

import com.google.gson.annotations.SerializedName

/**
 * Created by Hussain on 21/09/24.
 */
data class CalibrationRequest(
    @SerializedName("vincode")
    val vincode : String,
    @SerializedName("apsHighMin")
    val apsHighMin : String,
    @SerializedName("apsHighMax")
    val apsHighMax : String,
    @SerializedName("apsHighOut")
    val apsHighOut : String,
    @SerializedName("apsLowMin")
    val apsLowMin : String,
    @SerializedName("apsLowMax")
    val apsLowMax : String,
    @SerializedName("apsLowOut")
    val apsLowOut : String
)