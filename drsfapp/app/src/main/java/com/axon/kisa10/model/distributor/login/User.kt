package com.axon.kisa10.model.distributor.login

data class User(
    val email: String,
    val id: Int,
    val name: String,
    val phoneNumber: String,
    val photo: Photo,
    val provider: String,
    val socialId: String
)