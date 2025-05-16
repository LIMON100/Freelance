package com.axon.kisa10.model


import com.google.gson.annotations.SerializedName

data class LatestFirmwareVersionResponse(
    @SerializedName("createdAt")
    val createdAt: String,
    @SerializedName("file")
    val firmwareFile: File,
    @SerializedName("id")
    val id: String,
    @SerializedName("type")
    val type: String,
    @SerializedName("updatedAt")
    val updatedAt: String,
    @SerializedName("version")
    val version: String
)

data class File(
    @SerializedName("id")
    val id: String,
    @SerializedName("path")
    val path: String
)