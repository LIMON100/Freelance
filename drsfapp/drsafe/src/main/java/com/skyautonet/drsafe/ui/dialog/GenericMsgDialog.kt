package com.skyautonet.drsafe.ui.dialog

import android.app.AlertDialog
import android.app.Dialog
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.os.Bundle
import androidx.fragment.app.DialogFragment
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.LayoutGenericDialogBinding
import com.skyautonet.drsafe.util.DialogType

/**
 * Created by Hussain on 31/07/24.
 */
class GenericMsgDialog(
    private val type : DialogType = DialogType.GENERIC,
    private val title : String,
    private val subtitle : String
) : DialogFragment() {


    private lateinit var binding : LayoutGenericDialogBinding

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        binding = LayoutGenericDialogBinding.inflate(layoutInflater)
        when(type) {
            DialogType.CONFIRMATION -> Unit
            DialogType.DELETE -> Unit
            DialogType.FORMAT -> Unit
            DialogType.GENERIC -> {
                binding.tvTitle.text = title
                binding.tvSubtitle.text = subtitle
            }
            DialogType.FORMAT_COMPLETE -> {
                binding.tvTitle.text = title
                binding.tvTitle.setTextColor(resources.getColor(R.color.colorCritical,null))
                binding.ivHero.setImageResource(R.drawable.ic_no_sdcard)
                binding.tvOk.text = getString(R.string.okay)
            }

            DialogType.RESET_WIFI -> Unit
        }

        val builder = AlertDialog.Builder(requireActivity(),R.style.Theme_DrSafe_AlertDialog)
        isCancelable = false
        builder.setView(binding.root)

        binding.tvOk.setOnClickListener {
            dismiss()
        }

        val dialog = builder.create()
        return dialog
    }

}