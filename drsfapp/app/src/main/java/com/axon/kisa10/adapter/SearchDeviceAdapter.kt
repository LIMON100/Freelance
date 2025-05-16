package com.axon.kisa10.adapter

import android.bluetooth.BluetoothDevice
import android.content.Context
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.axon.kisa10.adapter.SearchDeviceAdapter.MyViewHolder
import com.axon.kisa10.model.ScanModel
import com.kisa10.R

class SearchDeviceAdapter(
    private val context: Context,
    private val deviceList: ArrayList<ScanModel>,
    private val onClick : (BluetoothDevice) -> Unit
) : RecyclerView.Adapter<MyViewHolder>() {
    inner class MyViewHolder(view: View) : RecyclerView.ViewHolder(view) {
        var tvDeviceName = view.findViewById<View>(R.id.tv_deviceName) as TextView
        var tvConnect = view.findViewById<View>(R.id.tv_connect) as TextView
        var tvDeviceAddress = view.findViewById<View>(R.id.tv_deviceAddress) as TextView
        var tvDeviceRssi = view.findViewById<View>(R.id.tv_deviceRssi) as TextView
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): MyViewHolder {
        val itemView = LayoutInflater.from(parent.context)
            .inflate(R.layout.adapter_search_device, parent, false)
        return MyViewHolder(itemView)
    }

    override fun onBindViewHolder(holder: MyViewHolder, position: Int) {
        if (position < deviceList.size) {
            val deviceModel = deviceList[position]
            holder.tvDeviceName.text = deviceModel.deviceName
            holder.tvDeviceAddress.text = deviceModel.deviceMacAddress
            holder.tvDeviceRssi.text = deviceModel.deviceRssi.toString()
            holder.tvConnect.setOnClickListener {
                onClick(deviceModel.bluetoothDevice)
            }
        }
    }

    override fun getItemCount(): Int {
        return deviceList.size
    }
}
