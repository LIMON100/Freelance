package com.axon.kisa10.model.register


import com.google.gson.annotations.SerializedName

data class RegisterVehicleResponse(
    @SerializedName("token")
    val token: String,
    @SerializedName("vehicleId")
    val vehicleId: String
)