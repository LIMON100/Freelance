package com.axon.kisa10.fragment.log

import android.annotation.SuppressLint
import android.content.Context
import android.os.Environment
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.axon.kisa10.util.AppConstants
import com.kisa10.databinding.LogListItemBinding
import me.saket.bytesize.binaryBytes
import java.io.File

/**
 * Created by Hussain on 10/09/24.
 */
class LogListAdapter(private val context: Context) : RecyclerView.Adapter<LogListAdapter.LogListVH>() {

    private val TAG = "LogListAdapter"
    private val logFiles = ArrayList<String>()

    inner class LogListVH(private val binding: LogListItemBinding) : RecyclerView.ViewHolder(binding.root) {

        var filename = ""

        fun bindView(fileName : String) {
            this.filename = fileName
            updateFileSize()
            binding.tvFileName.text = fileName
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
                val filepath = context.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)?.path.toString() + AppConstants.LOG_DIR + "/" + filename
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

        fun toggleCheckImg(visible : Boolean) {
            val visibility = if (visible) {
                View.VISIBLE
            } else {
                View.INVISIBLE
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

        fun setProgress(progress : Int) {
            binding.progressBar.progress = progress
            binding.tvProgress.text = "$progress%"
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): LogListVH {
        val inflater = LayoutInflater.from(parent.context)
        val binding = LogListItemBinding.inflate(inflater, parent, false)
        return LogListVH(binding)
    }

    override fun getItemCount(): Int {
        return logFiles.size
    }

    override fun onBindViewHolder(holder: LogListVH, position: Int) {
        holder.bindView(logFiles[position])
    }

    @SuppressLint("NotifyDataSetChanged")
    fun updateList(logList: Array<String>) {
        logFiles.clear()
        logFiles.addAll(logList)
        notifyDataSetChanged()
    }
}