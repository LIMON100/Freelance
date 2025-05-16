package com.skyautonet.drsafe.ui.dialog

import android.annotation.SuppressLint
import android.app.AlertDialog
import android.app.Dialog
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.os.Bundle
import androidx.core.content.res.ResourcesCompat
import androidx.fragment.app.DialogFragment
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.LayoutConfirmationDialogBinding
import com.skyautonet.drsafe.util.DialogType

/**
 * Created by Hussain on 30/07/24.
 */
class ConfirmationDialog(
    private val dialogType : DialogType,
    private val onClickPositive : () -> Unit = {},
    private val onClickNegative : () -> Unit = {}
) : DialogFragment() {

    private lateinit var binding : LayoutConfirmationDialogBinding

    @SuppressLint("SetTextI18n")
    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        binding = LayoutConfirmationDialogBinding.inflate(layoutInflater)
        var style : Int? = null
        when(dialogType) {
            DialogType.CONFIRMATION -> {
                binding.tvTitle.text = getString(R.string.confirmation)
                binding.tvSubtitle.text = getString(R.string.do_you_want_to_download_the_video)
                binding.ivHero.setImageResource(R.drawable.ic_confirmation)
                style = R.style.Theme_DrSafe_AlertDialog
                binding.tvYes.setTextColor(ResourcesCompat.getColor(resources,R.color.colorOnSurface,null))
                binding.tvCancel.setTextColor(ResourcesCompat.getColor(resources,R.color.colorCritical,null))
            }
            DialogType.DELETE -> {
                binding.tvTitle.text = getString(R.string.delete)
                binding.tvSubtitle.text = getString(R.string.delete_msg)
                binding.ivHero.setImageResource(R.drawable.ic_delete)
                binding.tvYes.text = getString(R.string.delete)
                style = R.style.Theme_DrSafe_AlertDialog
                binding.tvCancel.setTextColor(ResourcesCompat.getColor(resources,R.color.colorOnSurface,null))
                binding.tvYes.setTextColor(ResourcesCompat.getColor(resources,R.color.colorCritical,null))
            }

            DialogType.FORMAT -> {
                binding.tvTitle.text = getString(R.string.format)
                binding.tvSubtitle.text = getString(R.string.do_you_want_to_do_format)
                binding.ivHero.setImageResource(R.drawable.ic_format)
                binding.tvYes.text = getString(R.string.okay)
                style = R.style.Theme_DrSafe_AlertDialog
                binding.tvYes.setTextColor(ResourcesCompat.getColor(resources,R.color.colorOnSurface,null))
                binding.tvCancel.setTextColor(ResourcesCompat.getColor(resources,R.color.colorCritical,null))
            }

            DialogType.GENERIC -> Unit
            DialogType.FORMAT_COMPLETE -> Unit
            DialogType.RESET_WIFI -> {
                binding.tvTitle.text = getString(R.string.reset_wi_fi)
                binding.tvSubtitle.text = getString(R.string.do_you_want_to_reset_your_wifi)
                binding.ivHero.setImageResource(R.drawable.ic_format)
                style = R.style.Theme_DrSafe_AlertDialog
                binding.tvYes.text = getString(R.string.okay)
            }
        }

        val builder = if (style == null) {
            AlertDialog.Builder(requireContext())
        } else {
            AlertDialog.Builder(requireContext(),style)
        }
        isCancelable = false
        builder.setView(binding.root)

        binding.tvCancel.setOnClickListener {
            onClickNegative()
            dismiss()
        }

        binding.tvYes.setOnClickListener {
            onClickPositive()
            dismiss()
        }

        val dialog = builder.create()
        if (style != null) {
            dialog.window!!.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
        }
        return dialog
    }

}