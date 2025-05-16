package com.skyautonet.drsafe.ui

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.graphics.drawable.Drawable
import android.os.Bundle
import android.view.View
import androidx.core.content.res.ResourcesCompat
import androidx.fragment.app.Fragment
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import androidx.navigation.fragment.findNavController
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.LayoutToolbarBinding
import com.skyautonet.drsafe.util.AppConstants

/**
 * Created by Hussain on 29/07/24.
 */
open class BaseFragment : Fragment() {

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(requireContext())
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        localBroadcastManager.registerReceiver(bleReceiver, AppConstants.makeIntentFilter())
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        requireActivity().window.statusBarColor = getStatusBarColor()
        val bottomNavigationVisibility = if (shouldShowBottomNavigation()) {
            View.VISIBLE
        } else {
            View.GONE
        }
        val activity = requireActivity() as? MainActivity
        activity?.updateBottomNavigationVisibility(bottomNavigationVisibility)
    }

    protected fun setupToolbar(layout : LayoutToolbarBinding) {
        layout.apply {
            if (showToolbarImage()) {
                imgToolbar.visibility = View.VISIBLE
            } else {
                imgToolbar.visibility = View.INVISIBLE
            }

            toolbarTitle.text = getToolbarTitle()

            imgToolbar.setOnClickListener {
                onBackIconPressed()
            }

            if (showHeaderLogo()) {
                imgHeaderLogo.visibility = View.VISIBLE
            }
        }
    }

    open fun getStatusBarColor() : Int {
        return ResourcesCompat.getColor(resources, R.color.colorPrimary, null)
    }

    open fun getToolbarTitle() : String {
        return ""
    }

    open fun getToolbarImage() : Drawable? {
        return ResourcesCompat.getDrawable(resources, R.drawable.ic_back, null)
    }

    open fun showToolbarImage() : Boolean = false

    protected open fun onBackIconPressed() {
        findNavController().navigateUp()
    }

    protected open fun shouldShowBottomNavigation(): Boolean {
        return true
    }

    protected fun getWhiteStatusBarColor() = ResourcesCompat.getColor(resources, R.color.white,null)

    protected fun getPrimaryStatusBarColor() = ResourcesCompat.getColor(resources, R.color.colorPrimary, null)

    protected open fun showHeaderLogo() = false


    private val bleReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (intent.action == AppConstants.ACTION_DEVICE_CONNECTION_COMPLETE) {
                onDeviceConnected()
            }

            if (intent.action == AppConstants.ACTION_DEVICE_DISCONNECTED) {
                onDeviceDisconnected()
            }

            if (intent.action == AppConstants.ACTION_CHARACTERISTIC_CHANGED) {
                val data = intent.getStringExtra("data")
                val rawData = intent.getByteArrayExtra(AppConstants.RAW_DATA)
                onDataReceived(rawData, data)
            }
        }
    }

    protected open fun onDataReceived(rawData: ByteArray?, data: String?) {

    }

    protected open fun onDeviceDisconnected() {

    }

    protected open fun onDeviceConnected() {

    }

    override fun onDestroy() {
        super.onDestroy()
        localBroadcastManager.unregisterReceiver(bleReceiver)
    }
}