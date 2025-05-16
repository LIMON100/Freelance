package com.axon.kisa10.data

import android.annotation.SuppressLint
import android.content.Context
import android.os.Environment
import com.axon.kisa10.model.UserInfo
import com.axon.kisa10.util.SharedPrefKeys
import com.axon.kisa10.util.SharedPrefManager
import com.google.gson.Gson
import java.io.File
import java.io.FileOutputStream
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * Created by Hussain on 05/07/24.
 */
class SdiDataLogger private constructor(private val context : Context) {

    companion object {
        @SuppressLint("StaticFieldLeak")
        private lateinit var instance : SdiDataLogger

        fun getInstance(context: Context): SdiDataLogger {
            if (!this::instance.isInitialized) {
                instance = SdiDataLogger(context)
            }
            return instance
        }
    }

    private var file : File
    private var fileName : String
    init {
        val sharedPrefManager = SharedPrefManager.getInstance(context)
        val userInfoString = sharedPrefManager.getString(SharedPrefKeys.USER_INFO)
        val userInfo = Gson().fromJson(userInfoString,UserInfo::class.java)
        val phoneNumber = userInfo.phoneNumber
        val id = userInfo.id
        val date = SimpleDateFormat("yyyyMMdd", Locale.getDefault()).format(Date())
        fileName = "${id}_${phoneNumber}_$date" + ".txt"
        val downloadDirectory = context.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)?.path.toString()
        file = File(downloadDirectory,fileName)
    }

    suspend fun writeLogLine(line : String) {
        val outputStream = FileOutputStream(file,true)
        outputStream.use {
            it.write(line.toByteArray())
            it.write("\n".toByteArray())
            it.flush()
        }

    }
}