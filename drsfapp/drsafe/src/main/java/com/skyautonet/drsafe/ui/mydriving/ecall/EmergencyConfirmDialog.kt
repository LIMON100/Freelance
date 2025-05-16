package com.skyautonet.drsafe.ui.mydriving.ecall

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.DialogFragment
import com.skyautonet.drsafe.databinding.DialogEmergencyConfirmBinding

class EmergencyConfirmDialog(private val contactNumber: String, private val onConfirm: () -> Unit) : DialogFragment() {

    private lateinit var binding: DialogEmergencyConfirmBinding

    companion object {
        fun newInstance(contactNumber: String, onConfirm: () -> Unit): EmergencyConfirmDialog {
            return EmergencyConfirmDialog(contactNumber, onConfirm)
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = DialogEmergencyConfirmBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        binding.tvContactNumber.text = contactNumber

        binding.btnConfirm.setOnClickListener {
            onConfirm()
            dismiss()
        }

        binding.btnCancel.setOnClickListener {
            dismiss()
        }
    }
}
