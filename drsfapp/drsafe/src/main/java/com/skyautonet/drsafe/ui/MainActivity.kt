package com.skyautonet.drsafe.ui

import android.content.Intent
import android.os.Bundle
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import androidx.databinding.DataBindingUtil
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import androidx.navigation.NavController
import androidx.navigation.NavDestination
import androidx.navigation.fragment.NavHostFragment
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.ActivityMainBinding
import com.skyautonet.drsafe.service.DrSafeBleService
import com.skyautonet.drsafe.ui.search.SearchDeviceActivity
import com.skyautonet.drsafe.util.Menu
import com.skyautonet.drsafe.util.start

class MainActivity : BaseActivity() {

    private val TAG = "MainActivity"
    private lateinit var binding : ActivityMainBinding
    private var currentSelection = Menu.HOME

    private lateinit var navigationController : NavController

    private val localBroadcastManager by lazy {
        LocalBroadcastManager.getInstance(this)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = DataBindingUtil.setContentView(this,R.layout.activity_main)
        val navHostFragment = supportFragmentManager.findFragmentById(R.id.fragmentContainerView) as NavHostFragment
        navigationController = navHostFragment.navController
        setupBottomNavigation()
        setupNavigationListener()
    }

    private fun setupBottomNavigation() {

        binding.bottomNavigationView.myDrivingMenu.setOnClickListener {
            navigationController.navigate(R.id.action_global_dashboardFragment)
        }

        binding.bottomNavigationView.tripsMenu.setOnClickListener {
            navigationController.navigate(R.id.tripListFragment)
        }

        binding.bottomNavigationView.eventMenu.setOnClickListener {
            navigationController.navigate(R.id.action_global_eventListFragment)
        }

        binding.bottomNavigationView.ecallMenu.setOnClickListener {
            navigationController.navigate(R.id.action_global_ecallFragment)
        }

        binding.bottomNavigationView.settingMenu.setOnClickListener {
            navigationController.navigate(R.id.action_global_settingsFragment)
        }
    }

    private fun setMenu(menu : Menu) {
        unselectMenu(currentSelection)
        selectMenu(menu)
        currentSelection = menu
    }

    private fun selectMenu(menu: Menu) {
        when(menu) {
            Menu.HOME -> {
                binding.bottomNavigationView.imgMyDriving.setImageResource(R.drawable.my_driving_selected)
            }
            Menu.TRIPS -> {
                binding.bottomNavigationView.imgTrips.setImageResource(R.drawable.my_car_selected)
            }
            Menu.EVENT -> {
                binding.bottomNavigationView.imgEvent.setImageResource(R.drawable.dvr_selected)
            }
            Menu.ECALL -> {
                binding.bottomNavigationView.imgEcall.setImageResource(R.drawable.videos_selected)
            }
            Menu.SETTING -> {
                binding.bottomNavigationView.imgSetting.setImageResource(R.drawable.settings_selected)
            }
        }
    }

    private fun unselectMenu(menu: Menu) {
        when(menu) {
            Menu.HOME -> {
                binding.bottomNavigationView.imgMyDriving.setImageResource(R.drawable.my_driving_unselected)
            }
            Menu.TRIPS -> {
                binding.bottomNavigationView.imgTrips.setImageResource(R.drawable.my_driving_unselected)
            }
            Menu.EVENT -> {
                binding.bottomNavigationView.imgEvent.setImageResource(R.drawable.dvr_unselected)
            }
            Menu.ECALL -> {
                binding.bottomNavigationView.imgEcall.setImageResource(R.drawable.videos_unselected)
            }
            Menu.SETTING -> {
                binding.bottomNavigationView.imgSetting.setImageResource(R.drawable.settings_unselected)
            }
        }
    }

    private fun setupNavigationListener() {
        navigationController.addOnDestinationChangedListener { _, destination, _ ->
            Log.i(TAG, "onDestinationChanged: $destination")
            if (destination.label != currentSelection.getLabel()) {
                setMenu(Menu.getMenuForLabel(destination.label.toString()))
            }
        }
    }

    fun updateBottomNavigationVisibility(visibility: Int) {
        binding.bottomNavigationView.root.visibility = visibility
    }

    override fun onDestroy() {
        super.onDestroy()
        localBroadcastManager.sendBroadcast(Intent(DrSafeBleService.ACTION_START_BG_SCAN))
    }

    override fun onDeviceDisconnected() {
        super.onDeviceDisconnected()
        start(SearchDeviceActivity::class.java)
    }
}