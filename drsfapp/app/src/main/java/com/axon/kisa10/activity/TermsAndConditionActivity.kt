//package com.axon.kisa10.activity
//
//import android.annotation.SuppressLint
//import android.content.Intent
//import android.os.Bundle
//import android.util.Log
//import android.view.View
//import android.widget.ImageView
//import androidx.databinding.DataBindingUtil
//import com.axon.kisa10.distributor.LoginDistributorActivity
//import com.axon.kisa10.distributor.MainDistributorActivity
//import com.axon.kisa10.util.SharedPrefKeys
//import com.axon.kisa10.util.SharedPrefManager
//import com.google.firebase.auth.FirebaseAuth
//import com.axon.kisa10.BuildConfig
//import com.axon.kisa10.R
//import com.axon.kisa10.databinding.ActivityTermsAndConditionBinding
//
//class TermsAndConditionActivity : BaseActivity(), View.OnClickListener {
//    private lateinit var binding: ActivityTermsAndConditionBinding
//
//    private lateinit var sharedPrefManager: SharedPrefManager
//
//    override fun onCreate(savedInstanceState: Bundle?) {
//        super.onCreate(savedInstanceState)
//        binding = DataBindingUtil.setContentView(this, R.layout.activity_terms_and_condition)
//
//        sharedPrefManager = SharedPrefManager.getInstance(this)
//        binding.imgCheck1.setOnClickListener(this)
//        binding.imgCheck2.setOnClickListener(this)
//        binding.imgCheck3.setOnClickListener(this)
//
//        binding.details1.setOnClickListener(this)
//        binding.details2.setOnClickListener(this)
//        binding.details3.setOnClickListener(this)
//
//        binding.btnAgreeTerms.setOnClickListener(this)
//        binding.imgAgreeAll.setOnClickListener(this)
//    }
//
//    private fun checkAllTermsSelected(): Boolean {
//        val check1 = binding.imgCheck1.tag != null && (binding.imgCheck1.tag as Int) == 1
//        val check2 = binding.imgCheck2.tag != null && (binding.imgCheck2.tag as Int) == 1
//        val check3 = binding.imgCheck3.tag != null && (binding.imgCheck3.tag as Int) == 1
//
//        return check1 && check2 && check3
//    }
//
//    private fun showDetails1() {
//        binding.terms2.visibility = View.GONE
//        binding.terms3.visibility = View.GONE
//        binding.tvDetails.setText(R.string.eula_location_contents_script)
//        binding.detailsView.visibility = View.VISIBLE
//        binding.details1.tag = 1
//    }
//
//    private fun showDetails2() {
//        binding.terms1.visibility = View.GONE
//        binding.terms3.visibility = View.GONE
//        binding.tvDetails.setText(R.string.eula_privacy_contents_script)
//        binding.detailsView.visibility = View.VISIBLE
//        binding.details2.tag = 1
//    }
//
//    private fun showDetails3() {
//        binding.terms1.visibility = View.GONE
//        binding.terms2.visibility = View.GONE
//        binding.tvDetails.setText(R.string.eula_service_contents_script)
//        binding.detailsView.visibility = View.VISIBLE
//        binding.details3.tag = 1
//    }
//
//    private fun hideDetails() {
//        binding.terms1.visibility = View.VISIBLE
//        binding.terms2.visibility = View.VISIBLE
//        binding.terms3.visibility = View.VISIBLE
//        binding.detailsView.visibility = View.GONE
//    }
//
//    @SuppressLint("MissingPermission")
//    override fun onClick(view: View) {
//        if (view.id == R.id.details1) {
//            if (isDetailsVisible) {
//                hideDetails()
//            } else {
//                showDetails1()
//            }
//        }
//
//        if (view.id == R.id.details2) {
//            if (isDetailsVisible) {
//                hideDetails()
//            } else {
//                showDetails2()
//            }
//        }
//
//        if (view.id == R.id.details3) {
//            if (isDetailsVisible) {
//                hideDetails()
//            } else {
//                showDetails3()
//            }
//        }
//
//        if (view.id == R.id.imgCheck1) {
//            updateImageState(binding.imgCheck1)
//        }
//
//        if (view.id == R.id.imgCheck2) {
//            updateImageState(binding.imgCheck2)
//        }
//
//        if (view.id == R.id.imgCheck3) {
//            updateImageState(binding.imgCheck3)
//        }
//
//        if (view.id == R.id.imgAgreeAll) {
//            val checked = binding.imgAgreeAll.tag == true
//            if (checked) {
//                binding.imgAgreeAll.setImageResource(R.drawable.uncheck)
//                binding.imgAgreeAll.tag = false
//            } else {
//                binding.imgAgreeAll.setImageResource(R.drawable.check)
//                binding.imgAgreeAll.tag = true
//            }
//            updateImageState(binding.imgCheck1)
//            updateImageState(binding.imgCheck2)
//            updateImageState(binding.imgCheck3)
//        }
//
//        if (view.id == R.id.btnAgreeTerms) {
//            sharedPrefManager.setBool(SharedPrefKeys.TERMS_CONDITION_AGREE_KEY, true)
//            if (BuildConfig.IS_DISTRIBUTOR) {
//                // goto distributor login or search device
//                val isLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.DISTRIBUTOR_TOKEN).isNullOrBlank()
//                val intent = if (isLoggedIn) {
//                    Intent(this, SearchDeviceActivity::class.java)
//                } else {
//                    // goto distributer login
//                    Intent(this, LoginDistributorActivity::class.java)
//                }
//                startActivity(intent)
//                finish()
//            } else {
//                val isLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.KAKAO_TOKEN_KEY)
//                    .isNullOrBlank() || FirebaseAuth.getInstance().currentUser != null
//                if (!isLoggedIn) {
//                    val intent = Intent(this, LoginActivity::class.java)
//                    startActivity(intent)
//                    finish()
//                    return
//                } else {
//                    val intent = Intent(this, SearchDeviceActivity::class.java)
//                    startActivity(intent)
//                    finish()
//                    return
//                }
//            }
//        }
//    }
//
//    private fun updateImageState(imageView: ImageView) {
//        val tag = imageView.tag != null && (imageView.tag as Int) == 1
//        if (!tag) {
//            imageView.setImageResource(R.drawable.check)
//            imageView.tag = 1
//        } else {
//            imageView.setImageResource(R.drawable.uncheck)
//            imageView.tag = 0
//        }
//        Log.i(TAG, "updateImageState: $tag")
//        binding.btnAgreeTerms.isEnabled = checkAllTermsSelected()
//    }
//
//    private val isDetailsVisible: Boolean
//        get() = binding.detailsView.visibility == View.VISIBLE
//
//    companion object {
//        private const val TAG = "TermsAndConditionActivi"
//    }
//}



package com.axon.kisa10.activity // Assuming this is your package name

import android.annotation.SuppressLint
import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.ImageView
import androidx.databinding.DataBindingUtil // Make sure you have data binding enabled in your build.gradle
import com.axon.kisa10.BuildConfig
import com.axon.kisa10.R
import com.axon.kisa10.databinding.ActivityTermsAndConditionBinding // This will be generated from your new XML
import com.axon.kisa10.distributor.LoginDistributorActivity
import com.axon.kisa10.util.SharedPrefKeys
import com.axon.kisa10.util.SharedPrefManager
import com.google.firebase.auth.FirebaseAuth

class TermsAndConditionActivity : BaseActivity() { // Removed View.OnClickListener as we'll set individual listeners
    private lateinit var binding: ActivityTermsAndConditionBinding
    private lateinit var sharedPrefManager: SharedPrefManager

    companion object {
        private const val TAG = "TermsAndConditionActivity"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = DataBindingUtil.setContentView(this, R.layout.activity_terms_and_condition)
        sharedPrefManager = SharedPrefManager.getInstance(this)

        // Setup Toolbar (if your BaseActivity doesn't handle it or if it needs specific title)
        // If layout_toolbar.xml has an ID like 'toolbar_main', you'd access it via binding.toolbar.toolbarMain
        // For simplicity, assuming the toolbar included has elements like a back button handled by BaseActivity
        // and a title text view.
         binding.toolbar.toolbarTitle.text = getString(R.string.terms_title) // Example if your toolbar has tvToolbarTitle
        // binding.toolbar.ivToolbarBack.setOnClickListener { onBackPressedDispatcher.onBackPressed() } // Example

        setupListeners()
        updateAgreeButtonState() // Set initial state of the agree button
    }

    private fun setupListeners() {
        // Listeners for individual term checkboxes
        binding.chkTerms1.setOnCheckedChangeListener { _, _ -> updateAgreeButtonState() }
        binding.chkTerms2.setOnCheckedChangeListener { _, _ -> updateAgreeButtonState() }
        binding.chkTerms3.setOnCheckedChangeListener { _, _ -> updateAgreeButtonState() }

        // Listeners for expand/collapse detail views
        binding.imgTerms1.setOnClickListener {
            toggleDetailView(binding.llDetailViewTerms1, binding.imgTerms1)
        }
        binding.imgTerms2.setOnClickListener {
            toggleDetailView(binding.llDetailViewTerms2, binding.imgTerms2)
        }
        binding.imgTerms3.setOnClickListener {
            toggleDetailView(binding.llDetailViewTerms3, binding.imgTerms3)
        }

        // Listener for the main "Agree" button
        binding.btnAgree.setOnClickListener {
            if (it.isEnabled) { // Redundant check as button state handles this, but good practice
                sharedPrefManager.setBool(SharedPrefKeys.TERMS_CONDITION_AGREE_KEY, true)
                Log.i(TAG, "Terms and Conditions Agreed")
                navigateToNextScreen()
            }
        }
    }

    private fun toggleDetailView(detailView: View, arrowImageView: ImageView) {
        if (detailView.visibility == View.VISIBLE) {
            detailView.visibility = View.GONE
            arrowImageView.setImageResource(R.drawable.ic_arrow_down) // Set to down arrow
        } else {
            detailView.visibility = View.VISIBLE
            arrowImageView.setImageResource(com.tmapmobility.tmap.tmapsdk.ui.R.drawable.ic_arrow_up_gray_16) // Set to up arrow
        }
    }

    private fun updateAgreeButtonState() {
        val allChecked = binding.chkTerms1.isChecked &&
                binding.chkTerms2.isChecked &&
                binding.chkTerms3.isChecked
        binding.btnAgree.isEnabled = allChecked
    }

//    private fun navigateToNextScreen() {
//        if (BuildConfig.IS_DISTRIBUTOR) {
//            val isLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.DISTRIBUTOR_TOKEN).isNullOrBlank()
//            val intent = if (isLoggedIn) {
//                // If already logged in as distributor, likely go to SearchDevice or MainActivity (Distributor)
//                // Depending on your distributor flow, this might need adjustment.
//                // For now, let's assume SearchDeviceActivity is the next logical step if connected.
//                Intent(this, SearchDeviceActivity::class.java) // Or MainDistributorActivity if appropriate
//            } else {
//                Intent(this, LoginDistributorActivity::class.java)
//            }
//            startActivity(intent)
//        } else {
//            // Regular User Flow
//            val isSociallyLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.KAKAO_TOKEN_KEY).isNullOrBlank() ||
//                    FirebaseAuth.getInstance().currentUser != null
//            // Also check for your backend token to see if they are fully registered
//            val isFullyRegistered = !sharedPrefManager.getString(SharedPrefKeys.TOKEN).isNullOrBlank()
//
//            val intent = if (!isSociallyLoggedIn) {
//                Intent(this, LoginActivity::class.java)
//            } else if (!isFullyRegistered) {
//                // User is socially logged in but not yet fully registered with your backend
//                // (e.g., vehicle not yet associated). Usually proceeds to BLE connection then registration.
//                Intent(this, SearchDeviceActivity::class.java)
//            } else {
//                // User is socially logged in AND fully registered with your backend.
//                Intent(this, SearchDeviceActivity::class.java) // Or MainActivity if BLE is already connected
//            }
//            startActivity(intent)
//        }
//        finish()
//    }

    // Presumed to be in SplashActivity.kt or a similar initial routing Activity

//    private fun navigateToNextScreen() {
//        if (BuildConfig.IS_DISTRIBUTOR) {
//            // ----- DISTRIBUTOR / INSTALLER FLOW -----
//            val isLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.DISTRIBUTOR_TOKEN).isNullOrBlank()
//            val intent = if (isLoggedIn) {
//                // If already logged in as distributor, go to BLE search (then Main for distributor)
//                Intent(this, SearchDeviceActivity::class.java)
//            } else {
//                // If not logged in as distributor, go to Distributor Login screen
//                Intent(this, LoginDistributorActivity::class.java)
//            }
//            startActivity(intent)
//        } else {
//            // ----- REGULAR END-USER FLOW (MODIFIED TO SKIP SOCIAL LOGIN UI) -----
//
//            // Check if the user is fully registered with your backend (has your app's specific token)
//            val isFullyRegisteredWithBackend = !sharedPrefManager.getString(SharedPrefKeys.TOKEN).isNullOrBlank()
//
//            val intent: Intent
//            if (isFullyRegisteredWithBackend) {
//                // User has a backend token, meaning they have completed the full registration process before.
//                // Proceed to SearchDeviceActivity (which will then likely lead to MainActivity if BLE connects).
//                // This handles the "Auto Login" scenario.
//                intent = Intent(this, SearchDeviceActivity::class.java)
//            } else {
//                // User does NOT have a backend token. This means they are either:
//                // 1. A brand new user.
//                // 2. A user who only completed social login previously but not the vehicle registration step.
//                // In both cases, after terms, we want them to connect to BLE and then register vehicle details.
//                // So, we go directly to SearchDeviceActivity, bypassing LoginActivity.
//                intent = Intent(this, SearchDeviceActivity::class.java)
//            }
//            startActivity(intent)
//        }
//        finish() // Finish this Splash/Routing activity
//    }
    private fun navigateToNextScreen() {
        if (BuildConfig.IS_DISTRIBUTOR) {
            // ----- DISTRIBUTOR / INSTALLER FLOW -----
            val isLoggedIn = !sharedPrefManager.getString(SharedPrefKeys.DISTRIBUTOR_TOKEN).isNullOrBlank()
            val intent = if (isLoggedIn) {
                // If already logged in as distributor, go to BLE search (then Main for distributor)
                Intent(this, RegisterUserActivity::class.java)
            } else {
                // If not logged in as distributor, go to Distributor Login screen
                Intent(this, LoginDistributorActivity::class.java)
            }
            startActivity(intent)
        } else {
            // ----- REGULAR END-USER FLOW (MODIFIED TO SKIP SOCIAL LOGIN UI) -----

            // Check if the user is fully registered with your backend (has your app's specific token)
            val isFullyRegisteredWithBackend = !sharedPrefManager.getString(SharedPrefKeys.TOKEN).isNullOrBlank()

            val intent: Intent
            if (isFullyRegisteredWithBackend) {
                // User has a backend token, meaning they have completed the full registration process before.
                // Proceed to SearchDeviceActivity (which will then likely lead to MainActivity if BLE connects).
                // This handles the "Auto Login" scenario.
                intent = Intent(this, RegisterUserActivity::class.java)
            } else {
                // User does NOT have a backend token. This means they are either:
                // 1. A brand new user.
                // 2. A user who only completed social login previously but not the vehicle registration step.
                // In both cases, after terms, we want them to connect to BLE and then register vehicle details.
                // So, we go directly to SearchDeviceActivity, bypassing LoginActivity.
                intent = Intent(this, RegisterUserActivity::class.java)
            }
            startActivity(intent)
        }
        finish() // Finish this Splash/Routing activity
    }
}