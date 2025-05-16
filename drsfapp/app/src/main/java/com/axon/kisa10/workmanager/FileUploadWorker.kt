package com.axon.kisa10.workmanager

import android.content.Context
import android.os.Environment
import android.util.Log
import androidx.work.Worker
import androidx.work.WorkerParameters
import com.axon.kisa10.model.UserInfo
import com.axon.kisa10.network.RetrofitBuilder
import com.axon.kisa10.util.SharedPrefKeys
import com.axon.kisa10.util.SharedPrefManager
import com.google.gson.Gson
import com.skydoves.sandwich.isSuccess
import kotlinx.coroutines.runBlocking
import okhttp3.MediaType.Companion.toMediaTypeOrNull
import okhttp3.MultipartBody
import okhttp3.RequestBody
import okhttp3.RequestBody.Companion.asRequestBody
import java.io.File

/**
 * Created by Hussain on 05/07/24.
 */
class FileUploadWorker(
    private val context: Context,
    workerParameters: WorkerParameters
) : Worker(context, workerParameters) {

    private val TAG = "FileUploadWorker"
    private val axonApi = RetrofitBuilder.AxonApi
    val sharedPrefManager = SharedPrefManager.getInstance(context)
    override fun doWork(): Result = runBlocking {
        val userInfoString = sharedPrefManager.getString(SharedPrefKeys.USER_INFO)
        val userInfo = Gson().fromJson(userInfoString,UserInfo::class.java)
        val groupId = userInfo.id
        val vehicleId = sharedPrefManager.getString(SharedPrefKeys.VEHICLE_ID)
        val downloadDirectory = context.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)?.path.toString()
        val dir = File(downloadDirectory)
        val files = if (dir.exists()) {
            dir.listFiles()
        } else {
            null
        } ?: return@runBlocking Result.failure()
        val logFiles = files.filter { it.nameWithoutExtension.startsWith(groupId.toString(),ignoreCase = true) }
        if (logFiles.isEmpty()) {
            return@runBlocking Result.failure()
        } else {
            var uploaded = 0
            for (logFile in logFiles) {
                if (logFile.exists() && vehicleId != null) {
                    // upload data to server
                    val filePart = MultipartBody.Part.createFormData(
                        "file",
                        logFile.name,
                        logFile.asRequestBody("txt/*".toMediaTypeOrNull())
                    )
                    val groupIdReq = RequestBody.create("text/plain".toMediaTypeOrNull(), groupId.toString())
                    val vehicleIdReq = RequestBody.create("text/plain".toMediaTypeOrNull(), vehicleId.toString())
                    val type = RequestBody.create("text/plain".toMediaTypeOrNull(), "1")
                    val response = axonApi.uploadLogFile(filePart, groupIdReq, vehicleIdReq, type)
                    if (response.isSuccess) {
                        Log.i(TAG, "Successfully uploaded")
                        Log.i(TAG, "delete file : ${logFile.delete()}")
                        uploaded += 1
                    } else {
                        Log.i(TAG, "Failed to upload file")
                    }
                }
            }
            if (uploaded == logFiles.size) {
                return@runBlocking Result.success()
            } else {
                return@runBlocking Result.failure()
            }
        }
//        if (file.exists() && vehicleId != null) {
//            // upload data to server
//            val filePart = MultipartBody.Part.createFormData(
//                "file",
//                file.name,
//                file.asRequestBody("txt/*".toMediaTypeOrNull())
//            )
//            val groupIdReq = RequestBody.create("text/plain".toMediaTypeOrNull(), groupId.toString())
//            val vehicleIdReq = RequestBody.create("text/plain".toMediaTypeOrNull(), vehicleId.toString())
//            val response = axonApi.uploadLogFile(filePart, groupIdReq, vehicleIdReq)
//            if (response.isSuccess) {
//                Log.i(TAG, "Successfully uploaded")
//                Log.i(TAG, "delete file : ${file.delete()}")
//                return@runBlocking Result.success()
//            } else {
//                Log.i(TAG, "Failed to upload file")
//                return@runBlocking Result.failure()
//            }
//        } else {
//            return@runBlocking Result.failure()
//        }
    }
}