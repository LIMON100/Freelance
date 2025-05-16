package com.skyautonet.drsafe.model.login

data class LoginRequest(
    val licensePlate: String,
    val name: String,
    val phoneNumber: String,
    val provider: String,
    val token: String,
    val vincode: String
)