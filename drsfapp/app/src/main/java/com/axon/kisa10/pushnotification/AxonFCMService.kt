package com.axon.kisa10.pushnotification

import android.app.Notification
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.os.Build
import android.util.Log
import androidx.core.app.NotificationCompat
import androidx.work.Constraints
import androidx.work.Data
import androidx.work.ExistingWorkPolicy
import androidx.work.NetworkType
import androidx.work.OneTimeWorkRequestBuilder
import androidx.work.WorkManager
import com.axon.kisa10.activity.MainActivity
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.model.UserInfo
import com.axon.kisa10.model.fcm.FcmRequest
import com.axon.kisa10.network.RetrofitBuilder
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.SharedPrefKeys
import com.axon.kisa10.util.SharedPrefManager
import com.axon.kisa10.workmanager.EventUploadWorker
import com.axon.kisa10.workmanager.FileUploadWorker
import com.axon.kisa10.workmanager.LogUploadWorker
import com.google.firebase.messaging.FirebaseMessagingService
import com.google.firebase.messaging.RemoteMessage
import com.google.gson.Gson
import com.axon.kisa10.BuildConfig
import com.axon.kisa10.R
import com.skydoves.sandwich.isSuccess
import com.skydoves.sandwich.suspendOnSuccess
import kotlinx.coroutines.runBlocking

/**
 * Created by Hussain on 08/07/24.
 */
class AxonFCMService : FirebaseMessagingService() {

    private val TAG = "AxonFCMService"

    private val sharedPrefManager = SharedPrefManager.getInstance(this)

    private val notificationManager by lazy {
        getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
    }

    override fun onMessageReceived(message: RemoteMessage) {
        Log.i(TAG, "onMessageReceived: ${message.priority} ${message.data}")
        val event = message.data["event"]
        if (event == AppConstants.EVENT_START_SERVICE) {
            val intent = Intent(this,AxonBLEService::class.java).apply {
                action = AppConstants.START_FOREGROUND
            }
            startService(intent)
        } else if (event == AppConstants.EVENT_UPLOAD_LOG) {
            startWorkerManagerForLogUpload()
        } else if (event == AppConstants.EVENT_UPLOAD_EVENT) {
            startWorkerManagerForEventUpload()
        } else if (event == AppConstants.EVENT_UPDATE_STATE) {
            sendAppStatus()
        } else if (event == "updateFirmware") {
            val title = message.data["title"] ?: "Update Firmware"
            val body = message.data["body"] ?: "New firmware is available. Please update your device."
            val notification = createNotification(title, body)
            notificationManager.notify(4, notification)
        } else {
            Log.i(TAG, "Unknown event received : $event")
        }
    }

    private fun startWorkerManagerForEventUpload() {
        val workManager = WorkManager.getInstance(this)
        val workParams = Data.Builder()
        workParams.putString("deviceDir",sharedPrefManager.getString(AppConstants.LAST_CONNECTED_DEVICE))
        val oneTimeWorkRequest = OneTimeWorkRequestBuilder<EventUploadWorker>()
            .setInputData(workParams.build())
            .setConstraints(Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build())
            .build()
        workManager.enqueueUniqueWork(AppConstants.EVENT_UPLOAD_EVENT + "_DEVICE", ExistingWorkPolicy.KEEP,oneTimeWorkRequest)
    }

    private fun startWorkerManagerForLogUpload() {
        val workManager = WorkManager.getInstance(this)
        val workParams = Data.Builder()
        workParams.putString("deviceDir",sharedPrefManager.getString(AppConstants.LAST_CONNECTED_DEVICE))
        val oneTimeWorkRequest = OneTimeWorkRequestBuilder<LogUploadWorker>()
            .setInputData(workParams.build())
            .setConstraints(Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build())
            .build()
        workManager.enqueueUniqueWork(AppConstants.EVENT_UPLOAD_LOG + "_DEVICE", ExistingWorkPolicy.KEEP,oneTimeWorkRequest)

    }

    private fun sendAppStatus() {
        RetrofitBuilder.sharedPrefManager = sharedPrefManager
        val axonApi = RetrofitBuilder.AxonApi
        val gson = Gson()
        val id = gson.fromJson(sharedPrefManager.getString(SharedPrefKeys.USER_INFO), UserInfo::class.java)?.appInformation?.id ?: return
        runBlocking {
            axonApi.getAppInformation(id)
                .suspendOnSuccess {
                    val fcmToken = sharedPrefManager.getString(SharedPrefKeys.FCM_TOKEN)
                    val sdkInt = Build.VERSION.SDK_INT
                    val version = BuildConfig.VERSION_NAME
                    val newInfo = data.copy(appVersion = version, osVersion = sdkInt.toString(), tokenString = fcmToken.toString())
                    axonApi.uploadAppInformation(id,newInfo)
                }
        }
    }

    private fun startWorkerManagerForUpload() {
        val workManager = WorkManager.getInstance(this)
        val oneTimeWorkRequest = OneTimeWorkRequestBuilder<FileUploadWorker>()
            .setConstraints(Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build())
            .build()
        workManager.enqueueUniqueWork(AppConstants.EVENT_UPLOAD_LOG, ExistingWorkPolicy.KEEP,oneTimeWorkRequest)
    }

    override fun onNewToken(token: String) {
        Log.i(TAG, "onNewToken: $token")
        sharedPrefManager.setString(SharedPrefKeys.FCM_TOKEN, token)
        val fcmRequest = FcmRequest(token)
        RetrofitBuilder.sharedPrefManager = sharedPrefManager
        val gson = Gson()
        val userInfo = gson.fromJson(sharedPrefManager.getString(SharedPrefKeys.USER_INFO),UserInfo::class.java) ?: return
        val userId = userInfo.id
        runBlocking {
            val response = RetrofitBuilder.AxonApi.uploadFcmToken(userId.toString(),fcmRequest)
            Log.i(TAG, "onNewToken: ${response.isSuccess}")
        }
    }

    private fun createNotification(contentTitle : String, contentText : String) : Notification {
        val mainActivity = Intent(this,MainActivity::class.java)
        mainActivity.putExtra(AppConstants.EVENT_UPDATE_FIRMWARE,true)
        val pendingIntent = PendingIntent.getActivity(this, 8, mainActivity, PendingIntent.FLAG_IMMUTABLE)
        return NotificationCompat.Builder(this, AppConstants.AXON_BLE_NOTIFICATION_CHANNEL)
            .setContentTitle(contentTitle)
            .setContentText(contentText)
            .setOngoing(false)
            .setContentIntent(pendingIntent)
            .setAutoCancel(true)
            .setSmallIcon(R.mipmap.ic_drsafe_logo)
            .build()
    }
}