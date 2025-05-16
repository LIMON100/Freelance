package com.axon.kisa10.fragment

import android.annotation.SuppressLint
import android.bluetooth.BluetoothGattCharacteristic
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.os.CountDownTimer
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.fragment.app.Fragment
import androidx.fragment.app.viewModels
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.activity.MainActivity
import com.axon.kisa10.activity.MainActivity.Companion.setUpdating
import com.axon.kisa10.activity.MainActivity.onKeyBackPressedListener
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.data.CalibrationHistoryDb
import com.axon.kisa10.distributor.MainDistributorActivity
import com.axon.kisa10.model.distributor.calibration.DistributorCalibrationUploadRequest
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.AppConstants.makeIntentFilter
import com.axon.kisa10.util.AppMethods.hideProgressDialog
import com.axon.kisa10.util.AppMethods.setAlertDialog
import com.axon.kisa10.viewmodel.CalibrationViewModel
import com.kisa10.R
import com.kisa10.databinding.FragmentCalibrationProgressBinding
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class CalibrationFragment(private val bleService: AxonBLEService? = null) : Fragment(), onKeyBackPressedListener {
//    private lateinit var tv_ok: TextView
//    private lateinit var tv_update_status: TextView
//    private lateinit var tv_progress: TextView
//    private lateinit var tv_accelerator_status: TextView
//    private lateinit var iv_success: ImageView
//    private lateinit var iv_not_success: ImageView
//    private lateinit var rl_progress: RelativeLayout
//    private lateinit var rl_accelerator: RelativeLayout
//    private lateinit var seek_bar_progress: CircularSeekBar

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(requireContext())
    }

    private val calibrationViewModel by viewModels<CalibrationViewModel>()

    private var timer : CountDownTimer? = null

    private lateinit var binding : FragmentCalibrationProgressBinding

    private val calibDb by lazy {
        CalibrationHistoryDb.getDb(requireContext().applicationContext)
    }

    private val ioScope = CoroutineScope(Dispatchers.IO)

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentCalibrationProgressBinding.inflate(inflater, container, false)
        return binding.root
//        return inflater.inflate(R.layout.calibration_accelerator, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        initUi(view)
        localBroadcastManager.registerReceiver(mGattUpdateReceiver, makeIntentFilter())
        try {
            setHandler()
        } catch (e: InterruptedException) {
            e.printStackTrace()
        }
    }

    private fun initUi(view: View) {
//        tv_ok = view.findViewById(R.id.tv_ok)
//        tv_update_status = view.findViewById(R.id.tv_update_status)
//        tv_accelerator_status = view.findViewById(R.id.tv_accelerator_status)
//        iv_success = view.findViewById(R.id.iv_success)
//        iv_not_success = view.findViewById(R.id.iv_not_success)
//        tv_progress = view.findViewById(R.id.tv_progress)
//        rl_progress = view.findViewById(R.id.rl_progress)
//        rl_accelerator = view.findViewById(R.id.rl_accelerator)
//        Handler(Looper.getMainLooper()).postDelayed({
//            try {
//                rl_accelerator.visibility = View.GONE
//                tv_accelerator_status.visibility = View.GONE
//                rl_progress.visibility = View.VISIBLE
//                tv_update_status.visibility = View.VISIBLE
//            } catch (e: Exception) {
//                e.printStackTrace()
//            }
//        }, 3000)

//        seek_bar_progress = view.findViewById(R.id.seek_bar_progress)
        binding.cpCalibrationUpdate.setOnProgressChangeListener { progress, maxProgress ->
            val str_progress = progress.toInt().toString()
            binding.tvProgress.text = str_progress
            if (progress.toInt() < 100) {
                //tv_update_status.setGravity(Gravity.LEFT);
//                binding.tvFirmwareStatus.setText(R.string.in_calibration)
                binding.tvOk.isEnabled = false
            }
        }

        binding.tvOk.setOnClickListener {
            val intent = Intent(MainActivity.ACTION_CALIBRATION_SUCCESS)
            localBroadcastManager.sendBroadcast(intent)
            requireActivity().onBackPressed()
        }

        calibrationViewModel.successDistributorUpload.observe(viewLifecycleOwner) { event ->
            event?.getContentIfNotHandled()?.let { success ->
                binding.tvOk.isEnabled = true
                binding.cpCalibrationUpdate.setCurrentProgress(100.0)
                setUpdating(false)
                if (success) {
                    binding.tvFirmwareStatus.text = getString(R.string.calib_success)
                } else {
                    binding.tvFirmwareStatus.text = getString(R.string.something_went_wrong)
                }
            }
        }

        calibrationViewModel.error.observe(viewLifecycleOwner) { event ->
            event?.getContentIfNotHandled()?.let { error ->
                binding.tvOk.isEnabled = true
                binding.tvFirmwareStatus.text = getString(R.string.something_went_wrong)
            }
        }
    }


    @SuppressLint("DiscouragedApi")
    @Throws(InterruptedException::class)
    private fun setHandler() {
        Handler(Looper.getMainLooper()).postDelayed({
            var step = 0
            timer = object : CountDownTimer(40000L, 400L) {
                override fun onTick(p0: Long) {
                    step += 1
                    progressHandler(step)
                }

                override fun onFinish() {
                }

            }
            timer?.start()
        }, 3000L)
//        Handler(Looper.getMainLooper()).postDelayed({
//            progressHandler(step)
//        },3000)
    }

    private fun progressHandler(step: Int) {
        Handler(Looper.getMainLooper()).postDelayed({
            try {
                val currentProgress = binding.cpCalibrationUpdate.progress.toInt()
                if (currentProgress < 100) {
                    binding.cpCalibrationUpdate.setCurrentProgress(step.toDouble())
                    binding.tvOk.isEnabled = false
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }, 400L)
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
                        Log.d("result", "response :$response")
                        if (response?.startsWith("$$") == true) {
                            val responseArray = response.split(":")
                            if (responseArray[0].contains("DS_CALIB")) {
                                val content = responseArray[1].split(",")
                                if (content.size > 2) { // read calibration response
                                    val hmax = content[2]
                                    val hmin = content[3]
                                    val hbto = content[4]
                                    val lmax = content[5]
                                    val lmin = content[6]
                                    val lbto = content[7].trim().split(" ")[0].trim()

                                   val distributorCalibrationUploadRequest = arguments?.getSerializable(MainDistributorActivity.CALIBRATION_EXTRA_DATA) as? DistributorCalibrationUploadRequest

                                    if (distributorCalibrationUploadRequest == null) {
                                        Toast.makeText(requireContext(), getString(R.string.something_went_wrong), Toast.LENGTH_SHORT).show()
                                        requireActivity().onBackPressed()
                                        return
                                    }

                                    val dateFormat = SimpleDateFormat("dd.MM.yy", Locale.getDefault()).format(Date())
                                    val timeFormat = SimpleDateFormat("HH:mm", Locale.getDefault()).format(Date())

                                    val newData = distributorCalibrationUploadRequest.copy(
                                        caliMaxHigh = hmax.toInt(),
                                        caliMinHigh = hmin.toInt(),
                                        caliMaxLow = lmax.toInt(),
                                        caliMinLow = lmin.toInt(),
                                        highBTO = hbto.toInt(),
                                        lowBTO = lbto.toInt(),
                                    )

                                    val dto = newData.toDto(dateFormat, timeFormat)

                                    Log.i("Calibration", "$newData")

                                    calibrationViewModel.uploadDistributorCalibrationData(newData)

                                    ioScope.launch {
                                        try {
                                            calibDb.calibrationDao().save(dto)
                                        } catch (e : Exception) {
                                            e.printStackTrace()
                                        }
                                    }

                                } else { // write calibration response
                                    if (content[0].trim() == "0") {
                                        val cmd = "AT$\$DS_CALIB?\r\n".toByteArray(charset("euc-kr"))
                                        bleService?.writeCharacteristic(cmd, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)
                                        binding.cpCalibrationUpdate.setCurrentProgress(99.toDouble())
                                        setUpdating(false)
                                    } else {
                                        Toast.makeText(requireContext(), content.toString(), Toast.LENGTH_SHORT).show()
                                        requireActivity().finish()
                                    }
                                }
                            }
                        } else {
                            parseResponse(response)
                        }
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                }
            }
        }
    }

    private fun parseResponse(response: String?) {
        val responseArray = response?.split(",") ?: return
        if (responseArray[0].trim() == AppConstants.COMMAND_RES) {
            val command = responseArray[1].trim()
            when (command) {
//                AppConstants.COMMAND_DEVICE_CALIBRATION_WRITE -> if (responseArray[2] == AppConstants.RESPONSE_S) {
//                    if (BuildConfig.IS_DISTRIBUTOR) {
//                        bleService?.readDeviceData(AppConstants.COMMAND_DEVICE_CALIBRATION_READ)
//                        iv_success.visibility = View.VISIBLE
//                        iv_not_success.visibility = View.GONE
//                        rl_progress.visibility = View.GONE
//                        seek_bar_progress.progress = 100f
//                        setUpdating(false)
//                        tv_update_status.gravity = Gravity.CENTER
//                        tv_update_status.setText(getString(R.string.uploading_data_to_the_server))
//                    } else {
//                        iv_success.visibility = View.VISIBLE
//                        iv_not_success.visibility = View.GONE
//                        tv_ok.visibility = View.VISIBLE
//                        rl_progress.visibility = View.GONE
//                        seek_bar_progress.progress = 100f
//                        setUpdating(false)
//                        tv_update_status.gravity = Gravity.CENTER
//                        tv_update_status.setText(R.string.calibration_success)
//                    }
//                } else if (responseArray[2].trim() == AppConstants.RESPONSE_F) {
//                    iv_success.visibility = View.GONE
//                    iv_not_success.visibility = View.VISIBLE
//                    rl_progress.visibility = View.GONE
//                    seek_bar_progress.progress = 100f
//                    setUpdating(false)
//                    tv_update_status.gravity = Gravity.CENTER
//                    tv_update_status.setText(R.string.calibration_fail)
//                }

                AppConstants.COMMAND_DEVICE_CALIBRATION_READ -> {
                    if (responseArray.size == 10) {
                        val calibrationData = responseArray.subList(4,responseArray.size)
                        val distributorCalibrationUploadRequest = arguments?.getSerializable(MainDistributorActivity.CALIBRATION_EXTRA_DATA) as? DistributorCalibrationUploadRequest

                        if (distributorCalibrationUploadRequest == null) {
                            Toast.makeText(requireContext(), getString(R.string.something_went_wrong), Toast.LENGTH_SHORT).show()
                            requireActivity().onBackPressed()
                            return
                        }

                        val newData = distributorCalibrationUploadRequest.copy(
                            caliMaxHigh = calibrationData[0].toInt(),
                            caliMinHigh = calibrationData[1].toInt(),
                            caliMaxLow = calibrationData[3].toInt(),
                            caliMinLow = calibrationData[4].toInt(),
                        )

                        Log.i("Calibration", "$newData")

                        calibrationViewModel.uploadDistributorCalibrationData(newData)
                    }
                }
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
        super.onDestroyView()
    }

    override fun onAttach(context: Context) {
        super.onAttach(context)

        //메인뷰 액티비티의 뒤로가기 callback 붙이기
        (context as? MainActivity)?.setOnKeyBackPressedListener(this)
    }


    override fun onBackKey() {
        (requireActivity() as? MainActivity)?.setOnKeyBackPressedListener(null)
        //액티비티의 콜백을 직접호출
        requireActivity().onBackPressed()
    }

    companion object {
        fun newInstance(): CalibrationFragment {
            return CalibrationFragment()
        }
    }
}
