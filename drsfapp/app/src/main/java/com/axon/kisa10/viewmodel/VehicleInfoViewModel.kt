package com.axon.kisa10.viewmodel

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.axon.kisa10.model.distributor.project.ProjectResponse
import com.axon.kisa10.network.RetrofitBuilder
import com.axon.kisa10.util.Event
import com.skydoves.sandwich.message
import com.skydoves.sandwich.suspendOnError
import com.skydoves.sandwich.suspendOnException
import com.skydoves.sandwich.suspendOnSuccess
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

class VehicleInfoViewModel : ViewModel() {

    private val axonApi = RetrofitBuilder.AxonApi

    private val _successGetProjects = MutableLiveData<Event<ProjectResponse>>()
    val successGetProjects : LiveData<Event<ProjectResponse>> get() = _successGetProjects

    private val _successGetDevice = MutableLiveData<Event<ProjectResponse>>()
    val successGetDevice : LiveData<Event<ProjectResponse>> get() = _successGetDevice

    private val _error = MutableLiveData<Event<String>>()
    val error : LiveData<Event<String>> get() = _error

    fun getProjects() {
        viewModelScope.launch(Dispatchers.IO) {
            axonApi.getProjects(1,100)
                .suspendOnSuccess {
                    _successGetProjects.postValue(Event(data))
                }
                .suspendOnError {
                    _error.postValue(Event(message()))
                }
                .suspendOnException {
                    _error.postValue(Event(message ?: "Something Went Wrong"))
                }
        }
    }

    fun getDevices() {
        viewModelScope.launch(Dispatchers.IO) {
            axonApi.getDevices(1, 100)
                .suspendOnSuccess {
                    _successGetDevice.postValue(Event(data))
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