package com.axon.kisa10.fragment.log

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.fragment.app.Fragment
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.activity.MainActivity
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AxonCRC32
import com.axon.kisa10.util.AxonUtil
import com.axon.kisa10.util.LogUtil
import com.axon.kisa10.util.forEachVisibleHolder
import com.axon.kisa10.util.getDeviceDirectory
import com.axon.kisa10.util.getLogDownloadDirectory
import com.kisa10.R
import com.kisa10.databinding.FragmentLogListBinding
import java.io.File
import java.io.FileOutputStream
import java.util.LinkedList

/**
 * Created by Hussain on 10/09/24.
 */
class LogListFragment(private val bleService: AxonBLEService) : Fragment() {

    private val TAG = "LogListFragment"

    private lateinit var binding : FragmentLogListBinding

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(requireContext())
    }

    private val logListAdapter by lazy {
        LogListAdapter(requireContext())
    }

    private var logState = LOG_STATE.LOG_LIST
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
    private var logResponse = ""
    private var currentFileViewHolder : LogListAdapter.LogListVH? = null
    private var isDownloading = false

    override fun onAttach(context: Context) {
        super.onAttach(context)
        val intentFilter = IntentFilter()
        intentFilter.addAction(AppConstants.ACTION_DEVICE_DISCONNECTED)
        intentFilter.addAction(AppConstants.ACTION_CHARACTERISTIC_CHANGED)
        localBroadcastManager.registerReceiver(mBroadcastReceiver, intentFilter)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentLogListBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        binding.rvLogList.adapter = logListAdapter
        binding.btnDownload.setOnClickListener {
            binding.btnDownload.isEnabled = false
            val last7days = AxonUtil.getLast7DaysFrom().map { "$it.LOG" }
            for (i in last7days) {
                if (logFiles.contains(i)) {
                    downloadQueue.add(i)
                }
            }
//            if (downloadQueue.isEmpty()) {
//                downloadQueue.addAll(logFiles)
//            }
            downloadNextFile()
        }
        logListAdapter.registerAdapterDataObserver(object : androidx.recyclerview.widget.RecyclerView.AdapterDataObserver() {
            override fun onChanged() {
                Handler(Looper.getMainLooper()).postDelayed({
                    updateRecyclerView()
                },100)
            }
        })
        bleService.readDeviceData(AppConstants.COMMAND_STORED_LOG_FILE_LIST)
    }

    private fun downloadNextFile() {
        Log.i(TAG, "Remaining Download File : ${downloadQueue.size}")
        if (downloadQueue.isEmpty()) {
            Log.i(TAG, "Download Queue Empty")
            checkAllFilesDownloaded()
            binding.btnDownload.isEnabled = true
            if (isDownloading) {
                bleService.readDeviceData(AppConstants.COMMAND_REQUEST_FILE + "," + AppConstants.RESPONSE_E)
                isDownloading = false
            }
            return
        }
        isDownloading = true
        currentFileName = downloadQueue.poll() ?: return
        currentFileViewHolder = getViewHolderForFile()
        if (!logFileExist(currentFileName)) {
            currentFileViewHolder?.showProgress(true)
            Log.i(TAG, "starting download of file $currentFileName")
            Log.i(TAG, "sending cmd : ${AppConstants.COMMAND_REQUEST_FILE + "," + currentFileName}")
            bleService.readDeviceData(AppConstants.COMMAND_REQUEST_FILE + "," + currentFileName)
        } else {
            currentFileViewHolder?.showProgress(false)
            currentFileViewHolder?.toggleCheckImg(true)
            Log.i(TAG, "Download file $currentFileName already exist")
            downloadNextFile()
        }
    }

    private fun checkAllFilesDownloaded() {
        var downloaded = 0
        for (log in logFiles) {
            if (logFileExist(log)) {
                downloaded += 1
            }
        }
        if (downloaded == logFiles.size) {
            Log.i(TAG, "All files downloaded")
            binding.btnDownload.visibility = View.GONE
        }
    }

    private val mBroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (intent.action == AppConstants.ACTION_DEVICE_DISCONNECTED) {
                Toast.makeText(requireContext(), getString(R.string.device_disconnected), Toast.LENGTH_SHORT).show()
                localBroadcastManager.sendBroadcast(Intent(MainActivity.FIRMWARE_UPDATE_OK))
            }

            if (intent.action == AppConstants.ACTION_CHARACTERISTIC_CHANGED) {
                val response: String?
                try {
                    response = intent.getStringExtra("data")
                    val rawData = intent.getByteArrayExtra(AppConstants.RAW_DATA) ?: ByteArray(0)
                    parseResponse(response, rawData)
                    Log.i("102_RESPONSE", "Response : $response")
                } catch (e: Exception) {
                    e.printStackTrace()
                }
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
                            updateList(logList.split(",").toTypedArray())
                        } else {
                            Log.i(TAG,"Waiting for file list")
                            logResponse += logList
                            Log.i(TAG, "log partial : $logResponse")
                        }
                    }
                }

                if (responseArray[2].trim() == AppConstants.RESPONSE_F) {
                    val errorCode = responseArray[3].trim()
                    Toast.makeText(requireContext(), "Command Failed with Error Code: $errorCode", Toast.LENGTH_SHORT).show()
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
                        currentFileViewHolder?.showProgress(false)
                        currentFileViewHolder?.toggleCheckImg(true)
                        if (checkCrcCode()) {
                            bleService.readDeviceData(AppConstants.COMMAND_REQUEST_FILE + "," + AppConstants.RESPONSE_A)
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
            currentFileViewHolder?.setProgress(progress.toInt())
            Log.i(TAG, "received file size : ${receivedData.size}")
            if (receivedChunkSize >= chunkSize) {
                receivedData += dataChunk.slice(IntRange(0, dataChunk.size-2))
                Log.i(TAG, "chunk size : $receivedChunkSize")
                receivedChunkSize = 0
                if (receivedData.size >= fileSize) {
                    Log.i(TAG, "File Download Complete, Sending Confirmation : RESPONSE A")
                    Log.i(TAG, "received file size : ${receivedData.size}")                    // file received
                    currentFileViewHolder?.showProgress(false)
                    currentFileViewHolder?.toggleCheckImg(true)
                    if (checkCrcCode()) {
                        bleService.readDeviceData(AppConstants.COMMAND_REQUEST_FILE + "," + AppConstants.RESPONSE_A)
                    }
                } else {
                    // request next data portion
                    Log.i(TAG, "Requesting Next Data Portion : RESPONSE C")
                    bleService.readDeviceData(AppConstants.COMMAND_REQUEST_FILE + "," + AppConstants.RESPONSE_C)
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
                Log.i("LOG_LIST", "file list: $logResponse")
                updateList(logResponse.split(",").toTypedArray())
            }
        } else {
            Log.i(TAG, "Unknown command received : $response")
        }
    }

    private fun checkCrcCode(): Boolean {
        axonCRC32.reset()
        axonCRC32.update(receivedData.copyOfRange(0,receivedData.size-4), receivedData.size-4)
        val crc = receivedData.copyOfRange(receivedData.size-4, receivedData.size)
        val crcValue = bytesToInt(crc)
        receivedData = receivedData.copyOfRange(0, receivedData.size-4)
        Log.i(TAG, "crc received : $crcValue")
        Log.i(TAG, "crc value calculated : ${axonCRC32.value}")
        if (bytesToInt(crc) == axonCRC32.value) {
            Log.i(TAG, "crc matched")
            return true
        } else {
            return false
        }
    }

    private fun bytesToInt(bytes: ByteArray): Int {
        var result = 0
        for (i in bytes.indices) {
            result = result or (bytes[i].toInt() and 0xFF shl (8 * i))
        }
        return result
    }

    private fun updateList(logList: Array<String>) {
        val last7Days = AxonUtil.getLast7DaysString() + " " + getString(R.string.download)
        binding.btnDownload.text = last7Days
        this.logFiles = logList.map { it.trim().replace("\n","").replace("\t","") }.toTypedArray()
        var downloaded = 0
        for (log in logList) {
            if (logFileExist(log)) {
                downloaded += 1
            }
        }

        if (logList.isEmpty()) {
            binding.llLogList.visibility = View.GONE
            binding.llNoItems.visibility = View.VISIBLE
        } else if (downloaded == logList.size) {
            binding.llLogList.visibility = View.VISIBLE
            binding.llNoItems.visibility = View.GONE
            binding.btnDownload.visibility = View.GONE
            logListAdapter.updateList(logList)
        } else {
            binding.llLogList.visibility = View.VISIBLE
            binding.llNoItems.visibility = View.GONE
            logListAdapter.updateList(logList)
        }
    }

    private fun saveLogFile() {
        try {
            Log.i(TAG, "saving file $currentFileName")
            val filePath = requireContext().getLogDownloadDirectory() + bleService.getDeviceDirectory()
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
            val filepath = requireContext().getLogDownloadDirectory() + bleService.getDeviceDirectory() + filename
            val file = File(filepath)
            Log.i(TAG, "$filename exist : ${file.exists()}")
            return file.exists()
        } catch (e : Exception) {
            Log.e(TAG, "logFileExist", e.cause)
            return false
        }
    }

    private fun updateRecyclerView() {
        binding.rvLogList.forEachVisibleHolder<LogListAdapter.LogListVH> {
            it.toggleCheckImg(logFileExist(it.filename))
        }
    }

    private fun getViewHolderForFile(): LogListAdapter.LogListVH? {
        var viewHolder : LogListAdapter.LogListVH? = null
        binding.rvLogList.forEachVisibleHolder<LogListAdapter.LogListVH> {
            if (it.filename == currentFileName) {
                viewHolder = it
            }
        }
        return viewHolder
    }

    override fun onDestroy() {
        super.onDestroy()
        localBroadcastManager.unregisterReceiver(mBroadcastReceiver)
    }
}

enum class LOG_STATE {
    UNKNOWN,
    LOG_LIST,
    LOG_DOWNLOAD
}