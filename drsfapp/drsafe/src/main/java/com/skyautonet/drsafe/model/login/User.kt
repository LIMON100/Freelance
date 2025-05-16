package com.skyautonet.drsafe.model.login

data class User(
    val createdAt: String,
    val deletedAt: String,
    val email: String,
    val id: Int,
    val name: String,
    val phoneNumber: String,
    val provider: String,
    val socialId: String,
    val updatedAt: String,
)