package com.skyautonet.drsafe

import android.app.Application
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.os.Build
import com.skyautonet.drsafe.network.RetrofitBuilder
import com.google.firebase.FirebaseApp
import com.kakao.sdk.common.KakaoSdk
import com.skyautonet.drsafe.util.AppConstants
import com.skyautonet.drsafe.util.SharedPreferenceUtil

/**
 * Created by Hussain on 07/08/24.
 */
class MainApplication : Application() {

    override fun onCreate() {
        super.onCreate()

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val notificationChannel = NotificationChannel(
                AppConstants.DR_SAFE_BLE_NOTIFICATION_CHANNEL,
                AppConstants.DR_SAFE_BLE_NOTIFICATION_CHANNEL,
                NotificationManager.IMPORTANCE_DEFAULT
            )
            notificationChannel.setShowBadge(false)
            val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            notificationManager.createNotificationChannel(notificationChannel)


        }

        KakaoSdk.init(this, AppConstants.NATIVE_APP_KEY)
        FirebaseApp.initializeApp(this)
        RetrofitBuilder.sharedPrefManager = SharedPreferenceUtil.getInstance(this)
    }

}