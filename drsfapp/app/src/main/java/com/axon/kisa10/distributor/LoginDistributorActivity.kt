package com.axon.kisa10.distributor

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.widget.Toast
import androidx.activity.viewModels
import androidx.databinding.DataBindingUtil
import com.axon.kisa10.activity.BaseActivity
import com.axon.kisa10.activity.MainActivity
import com.axon.kisa10.activity.SearchDeviceActivity
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.ble.ServiceConnector
import com.axon.kisa10.util.SharedPrefKeys
import com.axon.kisa10.util.SharedPrefManager
import com.google.gson.Gson
import com.kisa10.R
import com.kisa10.databinding.ActivityLoginDistributorBinding

class LoginDistributorActivity : BaseActivity() {

    private var bleService : AxonBLEService? = null
    private val serviceConnector = ServiceConnector(
        { s ->
            this.bleService = s
        },
        { msg ->
            Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
        }
    )
    private lateinit var binding : ActivityLoginDistributorBinding

    private val viewModel by viewModels<LoginDistributorViewModel>()

    private val sharedPrefManager: SharedPrefManager by lazy {
        SharedPrefManager.getInstance(this)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = DataBindingUtil.setContentView(this, R.layout.activity_login_distributor)

        bindService(Intent(this, AxonBLEService::class.java),serviceConnector, Context.BIND_AUTO_CREATE)

        binding.btnLogin.setOnClickListener {
            val installerId = binding.edtIntallerId.text.toString().trim()
            val password = binding.edPassword.text.toString().trim()
            if (installerId.isNotBlank() && password.isNotBlank()) {
                viewModel.loginDistributon(binding.edtIntallerId.text.toString(), binding.edPassword.text.toString())
            }
//            val intent = if (bleService?.isConnected() == true) {
//                // goto main activity
//                Intent(this, MainActivity::class.java)
//            } else {
//                // goto search activity
//                Intent(this, SearchDeviceActivity::class.java)
//            }
//            startActivity(intent)
//            finish()
        }

        viewModel.success.observe(this) { event ->
            event?.getContentIfNotHandled()?.let { loginDistributorResponse ->
                sharedPrefManager.setString(SharedPrefKeys.DISTRIBUTOR_TOKEN, loginDistributorResponse.token)
                sharedPrefManager.setString(SharedPrefKeys.DISTRIBUTOR_USER_INFO, Gson().toJson(loginDistributorResponse))
                val intent = if (bleService?.isConnected() == true) {
                    // goto main activity
                    Intent(this, MainActivity::class.java)
                } else {
                    // goto search activity
                    Intent(this, SearchDeviceActivity::class.java)
                }
                startActivity(intent)
                finish()
            }
        }

        viewModel.error.observe(this) { event ->
            event?.getContentIfNotHandled()?.let { msg ->
                Toast.makeText(this@LoginDistributorActivity, msg, Toast.LENGTH_SHORT).show()
            }
        }
    }
}