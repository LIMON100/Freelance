package com.axon.kisa10.model.neartoroad


import com.google.gson.annotations.SerializedName

data class NearRoadHeader(
    @SerializedName("idxName")
    val idxName: String,
    @SerializedName("lane")
    val lane: Int,
    @SerializedName("laneType")
    val laneType: Int,
    @SerializedName("linkFacil")
    val linkFacil: Int,
    @SerializedName("linkId")
    val linkId: String,
    @SerializedName("oneway")
    val oneway: Int,
    @SerializedName("roadCategory")
    val roadCategory: Int,
    @SerializedName("roadName")
    val roadName: String,
    @SerializedName("speed")
    val speed: Int,
    @SerializedName("tlinkId")
    val tlinkId: String,
    @SerializedName("tollLink")
    val tollLink: Int,
    @SerializedName("totalDistance")
    val totalDistance: Int
)