package com.skyautonet.drsafe.ui.dialog

import android.app.AlertDialog
import android.app.Dialog
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import androidx.fragment.app.DialogFragment
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.LayoutFormatProgressDialogBinding

/**
* Created by Hussain on 01/08/24.
*/
class FormatProgressDialog(
    private val onComplete : () -> Unit
) : DialogFragment() {

    private lateinit var binding : LayoutFormatProgressDialogBinding

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        binding = LayoutFormatProgressDialogBinding.inflate(layoutInflater)
        val builder = AlertDialog.Builder(requireActivity(), R.style.Theme_DrSafe_AlertDialog)
        isCancelable = false
        builder.setView(binding.root)
        val dialog = builder.create()

        Handler(Looper.getMainLooper()).postDelayed({
            onComplete()
            dismiss()
        },3000)

        return dialog
    }
}