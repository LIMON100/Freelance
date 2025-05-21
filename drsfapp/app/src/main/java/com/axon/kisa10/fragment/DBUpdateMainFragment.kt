package com.axon.kisa10.fragment

import android.app.ProgressDialog
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.fragment.app.Fragment
import androidx.fragment.app.viewModels
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.activity.MainActivity
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.viewmodel.DbUpdateViewModel
import com.axon.kisa10.R
import com.axon.kisa10.databinding.FragmentFirmwareUpdateMainBinding
import java.io.File
import java.util.Calendar

/**
 * Created by Hussain on 24/09/24.
 */
class DBUpdateMainFragment(private val bleService: AxonBLEService) : Fragment() {

    private val TAG = "DBUpdateMainFragment"

    private lateinit var binding : FragmentFirmwareUpdateMainBinding
    private var currentDeviceVersion = ""
    private var currentServerVersion = ""
    private val viewModel by viewModels<DbUpdateViewModel>()
    private var filePath = ""
    private var savedFileName = ""
    
    private var progressDialog : ProgressDialog? = null

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(requireContext())
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        localBroadcastManager.registerReceiver(mGattUpdateReceiver, AppConstants.makeIntentFilter())
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentFirmwareUpdateMainBinding.inflate(inflater, container, false)
        progressDialog = ProgressDialog(requireContext())
        progressDialog?.setMessage(getString(R.string.downloading))
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        binding.ivUpdate.isClickable = false
        viewModel.dbVersionResponse.observe(viewLifecycleOwner) { event ->
            var response = event?.getContentIfNotHandled()
            if (response != null) {
                currentServerVersion = response.version
                binding.tvVersion2.text = currentServerVersion
                filePath = response.file.path
                compareVersion()
            }
        }

        viewModel.error.observe(viewLifecycleOwner) { event ->
            val response = event?.getContentIfNotHandled()
            if (response != null) {
                Toast.makeText(requireContext(), event.peekContent(), Toast.LENGTH_SHORT).show()
            }
        }

        viewModel.dbDownloadProgress.observe(viewLifecycleOwner) { event ->
            if (event.getContentIfNotHandled() != null) {
                val progress = event.peekContent()
                Log.i(TAG, "Downloading : $progress")
                if (progress != 100) {
                    binding.ivUpdate.text = "Downloading : $progress%"
                } else {
                    progressDialog?.hide()
                    binding.ivUpdate.text = getString(R.string.update)
                    binding.ivUpdate.isClickable = true
                    savedFileName = filePath.split("/").last()
                }
            }
        }

        binding.ivUpdate.setOnClickListener {
            val intent = Intent(MainActivity.ACTION_DB_UPDATE_FRAGMENT)
            intent.putExtra(AppConstants.DB_VERSION, currentServerVersion)
            intent.putExtra(AppConstants.FIRMWARE_FILE_NAME, savedFileName)
            localBroadcastManager.sendBroadcast(intent)
        }

        bleService.readDeviceData(AppConstants.COMMAND_DATABASE_VERSION)
    }

    private fun compareVersion() {
        if (currentDeviceVersion.isNotEmpty() && currentServerVersion.isNotEmpty()) {
            val deviceVersion = currentDeviceVersion.split(".")
            val serverVersion = currentServerVersion.split(".")
            val device = Calendar.getInstance().apply {
                set(Calendar.YEAR, deviceVersion.getOrNull(0)?.toIntOrNull() ?: 0)
                set(Calendar.MONTH, deviceVersion.getOrNull(1)?.toIntOrNull() ?: 0)
                set(Calendar.DAY_OF_MONTH, deviceVersion.getOrNull(2)?.toIntOrNull() ?: 0)
            }
            val server = Calendar.getInstance().apply {
                set(Calendar.YEAR, serverVersion.getOrNull(0)?.toIntOrNull() ?: 0)
                set(Calendar.MONTH, serverVersion.getOrNull(1)?.toIntOrNull() ?: 0)
                set(Calendar.DAY_OF_MONTH, serverVersion.getOrNull(2)?.toIntOrNull() ?: 0)
            }
            if (server.after(device)) {
                binding.tvUpdateMsg.text = getString(R.string.tv_lastest_not_version)
                downloadDbUpdateFile()
                binding.ivUpdate.visibility = View.VISIBLE
            } else {
                binding.tvUpdateMsg.text = getString(R.string.tv_lastest_version)
                binding.ivUpdate.visibility = View.INVISIBLE
            }
        }
    }

    private fun downloadDbUpdateFile() {
        val mainDirPath  = requireContext().getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)?.path.toString() + AppConstants.DB_DIR
        val mainDir = File(mainDirPath)
        if (!mainDir.exists()) {
            mainDir.mkdirs()
        }
        val file = File(mainDirPath, filePath.split("/").last())
        progressDialog?.show()
        viewModel.downloadDBUpdateFile(filePath, file)
    }

    override fun onDestroy() {
        super.onDestroy()
        progressDialog?.hide()
        progressDialog = null
        localBroadcastManager.unregisterReceiver(mGattUpdateReceiver)
    }

    private val mGattUpdateReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (intent.action == AppConstants.ACTION_CHARACTERISTIC_CHANGED) {
                val response = intent.getStringExtra("data")
                parseResponse(response)
            }
        }

    }

    private fun parseResponse(response: String?) {
        val responseArray = response?.split(",") ?: return
        if (responseArray[0] == AppConstants.COMMAND_RES) {
            val command = responseArray[1].trim()
            if (command == AppConstants.COMMAND_DATABASE_VERSION) {
                val status = responseArray[2].trim()
                if (status == AppConstants.RESPONSE_S) {
                    currentDeviceVersion = responseArray[3].trim()
                    binding.tvVersion.text = currentDeviceVersion
                    viewModel.getDbVersion()
                }

                if (status == AppConstants.RESPONSE_F) {
                    val errorCode = responseArray[3].trim()
                    // show error message
                    Toast.makeText(requireContext(), "Error: $errorCode", Toast.LENGTH_SHORT).show()
                }
            }
        }
    }
}