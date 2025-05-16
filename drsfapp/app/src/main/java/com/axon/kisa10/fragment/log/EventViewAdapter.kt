package com.axon.kisa10.fragment.log

import android.content.Context
import android.os.Environment
import android.util.Log
import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import androidx.recyclerview.widget.RecyclerView.ViewHolder
import com.axon.kisa10.util.AppConstants
import com.kisa10.databinding.ItemLogViewBinding
import me.saket.bytesize.binaryBytes
import java.io.File

/**
 * Created by Hussain on 19/09/24.
 */
class EventViewAdapter(
    private val context : Context,
    private val clickListener : (String) -> Unit
) : RecyclerView.Adapter<EventViewAdapter.EventViewHolder>() {

    private val TAG = "EventViewAdapter"
    private var eventFiles = emptyArray<String>()

    inner class EventViewHolder(private val binding : ItemLogViewBinding) : ViewHolder(binding.root) {

        private var fileName = ""

        fun bind(filename: String) {
            fileName = filename
            binding.tvFileName.text = filename
            binding.tvSize.text = checkFileSize().binaryBytes.toString()
            binding.root.setOnClickListener {
                clickListener(fileName)
            }
        }

        private fun checkFileSize(): Long {
            try {
                val filepath = context.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)?.path.toString() + AppConstants.EVENT_DIR + "/" + fileName
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

    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): EventViewHolder {
        val layoutInflater = LayoutInflater.from(parent.context)
        val binding = ItemLogViewBinding.inflate(layoutInflater, parent, false)
        return EventViewHolder(binding)
    }

    override fun getItemCount(): Int {
        return eventFiles.size
    }

    override fun onBindViewHolder(holder: EventViewHolder, position: Int) {
        holder.bind(eventFiles[position])
    }

    fun updateList(fileList: Array<String>) {
        this.eventFiles = fileList
    }
}