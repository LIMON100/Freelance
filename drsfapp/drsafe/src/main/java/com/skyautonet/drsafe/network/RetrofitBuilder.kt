package com.skyautonet.drsafe.network


import com.skyautonet.drsafe.util.SharedPreferenceUtil
import com.skydoves.sandwich.retrofit.adapters.ApiResponseCallAdapterFactory
import okhttp3.OkHttpClient
import okhttp3.logging.HttpLoggingInterceptor
import retrofit2.Retrofit
import retrofit2.converter.gson.GsonConverterFactory

/**
 * Created by Hussain on 01/07/24.
 */
object RetrofitBuilder {

    lateinit var sharedPrefManager: SharedPreferenceUtil

    private fun getRetrofit() = Retrofit.Builder()
        .baseUrl("http://45.115.154.174:23005/api/v1/")
        .addConverterFactory(GsonConverterFactory.create())
        .addCallAdapterFactory(ApiResponseCallAdapterFactory.create())
        .client(getOkhttp())
        .build()


    private fun getOkhttp() = OkHttpClient.Builder()
        .addInterceptor(getLoggingInterceptor())
        .addInterceptor(AuthInterceptor(sharedPrefManager))
        .build()


    private fun getLoggingInterceptor() = HttpLoggingInterceptor().also {
        it.setLevel(HttpLoggingInterceptor.Level.BODY)
    }

    val DrSafeApi: IDrSafeApi by lazy {
        getRetrofit().create(IDrSafeApi::class.java)
    }
}