package com.skyautonet.drsafe.ui.mydriving.ecall

import android.content.pm.PackageManager
import android.os.Bundle
import android.telephony.SmsManager
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.fragment.app.DialogFragment
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.DialogEmergencySmsFormatBinding

/**
 * Created by Hussain on 03/10/24.
 */
class EmergencySmsDialog(
    private val contactNumber: String, // Selected contact's phone number
    private val onApply: () -> Unit // This can handle any actions you need post-confirmation
) : DialogFragment() {

    private lateinit var binding: DialogEmergencySmsFormatBinding

    companion object {
        fun newInstance(contactNumber: String, onApply: () -> Unit): EmergencySmsDialog {
            return EmergencySmsDialog(contactNumber, onApply)
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = DialogEmergencySmsFormatBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.tvContactNumber.text = contactNumber

        binding.btnSendSms.setOnClickListener {
            sendEmergencySms()
        }

        binding.btnCancel.setOnClickListener {
            dismiss()
        }
    }

    private fun sendEmergencySms() {
        if (checkSmsPermission()) {
            val message = "Emergency: Assistance needed!"
            val smsManager = SmsManager.getDefault()
            smsManager.sendTextMessage(contactNumber, null, message, null, null)
            Toast.makeText(requireContext(), "SMS Sent", Toast.LENGTH_SHORT).show()
            dismiss()
        } else {
            requestSmsPermission()
        }
    }

    private fun checkSmsPermission(): Boolean {
        return ContextCompat.checkSelfPermission(
            requireContext(),
            android.Manifest.permission.SEND_SMS
        ) == PackageManager.PERMISSION_GRANTED
    }

    private fun requestSmsPermission() {
        ActivityCompat.requestPermissions(
            requireActivity(),
            arrayOf(android.Manifest.permission.SEND_SMS),
            100
        )
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == 100 &&
            grantResults.isNotEmpty() &&
            grantResults[0] == PackageManager.PERMISSION_GRANTED
        ) {
            sendEmergencySms()
        } else {
            Toast.makeText(requireContext(), "SMS permission denied", Toast.LENGTH_SHORT).show()
        }
    }
}
