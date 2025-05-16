package com.axon.kisa10.fragment

import android.bluetooth.BluetoothGattCharacteristic
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
import androidx.appcompat.app.AlertDialog
import androidx.fragment.app.Fragment
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.activity.MainActivity
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.IBackPressed
import com.kisa10.R
import com.kisa10.databinding.FragmentFirmwareUpdateBinding
import java.io.File
import java.util.LinkedList

/**
 * Created by Hussain on 24/09/24.
 */
class DBUpdateFragment(private val bleService: AxonBLEService) : Fragment(), IBackPressed {

    private val TAG = "DBUpdateFragment"
    private lateinit var binding: FragmentFirmwareUpdateBinding
    private var databaseVersion = ""
    private var savedFilename = ""
    private var chunkSize = bleService.getMtuSize()
    private var dataSent = 0
    private var totalDataSize = 0
    private var fileChunks = LinkedList<ByteArray>()
    private var isUpdating = false

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(requireContext())
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        databaseVersion = arguments?.getString(AppConstants.DB_VERSION) ?: ""
        savedFilename = arguments?.getString(AppConstants.FIRMWARE_FILE_NAME) ?: ""
        localBroadcastManager.registerReceiver(mGattUpdateReceiver, AppConstants.makeIntentFilter())
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentFirmwareUpdateBinding.inflate(inflater, container, false)
        databaseVersion = arguments?.getString(AppConstants.DB_VERSION) ?: ""
        bleService.readDeviceData(AppConstants.COMMAND_DATABASE_START_UPDATE + ",1,$databaseVersion")
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        binding.tvOk.isClickable = false
        binding.tvOk.setOnClickListener {
            val intent = Intent(MainActivity.FIRMWARE_UPDATE_OK)
            localBroadcastManager.sendBroadcast(intent)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
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
            if (command == AppConstants.COMMAND_DATABASE_START_UPDATE) { // 403
                val status = responseArray[2].trim()
                if (status == AppConstants.RESPONSE_S) {
                    // start update with 402 command
                    bleService.readDeviceData(AppConstants.COMMAND_DATABASE_TRANSFER + ",${getFileSize()},Z")
                }

                if (status == AppConstants.RESPONSE_F) {
                    val error = responseArray[3].trim()
                    // show error
                    Toast.makeText(requireContext(), error, Toast.LENGTH_SHORT).show()
                }
            }

            if (command == AppConstants.COMMAND_DATABASE_TRANSFER) { // 402
                val status = responseArray[2].trim()
                if (status == AppConstants.RESPONSE_S) {
                    isUpdating = true
                    chunkSize = responseArray[3].trim().toInt()
                    getFileChunks()
                    sendNextChunk()
                }

                if (status == AppConstants.RESPONSE_C) {
                    sendNextChunk()
                }

                if (status == AppConstants.RESPONSE_A) {
                    isUpdating = false
                    bleService.readDeviceData(AppConstants.COMMAND_DATABASE_END)
                }

                if (status == AppConstants.RESPONSE_F) {
                    isUpdating
                    val error = responseArray[3].trim()
                    // show error
                    Toast.makeText(requireContext(), error, Toast.LENGTH_SHORT).show()
                }
            }

            if (command == AppConstants.COMMAND_DATABASE_END) {
                val status = responseArray[2].trim()
                if (status == AppConstants.RESPONSE_S) {
                    AlertDialog.Builder(requireContext())
                        .setCancelable(false)
                        .setTitle(getString(R.string.db_update))
                        .setMessage(getString(R.string.update_success))
                        .show()
                    showTransferComplete()
                }

                if (status == AppConstants.RESPONSE_F) {
                    val error = responseArray[3].trim()
                    Toast.makeText(requireContext(), "Unable to end transfer : $error", Toast.LENGTH_SHORT).show()
                }
            }
        }
    }

    private fun getFileChunks() {
        val file = File(getFilePath())
        if (file.exists()) {
            val bytes = file.readBytes()
            fileChunks.clear()
            totalDataSize = bytes.size
            bytes.toList().chunked(chunkSize).map { fileChunks.add(it.toByteArray()) }
        }
    }

    private fun showTransferComplete() {
        binding.tvOk.visibility = View.VISIBLE
    }

    private fun sendNextChunk() {
        if (fileChunks.isNotEmpty()) {
            val chunk = fileChunks.poll() ?: return
            val mtuChunks = chunk.toList().chunked(bleService.getMtuSize())
            for (i in mtuChunks.indices) {
                val c = mtuChunks[i].toByteArray()
                if (bleService.writeCharacteristic(c, type = BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)) {
                    dataSent += c.size
                    val progress = (dataSent.toFloat() / totalDataSize) * 100f
                    binding.seekBarProgress.curProcess = progress.toInt()
                    binding.tvProgress.text = progress.toInt().toString()
                    Log.d(TAG, "sent $dataSent bytes")
                    Thread.sleep(50)
                }
            }
        } else {
            Log.i(TAG, "sendNextChunk: no data to sent.")
        }
    }

    private fun getFileSize() : Long {
        val file = File(getFilePath())
        return file.length()
    }

    private fun getFilePath() : String {
        return requireContext().getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)?.path.toString() + AppConstants.DB_DIR + "/$savedFilename"
    }

    override fun onBackPressed(): Boolean {
        if (isUpdating) {
            AlertDialog.Builder(requireContext())
                .setMessage("Update In Progress. Please Wait")
                .setPositiveButton("Ok") { dialog, _ ->
                    dialog.dismiss()
                }
                .show()
            return false
        } else {
            binding.tvOk.performClick()
            return false
        }
    }
}