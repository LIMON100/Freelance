package com.skyautonet.drsafe.ui.login

import android.app.ProgressDialog
import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.navigation.fragment.findNavController
import com.google.android.gms.auth.api.signin.GoogleSignIn
import com.google.android.gms.auth.api.signin.GoogleSignInOptions
import com.google.android.gms.common.api.ApiException
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.auth.GoogleAuthProvider
import com.google.gson.Gson
import com.kakao.sdk.user.UserApiClient
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentLoginMainBinding
import com.skyautonet.drsafe.ui.BaseFragment
import com.skyautonet.drsafe.util.AppConstants
import com.skyautonet.drsafe.util.SharedPreferenceUtil
import kotlin.math.truncate

/**
 * Created by Hussain on 01/08/24.
 */
@Suppress("DEPRECATION")
class LoginMainFragment : BaseFragment() {

    private lateinit var binding : FragmentLoginMainBinding

    private val RC_SIGNIN = 10

    private val auth by lazy {
        FirebaseAuth.getInstance()
    }

    private val sharedPrefManager by lazy {
        SharedPreferenceUtil.getInstance(requireContext())
    }

    private lateinit var progressDialog : ProgressDialog

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentLoginMainBinding.inflate(inflater, container, false)
        progressDialog = ProgressDialog(requireContext())
        progressDialog.setMessage(getString(R.string.please_wait))
        setupToolbar(binding.toolbar)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.loginWithKakao.root.setOnClickListener {
            progressDialog.show()
            UserApiClient.instance.loginWithKakaoAccount(requireContext()) { token, error ->
                progressDialog.hide()
                if (error != null) {
                    Toast.makeText(requireContext(), error.message, Toast.LENGTH_SHORT).show()
                    return@loginWithKakaoAccount
                } else {
                    val gson = Gson()
                    val tokenString = gson.toJson(token)
                    sharedPrefManager.setString(AppConstants.KAKAO_TOKEN_KEY, tokenString)
                    findNavController().navigate(R.id.action_loginMainFragment_to_loginEmailFragment)
                }
            }
        }
        binding.loginWithGoogle.root.setOnClickListener {
            val gso = GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_SIGN_IN)
                .requestIdToken(AppConstants.default_web_client_id)
                .requestEmail()
                .build()

            val googleSignInClient = GoogleSignIn.getClient(requireActivity(), gso)
            startActivityForResult(googleSignInClient.signInIntent, RC_SIGNIN)
            progressDialog.show()
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)

        if (requestCode == RC_SIGNIN) {
            val task = GoogleSignIn.getSignedInAccountFromIntent(data)
            try {
                val account = task.getResult(ApiException::class.java)
                if (account?.idToken != null) {
                    firebaseAuthWithGoogle(account.idToken!!)
                } else {
                    progressDialog.hide()
                    Toast.makeText(requireContext(), "Login Failed : ID token null.", Toast.LENGTH_SHORT).show()
                }
            } catch (e: ApiException) {
                Toast.makeText(requireContext(), "Login Failed : ${e.message}", Toast.LENGTH_SHORT).show()
                progressDialog.hide()
                e.printStackTrace()
            }
        }
    }

    private fun firebaseAuthWithGoogle(idToken: String) {
        val auth = FirebaseAuth.getInstance()
        val credentials = GoogleAuthProvider.getCredential(idToken, null)
        auth.signInWithCredential(credentials)
            .addOnCompleteListener { task ->
                progressDialog.hide()
                if (task.isSuccessful) {
//                    Toast.makeText(requireContext(), "Login Success : ${task.result.user?.displayName}", Toast.LENGTH_SHORT).show()
//                    gotoMainActivity()
                    sharedPrefManager.setString(AppConstants.GOOGLE_TOKEN_KEY, idToken)
                    findNavController().navigate(R.id.action_loginMainFragment_to_loginEmailFragment)
                } else {
                    Toast.makeText(requireContext(), task.exception?.message ?: "AddOnCompleteListener : Failure",Toast.LENGTH_SHORT).show()
//                    Log.e(TAG, task.exception?.message ?: "")
                    task.exception?.printStackTrace()
                }
            }
    }

    override fun getStatusBarColor(): Int {
        return getWhiteStatusBarColor()
    }

    override fun getToolbarTitle(): String {
        return getString(R.string.dr_safe)
    }

    override fun showToolbarImage(): Boolean {
        return true
    }

    override fun shouldShowBottomNavigation(): Boolean {
        return false
    }

    override fun showHeaderLogo(): Boolean {
        return true
    }
}