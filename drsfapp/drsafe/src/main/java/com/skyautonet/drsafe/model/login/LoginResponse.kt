package com.skyautonet.drsafe.model.login

data class LoginResponse(
    val refreshToken: String,
    val token: String,
    val tokenExpires: String,
    val user: User
)