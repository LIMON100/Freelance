package com.skyautonet.drsafe.ui.setting.adapter

import android.annotation.SuppressLint
import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import androidx.recyclerview.widget.RecyclerView.ViewHolder
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.ItemBrandBinding

/**
 * Created by Hussain on 31/07/24.
 */
class SelectableAdapter(private val onSelectedBrand : (String) -> Unit) : RecyclerView.Adapter<SelectableAdapter.SelectableItemViewHolder>() {

    private var currentSelected = -1
    private val brandList = mutableListOf<String>()

    inner class SelectableItemViewHolder(private val binding : ItemBrandBinding) : ViewHolder(binding.root) {
        fun bind(brand: String) {
            binding.tvBrand.text = brand
            binding.cardRoot.setOnClickListener {
                onSelectedBrand(brand)
                binding.cardRoot.isChecked = !binding.cardRoot.isChecked
                if (binding.cardRoot.isChecked) {
                    binding.imgSelect.setImageResource(R.drawable.ic_circle_selected)
                    currentSelected = adapterPosition
                } else {
                    binding.imgSelect.setImageResource(R.drawable.ic_circle_unselected)
                }
            }
        }

        fun unselect() {
            binding.cardRoot.isChecked = false
            binding.imgSelect.setImageResource(R.drawable.ic_circle_unselected)
        }

    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): SelectableItemViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        val binding = ItemBrandBinding.inflate(inflater, parent, false)
        return SelectableItemViewHolder(binding)
    }

    override fun getItemCount(): Int {
        return brandList.count()
    }

    override fun onBindViewHolder(holder: SelectableItemViewHolder, position: Int) {
        holder.bind(brandList[position])
    }

    @SuppressLint("NotifyDataSetChanged")
    fun submitBrandList(brands : List<String>) {
        brandList.clear()
        brandList.addAll(brands)
        notifyDataSetChanged()
    }
}

