package com.axon.kisa10.ble

import android.annotation.SuppressLint
import android.app.Service
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.IBinder
import android.util.Log
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.fragment.log.LOG_STATE
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AxonCRC32
import com.axon.kisa10.util.AxonUtil
import com.axon.kisa10.util.LogUtil
import com.axon.kisa10.util.getDeviceDirectory
import com.axon.kisa10.util.getLogDownloadDirectory
import java.io.File
import java.io.FileOutputStream
import java.util.LinkedList


/**
 * Created by Hussain on 26/09/24.
 */
class LogDownloadService : Service() {

    private val TAG = "LogDownloadService"
    override fun onBind(p0: Intent?): IBinder? {
        return null
    }

    private lateinit var axonBLEService: AxonBLEService

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(this)
    }

    private val serviceConnector = ServiceConnector({
        axonBLEService = it
        if (axonBLEService.isConnected()) {
            listLogFiles()
        } else {
            Log.i(TAG, "device not connected, stopping service")
            stopSelf()
        }
    }, {
        stopSelf()
    })

    private var logState = LOG_STATE.UNKNOWN
    private var logResponse = ""
    private var chunkSize = 0
    private var startIndex = 0
    private var fileSize = 0
    private var receivedData = ByteArray(0)
    private val axonCRC32 = AxonCRC32()
    private var receivedChunkSize = 0
    private var totalReceived = 0
    private var currentFileName = ""
    private var downloadQueue = LinkedList<String>()
    private var logFiles: Array<String> = arrayOf()
    private var isDownloading = false

    private fun listLogFiles() {
        axonBLEService.readDeviceData(AppConstants.COMMAND_STORED_LOG_FILE_LIST)
    }

    override fun onCreate() {
        super.onCreate()
        val intent = Intent(this, AxonBLEService::class.java)
        bindService(intent, serviceConnector, BIND_AUTO_CREATE)
        localBroadcastManager.registerReceiver(gattUpdateReceiver, AppConstants.makeIntentFilter())
    }

    override fun onDestroy() {
        super.onDestroy()
        unbindService(serviceConnector)
        localBroadcastManager.unregisterReceiver(gattUpdateReceiver)
    }

    private val gattUpdateReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (intent.action == AppConstants.ACTION_CHARACTERISTIC_CHANGED) {
                val data = intent.getStringExtra("data")
                val rawData = intent.getByteArrayExtra(AppConstants.RAW_DATA) ?: ByteArray(0)
                parseResponse(data, rawData)
            }
        }
    }

    private fun parseResponse(response: String?, rawData : ByteArray) {
        val responseArray = response?.split(",")?.toTypedArray() ?: return
        if (responseArray[0].trim() == AppConstants.COMMAND_RES) {
            val command = responseArray[1].trim()
            if (command == AppConstants.COMMAND_STORED_LOG_FILE_LIST) {
                Log.i(TAG, "LOG LIST RESPONSE")
                logState = LOG_STATE.LOG_LIST
                if (responseArray[2].trim() == AppConstants.RESPONSE_S) {
                    Log.i(TAG, "LogList -> RESPONSE S")
                    if (responseArray.size >= 4) {
                        var logList = ""
                        var cnt = 0
                        for (i in response.indices) {
                            if (response[i] == ',') {
                                cnt++
                            }

                            if (cnt == 3) {
                                logList = response.substring(i+1)
                                break
                            }
                        }
                        if (logList.endsWith(";")) {
                            logList = logList.replace(";","")
                            Log.i(TAG, "file list: $logList")
                            logFiles = logList.split(",").toTypedArray()
                            getLast3DaysFiles()
                        } else {
                            Log.i(TAG,"Waiting for file list")
                            logResponse += logList
                            Log.i(TAG, "log partial : $logResponse")
                        }
                    }
                }

                if (responseArray[2].trim() == AppConstants.RESPONSE_F) {
                    val errorCode = responseArray[3].trim()
//                    Toast.makeText(this, "Command Failed with Error Code: $errorCode", Toast.LENGTH_SHORT).show()
                }
            } else if (command == AppConstants.COMMAND_REQUEST_FILE) {
                Log.i(TAG, "Log Download Response")
                logState = LOG_STATE.LOG_DOWNLOAD
                val status = responseArray[2].trim().replace(";","")
                // if response D then extract parameter and data portion
                if (status == AppConstants.RESPONSE_D) {
                    fileSize = responseArray[3].trim().toInt()
                    startIndex = responseArray[4].trim().toInt()
                    chunkSize = responseArray[5].trim().toInt()

                    Log.i(TAG, "fileSize : $fileSize")
                    Log.i(TAG, "startIndex : $startIndex")
                    Log.i(TAG, "chunkSize : $chunkSize")

                    // extract data portion
                    var cnt = 0
                    var data = ByteArray(0)
                    for (i in rawData.indices) {
                        if (rawData[i].toChar() == ',') {
                            cnt++
                        }
                        if (cnt == 6) {
                            data = rawData.copyOfRange(i+1,rawData.size)
                            break
                        }
                    }
                    receivedData += data
                    receivedChunkSize = data.size
                    totalReceived += data.size
                    Log.i(TAG, "DATA : \n${LogUtil.bytesToHex(data)}")
                    // check if the file is received
                    if (receivedData.size >= fileSize) {
                        if (receivedData.last().toChar() == ';') {
                            receivedData = receivedData.copyOfRange(0, receivedData.size-1)
                        }
                        Log.i(TAG, "File Download Complete, Sending Confirmation : RESPONSE A")
                        Log.i(TAG, "received file size : ${receivedData.size}")
                        if (checkCrcCode()) {
                            axonBLEService.readDeviceData(AppConstants.COMMAND_REQUEST_FILE + "," + AppConstants.RESPONSE_A)
                        }
                    }

                    Log.i(TAG,"Total Received Length : ${receivedData.size}")
//                    Log.i(TAG, "chunk size : $receivedChunkSize")
                    Log.i("102_Total", "totalReceived : $totalReceived")

                } else if (status == AppConstants.RESPONSE_A) { // if response A then file download is complete
                    Log.i(TAG, "DEVICE RESPONSE A : Transfer Complete")
                    fileSize = 0
                    chunkSize = 0
                    startIndex = 0
                    receivedChunkSize = 0
                    // save data to file
                    saveLogFile()
                    receivedData = ByteArray(0)
                    downloadNextFile()
                } else if (status == AppConstants.RESPONSE_F) { // if response F then file download failed
                    // transfer failed
                    fileSize = 0
                    chunkSize = 0
                    startIndex = 0
                    receivedData = ByteArray(0)
                    receivedChunkSize = 0
                    axonCRC32.reset()
                    val code = responseArray[3]
                    Log.i(TAG, "Transfer Failed with error code : $code")
                    downloadNextFile()
                }
            } else {
                Log.i(TAG, "Unknown command received : $response")
                logState = LOG_STATE.UNKNOWN
            }
        } else if (logState == LOG_STATE.LOG_DOWNLOAD) {
            // receive data chunks
            Log.i(TAG, "Received Data Chunk")
            val dataChunk = rawData
            Log.i(TAG, "DATA : \n${LogUtil.bytesToHex(dataChunk)}")
            totalReceived += dataChunk.size
            Log.i("102_Total", "totalReceived : $totalReceived")
//            Log.i(TAG, "data : ${LogUtil.bytesToHex(dataChunk)}")
            receivedChunkSize += dataChunk.size
            val progress = (receivedData.size.toFloat() / fileSize.toFloat()) * 100
//            Log.i(TAG, "progress : ${progress.roundToInt()}")
            Log.i(TAG, "received file size : ${receivedData.size}")
            if (receivedChunkSize >= chunkSize) {
                receivedData += dataChunk.slice(IntRange(0, dataChunk.size-2))
                Log.i(TAG, "chunk size : $receivedChunkSize")
                receivedChunkSize = 0
                if (receivedData.size >= fileSize) {
                    Log.i(TAG, "File Download Complete, Sending Confirmation : RESPONSE A")
                    Log.i(TAG, "received file size : ${receivedData.size}")                    // file received
                    if (checkCrcCode()) {
                        axonBLEService.readDeviceData(AppConstants.COMMAND_REQUEST_FILE + "," + AppConstants.RESPONSE_A)
                    }
                } else {
                    // request next data portion
                    Log.i(TAG, "Requesting Next Data Portion : RESPONSE C")
                    axonBLEService.readDeviceData(AppConstants.COMMAND_REQUEST_FILE + "," + AppConstants.RESPONSE_C)
                }
            } else {
                receivedData += dataChunk
//                Log.i(TAG, "chunk size : $receivedChunkSize")
            }
        } else if (logState == LOG_STATE.LOG_LIST) {
            Log.i(TAG, "received additional log file data : $response")
            logResponse += response
            if (logResponse.endsWith(";")) {
                logResponse = logResponse.replace(";","")
                logFiles = logResponse.split(",").toTypedArray()
                Log.i("LOG_LIST", "file list: $logResponse")
                getLast3DaysFiles()
            }
        } else {
            Log.i(TAG, "Unknown command received : $response")
        }
    }

    @SuppressLint("NewApi")
    private fun getLast3DaysFiles() {
        if (logFiles.isEmpty()) {
            Log.d(TAG, "getLast3DaysFiles: no files to download")
            stopSelf()
            return
        }
//        val last3Days = AxonUtil.getNext3DaysFrom(logFiles.first().split(".")[0]).map { "$it.LOG" }
//        for (day in last3Days) {
//            if (logFiles.contains(day) && !logFileExist(day)) {
//                downloadQueue.add(day)
//            }
//        }
        for (logFile in logFiles) {
            if (!logFileExist(logFile)) {
                downloadQueue.add(logFile)
            } else {
                Log.i(TAG, "file already exist : $logFile")
            }
        }
        downloadNextFile()
    }

    private fun checkCrcCode(): Boolean {
        axonCRC32.reset()
        axonCRC32.update(receivedData.copyOfRange(0,receivedData.size-4), receivedData.size-4)
        val crc = receivedData.copyOfRange(receivedData.size-4, receivedData.size)
        val crcValue = AxonUtil.bytesToInt(crc)
        receivedData = receivedData.copyOfRange(0, receivedData.size-4)
        Log.i(TAG, "crc received : $crcValue")
        Log.i(TAG, "crc value calculated : ${axonCRC32.value}")
        if (AxonUtil.bytesToInt(crc) == axonCRC32.value) {
            Log.i(TAG, "crc matched")
            return true
        } else {
            return false
        }
    }


    private fun saveLogFile() {
        try {
            Log.i(TAG, "saving file $currentFileName")
            val filePath = this.getLogDownloadDirectory() + axonBLEService.getDeviceDirectory()
            val sd = File(filePath)
            if (!sd.exists()) {
                sd.mkdirs()
            }
            val bfile = File(sd, currentFileName)
            if (!bfile.exists()) {
                bfile.createNewFile()
            }
            val outputStream = FileOutputStream(bfile)
            outputStream.use {
                it.write(receivedData)
            }
            Log.i(TAG, "file saved $currentFileName")
        } catch (e : Exception) {
            Log.e(TAG, "Unable to save file $currentFileName", e.cause)
        }
    }

    private fun logFileExist(filename : String) : Boolean {
        try {
            val filepath = this.getLogDownloadDirectory() + axonBLEService.getDeviceDirectory() + filename
            val file = File(filepath)
            Log.i(TAG, "$filename exist : ${file.exists()}")
            return file.exists()
        } catch (e : Exception) {
            Log.e(TAG, "logFileExist", e.cause)
            return false
        }
    }

    private fun downloadNextFile() {
        Log.i(TAG, "Remaining Download File : ${downloadQueue.size}")
        if (downloadQueue.isEmpty()) {
            Log.i(TAG, "Download Queue Empty")
            if (isDownloading) {
                axonBLEService.readDeviceData(AppConstants.COMMAND_REQUEST_FILE + "," + AppConstants.RESPONSE_E)
                isDownloading = false
            }
            stopSelf()
            return
        }
        isDownloading = true
        currentFileName = downloadQueue.poll() ?: return
        if (!logFileExist(currentFileName)) {
            Log.i(TAG, "starting download of file $currentFileName")
            Log.i(TAG, "sending cmd : ${AppConstants.COMMAND_REQUEST_FILE + "," + currentFileName}")
            axonBLEService.readDeviceData(AppConstants.COMMAND_REQUEST_FILE + "," + currentFileName)
        } else {
            Log.i(TAG, "Download file $currentFileName already exist")
            downloadNextFile()
        }
    }


}