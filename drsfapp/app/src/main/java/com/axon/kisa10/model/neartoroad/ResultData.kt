package com.axon.kisa10.model.neartoroad


import com.google.gson.annotations.SerializedName

data class ResultData(
    @SerializedName("header")
    val header: NearRoadHeader,
    @SerializedName("linkPoints")
    val linkPoints: List<LinkPoint>
)