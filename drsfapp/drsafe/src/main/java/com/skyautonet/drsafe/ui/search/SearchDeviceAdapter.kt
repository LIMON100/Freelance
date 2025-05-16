package com.skyautonet.drsafe.ui.search

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.skyautonet.drsafe.databinding.ItemDeviceBinding
import com.skyautonet.drsafe.model.ScanModel

class SearchDeviceAdapter(val onClick : (ScanModel) -> Unit) : RecyclerView.Adapter<SearchDeviceAdapter.SearchDeviceViewHolder>() {

    private val deviceList = mutableListOf<ScanModel>()

    inner class SearchDeviceViewHolder(val binding : ItemDeviceBinding) : RecyclerView.ViewHolder(binding.root) {
        fun bind(device : ScanModel) {
            binding.tvName.text = device.deviceName
            binding.btnConnect.setOnClickListener {
                onClick(device)
            }
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): SearchDeviceViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        val binding = ItemDeviceBinding.inflate(inflater, parent, false)
        return SearchDeviceViewHolder(binding)
    }

    override fun getItemCount(): Int {
        return deviceList.size
    }

    override fun onBindViewHolder(holder: SearchDeviceViewHolder, position: Int) {
        holder.bind(deviceList[position])
    }

    fun addDevices(devices : List<ScanModel>) {
        deviceList.clear()
        deviceList.addAll(devices)
        notifyDataSetChanged()
    }
}