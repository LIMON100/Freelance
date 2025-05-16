package com.axon.kisa10.distributor

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.axon.kisa10.model.distributor.login.InstallerLogin
import com.axon.kisa10.model.distributor.login.InstallerLoginResponse
import com.axon.kisa10.network.RetrofitBuilder
import com.axon.kisa10.util.Event
import com.skydoves.sandwich.message
import com.skydoves.sandwich.suspendOnError
import com.skydoves.sandwich.suspendOnException
import com.skydoves.sandwich.suspendOnSuccess
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

class LoginDistributorViewModel : ViewModel() {

    private val axonApi = RetrofitBuilder.AxonApi

    private val _success = MutableLiveData<Event<InstallerLoginResponse>>()
    val success : LiveData<Event<InstallerLoginResponse>> get() = _success


    private var _error = MutableLiveData<Event<String>>()
    val error : LiveData<Event<String>> get() = _error


    fun loginDistributon(installerId : String, password : String) {
        val loginBody = InstallerLogin(installerId, password)
        viewModelScope.launch(Dispatchers.IO) {
            axonApi.loginDistributor(loginBody)
                .suspendOnSuccess {
                    _success.postValue(Event(data))
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