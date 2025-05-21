package com.axon.kisa10.distributor

import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.axon.kisa10.databinding.FragmentLocationProvinceBinding

class LocationFragment(private val locationList : List<String>, val type : LOCATION_TYPE) : Fragment() {

    companion object {
        enum class LOCATION_TYPE {
            PROVINCE, CITY
        }
    }

    private lateinit var binding : FragmentLocationProvinceBinding

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(requireContext())
    }

    private val adapter = LocationAdapter { location ->
        if (type == LOCATION_TYPE.PROVINCE) {
            val intent = Intent(MainDistributorActivity.ACTION_LOCATION_COUNTY)
            intent.putExtra(MainDistributorActivity.EXTRA_DATA_PROVINCE, location)
            localBroadcastManager.sendBroadcast(intent)
        } else {
            val intent = Intent(MainDistributorActivity.LOCATION_SELECTION_COMPLETE)
            intent.putExtra(MainDistributorActivity.EXTRA_DATA_PROVINCE, requireArguments().getString(MainDistributorActivity.EXTRA_DATA_PROVINCE))
            intent.putExtra(MainDistributorActivity.EXTRA_DATA_COUNTY, location)
            localBroadcastManager.sendBroadcast(intent)
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentLocationProvinceBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        binding.rvLocation.adapter = adapter
        adapter.addLocations(locationList)
    }


}