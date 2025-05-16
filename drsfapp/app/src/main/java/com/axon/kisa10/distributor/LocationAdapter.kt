package com.axon.kisa10.distributor

import android.annotation.SuppressLint
import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.kisa10.R
import com.kisa10.databinding.ItemLocationBinding

class LocationAdapter(val onClickLocation : (String) -> Unit) : RecyclerView.Adapter<LocationAdapter.LocationViewHolder>() {

    private val locationList = mutableListOf<String>()


    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): LocationViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        return LocationViewHolder(ItemLocationBinding.inflate(inflater, parent, false))
    }

    override fun getItemCount(): Int {
        return locationList.size
    }

    override fun onBindViewHolder(holder: LocationViewHolder, position: Int) {
        holder.bind(locationList[position])
    }

    @SuppressLint("NotifyDataSetChanged")
    fun addLocations(location : List<String>) {
        locationList.clear()
        locationList.addAll(location)
        notifyDataSetChanged()
    }

    inner class LocationViewHolder(private val binding : ItemLocationBinding) : RecyclerView.ViewHolder(binding.root) {

        fun bind(location : String) {
            binding.tvLocation.text = location
            binding.root.setOnClickListener {
                binding.imgCheck.setImageResource(R.drawable.ic_check)
                onClickLocation(location)
            }
        }
    }

}

