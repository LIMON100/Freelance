package com.axon.kisa10.network

import com.axon.kisa10.model.AppInformation
import com.axon.kisa10.model.LatestFirmwareVersionResponse
import com.axon.kisa10.model.UserInfo
import com.axon.kisa10.model.calibration.CalibrationRequest
import com.axon.kisa10.model.calibration.CalibrationResponse
import com.axon.kisa10.model.dbversion.DBVersionResponse
import com.axon.kisa10.model.distributor.calibration.DistributorCalibrationUploadRequest
import com.axon.kisa10.model.distributor.login.InstallerLogin
import com.axon.kisa10.model.distributor.login.InstallerLoginResponse
import com.axon.kisa10.model.distributor.project.ProjectResponse
import com.axon.kisa10.model.fcm.FcmRequest
import com.axon.kisa10.model.register.RegisterVehicleRequest
import com.axon.kisa10.model.register.RegisterVehicleResponse
import com.skydoves.sandwich.ApiResponse
import okhttp3.MultipartBody
import okhttp3.RequestBody
import retrofit2.http.Body
import retrofit2.http.GET
import retrofit2.http.Multipart
import retrofit2.http.PATCH
import retrofit2.http.POST
import retrofit2.http.Part
import retrofit2.http.Path
import retrofit2.http.Query

/**
 * Created by Hussain on 01/07/24.
 */
interface AxonApi {

    @POST("vehicles/register")
    suspend fun registerVehicle(@Body request : RegisterVehicleRequest) : ApiResponse<RegisterVehicleResponse>

    @GET("auth/me")
    suspend fun getUserInfo() : ApiResponse<UserInfo>

    @Multipart
    @POST("app-logs/upload")
    suspend fun uploadLogFile(
        @Part file : MultipartBody.Part,
        @Part("groupId") groupId : RequestBody,
        @Part("vehicleId") vehicleId : RequestBody,
        @Part("type") type : RequestBody
    ) : ApiResponse<Unit>

    @PATCH("app-informations/device-token/{userId}")
    suspend fun uploadFcmToken(
        @Path("userId") userId : String,
        @Body fcmRequest: FcmRequest
    ) : ApiResponse<AppInformation>

    @GET("app-informations/{id}")
    suspend fun getAppInformation(
        @Path("id") id : String
    ) : ApiResponse<AppInformation>

    @PATCH("app-informations/{id}")
    suspend fun uploadAppInformation(
        @Path("id") id : String,
        @Body appInformation: AppInformation
    ) : ApiResponse<AppInformation>

    @GET("firmwares/latestVersion")
    suspend fun getLatestFirmwareVersion(@Query("type") type : String) : ApiResponse<LatestFirmwareVersionResponse>

    @POST("calibrations")
    suspend fun sendCalibrationData(@Body calibrationData : CalibrationRequest) : ApiResponse<CalibrationResponse>

    @GET("dbs/latestVersion")
    suspend fun getLatestDBVersion() : ApiResponse<DBVersionResponse>

    @POST("auth/installer/login")
    suspend fun loginDistributor(@Body installerLogin : InstallerLogin) : ApiResponse<InstallerLoginResponse>

    @POST("vehicles/calibration")
    suspend fun uploadDistributorCalibrationData(@Body data : DistributorCalibrationUploadRequest) : ApiResponse<Unit>

    @GET("projects")
    suspend fun getProjects(@Query("page") page : Int, @Query("limit") limit : Int) : ApiResponse<ProjectResponse>

    @GET("devices")
    suspend fun getDevices(@Query("page") page: Int, @Query("limit") limit: Int) : ApiResponse<ProjectResponse>
}