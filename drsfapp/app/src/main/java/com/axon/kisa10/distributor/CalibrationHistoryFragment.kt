package com.axon.kisa10.distributor

import android.content.pm.ActivityInfo
import android.os.Bundle
import android.view.ContextThemeWrapper
import android.view.Gravity
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.LinearLayout
import android.widget.TextView
import androidx.fragment.app.Fragment
import com.axon.kisa10.data.CalibrationHistoryDb
import com.axon.kisa10.model.distributor.calibration.CalibrationHistory
import com.kisa10.R
import com.kisa10.databinding.FragmentCalibHistoryBinding
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext


class CalibrationHistoryFragment : Fragment() {

    private lateinit var binding : FragmentCalibHistoryBinding

    private val calibDb by lazy {
        CalibrationHistoryDb.getDb(requireContext().applicationContext)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        requireActivity().requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentCalibHistoryBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        GlobalScope.launch(Dispatchers.IO) {
            val calibrations = calibDb.calibrationDao().getAll()
            withContext(Dispatchers.Main) {
                setCalibrationHistory(calibrations)
            }
        }

    }

    private fun setCalibrationHistory(calibrations: List<CalibrationHistory>) {
        val table = binding.tbCalibration

        calibrations.forEach { data ->
            val linearLayout = LinearLayout(requireContext())

            val date = getTextView(data.date)

            val time = getTextView(data.time)

            val license = getTextView(data.licensePlate)

            val hMin = getTextView(data.caliMinHigh.toString())

            val lMin = getTextView(data.caliMinLow.toString())

            val hMax = getTextView(data.caliMaxHigh.toString())

            val lMax = getTextView(data.caliMinHigh.toString())

            val hBto = getTextView(data.highBTO.toString())

            val lBto = getTextView(data.lowBTO.toString())

            linearLayout.addView(date)
            linearLayout.addView(time)
            linearLayout.addView(license)
            linearLayout.addView(hMin)
            linearLayout.addView(lMin)
            linearLayout.addView(hMax)
            linearLayout.addView(lMax)
            linearLayout.addView(hBto)
            linearLayout.addView(lBto)

            linearLayout.invalidate()
            table.addView(linearLayout)

        }

        table.invalidate()
    }

    private fun getTextView(text : String) : TextView {
        val textView = TextView(ContextThemeWrapper(requireContext(), R.style.text_style_table_col),null,R.style.text_style_table_col)
        textView.text = text
        val params = LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f)
//        params.setMargins(margin,margin, margin, margin)
        textView.layoutParams = params
        textView.gravity = Gravity.CENTER
        textView.setTextAppearance(R.style.text_style_table_col)
        return textView
    }

    override fun onDestroyView() {
        super.onDestroyView()
        requireActivity().requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
    }
}