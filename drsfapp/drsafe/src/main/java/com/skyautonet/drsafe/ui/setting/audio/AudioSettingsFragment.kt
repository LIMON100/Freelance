package com.skyautonet.drsafe.ui.setting.audio

import android.graphics.drawable.Drawable
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentAudioSettingsBinding
import com.skyautonet.drsafe.ui.BaseFragment

/**
 * Created by Hussain on 01/08/24.
 */
class AudioSettingsFragment : BaseFragment() {

    private lateinit var binding: FragmentAudioSettingsBinding


    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentAudioSettingsBinding.inflate(inflater, container, false)
        setupToolbar(binding.toolbar)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.btnSave.setOnClickListener {
            Toast.makeText(requireContext(), getString(R.string.audio_settings_saved), Toast.LENGTH_SHORT).show()
            onBackIconPressed()
        }

        binding.btnCancel.setOnClickListener {
            onBackIconPressed()
        }
    }

    override fun getStatusBarColor(): Int {
        return getWhiteStatusBarColor()
    }

    override fun getToolbarTitle(): String {
        return getString(R.string.audio_setting)
    }

    override fun showToolbarImage(): Boolean {
        return true
    }

    override fun shouldShowBottomNavigation(): Boolean {
        return false
    }
}