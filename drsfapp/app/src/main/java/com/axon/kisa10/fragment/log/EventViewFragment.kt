package com.axon.kisa10.fragment.log

import android.os.Bundle
import android.os.Environment
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.databinding.FragmentEventViewBinding
import java.io.File

/**
 * Created by Hussain on 19/09/24.
 */
class EventViewFragment : Fragment() {

    private lateinit var binding : FragmentEventViewBinding

    private lateinit var eventViewAdapter: EventViewAdapter

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentEventViewBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        eventViewAdapter = EventViewAdapter(requireContext()) { fileName ->
            openFile(fileName)
        }
        binding.rvEventList.adapter = eventViewAdapter
        listEventFiles()
    }

    private fun listEventFiles() {
        val filePath = requireContext().getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)?.path.toString() + AppConstants.EVENT_DIR
        val mainDir = File(filePath)
        if (mainDir.exists()) {
            val files = mainDir.listFiles()
            val fileList = files?.filter { it.name.endsWith(".EVT") }?.map { it.name }?.toTypedArray() ?: emptyArray()
            if (fileList.isEmpty()) {
                binding.llNoItems.visibility = View.VISIBLE
                binding.rvEventList.visibility = View.GONE
            } else {
                binding.llNoItems.visibility = View.GONE
                binding.rvEventList.visibility = View.VISIBLE
                eventViewAdapter.updateList(fileList)
            }
        }
    }

    private fun openFile(fileName: String) {

    }
}