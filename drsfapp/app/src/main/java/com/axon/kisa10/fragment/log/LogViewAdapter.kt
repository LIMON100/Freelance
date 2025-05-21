package com.axon.kisa10.fragment.log

import android.content.Context
import android.util.Log
import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import androidx.recyclerview.widget.RecyclerView.ViewHolder
import com.axon.kisa10.util.getLogDownloadDirectory
import com.axon.kisa10.databinding.ItemLogViewBinding
import me.saket.bytesize.binaryBytes
import java.io.File

/**
 * Created by Hussain on 19/09/24.
 */
class LogViewAdapter(
    private val context : Context,
    private val deviceDir : String,
    private val clickListener: (String) -> Unit,
) : RecyclerView.Adapter<LogViewAdapter.LogViewHolder>() {

    private val TAG = "LogViewAdapter"

    private var fileList = emptyArray<String>()

    inner class LogViewHolder(private val binding : ItemLogViewBinding) : ViewHolder(binding.root) {

        private var filename : String = ""
        fun bind(fileName: String) {
            this.filename = fileName
            binding.tvFileName.text = fileName
            binding.tvSize.text = checkFileSize().binaryBytes.toString()
            binding.btnView.setOnClickListener {
                clickListener(fileName)
            }
        }

        private fun checkFileSize(): Long {
            try {
                val filepath = context.getLogDownloadDirectory() + deviceDir + filename
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

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): LogViewHolder {
        val layoutInflater = LayoutInflater.from(parent.context)
        val binding = ItemLogViewBinding.inflate(layoutInflater, parent, false)
        return LogViewHolder(binding)
    }

    override fun getItemCount(): Int {
        return fileList.size
    }

    override fun onBindViewHolder(holder: LogViewHolder, position: Int) {
        val fileName = fileList[position]
        holder.bind(fileName)
    }

    fun updateList(fileList: Array<String>) {
        this.fileList = fileList
        notifyDataSetChanged()
    }
}