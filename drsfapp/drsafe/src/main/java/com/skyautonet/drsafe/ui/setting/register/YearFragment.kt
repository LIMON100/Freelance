package com.skyautonet.drsafe.ui.setting.register

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.fragment.app.activityViewModels
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentSelectableBinding
import com.skyautonet.drsafe.ui.BaseFragment
import com.skyautonet.drsafe.ui.setting.adapter.SelectableAdapter
import com.skyautonet.drsafe.util.forEachVisibleHolder
import java.util.Calendar
import java.util.Date

/**
 * Created by Hussain on 31/07/24.
 */
class YearFragment : BaseFragment() {

    private lateinit var binding: FragmentSelectableBinding

    private val viewModel by activityViewModels<RegisterViewModel>()

    private var selectYear = ""

    private val selectableAdapter = SelectableAdapter { year ->
        selectYear = year
        viewModel.selectedYear(year.toInt())
        binding.rvBrand.forEachVisibleHolder<SelectableAdapter.SelectableItemViewHolder> { holder ->
            holder.unselect()
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentSelectableBinding.inflate(inflater, container, false)
        setupToolbar(binding.toolbar)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        binding.rvBrand.adapter = selectableAdapter
        val years = (1999..Calendar.getInstance().get(Calendar.YEAR)).map { it.toString() }.sortedDescending()
        selectableAdapter.submitBrandList(years)
        binding.btnConfirm.setOnClickListener {
            if (selectYear.isEmpty()) {
                Toast.makeText(requireContext(), getString(R.string.please_select_a_year), Toast.LENGTH_SHORT).show()
            } else {
                onBackIconPressed()
            }
        }
    }
    override fun getStatusBarColor(): Int {
        return getWhiteStatusBarColor()
    }

    override fun getToolbarTitle(): String {
        return getString(R.string.year_of_manufacture)
    }

    override fun showToolbarImage(): Boolean {
        return true
    }

    override fun shouldShowBottomNavigation(): Boolean {
        return false
    }
}