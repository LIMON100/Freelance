package com.skyautonet.drsafe.ui.setting.register

import android.graphics.drawable.Drawable
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.fragment.app.activityViewModels
import androidx.navigation.fragment.findNavController
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentRegistervehicleBinding
import com.skyautonet.drsafe.ui.BaseFragment

/**
 * Created by Hussain on 31/07/24.
 */
class RegisterVehicleFragment : BaseFragment() {

    private lateinit var binding : FragmentRegistervehicleBinding
    private val viewModel by activityViewModels<RegisterViewModel>()

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentRegistervehicleBinding.inflate(inflater, container, false)
        setupToolbar(binding.toolbar)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
//        binding.llBrand.setOnClickListener {
//            findNavController().navigate(R.id.action_registerVehicleFragment_to_brandFragment)
//        }
//
//        binding.llModel.setOnClickListener {
//            findNavController().navigate(R.id.action_registerVehicleFragment_to_modelFragment)
//        }
//
//        binding.llFuelType.setOnClickListener {
//            findNavController().navigate(R.id.action_registerVehicleFragment_to_fuelTypeFragment)
//        }
//
//        binding.llYearOfManufacture.setOnClickListener {
//            findNavController().navigate(R.id.action_registerVehicleFragment_to_yearFragment)
//        }

        binding.btnConfirm.setOnClickListener {
            Toast.makeText(requireContext(), getString(R.string.vehicle_saved), Toast.LENGTH_SHORT).show()
            onBackIconPressed()
        }

        viewModel.selectedBrand.observe(viewLifecycleOwner) { brand ->
            if (!brand.isNullOrEmpty()) {
                binding.tvBrand.text = brand
            }
        }

        viewModel.selectedModel.observe(viewLifecycleOwner) { model ->
            if (!model.isNullOrEmpty()) {
                binding.tvModel.text = model
            }
        }

        viewModel.selectedFuelType.observe(viewLifecycleOwner) { fuelType ->
            if (!fuelType.isNullOrEmpty()) {
                binding.tvFuelType.text = fuelType
            }
        }

        viewModel.selectedYear.observe(viewLifecycleOwner) { year ->
            if (year != null) {
                binding.tvYear.text = year.toString()
            }
        }
    }

    override fun getStatusBarColor(): Int {
        return getWhiteStatusBarColor()
    }

    override fun getToolbarTitle(): String {
        return getString(R.string.register_vehicle)
    }

    override fun showToolbarImage(): Boolean {
        return true
    }

    override fun shouldShowBottomNavigation(): Boolean {
        return false
    }
}