package com.skyautonet.drsafe.ui.setting.register

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel

/**
 * Created by Hussain on 31/07/24.
 */
class RegisterViewModel : ViewModel() {

    private var _selectedBrand = MutableLiveData<String>()
    val selectedBrand : LiveData<String> get() = _selectedBrand


    private var _selectedModel = MutableLiveData<String>()
    val selectedModel : LiveData<String> get() = _selectedModel



    private var _selectedFuelType = MutableLiveData<String>()
    val selectedFuelType : LiveData<String> get() = _selectedFuelType


    private var _selectedYear = MutableLiveData<Int>()
    val selectedYear : LiveData<Int> get() = _selectedYear


    fun setBrand(brand : String) {
        _selectedBrand.value = brand
    }

    fun setModel(model : String) {
        _selectedModel.value = model
    }

    fun setFuelType(type : String) {
        _selectedFuelType.value = type
    }

    fun selectedYear(year : Int) {
        _selectedYear.value = year
    }

}