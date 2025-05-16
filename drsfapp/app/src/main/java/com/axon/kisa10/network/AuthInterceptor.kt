package com.axon.kisa10.network

import com.axon.kisa10.util.SharedPrefKeys
import com.axon.kisa10.util.SharedPrefManager
import com.kisa10.BuildConfig
import okhttp3.Interceptor
import okhttp3.Request
import okhttp3.Response

/**
 * Created by Hussain on 01/07/24.
 */
class AuthInterceptor(private val sharedPrefManager: SharedPrefManager) : Interceptor {
    override fun intercept(chain: Interceptor.Chain): Response {
        val token = if (BuildConfig.IS_DISTRIBUTOR) {
            sharedPrefManager.getString(SharedPrefKeys.DISTRIBUTOR_TOKEN)
        } else {
            sharedPrefManager.getString(SharedPrefKeys.TOKEN)
        }
        if (token == null) {
            return chain.proceed(chain.request())
        } else {
            val request = request(chain.request(), token)
            return chain.proceed(request)
        }
    }

    private fun request(originalRequest: Request, token : String): Request {
        return originalRequest.newBuilder()
            .addHeader(
                "Authorization",
                "Bearer $token"
            )
            .build()

    }
}