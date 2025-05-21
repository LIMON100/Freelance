package com.axon.kisa10.activity

import android.app.ProgressDialog
import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.databinding.DataBindingUtil
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.util.SharedPrefKeys
import com.axon.kisa10.util.SharedPrefManager
import com.google.android.gms.auth.api.signin.GoogleSignIn
import com.google.android.gms.auth.api.signin.GoogleSignInOptions
import com.google.android.gms.common.api.ApiException
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.auth.GoogleAuthProvider
import com.google.gson.Gson
import com.kakao.sdk.user.UserApiClient
import com.axon.kisa10.R
import com.axon.kisa10.databinding.ActivityLoginBinding

class LoginActivity : BaseActivity() {
    private val TAG = "LoginActivity"

    private lateinit var sharedPrefManager: SharedPrefManager

    private lateinit var binding : ActivityLoginBinding

    private val RC_SIGNIN = 10

    private lateinit var progressDialog : ProgressDialog

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = DataBindingUtil.setContentView(this, R.layout.activity_login)
        progressDialog = ProgressDialog(this)
        progressDialog.setMessage(getString(R.string.please_wait))
        sharedPrefManager = SharedPrefManager.getInstance(this)
        binding.btnKakaoLogin.setOnClickListener {
            progressDialog.show()
            UserApiClient.instance.loginWithKakaoAccount(this@LoginActivity) { token, error ->
                progressDialog.hide()
                if (error != null) {
                    Toast.makeText(this, error.message, Toast.LENGTH_SHORT).show()
                    return@loginWithKakaoAccount
                } else {
                    val gson = Gson()
                    val tokenString = gson.toJson(token)
                    sharedPrefManager.setString(SharedPrefKeys.KAKAO_TOKEN_KEY, tokenString)
                    Log.i(TAG, tokenString)
                    gotoMainActivity()
                }
            }
        }

        binding.btnGoogleLogin.setOnClickListener {
            val gso = GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_SIGN_IN)
                .requestIdToken(AppConstants.GOOGLE_WEB_CLIENT_ID)
                .requestEmail()
                .build()

            val googleSignInClient = GoogleSignIn.getClient(this, gso)
            startActivityForResult(googleSignInClient.signInIntent, RC_SIGNIN)
            progressDialog.show()
        }
    }

    private fun gotoMainActivity() {
        val intent1 = Intent(this, SearchDeviceActivity::class.java)
        startActivity(intent1)
        finish()
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
                    Toast.makeText(this, "Login Failed : ID token null.",Toast.LENGTH_SHORT).show()
                }
            } catch (e: ApiException) {
                Toast.makeText(this, "Login Failed : ${e.message}",Toast.LENGTH_SHORT).show()
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
                    gotoMainActivity()
                } else {
                    Toast.makeText(this, task.exception?.message ?: "AddOnCompleteListener : Failure",Toast.LENGTH_SHORT).show()
                    Log.e(TAG, task.exception?.message ?: "")
                    task.exception?.printStackTrace()
                }
            }
    }
}