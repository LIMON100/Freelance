package com.axon.kisa10.model.calibration

import com.google.gson.annotations.SerializedName

/**
 * Created by Hussain on 21/09/24.
 */
data class CalibrationResponse(
//    "vincode": "KNANC81BBRS437063",
//"apsHighMin": 1234,
//"apsHighMax": 1234,
//"apsHighOut": 1234,
//"apsLowMin": 1234,
//"apsLowMax": 1234,
//"apsLowOut": 1234,
//"id": "string",
//"createdAt": "2024-09-21T06:37:42.019Z",
//"updatedAt": "2024-09-21T06:37:42.019Z"
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
    val apsLowOut : String,
    @SerializedName("id")
    val id : String,
    @SerializedName("createdAt")
    val createdAt : String,
    @SerializedName("updatedAt")
    val updatedAt : String
)