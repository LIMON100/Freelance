package com.axon.kisa10.viewmodel

import android.util.Log
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.axon.kisa10.model.dbversion.DBVersionResponse
import com.axon.kisa10.network.RetrofitBuilder
import com.axon.kisa10.util.AxonCRC32
import com.axon.kisa10.util.Event
import com.skydoves.sandwich.message
import com.skydoves.sandwich.suspendOnError
import com.skydoves.sandwich.suspendOnException
import com.skydoves.sandwich.suspendOnSuccess
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import java.io.BufferedInputStream
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream
import java.io.OutputStream
import java.net.URL
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.zip.Deflater

/**
 * Created by Hussain on 25/09/24.
 */
class DbUpdateViewModel : ViewModel() {

    private val TAG = "DbUpdateViewModel"
    private val axonApi = RetrofitBuilder.AxonApi

    private val axonCRC32 = AxonCRC32()

    private val _dbVersionResponse = MutableLiveData<Event<DBVersionResponse>>()
    val dbVersionResponse : LiveData<Event<DBVersionResponse>> get() = _dbVersionResponse

    private val _error = MutableLiveData<Event<String>>()
    val error : LiveData<Event<String>> get() = _error

    val dbDownloadProgress = MutableLiveData<Event<Int>>()

    fun getDbVersion() {
        viewModelScope.launch(Dispatchers.IO) {
            axonApi.getLatestDBVersion()
                .suspendOnSuccess {
                    _dbVersionResponse.postValue(Event(data))
                }
                .suspendOnError {
                    _error.postValue(Event(message()))
                }
                .suspendOnException {
                    _error.postValue(Event(message ?: ""))
                }
        }
    }

    fun downloadDBUpdateFile(filePath: String, file : File) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                var count: Int
                val url = URL(filePath)
                val connection = url.openConnection()
                connection.connect()
                val input: InputStream = BufferedInputStream(url.openStream())
                val output: OutputStream = FileOutputStream(file)
                val downloadData = input.readBytes()
//                while (downloadData.size < lengthOfFile) {
//                    if (input.read(data) == -1) {
//                        break
//                    }
//                    downloadData += data
//                    val progress = (downloadData.size.toFloat() * 100) / lengthOfFile
//                    dbDownloadProgress.postValue(Event(progress.toInt()))
////                    output.write(data, 0, count)
//                }
//                downloadData = downloadData.copyOfRange(0, lengthOfFile)
                var deflatedData = deflate(downloadData)
                axonCRC32.reset()
                axonCRC32.update(deflatedData, deflatedData.size)
                val byteBuffer = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(axonCRC32.value).array()
                deflatedData += byteBuffer
                output.write(deflatedData)
                Log.d(TAG, "downloadDBUpdateFile: ${deflatedData.size} ${axonCRC32.value}")
                output.flush()
                output.close()
                input.close()
                dbDownloadProgress.postValue(Event(100))
            } catch (e : Exception) {
                e.printStackTrace()
                dbDownloadProgress.postValue(Event(-1))
            }
        }
    }

    private fun deflate(data: ByteArray): ByteArray {
        val deflater = Deflater(Deflater.BEST_COMPRESSION);
        deflater.setInput(data)
        deflater.finish()
        val compressedData = ByteArray(data.size)
        val compressedDataLength = deflater.deflate(compressedData)
        deflater.end()
        return compressedData.copyOfRange(0, compressedDataLength)
    }
}