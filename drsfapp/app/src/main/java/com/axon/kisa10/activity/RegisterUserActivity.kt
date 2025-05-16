package com.axon.kisa10.activity

import android.app.ProgressDialog
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.activity.viewModels
import androidx.databinding.DataBindingUtil
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.ble.ServiceConnector
import com.axon.kisa10.model.register.RegisterVehicleRequest
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AppMethods.setAlertDialog
import com.axon.kisa10.util.SharedPrefKeys
import com.axon.kisa10.util.SharedPrefManager
import com.axon.kisa10.util.hideKeyboard
import com.axon.kisa10.viewmodel.RegisterViewModel
import com.google.firebase.auth.FirebaseAuth
import com.google.gson.Gson
import com.kisa10.R
import com.kisa10.databinding.ActivityRegisterUserBinding

@Suppress("DEPRECATION")
class RegisterUserActivity : BaseActivity() {

    private val TAG = "RegisterUserActivity"
    private lateinit var binding : ActivityRegisterUserBinding
    private val registerViewModel : RegisterViewModel by viewModels()
    private val sharedPrefManager by lazy {
        SharedPrefManager.getInstance(this)
    }
    private val progressDialog by lazy {
        ProgressDialog(this).also {
            it.setMessage(getString(R.string.please_wait))
        }
    }

    private var bleService : AxonBLEService? = null

    private val serviceConnector = ServiceConnector(
        { service ->
            this.bleService = service
            if (!service.isConnected()) {
                Toast.makeText(this@RegisterUserActivity, getString(R.string.device_disconnected), Toast.LENGTH_SHORT).show()
            } else {
                readVinCode()
                deviceAddress = bleService?.getBluetoothGatt()?.device?.address ?: ""
            }
        },{ msg ->
            Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
            finish()
        })

    private var vinCode : String = ""
    private var deviceAddress = ""

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(this)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = DataBindingUtil.setContentView(this, R.layout.activity_register_user)
        val serviceIntent = Intent(this, AxonBLEService::class.java)

        bindService(serviceIntent, serviceConnector, Context.BIND_AUTO_CREATE)

        val intentFilter = IntentFilter()
        intentFilter.addAction(AppConstants.ACTION_DEVICE_DISCONNECTED)
        intentFilter.addAction(AppConstants.ACTION_CHARACTERISTIC_CHANGED)

        localBroadcastManager.registerReceiver(mGattUpdateReceiver, intentFilter)

        setObservers()
        setupView()
    }

    private fun setupView() {

        if(sharedPrefManager.getString(SharedPrefKeys.KAKAO_TOKEN_KEY).isNullOrBlank()) {
            binding.edEmail.visibility = View.GONE
        } else {
            binding.edEmail.visibility = View.VISIBLE
        }

        binding.btnNext.setOnClickListener {
            binding.btnNext.hideKeyboard()
            validateData()
        }
    }

    private fun validateData() {
        if (binding.edVehicle.text.toString().isBlank()) {
            Toast.makeText(this, "Please Enter Vehicle Number",Toast.LENGTH_SHORT).show()
            return
        }

        if (binding.edName.text.isNullOrBlank()) {
            Toast.makeText(this,"Please Enter your name",Toast.LENGTH_SHORT).show()
            return
        }

        if (binding.edPhone.text.isNullOrBlank()) {
            Toast.makeText(this, "Please Enter your phone number.",Toast.LENGTH_SHORT).show()
            return
        }

        if (binding.edMemo.text.isNullOrBlank()) {
            Toast.makeText(this, "Please Enter your memo",Toast.LENGTH_SHORT).show()
            return
        }

        val vehicleNumber = binding.edVehicle.text.toString()
        val name = binding.edName.text.toString()
        val phoneNumber = binding.edPhone.text.toString()
        val memo = binding.edMemo.text.toString()
        val email = if(!sharedPrefManager.getString(SharedPrefKeys.KAKAO_TOKEN_KEY).isNullOrBlank()) {
            binding.edEmail.text.toString()
        } else {
            FirebaseAuth.getInstance().currentUser?.email ?: ""
        }

        if (email.isBlank()) {
            Toast.makeText(this, "Email not found", Toast.LENGTH_SHORT).show()
            return
        }

        progressDialog.show()

        val request = RegisterVehicleRequest(email,vehicleNumber,memo,name,phoneNumber,"",vinCode,deviceAddress)
        registerViewModel.registerVehicle(request)
    }

    private fun setObservers() {
        registerViewModel.registerVehicleResponse.observe(this) { response ->
            if (response?.token != null) {
                sharedPrefManager.setString(SharedPrefKeys.TOKEN, response.token)
                sharedPrefManager.setString(SharedPrefKeys.VEHICLE_ID, response.vehicleId)
            } else {
                showErrorToast(getString(R.string.invalid_token))
            }
            registerViewModel.getUserInfo()
        }

        registerViewModel.errorMsg.observe(this) { event ->
            progressDialog.hide()
            event?.getContentIfNotHandled()?.let { msg ->
                showErrorToast(msg)
            }
        }

        registerViewModel.userInfo.observe(this) { response ->
            if (response != null) {
                val gson = Gson()
                val responseString = gson.toJson(response)
                sharedPrefManager.setString(SharedPrefKeys.USER_INFO,responseString)
                registerViewModel.uploadFcmToken(sharedPrefManager.getString(SharedPrefKeys.FCM_TOKEN),response.id.toString())
            }
            progressDialog.hide()
            gotoMainActivity()
        }
    }

    private fun gotoMainActivity() {
        val intent = Intent(this, MainActivity::class.java)
        startActivity(intent)
        finish()
    }

    private fun showErrorToast(msg : String) {
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
    }

    private fun readVinCode() {
        bleService?.readDeviceData(AppConstants.COMMAND_VINCODE_READ)
    }


    private val mGattUpdateReceiver: BroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val action = intent.action
            when (action) {
                AppConstants.ACTION_DEVICE_DISCONNECTED -> {
                    setAlertDialog(this@RegisterUserActivity, getString(R.string.device_disconnected))
                }

                AppConstants.ACTION_CHARACTERISTIC_CHANGED -> {
                    val response: String?
                    try {
                        response = intent.getStringExtra("data")
                        parseResponse(response)
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                }
            }
        }
    }

    private fun parseResponse(response: String?) {
        val responseArray = response?.split(",")?.toTypedArray() ?: return
        if (responseArray[0].trim() == AppConstants.COMMAND_RES) {
            val command = responseArray[1].trim()
            when (command) {
                AppConstants.COMMAND_VINCODE_READ -> if (responseArray.size >= 4) {
                    if (responseArray[2].trim() == AppConstants.RESPONSE_S) {
                        vinCode = responseArray[3].trim()
                        if (vinCode.isNotEmpty()) {
                            sharedPrefManager.setString(SharedPrefKeys.VIN_CODE, vinCode)
                        }
                    }
                }
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        localBroadcastManager.unregisterReceiver(mGattUpdateReceiver)
        unbindService(serviceConnector)
    }
}