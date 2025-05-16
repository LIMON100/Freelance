package com.axon.kisa10.workmanager

import android.app.Notification
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.ActivityNotFoundException
import android.content.Context
import android.content.Intent
import android.net.Uri
import androidx.core.app.NotificationCompat
import androidx.work.Worker
import androidx.work.WorkerParameters
import com.axon.kisa10.util.AppConstants
import com.google.android.play.core.appupdate.AppUpdateManagerFactory
import com.google.android.play.core.install.model.UpdateAvailability
import com.kisa10.R


/**
 * Created by Hussain on 11/07/24.
 */
class UpdateCheckWorker(private val context : Context, workerParameters: WorkerParameters) : Worker(context, workerParameters) {

    private val notificationManager by lazy {
        context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
    }

    override fun doWork(): Result {
        val appUpdateManager = AppUpdateManagerFactory.create(context)

        val appUpdateInfoTask = appUpdateManager.appUpdateInfo

        appUpdateInfoTask.addOnSuccessListener { appUpdateInfo ->
            if (appUpdateInfo.updateAvailability() == UpdateAvailability.UPDATE_AVAILABLE) {
                val notification = createNotification("Update Available", "Please Update this app to latest version.")
                notificationManager.notify(10, notification)
            }
        }

        return Result.success()
    }

    private fun createNotification(contentTitle : String, contentText : String) : Notification {
        val packageName = context.packageName
        val packageIntent = try {
                Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=$packageName"))
        } catch (anfe: ActivityNotFoundException) {
                Intent(Intent.ACTION_VIEW, Uri.parse("https://play.google.com/store/apps/details?id=$packageName"))
        }
        val pendingIntent = PendingIntent.getActivity(context, 8, packageIntent, PendingIntent.FLAG_IMMUTABLE)
        return NotificationCompat.Builder(context, AppConstants.AXON_BLE_NOTIFICATION_CHANNEL)
            .setContentTitle(contentTitle)
            .setContentText(contentText)
            .setOngoing(false)
            .setContentIntent(pendingIntent)
            .setAutoCancel(true)
            .setSmallIcon(R.mipmap.ic_drsafe_logo)
            .build()
    }
}