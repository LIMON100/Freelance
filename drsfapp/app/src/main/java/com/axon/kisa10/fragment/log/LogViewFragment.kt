package com.axon.kisa10.fragment.log

import android.app.ProgressDialog
import android.content.Intent
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.activity.MainActivity
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.LogParser
import com.axon.kisa10.util.getDeviceDirectory
import com.axon.kisa10.util.getLogDownloadDirectory
import com.kisa10.databinding.FragmentLogviewBinding
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File

/**
 * Created by Hussain on 12/09/24.
 */
@Suppress("DEPRECATION")
class LogViewFragment(private val bleService: AxonBLEService) : Fragment() {

    private val TAG = "LogViewFragment"

    private lateinit var binding : FragmentLogviewBinding

    private var logData = ByteArray(0)

    private val ioScope = CoroutineScope(Dispatchers.IO)

    private lateinit var adapter : LogViewAdapter

    private val progressDialog: ProgressDialog by lazy {
        ProgressDialog(requireContext())
    }

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(requireContext())
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentLogviewBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        adapter = LogViewAdapter(requireContext(), bleService.getDeviceDirectory()) { fileName ->
            openFile(fileName)
        }
        binding.rvLogView.adapter = adapter
        listLogFiles()
    }

    private fun listLogFiles() {
        val filePath = requireContext().getLogDownloadDirectory() + bleService.getDeviceDirectory()
        val mainDir = File(filePath)
        val fileList = if (mainDir.exists()) {
            val files = mainDir.listFiles()
            files?.filter { it.name.endsWith(".LOG") }?.map { it.name }?.toTypedArray() ?: emptyArray()
        } else {
            emptyArray()
        }

        if (fileList.isEmpty()) {
            binding.llNoItems.visibility = View.VISIBLE
            binding.rvLogView.visibility = View.GONE
        } else {
            binding.llNoItems.visibility = View.GONE
            binding.rvLogView.visibility = View.VISIBLE
            adapter.updateList(fileList)
        }
    }

    private fun openFile(fileName: String) {
        val geoJsonFilename = fileName + "_vehicle_info_line.geojson"
        Log.i(TAG, "openFile: $geoJsonFilename")
        val filePath = requireContext().getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)?.path.toString() + AppConstants.LOG_DIR + "/" + geoJsonFilename
        val file = File(filePath)
        if (!file.exists()) {
            Log.i(TAG, "openFile: log not parsed")
            progressDialog.setMessage("Parsing Log Data")
            progressDialog.show()
            parseLogData(fileName)
            openLogMapView(fileName)
        } else {
            Log.i(TAG, "openFile: parsed file exisit")
            openLogMapView(fileName)
        }

    }

    private fun openLogMapView(fileName: String) {
        val intent = Intent(MainActivity.ACTION_LOG_MAP_VIEW_FRAGMENT)
        intent.putExtra(AppConstants.KEY_LOG_FILE,fileName)
        localBroadcastManager.sendBroadcast(intent)
    }

    private fun parseLogData(fileName: String) = ioScope.launch {
        val filePath = requireContext().getLogDownloadDirectory() + bleService.getDeviceDirectory() + fileName
        val file = File(filePath)
        if (file.exists()) {
            logData = file.readBytes()
            LogParser.parseEvent(requireContext(), logData, fileName.split(".")[0])
        }
        withContext(Dispatchers.Main) {
            progressDialog.dismiss()
        }
    }

}