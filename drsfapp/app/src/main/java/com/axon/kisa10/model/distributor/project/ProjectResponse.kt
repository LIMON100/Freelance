package com.axon.kisa10.model.distributor.project

data class ProjectResponse(
    val `data`: List<Data>,
    val total: Int
)

data class Data(
    val deviceId: String,
    val id: String,
    val memo: String,
    val name: String
)