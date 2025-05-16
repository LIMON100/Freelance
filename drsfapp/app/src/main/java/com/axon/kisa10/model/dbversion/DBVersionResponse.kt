package com.axon.kisa10.model.dbversion

import com.google.gson.annotations.SerializedName

/**
 * Created by Hussain on 25/09/24.
 */
data class DBVersionResponse(
    @SerializedName("id")
    val id : String,
    @SerializedName("version")
    val version : String,
    @SerializedName("createdAt")
    val createdAt : String,
    @SerializedName("updatedAt")
    val updatedAt : String,
    @SerializedName("file")
    val file : DBFile
)

data class DBFile(
    @SerializedName("id")
    val id: String,
    @SerializedName("path")
    val path : String
)