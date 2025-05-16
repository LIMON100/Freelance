package com.axon.kisa10.distributor

import android.bluetooth.BluetoothGattCharacteristic
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.AdapterView.OnItemSelectedListener
import android.widget.ArrayAdapter
import android.widget.Toast
import androidx.fragment.app.Fragment
import androidx.fragment.app.viewModels
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.ble.ServiceConnector
import com.axon.kisa10.model.distributor.calibration.DistributorCalibrationUploadRequest
import com.axon.kisa10.model.distributor.project.Data
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.viewmodel.VehicleInfoViewModel
import com.kisa10.R
import com.kisa10.databinding.FragmentVehicleInfoBinding

class VehicleInfoFragment : Fragment() {

    private lateinit var localBroadcastManager: LocalBroadcastManager

    private lateinit var bleService: AxonBLEService

    private val serviceConnector = ServiceConnector(
        { service ->
            bleService = service
            macAddress = bleService.getBluetoothGatt()?.device?.address ?: ""
            binding.edDeviceInfoMac.setText(macAddress)
            getVinCode()
        },
        { msg ->
            Toast.makeText(requireContext(), msg, Toast.LENGTH_SHORT).show()
        }
    )


    private lateinit var binding: FragmentVehicleInfoBinding

    private var vinCode = ""
    private var carType = ""
    private var macAddress = ""
    private var selectedProject = ""
    private var deviceId = ""

    private var projects = mutableListOf<Data>()
    private var devices = mutableListOf<Data>()

    private val projectList = mutableListOf<String>()
    private val deviceList = mutableListOf<String>()

    private val vehicleInfoViewModel by viewModels<VehicleInfoViewModel>()

    private val provinces by lazy {
        resources.getStringArray(R.array.Provinces)
    }

    private val locationList = mutableListOf<String>()
    private var location = ""

    override fun onAttach(context: Context) {
        super.onAttach(context)
        localBroadcastManager = LocalBroadcastManager.getInstance(context)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val serviceIntent = Intent(requireContext(), AxonBLEService::class.java)
        requireContext().bindService(serviceIntent, serviceConnector, Context.BIND_AUTO_CREATE)

        val intentFilter = IntentFilter()
        intentFilter.addAction(AppConstants.ACTION_CHARACTERISTIC_CHANGED)

        localBroadcastManager.registerReceiver(bleReceiver, intentFilter)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentVehicleInfoBinding.inflate(inflater, container, false)

        vehicleInfoViewModel.successGetProjects.observe(viewLifecycleOwner) { event ->
            event?.getContentIfNotHandled()?.let { projectResponse ->
                projects = projectResponse.data.toMutableList()
                projectList.clear()
                projects.forEach {
                    projectList.add(it.name)
                }.also {
                    val projectAdapter = ArrayAdapter(requireContext(), R.layout.spinner_item, projectList)
                    binding.edProject.adapter = projectAdapter
                    binding.edProject.onItemSelectedListener = object : OnItemSelectedListener {
                        override fun onItemSelected(
                            p0: AdapterView<*>?,
                            p1: View?,
                            p2: Int,
                            p3: Long
                        ) {
                            val data = projects.filter { it.name == projectList[p2] }.firstOrNull()
                            selectedProject = data?.id ?: ""
                        }

                        override fun onNothingSelected(p0: AdapterView<*>?) {
                        }

                    }
                }
            }
        }

        vehicleInfoViewModel.successGetDevice.observe(viewLifecycleOwner) { event ->
            event?.getContentIfNotHandled()?.let { deviceResponse ->
                devices = deviceResponse.data.toMutableList()
                deviceList.clear()
                devices.forEach {
                    deviceList.add(it.name)
                }.also {
                    val deviceAdapter = ArrayAdapter(requireContext(), R.layout.spinner_item, deviceList)
                    binding.edDevices.adapter = deviceAdapter
                    binding.edDevices.onItemSelectedListener = object : OnItemSelectedListener {
                        override fun onItemSelected(
                            p0: AdapterView<*>?,
                            p1: View?,
                            p2: Int,
                            p3: Long
                        ) {
                            val data = devices.filter { it.name == deviceList[p2] }.firstOrNull()
                            deviceId = data?.name ?: ""
                        }

                        override fun onNothingSelected(p0: AdapterView<*>?) {
                        }

                    }
                }
            }
        }

        vehicleInfoViewModel.error.observe(viewLifecycleOwner) { event ->
            event?.getContentIfNotHandled()?.let { msg ->
                Toast.makeText(requireContext(), msg, Toast.LENGTH_SHORT).show()
            }
        }

        vehicleInfoViewModel.getProjects()
        vehicleInfoViewModel.getDevices()

        val carTypeArray = requireContext().resources.getStringArray(R.array.car_types)
        val carTypeArrayAdapter = ArrayAdapter(requireContext(), R.layout.spinner_item, requireContext().resources.getStringArray(R.array.car_types))

        binding.edCarType.adapter = carTypeArrayAdapter
//        binding.edCarType.setSelection(0)

        binding.edCarType.onItemSelectedListener = object : OnItemSelectedListener {
            override fun onItemSelected(p0: AdapterView<*>?, p1: View?, p2: Int, p3: Long) {
                carType = carTypeArray[p2]
            }

            override fun onNothingSelected(p0: AdapterView<*>?) {
            }

        }

        provinces.forEach { province ->
            val cities = when (province) {
                getString(R.string.Seoul_special_city) -> {
                    resources.getStringArray(R.array.Seoul_special_city)
                }

                getString(R.string.Busan_Metropolitan_City) -> {
                    resources.getStringArray(R.array.Busan_Metropolitan_City)
                }

                getString(R.string.Daegu_Metropolitan_City) -> {
                    resources.getStringArray(R.array.Daegu_Metropolitan_City)
                }

                getString(R.string.Incheon_Metropolitan_City) -> {
                    resources.getStringArray(R.array.Incheon_Metropolitan_City)
                }

                getString(R.string.Gwangju_Metropolitan_City) -> {
                    resources.getStringArray(R.array.Gwangju_Metropolitan_City)
                }

                getString(R.string.Ulsan_Metropolitan_City) -> {
                    resources.getStringArray(R.array.Ulsan_Metropolitan_City)
                }

                getString(R.string.gyeonggi_do) -> {
                    resources.getStringArray(R.array.gyeonggi_do)
                }

                getString(R.string.Gangwon_do) -> {
                    resources.getStringArray(R.array.Gangwon_do)
                }

                getString(R.string.Chungcheongbuk_do) -> {
                    resources.getStringArray(R.array.Chungcheongbuk_do)
                }

                getString(R.string.Chungcheongnam_do) -> {
                    resources.getStringArray(R.array.Chungcheongnam_do)
                }

                getString(R.string.Jeollabuk_do) -> {
                    resources.getStringArray(R.array.Jeollabuk_do)
                }

                getString(R.string.Jeollanam_do) -> {
                    resources.getStringArray(R.array.Jeollanam_do)
                }

                getString(R.string.Gyeongsangbuk_do) -> {
                    resources.getStringArray(R.array.Gyeongsangbuk_do)
                }

                getString(R.string.Gyeongsangnam_do) -> {
                    resources.getStringArray(R.array.Gyeongsangnam_do)
                }

                getString(R.string.Jeju_Special_Self_Governing_Province) -> {
                    resources.getStringArray(R.array.Jeju_Special_Self_Governing_Province)
                }

                else -> {
                    emptyArray<String>()
                }
            }
            cities.forEach { city ->
                locationList.add("$city, $province")
            }
        }

        binding.edLocationProvince.setOnClickListener {
            val intent = Intent(MainDistributorActivity.ACTION_LOCATION_PROVINCE)
            intent.putExtra(MainDistributorActivity.EXTRA_DATA_PROVINCE, provinces)
            localBroadcastManager.sendBroadcast(intent)
        }

        val provinceAdapter = ArrayAdapter(requireContext(), R.layout.spinner_item, locationList)
//        binding.edLocationProvince.adapter = provinceAdapter
//        binding.edLocationProvince.onItemSelectedListener = object : OnItemSelectedListener {
//            override fun onItemSelected(p0: AdapterView<*>?, p1: View?, p2: Int, p3: Long) {
//                location = locationList[p2]
//            }
//
//            override fun onNothingSelected(p0: AdapterView<*>?) {
//            }
//
//        }

        binding.edDeviceInfoMac.setText(macAddress)

        binding.btnStartCalibration.setOnClickListener {
            if (validateForm()) {
                val licensePlate = binding.edLicense.text.toString()
                val vincode = binding.edVincode.text.toString()
                val mac = binding.edDeviceInfoMac.text.toString()

                val distributorCalibrationUploadRequest = DistributorCalibrationUploadRequest(
                    caliMaxHigh = 0,
                    caliMinHigh = 0,
                    caliMaxLow = 0,
                    caliMinLow = 0,
                    highBTO = 0,
                    lowBTO = 0,
                    deviceId = deviceId,
                    licensePlate = licensePlate,
                    location = location,
                    projectId = selectedProject,
                    vehicleType = carType,
                    vincode = vincode,
                    deviceMacAddress = mac
                )
                val intent = Intent(MainDistributorActivity.ACTION_CALIBRATION_START_FRAGMENT)
                intent.putExtra(MainDistributorActivity.CALIBRATION_EXTRA_DATA, distributorCalibrationUploadRequest)
                localBroadcastManager.sendBroadcast(intent)
            }
        }

        return binding.root
    }

    private fun validateForm(): Boolean {
        val licensePlate = binding.edLicense.text.toString()
        val deviceMacInfo = binding.edDeviceInfoMac.text.toString()
        val vincode = binding.edVincode.text.toString()

        if (licensePlate.isBlank()) {
            Toast.makeText(requireContext(), "Please Enter License Plate No.", Toast.LENGTH_SHORT).show()
            return false
        }
        if (deviceMacInfo.isBlank()){
            Toast.makeText(requireContext(), "Please Enter Device Mac Address", Toast.LENGTH_SHORT).show()
            return false
        }
        if (location.isBlank()) {
            Toast.makeText(requireContext(), "Please Select Location", Toast.LENGTH_SHORT).show()
            return false
        }
        if (selectedProject.isBlank()){
            Toast.makeText(requireContext(), "Please Select Project", Toast.LENGTH_SHORT).show()
            return false
        }
        if (carType.isBlank()) {
            Toast.makeText(requireContext(), "Please Select Car Type", Toast.LENGTH_SHORT).show()
            return false
        }
        if (vincode.isBlank()){
            Toast.makeText(requireContext(), "Please Enter VinCode", Toast.LENGTH_SHORT).show()
            return false
        }

        if (deviceId.isBlank()) {
            Toast.makeText(requireContext(), "Please Select Device Id", Toast.LENGTH_SHORT).show()
        }

        return true
    }

    private fun getVinCode() {
        val cmd = "AT$\$CINFO?\r\n".toByteArray(charset("euc-kr"))
        bleService.writeCharacteristic(cmd, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)
    }

    private val bleReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (intent.action == AppConstants.ACTION_CHARACTERISTIC_CHANGED) {
                val response = intent.getStringExtra("data")
                parseResponse(response)
            }
        }

    }

    private fun parseResponse(response : String?) {
        if (response?.contains("CINFO") == true) {
            vinCode = response.split(":")[1].split(",")[3]
            binding.edVincode.setText(vinCode)
        }
    }

    fun setLocation(location : String) {
        binding.edLocationProvince.setText(location)
        this.location = location
    }

}