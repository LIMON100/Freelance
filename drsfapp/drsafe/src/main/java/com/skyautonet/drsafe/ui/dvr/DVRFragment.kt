package com.skyautonet.drsafe.ui.dvr

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.content.res.ResourcesCompat
import androidx.navigation.fragment.findNavController
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentDvrBinding
import com.skyautonet.drsafe.ui.BaseFragment
import com.skyautonet.drsafe.ui.dialog.ConfirmationDialog
import com.skyautonet.drsafe.util.DialogType

/**
 * Created by Hussain on 29/07/24.
 */
class DVRFragment : BaseFragment() {

    private lateinit var binding : FragmentDvrBinding
    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentDvrBinding.inflate(inflater, container, false)
        setupToolbar(binding.toolbar)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        binding.dvrPanel.tvHeaderDownloadPlay.text = getString(R.string.download)
        binding.btnDownload.setOnClickListener {
            openConfirmationDialog()
        }
    }

    private fun openConfirmationDialog() {
        val dialog = ConfirmationDialog(DialogType.CONFIRMATION)
        dialog.show(requireActivity().supportFragmentManager,null)
    }

    override fun getStatusBarColor(): Int {
        return ResourcesCompat.getColor(resources, R.color.white, null)
    }

    override fun getToolbarTitle(): String {
        return getString(R.string.dvr_file_title)
    }

    override fun showToolbarImage(): Boolean {
        return false
    }

}