package com.axon.kisa10.model


import com.google.gson.annotations.SerializedName

data class UserInfo(
    @SerializedName("createdAt")
    val createdAt: String,
    @SerializedName("deletedAt")
    val deletedAt: Any,
    @SerializedName("email")
    val email: String,
    @SerializedName("id")
    val id: Int,
    @SerializedName("name")
    val name: String,
    @SerializedName("phoneNumber")
    val phoneNumber: String,
    @SerializedName("role")
    val role: Any,
    @SerializedName("socialId")
    val socialId: String,
    @SerializedName("status")
    val status: Any,
    @SerializedName("updatedAt")
    val updatedAt: String,
    @SerializedName("appInformation")
    val appInformation : AppInformation
)