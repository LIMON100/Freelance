package com.skyautonet.drsafe.network

import com.skyautonet.drsafe.model.login.LoginRequest
import com.skyautonet.drsafe.model.login.LoginResponse
import com.skydoves.sandwich.ApiResponse
import retrofit2.Response
import retrofit2.http.Body
import retrofit2.http.POST

/**
 * Created by Hussain on 01/07/24.
 */
interface IDrSafeApi {

    @POST("auth/third-party/login")
    suspend fun login(@Body loginRequest : LoginRequest) : ApiResponse<LoginResponse>
}