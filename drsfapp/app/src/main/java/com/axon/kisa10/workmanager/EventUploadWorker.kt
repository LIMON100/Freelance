package com.axon.kisa10.workmanager

import android.content.Context
import android.util.Log
import androidx.work.Worker
import androidx.work.WorkerParameters
import com.axon.kisa10.model.UserInfo
import com.axon.kisa10.network.RetrofitBuilder
import com.axon.kisa10.util.SharedPrefKeys
import com.axon.kisa10.util.SharedPrefManager
import com.axon.kisa10.util.getEventDownloadDirectory
import com.google.gson.Gson
import com.skydoves.sandwich.isSuccess
import kotlinx.coroutines.runBlocking
import okhttp3.MediaType.Companion.toMediaTypeOrNull
import okhttp3.MultipartBody
import okhttp3.RequestBody
import okhttp3.RequestBody.Companion.asRequestBody
import java.io.File

/**
 * Created by Hussain on 03/10/24.
 */
class EventUploadWorker(
    private val context : Context,
    private val workerParameters: WorkerParameters
) : Worker(context, workerParameters)  {

    private val TAG = "EventUploadWorker"
    private val axonApi = RetrofitBuilder.AxonApi
    val sharedPrefManager = SharedPrefManager.getInstance(context)

    override fun doWork(): Result = runBlocking {
        val userInfoString = sharedPrefManager.getString(SharedPrefKeys.USER_INFO)
        val userInfo = Gson().fromJson(userInfoString, UserInfo::class.java)
        val groupId = userInfo.id
        val vehicleId = sharedPrefManager.getString(SharedPrefKeys.VEHICLE_ID)
        val mainDirPath = context.getEventDownloadDirectory() + workerParameters.inputData.getString("deviceDir")
        val mainDir = File(mainDirPath)
        Log.i(TAG, "vehicleId : $vehicleId, groupId : $groupId")
        if (mainDir.exists()) {
            val logs = mainDir.listFiles()
            if (logs.isNullOrEmpty()) {
                Log.i(TAG, "doWork: file list empty")
                return@runBlocking Result.failure()
            }
            Log.i(TAG, "upload : ${logs.size} files")
            var uploaded = 0
            for (file in logs) {
                Log.i(TAG, "doWork: uploading : $file")
                val filePart = MultipartBody.Part.createFormData(
                    "file",
                    file.name,
                    file.asRequestBody("txt/*".toMediaTypeOrNull())
                )
                val groupIdReq = RequestBody.create("text/plain".toMediaTypeOrNull(), groupId.toString())
                val vehicleIdReq = RequestBody.create("text/plain".toMediaTypeOrNull(), vehicleId.toString())
                val type = RequestBody.create("text/plain".toMediaTypeOrNull(), "2")
                val response = axonApi.uploadLogFile(filePart, groupIdReq, vehicleIdReq, type)
                if (response.isSuccess) {
                    Log.i(TAG, "Successfully uploaded")
                    uploaded += 1
                } else {
                    Log.i(TAG, "upload failed : $file")
                }
            }

            if (uploaded == logs.size) {
                Log.d(TAG, "All Files Uploaded")
                return@runBlocking Result.success()
            } else {
                return@runBlocking Result.failure()
            }

        } else {
            Log.i(TAG, " path $mainDir does not exist.")
        }
        return@runBlocking Result.success()
    }
}