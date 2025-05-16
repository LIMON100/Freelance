package com.skyautonet.drsafe.ui.login

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.skyautonet.drsafe.model.login.LoginRequest
import com.skyautonet.drsafe.model.login.LoginResponse
import com.skyautonet.drsafe.network.RetrofitBuilder
import com.skydoves.sandwich.message
import com.skydoves.sandwich.suspendOnError
import com.skydoves.sandwich.suspendOnException
import com.skydoves.sandwich.suspendOnSuccess
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

class LoginViewModel : ViewModel() {

    private val drSafeApi = RetrofitBuilder.DrSafeApi

    private val _success = MutableLiveData<LoginResponse>()
    val success : LiveData<LoginResponse> get() = _success

    private val _error = MutableLiveData<String>()
    val error : LiveData<String> get() = _error

    fun login(loginRequest: LoginRequest) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                drSafeApi.login(loginRequest)
                    .suspendOnSuccess {
                        _success.postValue(data)
                    }
                    .suspendOnError {
                        _error.postValue(message())
                    }
                    .suspendOnException {
                        _error.postValue(message ?: "Something Went Wrong")
                    }
            } catch (e : Exception) {
                e.printStackTrace()
                _error.postValue(e.localizedMessage)
            }
        }
    }
}