package com.skyautonet.drsafe.ui.mydriving.ecall

import android.content.Intent
import android.content.pm.PackageManager
import android.os.Bundle
import android.provider.ContactsContract
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.fragment.app.DialogFragment
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentSelectContactBinding

/**
 * Created by Hussain on 03/10/24.
 */
class SelectContactDialog(private val onApplyClick: (String) -> Unit) : DialogFragment() {

    private lateinit var binding: FragmentSelectContactBinding
    private val SELECT_CONTACT_REQUEST_CODE = 101

    private val ecallAdapter = EcallAdapter {
        // Handle the Add Contact click
        openContactPicker()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setStyle(STYLE_NORMAL, R.style.Theme_DrSafe_AlertDialog)
        isCancelable = false
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentSelectContactBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        // Setting the adapter for RecyclerView
        binding.rvSelectContact.adapter = ecallAdapter

        // Example of adding items (you can modify based on your actual data)
        val items = listOf("Emergency Contact 1", "Emergency Contact 2", "Emergency Contact 3", "Add Contact")
        ecallAdapter.submitList(items)

        binding.btnCancel.setOnClickListener {
            dismiss()
        }

        binding.btnApply.setOnClickListener {
            // Logic to get the selected contact
            val selectedContact = getSelectedContact()
            if (selectedContact != null) {
                onApplyClick(selectedContact) // Pass the selected contact to the callback
            }
            dismiss()
        }
    }

    private fun getSelectedContact(): String? {
        // Logic to get the selected contact from the EcallAdapter
        // (you need to modify your EcallAdapter to store selected contact)
        return ecallAdapter.getSelectedContact()
    }

    private fun openContactPicker() {
        if (checkContactPermission()) {
            val intent = Intent(Intent.ACTION_PICK).apply {
                type = ContactsContract.CommonDataKinds.Phone.CONTENT_TYPE
            }
            startActivityForResult(intent, SELECT_CONTACT_REQUEST_CODE)
        } else {
            requestContactPermission()
        }
    }

    private fun checkContactPermission(): Boolean {
        return ContextCompat.checkSelfPermission(
            requireContext(),
            android.Manifest.permission.READ_CONTACTS
        ) == PackageManager.PERMISSION_GRANTED
    }

    private fun requestContactPermission() {
        ActivityCompat.requestPermissions(
            requireActivity(),
            arrayOf(android.Manifest.permission.READ_CONTACTS),
            SELECT_CONTACT_REQUEST_CODE
        )
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == SELECT_CONTACT_REQUEST_CODE && resultCode == AppCompatActivity.RESULT_OK) {
            data?.data?.let { contactUri ->
                val cursor = requireContext().contentResolver.query(
                    contactUri,
                    arrayOf(
                        ContactsContract.CommonDataKinds.Phone.DISPLAY_NAME,
                        ContactsContract.CommonDataKinds.Phone.NUMBER
                    ),
                    null,
                    null,
                    null
                )
                cursor?.use {
                    if (it.moveToFirst()) {
                        val contactNumber = it.getString(it.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.Phone.NUMBER))
                        onApplyClick(contactNumber) // Pass the contact number to the callback
                    }
                }
            }
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == SELECT_CONTACT_REQUEST_CODE &&
            grantResults.isNotEmpty() &&
            grantResults[0] == PackageManager.PERMISSION_GRANTED
        ) {
            openContactPicker()
        } else {
            Toast.makeText(requireContext(), "Permission denied", Toast.LENGTH_SHORT).show()
        }
    }
}
