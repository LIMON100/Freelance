package com.axon.kisa10.model.register


import com.google.gson.annotations.SerializedName

data class RegisterVehicleRequest(
    @SerializedName("email")
    val email: String,
    @SerializedName("licensePlate")
    val licensePlate: String,
    @SerializedName("memo")
    val memo: String,
    @SerializedName("name")
    val name: String,
    @SerializedName("phoneNumber")
    val phoneNumber: String,
    @SerializedName("socialId")
    val socialId: String,
    @SerializedName("vincode")
    val vinCode : String,
    @SerializedName("bleAddress")
    val bleAddress : String,
    @SerializedName("deviceModel")
    val deviceModel : String = "AXON"
)