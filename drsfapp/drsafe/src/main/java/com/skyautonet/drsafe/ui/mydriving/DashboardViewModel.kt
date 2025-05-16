package com.skyautonet.drsafe.ui.viewmodel

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import java.time.LocalDateTime

class DashboardViewModel : ViewModel() {

    // LiveData to hold speed data as a list of Pair<LocalDateTime, Double>
    private val _speedData = MutableLiveData<List<Pair<LocalDateTime, Double>>>()
    val speedData: LiveData<List<Pair<LocalDateTime, Double>>> get() = _speedData

    // LiveData to hold RPM data as a list of Pair<LocalDateTime, Int>
    private val _rpmData = MutableLiveData<List<Pair<LocalDateTime, Int>>>()
    val rpmData: LiveData<List<Pair<LocalDateTime, Int>>> get() = _rpmData

    // Internal lists to maintain current speed and RPM data
    private val currentSpeedData = mutableListOf<Pair<LocalDateTime, Double>>()
    private val currentRpmData = mutableListOf<Pair<LocalDateTime, Int>>()

    // Method to update speed data with timestamped pairs
    fun updateSpeedData(newSpeedData: List<Pair<LocalDateTime, Double>>) {
        currentSpeedData.addAll(newSpeedData) // Add all new pairs to the list
        _speedData.value = currentSpeedData // Update LiveData with the new list
    }

    // Method to update RPM data with timestamped pairs
    fun updateRpmData(newRpmData: List<Pair<LocalDateTime, Int>>) {
        currentRpmData.addAll(newRpmData) // Add all new pairs to the list
        _rpmData.value = currentRpmData // Update LiveData with the new list
    }
}
