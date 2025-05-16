package com.skyautonet.drsafe.ui.termsandcondition

import android.os.Bundle
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.res.ResourcesCompat
import androidx.databinding.DataBindingUtil
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.ActivityTermsAndConditionBinding
import com.skyautonet.drsafe.databinding.LayoutToolbarBinding
import com.skyautonet.drsafe.ui.StartupActivity
import com.skyautonet.drsafe.util.SharedPreferenceUtil
import com.skyautonet.drsafe.util.start

/**
 * Created by Hussain on 02/08/24.
 */
class TermsAndConditionActivity : AppCompatActivity() {

    private lateinit var binding : ActivityTermsAndConditionBinding

    private val checked = mutableListOf(false,false,false)

    private val sharedPreferenceUtil by lazy {
        SharedPreferenceUtil.getInstance(this)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = DataBindingUtil.setContentView(this, R.layout.activity_terms_and_condition)
        window.statusBarColor = ResourcesCompat.getColor(resources, R.color.white, null)
        setupToolbar(binding.toolbar)
        setupView()
    }

    private fun setupView() {
        binding.chkTerms1.setOnCheckedChangeListener { _, isChecked ->
            checked[0] = isChecked
            checkAllSelected()
        }

        binding.chkTerms2.setOnCheckedChangeListener { _, isChecked ->
            checked[1] = isChecked
            checkAllSelected()
        }

        binding.chkTerms3.setOnCheckedChangeListener { _, isChecked ->
            checked[2] = isChecked
            checkAllSelected()
        }

        binding.imgTerms1.setOnClickListener {
            val tag = it.tag as? Boolean
            if (tag == null) {
                binding.llDetailViewTerms1.visibility = View.VISIBLE
                binding.imgTerms1.setImageResource(R.drawable.ic_arrow_up)
                binding.imgTerms1.tag = true
            } else if (tag) {
                binding.llDetailViewTerms1.visibility = View.GONE
                binding.imgTerms1.setImageResource(R.drawable.ic_arrow_down)
                binding.imgTerms1.tag = null
            }
        }

        binding.imgTerms2.setOnClickListener {
            val tag = it.tag as? Boolean
            if (tag == null) {
                binding.llDetailViewTerms2.visibility = View.VISIBLE
                binding.imgTerms2.setImageResource(R.drawable.ic_arrow_up)
                binding.imgTerms2.tag = true
            } else if (tag) {
                binding.llDetailViewTerms2.visibility = View.GONE
                binding.imgTerms2.setImageResource(R.drawable.ic_arrow_down)
                binding.imgTerms2.tag = null
            }
        }

        binding.imgTerms3.setOnClickListener {
            val tag = it.tag as? Boolean
            if (tag == null) {
                binding.llDetailViewTerms3.visibility = View.VISIBLE
                binding.imgTerms3.setImageResource(R.drawable.ic_arrow_up)
                binding.imgTerms3.tag = true
            } else if (tag) {
                binding.llDetailViewTerms3.visibility = View.GONE
                binding.imgTerms3.setImageResource(R.drawable.ic_arrow_down)
                binding.imgTerms3.tag = null
            }
        }

        binding.btnAgree.setOnClickListener {
            sharedPreferenceUtil.setTermsAndConditionsAgreed()
            gotoLoginActivity()
        }
    }

    private fun gotoLoginActivity() {
        start(StartupActivity::class.java)
    }

    private fun checkAllSelected() {
        binding.btnAgree.isEnabled = checked.all { it }
    }

    private fun setupToolbar(toolbar: LayoutToolbarBinding) {
        toolbar.apply {
            toolbarTitle.text = getString(R.string.terms_condition)
            imgHeaderLogo.visibility = View.GONE
            imgToolbar.visibility = View.VISIBLE
        }

    }
}