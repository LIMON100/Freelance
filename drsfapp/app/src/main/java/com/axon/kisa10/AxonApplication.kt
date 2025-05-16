package com.axon.kisa10

import android.app.Application
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.os.Build
import android.util.Log
import com.axon.kisa10.network.RetrofitBuilder
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.LocaleHelper
import com.axon.kisa10.util.SharedPrefKeys
import com.axon.kisa10.util.SharedPrefManager
import com.google.firebase.FirebaseApp
import com.google.firebase.messaging.FirebaseMessaging
import com.kakao.sdk.common.KakaoSdk
/**
 * Created by Hussain on 26/06/24.
 */
class AxonApplication : Application() {

//    private val appLocale = "en"
    private val appLocale = "ko"

    override fun onCreate() {
        super.onCreate()

        KakaoSdk.init(this, AppConstants.NATIVE_APP_KEY)
        FirebaseApp.initializeApp(this)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val notificationChannel = NotificationChannel(
                AppConstants.AXON_BLE_NOTIFICATION_CHANNEL,
                AppConstants.AXON_BLE_NOTIFICATION_CHANNEL,
                NotificationManager.IMPORTANCE_DEFAULT
            )
            notificationChannel.setShowBadge(false)
            val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            notificationManager.createNotificationChannel(notificationChannel)
        }
        val sharedPrefManager = SharedPrefManager.getInstance(this)
        RetrofitBuilder.sharedPrefManager = sharedPrefManager
        LocaleHelper.setLocale(applicationContext,appLocale)
        val fcmToken = sharedPrefManager.getString(SharedPrefKeys.FCM_TOKEN)
        FirebaseMessaging.getInstance().token.addOnSuccessListener { token ->
            Log.i("AxonApplication", token)
            if (fcmToken != token) {
                sharedPrefManager.setString(SharedPrefKeys.FCM_TOKEN, token)
            }
        }
        .addOnFailureListener {
            Log.e("AxonApplication", "Failed to get token")
        }
        Log.i("AxonApplication", "onCreate: ${sharedPrefManager.getString(SharedPrefKeys.TOKEN)}")
        FirebaseMessaging.getInstance().subscribeToTopic("firmwareUpdate").addOnCompleteListener {
            if (it.isSuccessful) {
                Log.i("AxonApplication", "subscribed to topic.")
            } else {
                Log.i("AxonApplication", "Failed to subscribe to topic.")
            }
        }
    }
}