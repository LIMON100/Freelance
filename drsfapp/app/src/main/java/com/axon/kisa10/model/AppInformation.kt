package com.axon.kisa10.model


import com.google.gson.annotations.SerializedName

data class AppInformation(
    @SerializedName("appVersion")
    val appVersion: String,
    @SerializedName("createdAt")
    val createdAt: String,
    @SerializedName("id")
    val id: String,
    @SerializedName("lastAccessServerDatetime")
    val lastAccessServerDatetime: String?,
    @SerializedName("lastAttemptedUploadFile")
    val lastAttemptedUploadFile: String,
    @SerializedName("lastConnectedDatetime")
    val lastConnectedDatetime: String,
    @SerializedName("lastDeviceMacAddress")
    val lastDeviceMacAddress: String,
    @SerializedName("lastDeviceMemoryState")
    val lastDeviceMemoryState: String,
    @SerializedName("lastDeviceOBDState")
    val lastDeviceOBDState: String,
    @SerializedName("lastDisconnectedDatetime")
    val lastDisconnectedDatetime: String,
    @SerializedName("lastDownloadedFile")
    val lastDownloadedFile: String,
    @SerializedName("lastFirmwareVersion")
    val lastFirmwareVersion: String,
    @SerializedName("lastFoundDeviceDatetime")
    val lastFoundDeviceDatetime: String,
    @SerializedName("osVersion")
    val osVersion: String,
    @SerializedName("qualificationNumber")
    val qualificationNumber: String,
    @SerializedName("serviceStatus")
    val serviceStatus: Boolean,
    @SerializedName("ssaid")
    val ssaid: String,
    @SerializedName("tokenString")
    val tokenString: String,
    @SerializedName("tokenUpdateDate")
    val tokenUpdateDate: String,
    @SerializedName("updatedAt")
    val updatedAt: String
)