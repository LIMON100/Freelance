package com.skyautonet.drsafe.ui.login

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.fragment.app.viewModels
import com.google.gson.Gson
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentLoginEmailBinding
import com.skyautonet.drsafe.model.login.LoginRequest
import com.skyautonet.drsafe.service.DrSafeBleService
import com.skyautonet.drsafe.service.ServiceConnector
import com.skyautonet.drsafe.ui.BaseFragment
import com.skyautonet.drsafe.ui.MainActivity
import com.skyautonet.drsafe.ui.search.SearchDeviceActivity
import com.skyautonet.drsafe.util.AppConstants
import com.skyautonet.drsafe.util.SharedPreferenceUtil
import com.skyautonet.drsafe.util.start

/**
 * Created by Hussain on 02/08/24.
 */
class LoginEmailFragment : BaseFragment() {

    private lateinit var binding: FragmentLoginEmailBinding

    private val sharedPrefManager by lazy {
        SharedPreferenceUtil.getInstance(requireContext())
    }

    private val loginViewModel by viewModels<LoginViewModel>()

    private var bleService : DrSafeBleService? = null

    private var vinCode = "000000000000"

    private val serviceConnector = ServiceConnector(
        { service ->
            bleService = service
            getVincode()
        },
        { err ->
            Toast.makeText(requireContext(), err, Toast.LENGTH_SHORT).show()
        }
    )

    private fun getVincode() {
        val cmd = "AT$\$CINFO?\r\n".toByteArray(charset("euc-kr"))
        bleService?.writeCharacteristic(cmd)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentLoginEmailBinding.inflate(inflater, container, false)
        setupToolbar(binding.toolbar)
        val intent = Intent(requireContext(), DrSafeBleService::class.java)
        requireActivity().bindService(intent, serviceConnector, Context.BIND_AUTO_CREATE)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        loginViewModel.success.observe(viewLifecycleOwner) { loginResponse ->
            sharedPrefManager.setString(AppConstants.TOKEN, loginResponse.token)
            val gson = Gson()
            val data = gson.toJson(loginResponse)
            sharedPrefManager.setData(AppConstants.USER_INFO, data)
            gotoMainActivity()
        }

        loginViewModel.error.observe(viewLifecycleOwner) { error ->
            if (!error.isNullOrBlank()) {
                Toast.makeText(requireContext(), error, Toast.LENGTH_SHORT).show()
            }
        }

        binding.btnLogin.setOnClickListener {
            val userName = binding.edName.text.toString()
            val phoneNumber = binding.edPhone.text.toString()
            val license = binding.edPlate.text.toString()

            if (userName.isBlank()) {
                Toast.makeText(requireContext(),"User Name required", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            if (phoneNumber.isBlank()) {
                Toast.makeText(requireContext(),"Phone No. required", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            if (license.isBlank()) {
                Toast.makeText(requireContext(),"License No. required", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            val tokenProvider = if (sharedPrefManager.getString(AppConstants.GOOGLE_TOKEN_KEY) != null) {
                "Google"
            } else {
                "Kakao"
            }

            val token = sharedPrefManager.getString(AppConstants.GOOGLE_TOKEN_KEY) ?: sharedPrefManager.getString(AppConstants.KAKAO_TOKEN_KEY)

            if (token == null) {
                Toast.makeText(requireContext(), "Token Invalid", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            val loginRequest = LoginRequest(
                license,
                userName,
                phoneNumber,
                tokenProvider,
                token,
                vinCode
            )

            loginViewModel.login(loginRequest)

        }

    }

    private fun gotoMainActivity() {
        start(MainActivity::class.java)
    }

    override fun getStatusBarColor(): Int {
        return getWhiteStatusBarColor()
    }

    override fun getToolbarTitle(): String {
        return getString(R.string.dr_safe)
    }

    override fun showHeaderLogo(): Boolean {
        return true
    }

    override fun showToolbarImage(): Boolean {
        return true
    }

    override fun shouldShowBottomNavigation(): Boolean {
        return false
    }

    override fun onDataReceived(rawData: ByteArray?, data: String?) {
        super.onDataReceived(rawData, data)
        if (data?.contains("CINFO") == true) {
            vinCode = data.split(":")[1].split(",")[3]
        }
    }
}