package com.axon.kisa10.model.neartoroad


import com.google.gson.annotations.SerializedName

data class LinkPoint(
    @SerializedName("location")
    val location: Location
)