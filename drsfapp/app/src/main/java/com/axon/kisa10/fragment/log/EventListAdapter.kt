package com.axon.kisa10.fragment.log

import android.content.Context
import android.os.Environment
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.databinding.EventListItemBinding
import me.saket.bytesize.binaryBytes
import java.io.File

/**
 * Created by Hussain on 11/09/24.
 */
class EventListAdapter(private val context: Context) : RecyclerView.Adapter<EventListAdapter.EventListHolder>() {

    private val TAG = "EventListAdapter"
    private val eventList = ArrayList<String>()

    inner class EventListHolder(private val binding : EventListItemBinding) : RecyclerView.ViewHolder(binding.root) {

        var filename = ""

        fun bindView(filename: String) {
            this.filename = filename
            updateFileSize()
            binding.tvFileName.text = filename
        }

        private fun updateFileSize() {
            val size = checkFileSize()
            if (size > 0) {
                binding.tvSize.visibility = View.VISIBLE
                binding.tvSize.text = size.binaryBytes.toString()
            } else {
                binding.tvSize.visibility = View.INVISIBLE
            }
        }

        private fun checkFileSize(): Long {
            try {
                val filepath = context.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)?.path.toString() + AppConstants.EVENT_DIR + "/" + filename
                val file = File(filepath)
                return if (file.exists()) {
                    file.length()
                } else {
                    0
                }
            } catch (e : Exception) {
                Log.e(TAG, "logFileExist", e.cause)
                return 0
            }
        }

        fun showProgress(visible: Boolean) {
            val visibility = if (visible) {
                View.VISIBLE
            } else {
                View.GONE
            }
            binding.progressBar.visibility = visibility
            binding.tvProgress.visibility = visibility
        }

        fun toggleCheckImg(visible: Boolean) {
            val visibility = if (visible) {
                View.VISIBLE
            } else {
                View.GONE
            }
        }

        fun setProgress(progress: Int) {
            binding.progressBar.progress = progress
            binding.tvProgress.text = "$progress%"
        }

    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): EventListHolder {
        val inflater = LayoutInflater.from(parent.context)
        val binding = EventListItemBinding.inflate(inflater, parent, false)
        return EventListHolder(binding)
    }

    override fun getItemCount(): Int {
        return eventList.size
    }

    override fun onBindViewHolder(holder: EventListHolder, position: Int) {
        holder.bindView(eventList[position])
    }

    fun updateList(eventList : Array<String>) {
        this.eventList.clear()
        this.eventList.addAll(eventList)
        notifyDataSetChanged()
    }
}