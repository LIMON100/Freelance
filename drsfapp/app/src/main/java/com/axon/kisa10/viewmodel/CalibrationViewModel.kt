package com.axon.kisa10.viewmodel

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.axon.kisa10.model.calibration.CalibrationRequest
import com.axon.kisa10.model.calibration.CalibrationResponse
import com.axon.kisa10.model.distributor.calibration.DistributorCalibrationUploadRequest
import com.axon.kisa10.network.RetrofitBuilder
import com.axon.kisa10.util.Event
import com.skydoves.sandwich.message
import com.skydoves.sandwich.suspendOnError
import com.skydoves.sandwich.suspendOnException
import com.skydoves.sandwich.suspendOnSuccess
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

/**
 * Created by Hussain on 21/09/24.
 */
class CalibrationViewModel : ViewModel() {

    private val axonApi = RetrofitBuilder.AxonApi

    private val _success = MutableLiveData<Event<CalibrationResponse>>()
    val success : LiveData<Event<CalibrationResponse>> get() = _success

    private val _successDistributorUpload = MutableLiveData<Event<Boolean>>()
    val successDistributorUpload : LiveData<Event<Boolean>> get() = _successDistributorUpload

    private val _error = MutableLiveData<Event<String>>()
    val error : LiveData<Event<String>> get() = _error


    fun sendCalibrationData(vinCode : String,calibrationData : List<String>) = viewModelScope.launch(Dispatchers.IO) {
        val calibrationRequest = CalibrationRequest(
            vincode = vinCode,
            apsHighMax = calibrationData[0],
            apsHighMin = calibrationData[1],
            apsHighOut = calibrationData[2],
            apsLowMax = calibrationData[3],
            apsLowMin = calibrationData[4],
            apsLowOut = calibrationData[5]
        )
        axonApi.sendCalibrationData(calibrationRequest)
            .suspendOnSuccess {
                _success.postValue(Event(data))
            }
            .suspendOnError {
                _error.postValue(Event(message()))
            }
            .suspendOnException {
                _error.postValue(Event(message ?: ""))
            }
    }

    fun uploadDistributorCalibrationData(newData: DistributorCalibrationUploadRequest) {
        viewModelScope.launch(Dispatchers.IO) {
            axonApi.uploadDistributorCalibrationData(newData)
                .suspendOnSuccess {
                    _successDistributorUpload.postValue(Event(true))
                }
                .suspendOnError {
                    _error.postValue(Event(message()))
                }
                .suspendOnException {
                    _error.postValue(Event(message ?: "Something Went Wrong"))
                }
        }
    }

}