package com.axon.kisa10.viewmodel

import android.util.Log
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.axon.kisa10.model.UserInfo
import com.axon.kisa10.model.fcm.FcmRequest
import com.axon.kisa10.model.register.RegisterVehicleRequest
import com.axon.kisa10.model.register.RegisterVehicleResponse
import com.axon.kisa10.network.RetrofitBuilder
import com.axon.kisa10.util.Event
import com.skydoves.sandwich.message
import com.skydoves.sandwich.retrofit.errorBody
import com.skydoves.sandwich.suspendOnError
import com.skydoves.sandwich.suspendOnException
import com.skydoves.sandwich.suspendOnSuccess
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

/**
 * Created by Hussain on 01/07/24.
 */
class RegisterViewModel : ViewModel() {


    private val axonApi = RetrofitBuilder.AxonApi
    private val defaultScope = Dispatchers.IO

    private val _registerVehicleResponse = MutableLiveData<RegisterVehicleResponse>()
    val registerVehicleResponse : LiveData<RegisterVehicleResponse> get() = _registerVehicleResponse

    private val _userInfo = MutableLiveData<UserInfo>()
    val userInfo : LiveData<UserInfo> get() = _userInfo

    private val _errorMsg = MutableLiveData<Event<String>>()
    val errorMsg : LiveData<Event<String>> get() = _errorMsg

    fun registerVehicle(request : RegisterVehicleRequest) {
        viewModelScope.launch(defaultScope) {
            axonApi.registerVehicle(request)
                .suspendOnSuccess {
                    _registerVehicleResponse.postValue(data)
                }
                .suspendOnError {
                    _errorMsg.postValue(Event(this.message()))
                }
                .suspendOnException {
                    _errorMsg.postValue(Event(this.message()))
                }
        }
    }

    fun getUserInfo() {
        viewModelScope.launch(defaultScope) {
            axonApi.getUserInfo()
                .suspendOnSuccess {
                    _userInfo.postValue(data)
                }
                .suspendOnException {
                    _errorMsg.postValue(Event(this.message()))
                }
                .suspendOnError {
                    _errorMsg.postValue(Event(this.message()))
                }
        }
    }

    fun uploadFcmToken(token: String?,userId : String) = token?.let {
        viewModelScope.launch(defaultScope) {
            axonApi.uploadFcmToken(userId,FcmRequest(it))
                .suspendOnSuccess {
                    Log.i("AxonApplication", "uploadFcmToken: success ")
                }
                .suspendOnError {
                    Log.e("AxonApplication", "uploadFcmToken failed : $errorBody")
                }
                .suspendOnException {
                    Log.e("AxonApplication", "uploadFcmToken failed : ${this.message}",this.throwable)
                }
        }
    }
}