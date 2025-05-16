package com.skyautonet.drsafe.ui.setting.dvr

import android.graphics.drawable.Drawable
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentDvrSettingsBinding
import com.skyautonet.drsafe.ui.BaseFragment
import com.skyautonet.drsafe.ui.dialog.ConfirmationDialog
import com.skyautonet.drsafe.ui.dialog.FormatProgressDialog
import com.skyautonet.drsafe.ui.dialog.GenericMsgDialog
import com.skyautonet.drsafe.util.DialogType

/**
 * Created by Hussain on 01/08/24.
 */
class DvrSettingsFragment : BaseFragment() {

    private lateinit var binding : FragmentDvrSettingsBinding

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentDvrSettingsBinding.inflate(inflater, container, false)
        setupToolbar(binding.toolbar)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.btnFormat.setOnClickListener {
            showFormatDialog()
        }

        binding.btnSave.setOnClickListener {
            Toast.makeText(requireContext(), getString(R.string.settings_saved),Toast.LENGTH_SHORT).show()
            onBackIconPressed()
        }

        binding.btnCancel.setOnClickListener {
            onBackIconPressed()
        }
    }

    private fun showFormatDialog() {
        val dialog = ConfirmationDialog(DialogType.FORMAT, {
            startFormat()
        },{})
        dialog.show(requireActivity().supportFragmentManager, null)
    }

    private fun startFormat() {
        val dialog = FormatProgressDialog {
            showFormatCompleteDialog()
        }
        dialog.show(requireActivity().supportFragmentManager, null)
    }

    private fun showFormatCompleteDialog() {
        val dialog = GenericMsgDialog(DialogType.FORMAT_COMPLETE, getString(R.string.format_complete_title), getString(R.string.format_complete_subtitle))
        dialog.show(requireActivity().supportFragmentManager, null)
    }

    override fun getStatusBarColor(): Int {
        return getWhiteStatusBarColor()
    }

    override fun getToolbarTitle(): String {
        return getString(R.string.dvr_options)
    }

    override fun showToolbarImage(): Boolean {
        return true
    }

    override fun shouldShowBottomNavigation(): Boolean {
        return false
    }
}