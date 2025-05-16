package com.axon.kisa10.activity

import android.content.Context
import androidx.appcompat.app.AppCompatActivity
import com.axon.kisa10.util.LocaleHelper

/**
 * Created by Hussain on 28/06/24.
 */
open class BaseActivity : AppCompatActivity() {

    override fun attachBaseContext(newBase: Context) {
        super.attachBaseContext(LocaleHelper.onAttach(newBase,"en"))
    }
}