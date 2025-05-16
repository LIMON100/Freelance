package com.skyautonet.drsafe.ui.mydriving.ecall

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.skyautonet.drsafe.databinding.ItemAddContactBinding
import com.skyautonet.drsafe.databinding.ItemEmergencyContactBinding

class EcallAdapter(private val onClickAddContact: () -> Unit) : RecyclerView.Adapter<RecyclerView.ViewHolder>() {

    private var selectedContact: String? = null

    inner class EcallViewHolder(private val binding: ItemEmergencyContactBinding) : RecyclerView.ViewHolder(binding.root) {
        fun bindView(contact: String) {
            // Set emergency contact data here (if needed)
        }
    }

    inner class AddContactViewHolder(private val binding: ItemAddContactBinding) : RecyclerView.ViewHolder(binding.root) {
        fun bindView() {
            binding.root.setOnClickListener {
                onClickAddContact()
            }
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): RecyclerView.ViewHolder {
        return when (viewType) {
            0 -> {
                val binding = ItemEmergencyContactBinding.inflate(LayoutInflater.from(parent.context), parent, false)
                EcallViewHolder(binding)
            }
            else -> {
                val binding = ItemAddContactBinding.inflate(LayoutInflater.from(parent.context), parent, false)
                AddContactViewHolder(binding)
            }
        }
    }

    override fun getItemCount(): Int = 4

    override fun onBindViewHolder(holder: RecyclerView.ViewHolder, position: Int) {
        when (holder) {
            is EcallViewHolder -> {
                // Bind the emergency contact data here
            }
            is AddContactViewHolder -> {
                holder.bindView()
            }
        }
    }

    override fun getItemViewType(position: Int): Int {
        return if (position < 3) 0 else 1
    }

    fun getSelectedContact(): String? {
        return selectedContact
    }

    fun submitList(items: List<String>) {
        // Logic to submit a list of items, possibly updating contacts list
    }
}
