package com.axon.kisa10.model.neartoroad


import com.google.gson.annotations.SerializedName

data class NearToRoadResponse(
    @SerializedName("resultData")
    val resultData: ResultData
)