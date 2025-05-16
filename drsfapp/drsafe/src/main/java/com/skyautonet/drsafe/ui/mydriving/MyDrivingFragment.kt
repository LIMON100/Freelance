package com.skyautonet.drsafe.ui.mydriving

import DrivingScoreFragment
import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.annotation.RequiresApi
import androidx.fragment.app.FragmentManager
import com.skyautonet.drsafe.BuildConfig
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentMyDrivingBinding
import com.skyautonet.drsafe.ui.BaseFragment
import com.skyautonet.drsafe.ui.mydriving.dashboard.DashboardFragment
import com.skyautonet.drsafe.ui.mydriving.ecall.EcallFragment
import com.skyautonet.drsafe.ui.mydriving.trips.TripsFragment

/**
 * Created by Hussain on 24/09/24.
 */
@RequiresApi(Build.VERSION_CODES.O)
class MyDrivingFragment : BaseFragment() {

    private lateinit var binding : FragmentMyDrivingBinding

    private lateinit var fragmentManager: FragmentManager

    private var currentTag = DASHBOARD_FRAGMENT_TAG


    companion object {
        const val DASHBOARD_FRAGMENT_TAG = "DASHBOARD_FRAGMENT_TAG"
        const val TRIPS_FRAGMENT_TAG = "TRIPS_FRAGMENT_TAG"
        const val DRIVING_SCORE_FRAGMENT_TAG = "DRIVING_SCORE_FRAGMENT_TAG"
        const val ECALL_FRAGMENT_TAG = "ECALL_FRAGMENT_TAG"
    }

    private val dashboardFragment by lazy {
        DashboardFragment()
    }

    private val tripsFragment by lazy {
        TripsFragment()
    }

    private val drivingScoreFragment by lazy {
        DrivingScoreFragment()
    }

    private val ecallFragment by lazy {
        EcallFragment()
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentMyDrivingBinding.inflate(inflater, container, false)
        setupToolbar(binding.toolbar)
        fragmentManager = childFragmentManager
        return binding.root
    }



    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        if (BuildConfig.DEBUG) {
            binding.toolbar.toolbarTitle.setOnClickListener {
                // Add debug-specific actions here
//                findNavController().navigate(R.id.action_mydrive_to_Data_Collection_Test)
//                val intent = Intent(requireContext(), StartActivity::class.java)
//                startActivity(intent)
            }
        }

        fragmentManager.beginTransaction().replace(R.id.myDrivingFragmentContainer, DashboardFragment(), DASHBOARD_FRAGMENT_TAG).commit()
        binding.tbMyDriving.rgMyDrivingTab.setOnCheckedChangeListener { _, checkedId ->
            when(checkedId) {
                R.id.rdDashboard -> {
                    val fragment = fragmentManager.findFragmentByTag(DASHBOARD_FRAGMENT_TAG)
                    if (fragment != null) {
                        fragmentManager.beginTransaction().replace(R.id.myDrivingFragmentContainer, fragment, DASHBOARD_FRAGMENT_TAG).commit()
                    } else {
                        fragmentManager.beginTransaction().replace(R.id.myDrivingFragmentContainer, dashboardFragment, DASHBOARD_FRAGMENT_TAG).commit()
                    }
                    currentTag = DASHBOARD_FRAGMENT_TAG
                    setupToolbar(binding.toolbar)
                    binding.tbMyDriving.tbMyDrivingRoot.setBackgroundColor(getToolbarBackgroundColor())
                }
                R.id.rdTrips -> {
                    val fragment = fragmentManager.findFragmentByTag(TRIPS_FRAGMENT_TAG)
                    if (fragment != null) {
                        fragmentManager.beginTransaction().replace(R.id.myDrivingFragmentContainer, fragment, TRIPS_FRAGMENT_TAG).commit()
                    } else {
                        fragmentManager.beginTransaction().replace(R.id.myDrivingFragmentContainer, tripsFragment, TRIPS_FRAGMENT_TAG).commit()
                    }
                    currentTag = TRIPS_FRAGMENT_TAG
                    setupToolbar(binding.toolbar)
                    binding.tbMyDriving.tbMyDrivingRoot.setBackgroundColor(getToolbarBackgroundColor())
                }
                R.id.rdDrivingScore -> {
                    val fragment = fragmentManager.findFragmentByTag(DRIVING_SCORE_FRAGMENT_TAG)
                    if (fragment != null) {
                        fragmentManager.beginTransaction().replace(R.id.myDrivingFragmentContainer, fragment, DRIVING_SCORE_FRAGMENT_TAG).commit()
                    } else {
                        fragmentManager.beginTransaction().replace(R.id.myDrivingFragmentContainer, drivingScoreFragment, DRIVING_SCORE_FRAGMENT_TAG).commit()
                    }
                    currentTag = DRIVING_SCORE_FRAGMENT_TAG
                    setupToolbar(binding.toolbar)
                    binding.tbMyDriving.tbMyDrivingRoot.setBackgroundColor(getToolbarBackgroundColor())
                }
                R.id.rdECall -> {
                    val fragment = fragmentManager.findFragmentByTag(ECALL_FRAGMENT_TAG)
                    if (fragment != null) {
                        fragmentManager.beginTransaction().replace(R.id.myDrivingFragmentContainer, fragment, ECALL_FRAGMENT_TAG).commit()
                    } else {
                        fragmentManager.beginTransaction().replace(R.id.myDrivingFragmentContainer, ecallFragment, ECALL_FRAGMENT_TAG).commit()
                    }
                    currentTag = ECALL_FRAGMENT_TAG
                    setupToolbar(binding.toolbar)
                    binding.tbMyDriving.tbMyDrivingRoot.setBackgroundColor(getToolbarBackgroundColor())
                }
            }
        }
    }

    private fun getToolbarBackgroundColor(): Int {
        return when (currentTag) {
            DASHBOARD_FRAGMENT_TAG -> {
                resources.getColor(R.color.colorPrimary, null)
            }

            else -> {
                resources.getColor(R.color.colorSurface, null)
            }
        }
    }

    override fun getStatusBarColor(): Int {
        return when (currentTag) {
            DASHBOARD_FRAGMENT_TAG -> {
                resources.getColor(R.color.colorPrimary, null)
            }

            else -> {
                resources.getColor(R.color.colorSurface, null)
            }
        }
    }

    override fun getToolbarTitle(): String {
        return resources.getString(R.string.my_driving)
    }

    override fun showToolbarImage(): Boolean {
        return false
    }

    fun getToolbarTextColor(): Int {
        when (currentTag) {
            DASHBOARD_FRAGMENT_TAG -> {
                return resources.getColor(R.color.white, null)
            }
            else -> {
                return resources.getColor(R.color.black, null)
            }
        }
    }
}