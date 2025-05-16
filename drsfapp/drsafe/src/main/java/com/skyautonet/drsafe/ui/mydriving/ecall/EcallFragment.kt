package com.skyautonet.drsafe.ui.mydriving.ecall

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.content.res.ResourcesCompat
import androidx.core.view.isVisible
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentEcallBinding
import com.skyautonet.drsafe.ui.BaseFragment

/**
 * Created by Hussain on 30/09/24.
 */
class EcallFragment : BaseFragment() {

    private lateinit var binding: FragmentEcallBinding

    private val ecallAdapter by lazy {
        EcallAdapter {
            if (binding.swAutoUpdate.isChecked) {
                showSelectContactDialog()
            }
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentEcallBinding.inflate(inflater, container, false)
        setupToolbar(binding.toolbar)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        binding.rvEcall.adapter = ecallAdapter
        ecallAdapter.notifyDataSetChanged()

        binding.swAutoUpdate.toggle()
        val isChecked = binding.swAutoUpdate.isChecked
        binding.rvEcall.isEnabled = isChecked
        binding.layer.isVisible = !isChecked

        binding.swAutoUpdate.setOnClickListener {
            binding.swAutoUpdate.toggle()
            val isChecked = binding.swAutoUpdate.isChecked
            binding.rvEcall.isEnabled = isChecked
            binding.layer.isVisible = !isChecked
        }
    }

    private fun showSelectContactDialog() {
        val fragment = requireActivity().supportFragmentManager.findFragmentByTag("select_contact") as? SelectContactDialog
        if (fragment == null) {
            val selectContactDialog = SelectContactDialog { contactNumber ->
                showEmergencyConfirmDialog(contactNumber)
            }
            selectContactDialog.show(requireActivity().supportFragmentManager, "select_contact")
        } else {
            fragment.show(requireActivity().supportFragmentManager, "select_contact")
        }
    }

    private fun showEmergencyConfirmDialog(contactNumber: String) {
        val fragment = requireActivity().supportFragmentManager.findFragmentByTag("emergency_confirm") as? EmergencyConfirmDialog
        if (fragment == null) {
            val emergencyConfirmDialog = EmergencyConfirmDialog.newInstance(contactNumber) {
                showEmergencyFormatSmsDialog(contactNumber)
            }
            emergencyConfirmDialog.show(requireActivity().supportFragmentManager, "emergency_confirm")
        } else {
            fragment.show(requireActivity().supportFragmentManager, "emergency_confirm")
        }
    }

    private fun showEmergencyFormatSmsDialog(contactNumber: String) {
        val fragment = requireActivity().supportFragmentManager.findFragmentByTag("emergency_sms_format") as? EmergencySmsDialog
        if (fragment == null) {
            val emergencySmsDialog = EmergencySmsDialog.newInstance(contactNumber) {
                // Save format or handle actions here
            }
            emergencySmsDialog.show(requireActivity().supportFragmentManager, "emergency_sms_format")
        } else {
            fragment.show(requireActivity().supportFragmentManager, "emergency_sms_format")
        }
    }

    override fun showToolbarImage(): Boolean {
        return false
    }

    override fun getStatusBarColor(): Int {
        return ResourcesCompat.getColor(resources, R.color.white, null)
    }

    override fun getToolbarTitle(): String {
        return getString(R.string.e_call)
    }
}
