package com.axon.kisa10.fragment

import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import androidx.core.os.LocaleListCompat
import androidx.fragment.app.Fragment
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.LocaleHelper
import com.axon.kisa10.util.SharedPrefKeys
import com.axon.kisa10.util.SharedPrefManager
import com.axon.kisa10.R
import com.axon.kisa10.databinding.FragmentChangeLanguageBinding
import java.util.Locale

/**
 * Created by Hussain on 28/06/24.
 */
class ChangeLanguageFragment : Fragment() {

    private val TAG = "ChangeLanguageFragment"
    private lateinit var binding : FragmentChangeLanguageBinding
    private var currentLocale = ""
    private var selectedLocale = ""
    private lateinit var sharedPrefManager: SharedPrefManager
    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentChangeLanguageBinding.inflate(inflater, container, false)
        sharedPrefManager = SharedPrefManager.getInstance(requireContext())
        return binding.root
    }
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        currentLocale = sharedPrefManager.getString(SharedPrefKeys.APP_LOCALE) ?: "en"
        if (currentLocale.lowercase().contains("en")) {
            binding.imgEnglish.setImageResource(R.drawable.ic_check_circle)
        } else if (currentLocale.lowercase().contains("ko")) {
            binding.imgKorean.setImageResource(R.drawable.ic_check_circle)
        }

        binding.llEnglish.setOnClickListener {
            checkImage(binding.imgEnglish)
            uncheckImage(binding.imgKorean)
            selectedLocale = "english"
        }

        binding.llKorean.setOnClickListener {
            checkImage(binding.imgKorean)
            uncheckImage(binding.imgEnglish)
            selectedLocale = "korean"
        }

        binding.btnSave.setOnClickListener {
            if (currentLocale == selectedLocale) {
                return@setOnClickListener
            }

            val appLocale = if (selectedLocale == "english") {
                "en"
            } else if (selectedLocale == "korean") {
                "ko"
            } else {
                // empty
                null
            }

            if (appLocale != null) {
                LocaleHelper.setLocale(requireContext(),appLocale)
                requireActivity().recreate()
            }

        }

        Log.i(TAG, "onViewCreated: $currentLocale")
    }

    private fun checkImage(image : ImageView) = image.setImageResource(R.drawable.ic_check_circle)

    private fun uncheckImage(image : ImageView) = image.setImageResource(R.drawable.ic_uncheck_circle)

}