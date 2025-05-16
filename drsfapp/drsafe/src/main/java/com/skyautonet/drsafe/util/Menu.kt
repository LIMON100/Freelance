package com.skyautonet.drsafe.util

/**
 * Created by Hussain on 29/07/24.
 */
enum class Menu(private val label: String) {
    HOME("Home"), TRIPS("TRIPS"), EVENT("EVENT"), ECALL("ECALL"), SETTING("Setting");


    fun getLabel() : String {
        return label
    }

    companion object {
        fun getMenuForLabel(label : String) : Menu {
            return when(label) {
                HOME.label -> HOME
                TRIPS.label -> TRIPS
                EVENT.label -> EVENT
                ECALL.label -> ECALL
                SETTING.label -> SETTING
                else -> HOME
            }
        }
    }
}