package com.axon.kisa10.fragment

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import android.widget.Toast
import android.widget.ToggleButton
import androidx.fragment.app.Fragment
import androidx.fragment.app.viewModels
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.activity.MainActivity
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.ble.ServiceConnector
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AppConstants.makeIntentFilter
import com.axon.kisa10.util.AppMethods.hideProgressDialog
import com.axon.kisa10.util.AppMethods.setAlertDialog
import com.axon.kisa10.util.AppMethods.showProgressDialog
import com.axon.kisa10.util.CheckAutoUpdate.isNewUpdate
import com.axon.kisa10.util.CheckSelfPermission.checkPermissionForReadExtertalStorage
import com.axon.kisa10.util.CheckSelfPermission.checkPermissionForWriteExtertalStorage
import com.axon.kisa10.util.CheckSelfPermission.checkStoragePermission
import com.axon.kisa10.util.Network.Companion.isConnectionFast
import com.axon.kisa10.viewmodel.FirmwareViewModel
import com.kisa10.R
import java.io.File
import java.util.Locale

class FirmwareUpdateMainFragment() : Fragment(), View.OnClickListener {
    private val TAG = "FirmwareUpdateMainFragm"
    private lateinit var tv_version: TextView
    private lateinit var tv_version2: TextView
    private lateinit var iv_update: TextView
    private lateinit var mTvUpdateMsg: TextView
    private lateinit var toggle_update_notification: ToggleButton
    private lateinit var toggle_auto_update: ToggleButton
    private lateinit var mode_onoff: ToggleButton
    private var isBackStacked = false
    private var rededVersion = "V1.0"
    private var firmwareVersion = "0"
    private var m_UpdateCheck = false
    private var m_Updatekey = false
    private var m_UpdateCheck_latest = false
    private var ServerVersion = "V1.0"
    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(requireContext())
    }
    private val viewModel by viewModels<FirmwareViewModel>()

    private lateinit var bleService : AxonBLEService

    private val serviceConnector = ServiceConnector(
        { service ->
            this.bleService = service
            bleService.readDeviceData(AppConstants.COMMAND_FIRMWARE_READ) //read 커맨드 전송
        },{ msg ->
            Toast.makeText(requireContext(), msg, Toast.LENGTH_SHORT).show()
        })
    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        val view = inflater.inflate(R.layout.fragment_firmware_update_main, container, false)
        val intent = Intent(requireContext(), AxonBLEService::class.java)
        requireActivity().bindService(intent, serviceConnector, Context.BIND_AUTO_CREATE)
        return view
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        initUi(view)
        setupObservers()
        localBroadcastManager.registerReceiver(mGattUpdateReceiver, makeIntentFilter()) //broadcast 달아주기
        checkStoragePermission(activity)
        isBackStacked = false
    }

    private fun setupObservers() {
        viewModel.firmwareResponse.observe(viewLifecycleOwner) {
            Log.i(TAG, "onViewCreated: $it")
            if (it == null) {
                hideProgressDialog(requireContext())
                setAlertDialog(requireActivity(), getString(R.string.webresponse)) // + response);
            } else {
                hideProgressDialog(requireContext())
                ServerVersion = "V " + it.version.toString()
                tv_version2.text = ServerVersion
                if (!isNewUpdate(it.version, firmwareVersion)) { //단말기 최신버전이면 false CheckAutoUpdate.isNewUpdate(version_server,firmwareVersion)
                    if (m_UpdateCheck) { //parseResponse를 했으면
                        m_UpdateCheck_latest = true //최신버전입니다
                        m_UpdateCheck = false
                    }
                } else { //최신버전이 아니면
                    if (m_Updatekey) { //업데이트버튼 클릭했으면
                        m_Updatekey = false
                        getFirmwareForDevice(it.firmwareFile.path) //펌웨어 업데이트
                    }
                }
                //문구 띄우기
                displayUpdateNotice()
            }
        }
        viewModel.errorMsg.observe(viewLifecycleOwner) { msg ->
            Log.i(TAG, "onViewCreated: $msg")
            mTvUpdateMsg.text = getString(R.string.failed_to_get_firmware_version_from_server)
        }

        viewModel.firmwareDownloadProgress.observe(viewLifecycleOwner) { event ->
            event?.getContentIfNotHandled()?.let { progress ->
                if (progress == -1) {
                    hideProgressDialog(activity)
                    setAlertDialog(requireActivity(), getString(R.string.dloadexception)) {
                        startActivity(Intent(requireActivity(), MainActivity::class.java))
                        requireActivity().finish()
                    }
                }

                if (progress == 100) {
                    unRegisterReceiver()
                    hideProgressDialog(activity)
                    val intent = Intent(MainActivity.FIRMWARE_UPDATE_CLICK)
                    localBroadcastManager.sendBroadcast(intent)
                }
            }
        }
    }

    private fun initUi(view: View) {
        tv_version = view.findViewById(R.id.tv_version)
        toggle_update_notification = view.findViewById(R.id.toggle_update_notification)
        toggle_auto_update = view.findViewById(R.id.toggle_auto_update)
        iv_update = view.findViewById(R.id.iv_update)
        mTvUpdateMsg = view.findViewById(R.id.tv_update_msg)
        iv_update.setOnClickListener(this)
        tv_version.text = rededVersion
        tv_version2 = view.findViewById(R.id.tv_version2)
        tv_version.text = ServerVersion

        mode_onoff = view.findViewById(R.id.mode_onoff)
        mode_onoff.setOnClickListener(this)
    }

    override fun onClick(view: View) {
        when (view.id) {
            R.id.iv_update -> {
                Log.d("sss", "firm")
                if (bleService.isConnected()) { //읽고쓰기가능
                    if (isConnectionFast(requireActivity())) { //네트워크 접속
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                            m_Updatekey = true //업데이트 하겠다
                            viewModel.getLatestFirmwareVersion()
                        } else {
                            if (checkPermissionForReadExtertalStorage(requireActivity()) || checkPermissionForWriteExtertalStorage(
                                    requireActivity()
                                )
                            ) { //권한체크
                                m_Updatekey = true //업데이트 하겠다
//                                GetVersion()
                                viewModel.getLatestFirmwareVersion()
                            } else {
                                checkStoragePermission(activity)
                            }
                        }
                    } else { //네트워크 X
                        setAlertDialog(requireActivity(), getString(R.string.CheckInternetConnection))
                    }
                }
            }

            R.id.mode_onoff -> {
                val sharedPref = this.requireActivity().getPreferences(Context.MODE_PRIVATE)
                val editor = sharedPref.edit()
                editor.putString("MODE_ONOFF", "MODE_ON")
                editor.apply()
                setAlertDialog(requireActivity(), getString(R.string.mode_changesuccess))
            }
        }
    }

    private fun unRegisterReceiver() {
        try {
            localBroadcastManager.unregisterReceiver(mGattUpdateReceiver)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    override fun onDestroyView() {
        unRegisterReceiver()
        try {
            requireActivity().unbindService(serviceConnector)
        } catch (e : Exception) {
            e.printStackTrace()
        }
        super.onDestroyView()
    }

    override fun onStop() {
        super.onStop()
        if (!requireActivity().isChangingConfigurations) {
            isBackStacked = true
        }
    }

    private val mGattUpdateReceiver: BroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val action = intent.action
            when (action) {
                AppConstants.ACTION_DEVICE_DISCONNECTED -> {
                    hideProgressDialog(activity)
                    setAlertDialog(requireActivity(), getString(R.string.device_disconnected))
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
        val responseArray = response!!.split(",")
        if (responseArray[0].trim() == AppConstants.COMMAND_RES) {
            val command = responseArray[1].trim()
            when (command) {
                AppConstants.COMMAND_FIRMWARE_READ -> {
                    Log.d("FirmwareVersion", responseArray[3].trim())
                    if (responseArray.size == 4) {
                        hideProgressDialog(activity)
                        if (responseArray[2].trim() == AppConstants.RESPONSE_S) {
                            var version = responseArray[3].trim()
                            firmwareVersion = version
                            if (!version.uppercase(Locale.getDefault()).startsWith("V")) {
                                version = "V $version"
                            }
                            rededVersion = version
                            tv_version.text = version
                            m_UpdateCheck = true
//                            GetVersion()
                            viewModel.getLatestFirmwareVersion()
                        }
                    }
                }
            }
        }
    }

    private fun displayUpdateNotice() {
        if (activity != null) {
            requireActivity().runOnUiThread {
                iv_update.isSelected = true
                iv_update.isClickable = true
                if (m_UpdateCheck_latest) //최신버전이면
                {
                    iv_update.isSelected = false
                    iv_update.isClickable = false
                    //m_UpdateCheck_latest = false; //??
                    mTvUpdateMsg.visibility = View.VISIBLE
                    mTvUpdateMsg.setText(R.string.tv_lastest_version)
                } else  //최신버전 아니면
                {
                    mTvUpdateMsg.visibility = View.VISIBLE
                    mTvUpdateMsg.setText(R.string.tv_lastest_not_version)
                }
            }
        }
    }


    private fun getFirmwareForDevice(url : String) {
        val filePath = requireContext().cacheDir.toString()
        val sd = File(filePath, AppConstants.AXON_DIR)
        if (!sd.exists()) {
            sd.mkdirs()
        }
        val bfile = File(sd, AppConstants.FIRMWARE_FILE_NAME)
        showProgressDialog(activity)
        viewModel.downloadFirmware(url, bfile)
    }

}