package com.axon.kisa10.fragment.log

import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.activity.MainActivity
import com.axon.kisa10.databinding.FragmentDataMenuBinding

/**
 * Created by Hussain on 10/09/24.
 */
class DataMenuFragment : Fragment() {

    private lateinit var binding : FragmentDataMenuBinding

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(requireContext())
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentDataMenuBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        binding.llDataUpdate.setOnClickListener {
            localBroadcastManager.sendBroadcast(Intent(MainActivity.ACTION_LOG_UPDATE_FRAGMENT))
        }

        binding.llEventUpdate.setOnClickListener {
            localBroadcastManager.sendBroadcast(Intent(MainActivity.ACTION_EVENT_UPDATE_FRAGMENT))
        }

        binding.llLogView.setOnClickListener {
            localBroadcastManager.sendBroadcast(Intent(MainActivity.ACTION_LOG_VIEW_FRAGMENT))
        }

        binding.llEventView.setOnClickListener {
            localBroadcastManager.sendBroadcast(Intent(MainActivity.ACTION_EVENT_VIEW_FRAGMENT))
        }
    }
}