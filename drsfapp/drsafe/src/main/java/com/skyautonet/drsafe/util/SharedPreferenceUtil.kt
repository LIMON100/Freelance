package com.skyautonet.drsafe.util

import android.annotation.SuppressLint
import android.content.Context
import android.content.SharedPreferences
import com.google.gson.Gson

/**
 * Created by Hussain on 02/08/24.
 */

class SharedPreferenceUtil private constructor(context: Context) {

    private val sharedPreference =
        context.getSharedPreferences(Constants.SHARED_PREF_NAME, Context.MODE_PRIVATE)

    companion object {

        private var sharedPreferenceUtil: SharedPreferenceUtil? = null

        fun getInstance(context: Context): SharedPreferenceUtil {
            if (sharedPreferenceUtil == null) {
                sharedPreferenceUtil = SharedPreferenceUtil(context)
            }
            return sharedPreferenceUtil!!
        }
    }

    fun setTermsAndConditionsAgreed() {
        sharedPreference.edit().apply {
            putBoolean(Constants.TERMS_AGREED, true)
            apply()
        }
    }

    fun getTermsAndConditionAgreed(): Boolean {
        return sharedPreference.getBoolean(Constants.TERMS_AGREED, false)
    }

    fun setString(key: String?, value: String?) {
        val editor = sharedPreference.edit()
        editor.putString(key, value)
        editor.apply()
    }

    fun getString(key: String?): String? {
        return sharedPreference.getString(key, null)
    }

    fun setData(key: String, data : String) {
        val editor = sharedPreference.edit()
        editor.putString(key, data)
        editor.apply()
    }
}