package com.axon.kisa10.workmanager

import android.content.Context
import android.content.Intent
import android.util.Log
import androidx.work.Worker
import androidx.work.WorkerParameters
import com.axon.kisa10.ble.LogDownloadService

/**
 * Created by Hussain on 03/10/24.
 */
class LogDownloadWorker(
    private val context: Context,
    private val workerParameters: WorkerParameters
) : Worker(context, workerParameters) {

    private val TAG = "LogDownloadWorker"

    override fun doWork(): Result {
        val serviceIntent = Intent(context, LogDownloadService::class.java)
        val success = context.startService(serviceIntent)
        if (success != null) {
            Log.i(TAG, "service started")
            return Result.success()
        } else {
            Log.i(TAG, "unable to start service")
            return Result.failure()
        }
    }
}