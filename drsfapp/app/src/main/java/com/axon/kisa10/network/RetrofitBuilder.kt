package com.axon.kisa10.network

import com.axon.kisa10.util.SharedPrefManager
import com.kisa10.BuildConfig
import com.skydoves.sandwich.retrofit.adapters.ApiResponseCallAdapterFactory
import okhttp3.OkHttpClient
import okhttp3.logging.HttpLoggingInterceptor
import retrofit2.Retrofit
import retrofit2.converter.gson.GsonConverterFactory

/**
 * Created by Hussain on 01/07/24.
 */
object RetrofitBuilder {

    lateinit var sharedPrefManager: SharedPrefManager
    private fun getRetrofit() = Retrofit.Builder()
        .baseUrl(BuildConfig.BASE_URL)
        .addConverterFactory(GsonConverterFactory.create())
        .addCallAdapterFactory(ApiResponseCallAdapterFactory.create())
        .client(getOkhttp())
        .build()

    private fun getOkhttp() = OkHttpClient.Builder()
        .addInterceptor(getLoggingInterceptor())
        .addInterceptor(AuthInterceptor(sharedPrefManager))
        .build()


    fun getLoggingInterceptor() = HttpLoggingInterceptor().also {
        it.setLevel(HttpLoggingInterceptor.Level.BODY)
    }

    val AxonApi by lazy {
        getRetrofit().create(AxonApi::class.java)
    }
}