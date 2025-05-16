package com.skyautonet.drsafe.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import com.skyautonet.drsafe.databinding.FragmentHomeBinding

/**
 * Created by Hussain on 27/07/24.
 */
class HomeFragment : BaseFragment() {

    private lateinit var binding : FragmentHomeBinding

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        binding = FragmentHomeBinding.inflate(inflater, container, false)
        return  binding.root
    }

}