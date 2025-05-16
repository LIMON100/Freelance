package com.axon.kisa10.activity

import android.annotation.SuppressLint
import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.ImageView
import androidx.databinding.DataBindingUtil
import com.axon.kisa10.distributor.LoginDistributorActivity
import com.axon.kisa10.distributor.MainDistributorActivity
import com.axon.kisa10.util.SharedPrefKeys
import com.axon.kisa10.util.SharedPrefManager
import com.google.firebase.auth.FirebaseAuth
import com.kisa10.BuildConfig
import com.kisa10.R
import com.kisa10.databinding.ActivityTermsAndConditionBinding

class TermsAndConditionActivity : BaseActivity(), View.OnClickListener {
    private lateinit var binding: ActivityTermsAndConditionBinding

    private lateinit var sharedPrefManager: SharedPrefManager

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = DataBindingUtil.setContentView(this, R.layout.activity_terms_and_condition)

        sharedPrefManager = SharedPrefManager.getInstance(this)
        binding.imgCheck1.setOnClickListener(this)
        binding.imgCheck2.setOnClickListener(this)
        binding.imgCheck3.setOnClickListener(this)

        binding.details1.setOnClickListener(this)
        binding.details2.setOnClickListener(this)
        binding.details3.setOnClickListener(this)

        binding.btnAgreeTerms.setOnClickListener(this)
        binding.imgAgreeAll.setOnClickListener(this)
    }

    private fun checkAllTermsSelected(): Boolean {
        val check1 = binding.imgCheck1.tag != null && (binding.imgCheck1.tag as Int) == 1
        val check2 = binding.imgCheck2.tag != null && (binding.imgCheck2.tag as Int) == 1
        val check3 = binding.imgCheck3.tag != null && (binding.imgCheck3.tag as Int) == 1

        return check1 && check2 && check3
    }

    private fun showDetails1() {
        binding.terms2.visibility = View.GONE
        binding.terms3.visibility = View.GONE
        binding.tvDetails.setText(R.string.eula_location_contents_script)
        binding.detailsView.visibility = View.VISIBLE
        binding.details1.tag = 1
    }

    private fun showDetails2() {
        binding.terms1.visibility = View.GONE
        binding.terms3.visibility = View.GONE
        binding.tvDetails.setText(R.string.eula_privacy_contents_script)
        binding.detailsView.visibility = View.VISIBLE
        binding.details2.tag = 1
    }

    private fun showDetails3() {
        binding.terms1.visibility = View.GONE
        binding.terms2.visibility = View.GONE
        binding.tvDetails.setText(R.string.eula_service_contents_script)
        binding.detailsView.visibility = View.VISIBLE
        binding.details3.tag = 1
    }

    private fun hideDetails() {
        binding.terms1.visibility = View.VISIBLE
        binding.terms2.visibility = View.VISIBLE
        binding.terms3.visibility = View.VISIBLE
        binding.detailsView.visibility = View.GONE
    }

    @SuppressLint("MissingPermission")
    override fun onClick(view: View) {
        if (view.id == R.id.details1) {
            if (isDetailsVisible) {
                hideDetails()
            } else {
                showDetails1()
            }
        }

        if (view.id == R.id.details2) {
            if (isDetailsVisible) {
                hideDetails()
            } else {
                showDetails2()
            }
        }

        if (view.id == R.id.details3) {
            if (isDetailsVisible) {
                hideDetails()
            } else {
                showDetails3()
            }
        }

        if (view.id == R.id.imgCheck1) {
            updateImageState(binding.imgCheck1)
        }

        if (view.id == R.id.imgCheck2) {
            updateImageState(binding.imgCheck2)
        }

        if (view.id == R.id.imgCheck3) {
            updateImageState(binding.imgCheck3)
        }

        if (view.id == R.id.imgAgreeAll) {
            val checked = binding.imgAgreeAll.tag == true
            if (checked) {
                binding.imgAgreeAll.setImageResource(R.drawable.uncheck)
                binding.imgAgreeAll.tag = false
            } else {
                binding.imgAgreeAll.setImageResource(R.drawable.check)
                binding.imgAgreeAll.tag = true
            }
            updateImageState(binding.imgCheck1)
            updateImageState(binding.imgCheck2)
            updateImageState(binding.imgCheck3)
        }

        if (view.id == R.id.btnAgreeTerms) {
            sharedPrefManager.setBool(SharedPrefKeys.TERMS_CONDITION_AGREE_KEY, true)
            if (BuildConfig.IS_DISTRIBUTOR) {
                // goto distributor login or search device
                val isLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.DISTRIBUTOR_TOKEN).isNullOrBlank()
                val intent = if (isLoggedIn) {
                    Intent(this, SearchDeviceActivity::class.java)
                } else {
                    // goto distributer login
                    Intent(this, LoginDistributorActivity::class.java)
                }
                startActivity(intent)
                finish()
            } else {
                val isLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.KAKAO_TOKEN_KEY)
                    .isNullOrBlank() || FirebaseAuth.getInstance().currentUser != null
                if (!isLoggedIn) {
                    val intent = Intent(this, LoginActivity::class.java)
                    startActivity(intent)
                    finish()
                    return
                } else {
                    val intent = Intent(this, SearchDeviceActivity::class.java)
                    startActivity(intent)
                    finish()
                    return
                }
            }
        }
    }

    private fun updateImageState(imageView: ImageView) {
        val tag = imageView.tag != null && (imageView.tag as Int) == 1
        if (!tag) {
            imageView.setImageResource(R.drawable.check)
            imageView.tag = 1
        } else {
            imageView.setImageResource(R.drawable.uncheck)
            imageView.tag = 0
        }
        Log.i(TAG, "updateImageState: $tag")
        binding.btnAgreeTerms.isEnabled = checkAllTermsSelected()
    }

    private val isDetailsVisible: Boolean
        get() = binding.detailsView.visibility == View.VISIBLE

    companion object {
        private const val TAG = "TermsAndConditionActivi"
    }
}