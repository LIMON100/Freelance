package com.skyautonet.drsafe.network

import com.skyautonet.drsafe.util.AppConstants
import com.skyautonet.drsafe.util.SharedPreferenceUtil
import okhttp3.Interceptor
import okhttp3.Request
import okhttp3.Response

/**
 * Created by Hussain on 01/07/24.
 */
class AuthInterceptor(private val sharedPrefManager: SharedPreferenceUtil) : Interceptor {
    override fun intercept(chain: Interceptor.Chain): Response {
        val token = sharedPrefManager.getString(AppConstants.TOKEN)
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