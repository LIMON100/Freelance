package com.skyautonet.drsafe.ui.viewmodel

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel

// EventDashboardViewModel inherits from DashboardViewModel
class EventDashboardViewModel : ViewModel() {

    // LiveData to hold the counts for various events
    private val _fastAcceleration = MutableLiveData<Int>()
    val fastAcceleration: LiveData<Int> get() = _fastAcceleration

    private val _suddenDeparture = MutableLiveData<Int>()
    val suddenDeparture: LiveData<Int> get() = _suddenDeparture

    private val _rapidDeceleration = MutableLiveData<Int>()
    val rapidDeceleration: LiveData<Int> get() = _rapidDeceleration

    private val _suddenStop = MutableLiveData<Int>()
    val suddenStop: LiveData<Int> get() = _suddenStop

    private val _radicalCourseChange = MutableLiveData<Int>()
    val radicalCourseChange: LiveData<Int> get() = _radicalCourseChange

    private val _overtakingSpeed = MutableLiveData<Int>()
    val overtakingSpeed: LiveData<Int> get() = _overtakingSpeed

    private val _sharpTurns = MutableLiveData<Int>()
    val sharpTurns: LiveData<Int> get() = _sharpTurns

    private val _suddenUTurn = MutableLiveData<Int>()
    val suddenUTurn: LiveData<Int> get() = _suddenUTurn

    private val _continuousOperation = MutableLiveData<Int>()
    val continuousOperation: LiveData<Int> get() = _continuousOperation

    // Initialize the counts to 0
    init {
        _fastAcceleration.value = 0
        _suddenDeparture.value = 0
        _rapidDeceleration.value = 0
        _suddenStop.value = 0
        _radicalCourseChange.value = 0
        _overtakingSpeed.value = 0
        _sharpTurns.value = 0
        _suddenUTurn.value = 0
        _continuousOperation.value = 0
    }

    // Method to update the event counts based on event type and explicit count
    fun updateEventCount(eventType: String, count: Int? = null) {
        when (eventType) {
            "Fast Acceleration" -> _fastAcceleration.value = count ?: (_fastAcceleration.value ?: 0) + 1
            "Sudden Departure" -> _suddenDeparture.value = count ?: (_suddenDeparture.value ?: 0) + 1
            "Rapid Deceleration" -> _rapidDeceleration.value = count ?: (_rapidDeceleration.value ?: 0) + 1
            "Sudden Stop" -> _suddenStop.value = count ?: (_suddenStop.value ?: 0) + 1
            "Radical Course Change" -> _radicalCourseChange.value = count ?: (_radicalCourseChange.value ?: 0) + 1
            "Overtaking Speed" -> _overtakingSpeed.value = count ?: (_overtakingSpeed.value ?: 0) + 1
            "Sharp Turns" -> _sharpTurns.value = count ?: (_sharpTurns.value ?: 0) + 1
            "Sudden U-Turn" -> _suddenUTurn.value = count ?: (_suddenUTurn.value ?: 0) + 1
            "Continuous Operation" -> _continuousOperation.value = count ?: (_continuousOperation.value ?: 0) + 1
        }
    }

    // Method to get the total count of all events

}
