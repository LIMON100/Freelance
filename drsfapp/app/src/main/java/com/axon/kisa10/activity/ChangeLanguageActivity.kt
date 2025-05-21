package com.axon.kisa10.activity

import android.content.Intent
import android.content.res.Configuration
import android.os.Bundle
import android.widget.FrameLayout
import android.widget.Toast
import com.axon.kisa10.fragment.ChangeLanguageFragment
import com.axon.kisa10.R

/**
 * Created by Hussain on 28/06/24.
 */
class ChangeLanguageActivity : BaseActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (savedInstanceState != null) {
            val stateChanged = savedInstanceState.getBoolean(STATE_CHANGE)
            if (stateChanged) {
                val intent = Intent(this,LoginActivity::class.java)
                startActivity(intent)
                finish()
            }
        }
        setContentView(R.layout.activity_change_language)
        val fragment = ChangeLanguageFragment()

        val transaction = supportFragmentManager.beginTransaction()
        transaction.add(R.id.frame_Layout, fragment, MainActivity.ACTION_CHANGE_LANGUAGE)
        transaction.commit()
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        if (isChangingConfigurations) {
            outState.putBoolean(STATE_CHANGE, true)
        }
    }

    companion object {
        const val STATE_CHANGE = "STATE_CHANGING"
    }
}