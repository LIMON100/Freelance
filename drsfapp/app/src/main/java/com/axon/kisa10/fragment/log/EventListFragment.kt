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
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AxonCRC32
import com.axon.kisa10.util.AxonUtil
import com.axon.kisa10.util.LogUtil
import com.axon.kisa10.util.forEachVisibleHolder
import com.axon.kisa10.util.getDeviceDirectory
import com.axon.kisa10.util.getEventDownloadDirectory
import com.kisa10.R
import com.kisa10.databinding.FragmentEventListBinding
import java.io.File
import java.io.FileOutputStream
import java.util.LinkedList

/**
 * Created by Hussain on 11/09/24.
 */
class EventListFragment(private val bleService: AxonBLEService) : Fragment() {

    private val TAG = "EventListFragment"

    private lateinit var binding : FragmentEventListBinding

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(requireContext())
    }

    private val eventListAdapter by lazy {
        EventListAdapter(requireContext())
    }

    private var logState = EventState.UNKNOWN
    private var fileSize = 0
    private var startIndex = 0
    private var chunkSize = 0
    private var receivedData = ByteArray(0)
    private var receivedChunkSize = 0
    private var currentFileName = ""
    private var eventList: Array<String> = emptyArray()
    private var downloadQueue = LinkedList<String>()
    private val axonCRC32 = AxonCRC32()
    private var logResponse = ""
    private var currentFileViewHolder : EventListAdapter.EventListHolder? = null
    private var isDownloading = false

    override fun onAttach(context: Context) {
        super.onAttach(context)
        val intentFilter = IntentFilter()
        intentFilter.addAction(AppConstants.ACTION_CHARACTERISTIC_CHANGED)
        intentFilter.addAction(AppConstants.ACTION_DEVICE_DISCONNECTED)
        localBroadcastManager.registerReceiver(mGattUpdateReceiver, intentFilter)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentEventListBinding.inflate(inflater,container,false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        binding.rvEventList.adapter = eventListAdapter

        binding.btnDownload.setOnClickListener {
            binding.btnDownload.isEnabled = false
            downloadQueue.addAll(eventList)
            val last7days = AxonUtil.getLast7DaysFrom().map { "$it.EVT" }
            for (i in last7days) {
                if (eventList.contains(i)) {
                    downloadQueue.add(i)
                }
            }
//            bleService.stopMapRequestTimer()
            downloadNextFile()
        }
        eventListAdapter.registerAdapterDataObserver(object : androidx.recyclerview.widget.RecyclerView.AdapterDataObserver() {
            override fun onChanged() {
                Handler(Looper.getMainLooper()).postDelayed({
                    updateRecyclerView()
                },100)
            }
        })
        bleService.readDeviceData(AppConstants.COMMAND_STORED_EVENT_FILE_LIST)
    }

    private fun downloadNextFile() {
        Log.i(TAG, "Remaining Download File : ${downloadQueue.size}")
        if (downloadQueue.isEmpty()) {
            Log.i(TAG, "Download Queue Empty")
            checkAllFilesDownloaded()
            binding.btnDownload.isEnabled = true
            if (isDownloading) {
                bleService.readDeviceData(AppConstants.COMMAND_REQUEST_FILE + "," + AppConstants.RESPONSE_E)
            }
            return
        }
        isDownloading = true
        currentFileName = downloadQueue.poll() ?: return
        currentFileViewHolder = getViewHolderForFile()
        if (!eventFileExist(currentFileName)) {
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

    private val mGattUpdateReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val response = intent.getStringExtra("data")
            val rawData = intent.getByteArrayExtra(AppConstants.RAW_DATA) ?: ByteArray(0)
            parseResponse(response, rawData)
        }
    }

    private fun parseResponse(response: String?, rawData : ByteArray) {
        val responseArray = response?.split(",")?.toTypedArray() ?: return
        if (responseArray[0].trim() == AppConstants.COMMAND_RES) {
            val command = responseArray[1].trim()
            if (command == AppConstants.COMMAND_STORED_EVENT_FILE_LIST) {
                Log.i(TAG, "EVENT LIST RESPONSE")
                logState = EventState.EVENT_LIST
                if (responseArray[2].trim() == AppConstants.RESPONSE_S) {
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

                if (responseArray[2].trim() == AppConstants.RESPONSE_F) {
                    val errorCode = responseArray[3].trim()
                    Toast.makeText(requireContext(), "Command Failed with Error Code: $errorCode", Toast.LENGTH_SHORT).show()
                }
            }

            if (command == AppConstants.COMMAND_REQUEST_FILE) {
                Log.i(TAG, "Event Download Response")
                logState = EventState.EVENT_DOWNLOAD
                val status = responseArray[2].trim().replace(";","")
                if (status == AppConstants.RESPONSE_D) {
                    fileSize = responseArray[3].trim().toInt()
                    startIndex = responseArray[4].trim().toInt()
                    chunkSize = responseArray[5].trim().toInt()

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
                    Log.i(TAG, "DATA : ${LogUtil.bytesToHex(data)}")
                    // check if the file is received
                    if (receivedData.size >= fileSize) {
                        receivedData = receivedData.copyOfRange(0, receivedData.size-1)
                        currentFileViewHolder?.showProgress(false)
                        currentFileViewHolder?.toggleCheckImg(true)
                        Log.i(TAG, "File Download Complete, Sending Confirmation : RESPONSE A")
                        Log.i(TAG, "received file size : ${receivedData.size}")
                        if (checkCrcCode()) {
                            bleService.readDeviceData(AppConstants.COMMAND_REQUEST_FILE + "," + AppConstants.RESPONSE_A)
                        }
                    }

                    Log.i(TAG,"Total Received Length : ${receivedData.size}")
                    Log.i(TAG, "Total chunk size : $receivedChunkSize")

                } else if (status == AppConstants.RESPONSE_A) {
                    // transfer complete
                    fileSize = 0
                    chunkSize = 0
                    startIndex = 0
                    receivedChunkSize = 0
                    // save data to file
                    saveDataToFile()
                    receivedData = ByteArray(0)
                    downloadNextFile()
                } else if (status == AppConstants.RESPONSE_F) {
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
            }
        } else if (logState == EventState.EVENT_DOWNLOAD) {
            // receive data chunks
            Log.i(TAG, "Received Data Chunk")
            val dataChunk = rawData
            Log.i(TAG, "DATA : \n${LogUtil.bytesToHex(dataChunk)}")
            val progress = (receivedData.size.toFloat() / fileSize.toFloat()) * 100
            Log.i(TAG, "progress : $progress")
            currentFileViewHolder?.setProgress(progress.toInt())
            Log.i(TAG, "received file size : ${receivedData.size}")
            receivedChunkSize += dataChunk.size
            if (receivedChunkSize >= chunkSize) {
                receivedData += dataChunk.copyOfRange(0, dataChunk.size-1)
                Log.i(TAG,"dataChunk : ${LogUtil.bytesToHex(dataChunk.copyOfRange(0, dataChunk.size-1))}")
                Log.i(TAG, "chunk size : $receivedChunkSize")
                receivedChunkSize = 0
                if (receivedData.size >= fileSize) {
                    // file received
                    currentFileViewHolder?.showProgress(false)
                    currentFileViewHolder?.toggleCheckImg(true)
                    Log.i(TAG, "File Download Complete, Sending Confirmation : RESPONSE A")
                    Log.i(TAG, "received file size : ${receivedData.size}")
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
                Log.i(TAG, "received partial chunk : ${response.length}")
            }
        } else if (logState == EventState.EVENT_LIST) {
            Log.i(TAG, "received additional log file data : $response")
            logResponse += response
            if (logResponse.endsWith(";")) {
                logResponse = logResponse.replace(";","")
                Log.i(TAG, "file list: $logResponse")
                updateList(logResponse.split(",").toTypedArray())
            }
        } else {
            Log.i(TAG, "Unknown command received : $response")
        }
    }

    private fun checkCrcCode(): Boolean {
        axonCRC32.reset()
        axonCRC32.update(receivedData.copyOfRange(0, receivedData.size - 4), receivedData.size-4)
        val crc = receivedData.copyOfRange(receivedData.size-4, receivedData.size)
        val crcValue = bytesToInt(crc)
        receivedData = receivedData.copyOfRange(0, receivedData.size-4)
        Log.i(TAG, "crc received : $crcValue")
        Log.i(TAG, "crc value : ${axonCRC32.value}")
        if (bytesToInt(crc) == axonCRC32.value) {
            Log.i(TAG, "crc matched")
            return true
        } else {
            return false
        }
    }

    private fun saveDataToFile() {
        try {
            Log.i(TAG, "saving file $currentFileName")
            val filePath = requireContext().getEventDownloadDirectory() + bleService.getDeviceDirectory()
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

    private fun eventFileExist(filename : String): Boolean {
        try {
            val filePath = requireContext().getEventDownloadDirectory() + bleService.getDeviceDirectory() + filename
            val file = File(filePath)
            Log.i(TAG, "$filename exist : ${file.exists()}")
            return file.exists()
        } catch (e : Exception) {
            Log.e(TAG, "logFileExist", e.cause)
            return false
        }
    }

    private fun updateList(eventList: Array<String>) {
        val last7Days = AxonUtil.getLast7DaysString() + " " + getString(R.string.download)
        binding.btnDownload.text = last7Days
        this.eventList = eventList
        var downloaded = 0
        for (log in eventList) {
            if (eventFileExist(log)) {
                downloaded += 1
            }
        }

        if (eventList.isEmpty()) {
            binding.llListView.visibility = View.GONE
            binding.llNoItems.visibility = View.VISIBLE
        } else if (downloaded == eventList.size) {
            binding.llListView.visibility = View.VISIBLE
            binding.llNoItems.visibility = View.GONE
            binding.btnDownload.visibility = View.GONE
            eventListAdapter.updateList(eventList)
        } else {
            binding.llListView.visibility = View.VISIBLE
            binding.llNoItems.visibility = View.GONE
            eventListAdapter.updateList(eventList)
        }
    }

    private fun updateRecyclerView() {
        binding.rvEventList.forEachVisibleHolder<EventListAdapter.EventListHolder> {
            it.toggleCheckImg(eventFileExist(it.filename))
        }
    }

    private fun getViewHolderForFile(): EventListAdapter.EventListHolder? {
        var viewHolder : EventListAdapter.EventListHolder? = null
        binding.rvEventList.forEachVisibleHolder<EventListAdapter.EventListHolder> {
            if (it.filename == currentFileName) {
                viewHolder = it
            }
        }
        return viewHolder
    }

    override fun onDestroy() {
        super.onDestroy()
        localBroadcastManager.unregisterReceiver(mGattUpdateReceiver)
    }

    private fun bytesToInt(bytes: ByteArray): Int {
        var result = 0
        for (i in bytes.indices) {
            result = result or (bytes[i].toInt() and 0xFF shl (8 * i))
        }
        return result
    }

    private fun checkAllFilesDownloaded() {
        var downloaded = 0
        for (log in eventList) {
            if (eventFileExist(log)) {
                downloaded += 1
            }
        }
        if (downloaded == eventList.size) {
            Log.i(TAG, "All files downloaded")
            binding.btnDownload.visibility = View.GONE
        }
    }

}

private enum class EventState {
    UNKNOWN,
    EVENT_LIST,
    EVENT_DOWNLOAD
}