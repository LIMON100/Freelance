package com.axon.kisa10.util

import android.content.Context

class SharedPrefManager private constructor(context: Context) {

    private val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    companion object {
        private lateinit var sharedPrefManager : SharedPrefManager
        const val PREFS_NAME: String = "appname_prefs"

        @JvmStatic
        fun getInstance( context: Context ) : SharedPrefManager {
            if (!this::sharedPrefManager.isInitialized) {
                sharedPrefManager = SharedPrefManager(context)
            }
            return sharedPrefManager
        }
    }

    fun sharedPreferenceExist(key: String?): Boolean {
        return if (!prefs.contains(key)) {
            true
        } else {
            false
        }
    }

    fun setInt(key: String?, value: Int) {
        val editor = prefs.edit()
        editor.putInt(key, value)
        editor.apply()
    }

    fun getInt(key: String?): Int {
        return prefs.getInt(key, 0)
    }

    fun setDeviceMacAddress(key: String?, value: String?) {
        val editor = prefs.edit()
        editor.putString(key, value)
        editor.apply()
    }

    fun getDeviceMacAddress(key: String?): String? {
        return prefs.getString(key, "iSensor")
    }

    fun setBool(key: String?, value: Boolean) {
        val editor = prefs.edit()
        editor.putBoolean(key, value)
        editor.apply()
    }

    fun getBool(key: String?): Boolean {
        return prefs.getBoolean(key, false)
    }

    fun setString(key: String?, value: String?) {
        val editor = prefs.edit()
        editor.putString(key, value)
        editor.apply()
    }

    fun getString(key: String?): String? {
        return prefs.getString(key, "")
    }
}