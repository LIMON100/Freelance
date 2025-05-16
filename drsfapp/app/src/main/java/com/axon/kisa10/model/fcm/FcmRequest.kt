package com.axon.kisa10.model.fcm

import com.google.gson.annotations.SerializedName

/**
 * Created by Hussain on 08/07/24.
 */
data class FcmRequest(
    @SerializedName("deviceToken")
    val deviceToken : String
)