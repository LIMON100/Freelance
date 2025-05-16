package com.axon.kisa10.viewmodel

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.axon.kisa10.model.LatestFirmwareVersionResponse
import com.axon.kisa10.network.RetrofitBuilder
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

/**
 * Created by Hussain on 10/07/24.
 */
class FirmwareViewModel : ViewModel() {

    private val axonApi = RetrofitBuilder.AxonApi

    private val _firmwareResponse = MutableLiveData<LatestFirmwareVersionResponse>()
    val firmwareResponse : LiveData<LatestFirmwareVersionResponse> get() = _firmwareResponse


    private val _errorMsg = MutableLiveData<Event<String>>()
    val errorMsg : LiveData<Event<String>> get() = _errorMsg

    private val _firmwareDownloadProgress = MutableLiveData<Event<Int>>()
    val firmwareDownloadProgress : LiveData<Event<Int>> get() = _firmwareDownloadProgress


    fun getLatestFirmwareVersion() {
        viewModelScope.launch {
            axonApi.getLatestFirmwareVersion("stm32")
                .suspendOnSuccess {
                    _firmwareResponse.postValue(data)
                }
                .suspendOnError {
                    _errorMsg.postValue(Event(message()))
                }
                .suspendOnException {
                    _errorMsg.postValue(Event(message ?: ""))
                }
        }
    }

    fun downloadFirmware(url : String, file : File) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                var count: Int
                val url = URL(url)
                val connection = url.openConnection()
                connection.connect()
                val lengthOfFile = connection.contentLength
                val input: InputStream = BufferedInputStream(
                    url.openStream(),
                    8192
                )
                val output: OutputStream = FileOutputStream(file)
                val data = ByteArray(1024)
                var total: Long = 0
                while ((input.read(data).also { count = it }) != -1) {
                    total += count.toLong()
                    val progress = (total.toFloat() * 100) / lengthOfFile
                    _firmwareDownloadProgress.postValue(Event(progress.toInt()))
                    output.write(data, 0, count)
                }
                output.flush()
                output.close()
                input.close()
            } catch (e : Exception) {
                e.printStackTrace()
                _firmwareDownloadProgress.postValue(Event(-1))
            }
        }
    }

}