package com.skyautonet.drsafe.ui.mydriving.trips

import android.content.Context
import android.content.SharedPreferences
import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.annotation.RequiresApi
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProvider
import com.github.mikephil.charting.charts.LineChart
import com.github.mikephil.charting.charts.PieChart
import com.github.mikephil.charting.components.AxisBase
import com.github.mikephil.charting.components.Description
import com.github.mikephil.charting.components.XAxis
import com.github.mikephil.charting.components.YAxis
import com.github.mikephil.charting.data.Entry
import com.github.mikephil.charting.data.LineData
import com.github.mikephil.charting.data.LineDataSet
import com.github.mikephil.charting.data.PieData
import com.github.mikephil.charting.data.PieDataSet
import com.github.mikephil.charting.data.PieEntry
import com.github.mikephil.charting.formatter.ValueFormatter
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentTripsBinding
import com.skyautonet.drsafe.ui.BaseFragment
import com.skyautonet.drsafe.ui.viewmodel.DashboardViewModel
import com.skyautonet.drsafe.ui.viewmodel.EventDashboardViewModel
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter

/**
 * Created by Hussain on 30/09/24.
 */
@RequiresApi(Build.VERSION_CODES.O)
class TripsFragment : BaseFragment() {

    private lateinit var binding: FragmentTripsBinding
    private lateinit var dashboardViewModel: DashboardViewModel
    private lateinit var eventDashboardViewModel: EventDashboardViewModel // Added EventDashboardViewModel

    private val speedData = mutableListOf<Pair<LocalDateTime, Double>>()
    private val rpmData = mutableListOf<Pair<LocalDateTime, Int>>()
    private lateinit var sharedPreferences: SharedPreferences

    private val tripsAdapter by lazy {
        TripsAdapter(requireContext(), sharedPreferences)
    }
    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentTripsBinding.inflate(inflater, container, false)
        return binding.root
    }

    @RequiresApi(Build.VERSION_CODES.O)
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        dashboardViewModel = ViewModelProvider(requireActivity()).get(DashboardViewModel::class.java)
        eventDashboardViewModel = ViewModelProvider(requireActivity()).get(EventDashboardViewModel::class.java) // Added ViewModel
        sharedPreferences = requireContext().getSharedPreferences("com.skyautonet.drsafe", Context.MODE_PRIVATE)

        val tvPie: TextView = view.findViewById(R.id.tvPie)
        val tvSuddenDip: TextView = view.findViewById(R.id.tvSuddenDip)
        val tvRapidAcc: TextView = view.findViewById(R.id.tvRapidAcc)
        val tvRapidDec: TextView = view.findViewById(R.id.tvRapidDec)
        val tvSuddenStop: TextView = view.findViewById(R.id.tvSuddenStop)
        val tvCourseChange: TextView = view.findViewById(R.id.tvCourseChange)
        val tvSharpTurns: TextView = view.findViewById(R.id.tvSharpTurns)
        val tvSuddenTurns: TextView = view.findViewById(R.id.tvSuddenTurns)
        val tvLongDrive: TextView = view.findViewById(R.id.tvLongDrive)


        eventDashboardViewModel.fastAcceleration.observe(viewLifecycleOwner) { count ->
            tvRapidAcc.text = count.toString()

        }

        eventDashboardViewModel.suddenDeparture.observe(viewLifecycleOwner) { count ->
            tvSuddenDip.text = count.toString()
        }

        eventDashboardViewModel.rapidDeceleration.observe(viewLifecycleOwner) { count ->
            tvRapidDec.text = count.toString()

        }

        eventDashboardViewModel.suddenStop.observe(viewLifecycleOwner) { count ->
            tvSuddenStop.text = count.toString()

        }

        eventDashboardViewModel.radicalCourseChange.observe(viewLifecycleOwner) { count ->
            tvCourseChange.text = count.toString()

        }

        eventDashboardViewModel.overtakingSpeed.observe(viewLifecycleOwner) { count ->
            tvPie.text = count.toString()        }

        eventDashboardViewModel.sharpTurns.observe(viewLifecycleOwner) { count ->
            tvSharpTurns.text = count.toString()

        }

        eventDashboardViewModel.suddenUTurn.observe(viewLifecycleOwner) { count ->
            tvSuddenTurns.text = count.toString()

        }

        eventDashboardViewModel.continuousOperation.observe(viewLifecycleOwner) { count ->
            tvLongDrive.text = count.toString()

        }
        dashboardViewModel.speedData.observe(viewLifecycleOwner, Observer { data ->
            speedData.clear()
            speedData.addAll(data) // Ensure it holds Pair<LocalDateTime, Double>
            setupChart(binding.lineChartSpeed, binding.lineChartRPM)
        })
        dashboardViewModel.rpmData.observe(viewLifecycleOwner, Observer { rpm ->
            rpmData.clear()
            rpmData.addAll(rpm)  // Add updated data to the list
            println("Updated rpmData: $rpmData") // Print the updated rpmData
            setupChart(binding.lineChartSpeed, binding.lineChartRPM)  // Reconfigure charts with new data
        })

        setupPieChart(binding.otPieChart)
        setupChart(binding.lineChartSpeed, binding.lineChartRPM)
        binding.rvTrips.adapter = tripsAdapter

        Log.d("TripsFragment", "RecyclerView Adapter set")
        //tripsAdapter.reinitialize()
        tripsAdapter.observeCsvFiles()
        tripsAdapter.printTrips()
        //tripsAdapter.clearData()

    }



    private fun setupPieChart(pieChart: PieChart) {
        val entries = listOf(
            PieEntry(20f, ""),
            PieEntry(34f, ""),
            PieEntry(48f, "")
        )

        val dataSet = PieDataSet(entries, "").apply {
            colors = listOf(
                Color.rgb(73, 183, 85),
                Color.rgb(222, 74, 69),
                Color.rgb(248, 181, 48)
            )
            setDrawValues(false) // Hide value labels
        }

        val data = PieData(dataSet)
        pieChart.data = data

        pieChart.description.isEnabled = false // Hide description
        pieChart.legend.isEnabled = false // Hide legend
        pieChart.isDrawHoleEnabled = true
        pieChart.holeRadius = 75f

        pieChart.invalidate() // Refresh the chart
    }

    @RequiresApi(Build.VERSION_CODES.O)
    private fun configureChart(chart: LineChart, isTimeBased: Boolean = false, data: List<Pair<LocalDateTime, Double>> = listOf(), isRpmChart: Boolean) {
        chart.apply {
            xAxis.apply {
                textSize = 12f
                position = XAxis.XAxisPosition.BOTTOM
                textColor = Color.BLACK
                isEnabled = true
                disableGridDashedLine()
                setDrawAxisLine(false)
                setDrawGridLines(false)
                setAvoidFirstLastClipping(true)

                if (isTimeBased) {
                    valueFormatter = TimeAxisValueFormatter(data) // Use time formatter for the X-axis
                    labelRotationAngle = 45f
                    setLabelCount(6, true)
                }
            }

            description = Description().apply {
                isEnabled = false
            }

            axisLeft.apply {
                removeAllLimitLines()
                setPosition(YAxis.YAxisLabelPosition.OUTSIDE_CHART)
                textColor = Color.BLACK
                setDrawLimitLinesBehindData(false)
                setDrawGridLines(false)
                setDrawAxisLine(false)

                // Calculate the min and max values for dynamic range setting
                val minValue = if (isRpmChart) rpmData.minOfOrNull { it.second }?.toFloat() ?: 0f else speedData.minOfOrNull { it.second }?.toFloat() ?: 0f
                val maxValue = if (isRpmChart) rpmData.maxOfOrNull { it.second }?.toFloat() ?: 0f else speedData.maxOfOrNull { it.second }?.toFloat() ?: 120f

                // Add some padding to the axis maximum and minimum for better visibility
                axisMaximum = maxValue * 1.1f  // 10% padding above the max value
                axisMinimum = minValue * 0.9f  // 10% padding below the min value
            }

            axisRight.isEnabled = false

            legend.isEnabled = false
            setExtraOffsets(5f, 5f, 5f, 15f)
            invalidate()
        }
    }
    //@RequiresApi(Build.VERSION_CODES.O)
    private fun setupChart(speedChart: LineChart, rpmChart: LineChart) {
        val sortedSpeedData = speedData.sortedBy { it.first }
        val sortedRpmData = rpmData.sortedBy { it.first }
        println("Sorted Rpm Data is : $sortedRpmData")
        // Convert speedData to a list of Entry objects
        val speedEntries = sortedSpeedData.mapIndexed { index, data ->
            Entry(index.toFloat(), data.second.toFloat()) // Use second for y-axis
        }

        // Convert rpmData to a list of Entry objects
        val rpmEntries = sortedRpmData.mapIndexed { index, data ->
            Entry(index.toFloat(), data.second.toFloat())
        }

        // Create LineDataSet for speed
        val speedLineData = LineDataSet(speedEntries, "Speed").apply {
            color = Color.RED
            setDrawCircles(false)
            valueTextSize = 12f
            lineWidth = 2f
            setDrawValues(false)
        }

        // Create LineDataSet for rpm
        val rpmLineData = LineDataSet(rpmEntries, "RPM").apply {
            color = Color.BLUE
            setDrawCircles(false)
            valueTextSize = 12f
            lineWidth = 2f
            setDrawValues(false)
        }

        // Set the data to the charts
        speedChart.data = LineData(speedLineData)
        rpmChart.data = LineData(rpmLineData)

        // Pass the raw data to the chart configuration
        configureChart(speedChart, isTimeBased = true, data = sortedSpeedData, isRpmChart = false)
        configureChart(rpmChart, isTimeBased = true, data = sortedRpmData.map { it.first to it.second.toDouble() }, isRpmChart = true)
    }
    class TimeAxisValueFormatter(private val data: List<Pair<LocalDateTime, Double>>) : ValueFormatter() {
        @RequiresApi(Build.VERSION_CODES.O)
        private val formatter = DateTimeFormatter.ofPattern("HH:mm:ss") // Customize the time format

        @RequiresApi(Build.VERSION_CODES.O)
        override fun getAxisLabel(value: Float, axis: AxisBase?): String {

            val index = value.toInt()

            return if (index in data.indices) {
                data[index].first.format(formatter) // Format the LocalDateTime as a string
            } else {
                ""
            }
        }
    }
}
