package com.skyautonet.drsafe.ui.adapter

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import androidx.recyclerview.widget.RecyclerView.ViewHolder
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.ItemFilesDvrBinding

/**
 * Created by Hussain on 07/08/24.
 */
class VideoListAdapter(private val onPlay : () -> Unit) : RecyclerView.Adapter<VideoListAdapter.VideoViewHolder>() {

    inner class VideoViewHolder(private val binding : ItemFilesDvrBinding) : ViewHolder(binding.root) {
        fun bind() {
            binding.apply {
                tvDVRDate.text = "sample.mp4"
                tvDVRSize.text = "115M"
                binding.ivPlay.setImageResource(R.drawable.ic_play)
                binding.ivPlay.setOnClickListener {
                    onPlay()
                }
            }
        }

    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): VideoViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        val binding = ItemFilesDvrBinding.inflate(inflater, parent, false)
        return VideoViewHolder(binding)
    }

    override fun getItemCount(): Int {
        return 1
    }

    override fun onBindViewHolder(holder: VideoViewHolder, position: Int) {
        holder.bind()
    }
}