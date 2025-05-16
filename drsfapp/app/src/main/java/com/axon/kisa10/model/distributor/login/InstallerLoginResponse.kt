package com.axon.kisa10.model.distributor.login

data class InstallerLoginResponse(
    val refreshToken: String,
    val token: String,
    val tokenExpires: String,
    val user: User
)