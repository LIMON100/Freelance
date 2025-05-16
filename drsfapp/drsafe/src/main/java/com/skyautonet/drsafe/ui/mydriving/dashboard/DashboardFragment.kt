package com.skyautonet.drsafe.ui.mydriving.dashboard

import android.annotation.SuppressLint
import android.content.Context
import android.content.SharedPreferences
import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.os.FileObserver
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import androidx.annotation.RequiresApi
import androidx.appcompat.widget.AppCompatSpinner
import androidx.fragment.app.activityViewModels
import com.github.mikephil.charting.charts.LineChart
import com.github.mikephil.charting.components.Description
import com.github.mikephil.charting.components.XAxis
import com.github.mikephil.charting.components.YAxis
import com.github.mikephil.charting.data.Entry
import com.github.mikephil.charting.data.LineData
import com.github.mikephil.charting.data.LineDataSet
import com.github.mikephil.charting.formatter.IndexAxisValueFormatter
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentDashboardBinding
import com.skyautonet.drsafe.ui.BaseFragment
import com.skyautonet.drsafe.ui.viewmodel.DashboardViewModel
import com.skyautonet.drsafe.ui.viewmodel.EventDashboardViewModel
import java.io.BufferedReader
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.io.InputStreamReader
import java.io.OutputStreamWriter
import java.sql.Types.NULL
import java.time.DayOfWeek
import java.time.Duration
import java.time.LocalDate
import java.time.LocalDateTime
import java.time.YearMonth
import java.time.format.DateTimeFormatterBuilder
import java.time.format.DateTimeParseException
import java.time.temporal.ChronoField
import java.time.temporal.ChronoUnit
import java.util.Calendar
import kotlin.math.absoluteValue
import kotlin.random.Random

/**
 * Created by Hussain on 30/09/24.
 */

@RequiresApi(Build.VERSION_CODES.O)
class DashboardFragment : BaseFragment() {

    private lateinit var binding : FragmentDashboardBinding
    private val dashboardViewModel: DashboardViewModel by activityViewModels()
    private val eventDashboardViewModel: EventDashboardViewModel by activityViewModels()
    //private var downloadsDirectory = File(requireContext().filesDir, "axon/data")

    private lateinit var sharedPreferences: SharedPreferences
    private val processedFiles = mutableSetOf<String>()
    private var totalScore: Double = 0.0
    private var cumulativeScoreCount: Int = 0
    private var weeklyScores = mutableListOf<WeeklyDrivingScore>()

    private var currentWeekScore: WeeklyScore? = null
    private var previousWeekScore: WeeklyScore? = null
    private val currentWeek = mutableListOf<RowData>()
    private val previousWeek = mutableListOf<RowData>()
    private var dailySafeScores = MutableList(7) {mutableListOf<Double>() }

    private val totalDistanceList = mutableListOf<Double>()
    private var cumulativeDistance = 0.0
    private var cumulativeSpeedSum = 0.0
    private var speedEntryCount = 0
    private lateinit var fileObserver: FileObserver
    private val cumulativeScore = mutableListOf<Double>()
    var currentWeekScores = MutableList(7) { 0.0 }
    var previousWeekScores = MutableList(7) { 0.0 }
    private var currentMonthSafeScore = MutableList<Float>(31) { 0f }
    private var previousMonthSafeScore =MutableList<Float>(31) { 0f }
    @RequiresApi(Build.VERSION_CODES.O)
    val formatter = DateTimeFormatterBuilder()
        .appendPattern("dd-MM-yyyy HH:mm") // Full pattern with hours, minutes, and seconds
        .optionalStart()
        .appendFraction(ChronoField.NANO_OF_SECOND, 0, 6, true) // Handle fractional seconds (up to microseconds)
        .optionalEnd()
        .toFormatter()

    var currentMonthDaysSafeScore = MutableList(31) { mutableListOf<Float>() }
    var previousMonthDaysSafeScore = MutableList(31) { mutableListOf<Float>() }
    val gson = Gson()
    val speedDataPerWeek = mutableListOf<Pair<LocalDateTime, Double>>()
    val rpmDataPerWeek = mutableListOf<Pair<LocalDateTime, Int>>()

    val currentWeekSpeed = mutableListOf<Pair<LocalDateTime, Double>>()
    val currentWeekRpm = mutableListOf<Pair<LocalDateTime, Int>>()

    val speedPerMinute = mutableListOf<Pair<LocalDateTime, Int>>()
    val rpmPerMinute = mutableListOf<Pair<LocalDateTime, Int>>()

    val WEIGHT_FAST_ACCELERATION = 2.0
    val WEIGHT_SUDDEN_DEPARTURE = 1.5
    val WEIGHT_RAPID_DECELERATION = 2.0
    val WEIGHT_SUDDEN_STOP = 1.5
    val WEIGHT_RADICAL_COURSE_CHANGE = 2.0
    val WEIGHT_OVERTAKING_SPEED = 1.5
    val WEIGHT_SHARP_TURNS = 1.5
    val WEIGHT_SUDDEN_U_TURN = 2.0
    val WEIGHT_CONTINUOUS_OPERATION = 1.0
    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentDashboardBinding.inflate(inflater, container, false)
        sharedPreferences = requireContext().getSharedPreferences("DrivingScores", Context.MODE_PRIVATE)
        return binding.root
    }

    @RequiresApi(Build.VERSION_CODES.O)
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        observeCsvFiles()
        //createCsvFile()
        readCsvFiles()
        displayHardBrakingEvents()
        displayAverageSpeed()
        displayTotalDrivingTime()
        displayHardAccelerationEvents()

        val spScore = view.findViewById<AppCompatSpinner>(R.id.spScore)
        spScore.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                // Handle the selection logic here
                if (position in 0..1) {
                    when (position ) {
                        0 -> updateUIWithScore() // Call weekly function
                        1 -> updateUIWithMonthlyScore() // Call monthly function
                    }
                }
            }



            override fun onNothingSelected(parent: AdapterView<*>?) {
                // Handle when no item is selected (if necessary)
            }
        }
        binding.appCompatSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                when (position) {
                    0 -> {
                        // Weekly selected
                        showWeeklyLabels()
                        hideMonthlyLabels()
                        setupChart(binding.chart, true)

                    }
                    1 -> {
                        // Monthly selected
                        showMonthlyLabels()
                        hideWeeklyLabels()
                        setupChart(binding.chart, false)

                    }
                }
            }

            override fun onNothingSelected(parent: AdapterView<*>?) {
                // Optional: Default to weekly if nothing is selected
                showWeeklyLabels()
                hideMonthlyLabels()
                setupChart(binding.chart, true)
            }
        }
        // Load speed and RPM data from preferences
        val (speedDataPairs, rpmDataPairs) = loadSpeedRPMDataFromPreferences()

        println("Retrieved speed data: $speedDataPairs") // List<Pair<LocalDateTime, Double>>
        println("Retrieved rpm data: $rpmDataPairs")    // List<Pair<LocalDateTime, Int>>

        dashboardViewModel.updateSpeedData(speedDataPairs)
        dashboardViewModel.updateRpmData(rpmDataPairs)
//        val locationTrackingDeviceError= sharedPreferences.getInt("locationTrackingDeviceError",0)
        // Assume sharedPreferences is already initialized
        val fastAcceleration = sharedPreferences.getInt("rapidAcceleration", 0)
        val suddenDeparture = sharedPreferences.getInt("suddenDeparture", 0)
        val rapidDeceleration = sharedPreferences.getInt("rapidDeceleration", 0)
        val suddenStop = sharedPreferences.getInt("suddenStop", 0)
        val radicalCourseChange = sharedPreferences.getInt("radicalCourseChange", 0)
        val overtakingSpeed = sharedPreferences.getInt("overtakingSpeed", 0)
        val sharpTurns = sharedPreferences.getInt("sharpTurns", 0)
        val suddenUTurn = sharedPreferences.getInt("suddenUTurn", 0)
        val continuousOperation = sharedPreferences.getInt("continuousOperation", 0)
//        eventDashboardViewModel.updateEventCount("location Tracking Device Error",locationTrackingDeviceError)
        eventDashboardViewModel.updateEventCount("Fast Acceleration", fastAcceleration)
        eventDashboardViewModel.updateEventCount("Sudden Departure", suddenDeparture)
        eventDashboardViewModel.updateEventCount("Rapid Deceleration", rapidDeceleration)
        eventDashboardViewModel.updateEventCount("Sudden Stop", suddenStop)
        eventDashboardViewModel.updateEventCount("Radical Course Change", radicalCourseChange)
        eventDashboardViewModel.updateEventCount("Overtaking Speed", overtakingSpeed)
        eventDashboardViewModel.updateEventCount("Sharp Turns", sharpTurns)
        eventDashboardViewModel.updateEventCount("Sudden U-Turn", suddenUTurn)
        eventDashboardViewModel.updateEventCount("Continuous Operation", continuousOperation)
    }


    fun showWeeklyLabels() {
        binding.llThisWeekLabel.visibility = View.VISIBLE
        binding.llPrevWeekLabel.visibility = View.VISIBLE
    }

    fun hideWeeklyLabels() {
        binding.llThisWeekLabel.visibility = View.GONE
        binding.llPrevWeekLabel.visibility = View.GONE
    }

    fun showMonthlyLabels() {
        binding.llThisMonthLabel.visibility = View.VISIBLE
        binding.llPrevMonthLabel.visibility = View.VISIBLE
    }

    fun hideMonthlyLabels() {
        binding.llThisMonthLabel.visibility = View.GONE
        binding.llPrevMonthLabel.visibility = View.GONE
    }



    private fun displayTotalDrivingTime() {
        val totalTime = sharedPreferences?.getFloat("averageDrivingTime", 0f)
        val displayedTime = if (totalTime?.absoluteValue ?: 0f < 0.0001f) 0f else totalTime ?: 0f
        println("displayTotalTime === $totalTime")

        binding.tvTime.text = String.format("%.1f", displayedTime)
        binding.pgDrivingTime.progress = (totalTime ?: 0f).coerceIn(0f, 100f)
    }

    private fun displayHardAccelerationEvents() {
        val (totalHardAcceleration, _) = getWeeklyData(requireContext())
        println("displayHardAccelerationEvents$totalHardAcceleration")
        binding.tvEvents.text = totalHardAcceleration.toString()
        binding.pgHAE.progress = totalHardAcceleration.toFloat().coerceIn(0f, 100f)
    }

    private fun displayHardBrakingEvents() {
        val (_, totalHardBraking) = getWeeklyData(requireContext())
        binding.tvHABEvents.text = totalHardBraking.toString()
        binding.pgHBE.progress = totalHardBraking.toFloat().coerceIn(0f, 100f)    }

    @RequiresApi(Build.VERSION_CODES.O)
    private fun setupChart(chart: LineChart, isWeekly: Boolean) {
        if (isWeekly) {
            //val dailySafeScoresJson = sharedPreferences.getString("daily_safe_scores", null)
            val currentWeekScoresJson = sharedPreferences.getString("current_week_scores", null)
            if (currentWeekScoresJson.isNullOrEmpty()) {
                Log.d("Error", "Current Week Scores not found or invalid JSON.")
                return // Exit the function early if the data is missing
            }
            val currentWeekScores = try {
                gson.fromJson(currentWeekScoresJson, Array<Float>::class.java).toList()
            } catch (e: Exception) {
                Log.e("Error", "Failed to parse current week scores", e)
                emptyList<Float>() // Return an empty list if parsing fails
            }
            Log.d("Parsed Current Week Scores", currentWeekScores.toString())
            Log.d("Previous Week Data", previousWeekScores.toString())
            if (isCurrentWeekCompleted()) {
                //previousWeekScores = currentWeekScores.map { it.average() }.toMutableList()
                previousWeekScores = currentWeekScores.map { it.toDouble() }.toMutableList()
                val previousWeekScoresJson = gson.toJson(previousWeekScores)
                sharedPreferences.edit().putString("previous_week_scores", previousWeekScoresJson).apply()

                Log.d("Previous Week Data After Transfer", previousWeekScores.toString())

                dailySafeScores = MutableList(7) { mutableListOf() }
            }
            val currentWeekEntries = currentWeekScores.mapIndexed { index, scores ->
                val averageScore = if (scores != 0.0f) scores else 0.0f
                val entry=Entry(index.toFloat(), averageScore) // X-axis: day index (0-6)
                Log.d("CurrentWeekEntry", "Day: $index, X: ${entry.x}, Y: ${entry.y}")
                entry
            }

            val currentWeekData = LineDataSet(currentWeekEntries, "Current Week Scores").apply {
                setColor(resources.getColor(R.color.colorPrimary, null)) // Primary color for current week
                setDrawCircles(false)
                highLightColor = Color.BLUE // Highlight in blue
                valueTextSize = 18f
                lineWidth = 2f
                setDrawValues(false)
                setDrawHorizontalHighlightIndicator(false)
            }
            val previousWeekScoresJson = sharedPreferences.getString("previous_week_scores", null)
            previousWeekScores = if (!previousWeekScoresJson.isNullOrEmpty()) {
                try {
                    gson.fromJson(previousWeekScoresJson, Array<Double>::class.java).toMutableList()
                } catch (e: Exception) {
                    Log.e("Error", "Failed to parse previous week scores", e)
                    mutableListOf() // Return an empty list if parsing fails
                }
            } else {
                Log.d("Previous Week Scores Not Found", "Using default empty list")
                mutableListOf()
            }

            // Assuming previousWeekScores is a list of scores for the previous week (7 days)
            val previousWeekEntries = previousWeekScores.mapIndexed { index, score ->
                Entry(index.toFloat(), score.toFloat()) // X-axis: day index (0-6)
            }


            // Set up the LineDataSet for the previous week scores
            val previousWeekData = LineDataSet(previousWeekEntries, "Previous Week Scores").apply {
                setColor(resources.getColor(R.color.colorLine, null)) // Different color for previous week
                setDrawCircles(false)
                highLightColor = Color.GRAY // Set highlight to gray for previous week
                axisDependency = YAxis.AxisDependency.RIGHT
                valueTextSize = 18f
                lineWidth = 2f
                setDrawValues(false)
                setDrawHorizontalHighlightIndicator(false)
            }

            // Combine both current and previous week datasets
            chart.data = LineData(listOf(currentWeekData, previousWeekData))
            val daysOfWeek = arrayOf("Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat")
            chart.xAxis.valueFormatter = IndexAxisValueFormatter(daysOfWeek)
            val allEntries = currentWeekEntries + previousWeekEntries
            val yAxisMin = allEntries.minOfOrNull { it.y } ?: 0f
            val yAxisMax = allEntries.maxOfOrNull { it.y } ?: 10f // Set a sensible default max
            chart.axisLeft.axisMinimum = yAxisMin - 1f // Add some padding below
            chart.axisLeft.axisMaximum = yAxisMax + 1f
        } else {
            val currentDate = LocalDate.now()
            val daysInMonth = currentDate.lengthOfMonth()
            val currentMonthSafeScoreJson = sharedPreferences?.getString("currentmonth_scores", null)
            val currentMonthSafeScore = gson.fromJson(currentMonthSafeScoreJson, Array<Float>::class.java)?.toList() ?: emptyList()

            Log.d("Current Month Data==", currentMonthSafeScore.toString())
            val currentMonthScores = if (currentMonthSafeScore.isNotEmpty()) {
                currentMonthSafeScore
            } else {
                List(daysInMonth) { 0.0f } // Default scores for the month
            }
            Log.d("Parsed currentMonthScores", "${currentMonthScores.size}")
            val trimmedCurrentMonthScores = currentMonthScores
                .dropWhile { it == 0.0f }  // Drop leading zeros
                .dropLastWhile { it == 0.0f } // Drop trailing zeros


            // Check if the current month has ended
            if (isCurrentMonthCompleted()) {
                previousMonthSafeScore = trimmedCurrentMonthScores .map { it }.toMutableList()
                val previousMonthScoresJson = gson.toJson(previousMonthSafeScore)
                sharedPreferences.edit().putString("previous_month_scores", previousMonthScoresJson).apply()

                Log.d("Previous Month Data After Transfer", previousMonthSafeScore.toString())

                // Reset current month's scores for new data
                currentMonthDaysSafeScore = MutableList(daysInMonth) { mutableListOf() }
            }

            // Generate entries for the current month
            val currentMonthEntries = trimmedCurrentMonthScores .take(daysInMonth).mapIndexed { index, scores ->
                val averageScore = if (scores.isNaN() || scores == 0.0f) 0.0f else scores
                Log.d("Mapped Entry", "Index: $index, Score: $averageScore")
                Entry(index.toFloat(), averageScore) // X-axis: day index (0-29/30/31)
            }

            val currentMonthData = LineDataSet(currentMonthEntries, "Current Month Scores").apply {
                setColor(resources.getColor(R.color.colorPrimary, null)) // Primary color for current month
                setDrawCircles(false)
                highLightColor = Color.GREEN // Highlight in green
                valueTextSize = 18f
                lineWidth = 2f
                setDrawValues(false)
                setDrawHorizontalHighlightIndicator(false)
            }

            val previousMonthScoresJson = sharedPreferences.getString("previous_month_scores", null)
            if (!previousMonthScoresJson.isNullOrEmpty()) {
                previousMonthSafeScore = gson.fromJson(previousMonthScoresJson, Array<Float>::class.java).toMutableList()
                Log.d("Loaded Previous Month Scores", previousMonthSafeScore.toString())
            } else {
                Log.d("Previous Month Scores Not Found", "Using default empty list")
                previousMonthSafeScore = mutableListOf()
            }
            val trimmedPreviousMonthScores = previousMonthSafeScore
                .dropWhile { it == 0.0f }
                .dropLastWhile { it == 0.0f }

            // Generate entries for the previous month
            val previousMonthEntries = trimmedPreviousMonthScores .take(daysInMonth).mapIndexed { index, score ->
                Entry(index.toFloat(), score.toFloat()) // X-axis: day index
            }

            val previousMonthData = LineDataSet(previousMonthEntries, "Previous Month Scores").apply {
                setColor(resources.getColor(R.color.colorLine, null)) // Different color for previous month
                setDrawCircles(false)
                highLightColor = Color.GRAY // Set highlight to gray for previous month
                axisDependency = YAxis.AxisDependency.RIGHT
                valueTextSize = 18f
                lineWidth = 2f
                setDrawValues(false)
                setDrawHorizontalHighlightIndicator(false)
            }


            chart.data = LineData(listOf(currentMonthData, previousMonthData))
            val daysOfMonth = (1..daysInMonth).map { it.toString() }.toTypedArray()
            chart.xAxis.valueFormatter = IndexAxisValueFormatter(daysOfMonth)
            val allEntries = currentMonthEntries + previousMonthEntries
            val yAxisMin = allEntries.minOfOrNull { it.y } ?: 0f
            val yAxisMax = allEntries.maxOfOrNull { it.y } ?: 10f // Set a sensible default max
            chart.axisLeft.axisMinimum = yAxisMin - 1f // Add some padding below
            chart.axisLeft.axisMaximum = yAxisMax + 1f
        }


        // Set the data to the chart and configure it
        chart.apply {
            configureChart(this) // Make sure this method is setting the chart appearance properly
            invalidate() // Refresh the chart with new data
        }
    }

    private fun configureChart(speedChart: LineChart) {
        speedChart.apply {
            // Bottom axis
            xAxis.textSize = 12f
            xAxis.position = XAxis.XAxisPosition.BOTTOM
            xAxis.textColor = Color.BLACK
            xAxis.isEnabled = true
            xAxis.disableGridDashedLine()
            xAxis.setDrawAxisLine(false)
            xAxis.setDrawGridLines(false)
            xAxis.setAvoidFirstLastClipping(true)

            description = Description().apply {
                isEnabled = false
            }

            // Left axis
            val leftAxis = axisLeft
            leftAxis.removeAllLimitLines()
            leftAxis.setPosition(YAxis.YAxisLabelPosition.OUTSIDE_CHART)
            leftAxis.textColor = Color.BLACK
            leftAxis.setDrawLimitLinesBehindData(false)
            leftAxis.setDrawGridLines(false)
            leftAxis.setDrawAxisLine(false)
            leftAxis.axisMaximum = 120f
            leftAxis.axisMinimum = 0f

            // Right axis
            axisRight.isEnabled = false

            legend.isEnabled = false
            setExtraOffsets(5f, 5f, 5f, 15f)
            invalidate()
        }
    }
    private fun isCurrentWeekCompleted(): Boolean {
        val currentDayOfWeek = Calendar.getInstance().get(Calendar.DAY_OF_WEEK) - 1
        return dailySafeScores.size > currentDayOfWeek &&
                dailySafeScores.take(currentDayOfWeek + 1).all { it.isNotEmpty() }
    }

    private fun isCurrentMonthCompleted(): Boolean {
        val today  = LocalDate.now()
        val lastDayOfMonth = today.withDayOfMonth(today.month.length(today.isLeapYear()))  // Get last day of the current month
        return today.isEqual(lastDayOfMonth)    }
    @RequiresApi(Build.VERSION_CODES.O)
    private fun getCurrentMonthDays(): Int {
        val currentDate = LocalDate.now()
        return currentDate.lengthOfMonth() // Number of days in the current month
    }

    private fun observeCsvFiles() {
        val directory = File(requireContext().filesDir, "axon/data")  // Create File object for directory
        println("Observing directory: ${directory.absolutePath}")
        if (!directory.exists()) {
            println("Axon data directory does not exist. Creating it.")
            if (directory.mkdirs()) {
                println("Directory created successfully.")
            } else {
                println("Failed to create directory.")
            }
        }

        fileObserver = object : FileObserver(directory) {  // Pass the File object instead of a string path
            override fun onEvent(event: Int, path: String?) {
                println("Event triggered: $event, Path: $path")

                if (event == FileObserver.CREATE && path?.endsWith(".csv") == true) {
                    println("New CSV file created: $path")
                    readCsvFiles()  // Read CSV files when a new one is created
                }
            }
        }

        // Start observing the directory
        fileObserver.startWatching()
        println("File observer started.")
    }




    private fun readCsvFiles() {
        // Path to the "axon/data" directory in internal storage
        val axonDataDirectory = File(requireContext().filesDir, "axon/data")

        // Check if the subdirectory exists and is a directory
        if (!axonDataDirectory.exists() || !axonDataDirectory.isDirectory) {
            Log.d("DashboardFragment", "Axon data directory does not exist: ${axonDataDirectory.absolutePath}")
            return
        }

        // List files in the directory
        val files = axonDataDirectory.listFiles()

        // If no files or directory is null, log a message and return
        if (files.isNullOrEmpty()) {
            Log.d("DashboardFragment", "No files found in the directory.")
            return
        }

        Log.d("DashboardFragment", "Reading files from: ${axonDataDirectory.absolutePath}")

        var hasCsvFile = false

        // Process each file in the directory
        files.forEach { file ->
            if (file.isFile && file.extension == "csv") {
                Log.d("DashboardFragment", "Processing file: ${file.name}")
                readCsvFile(file) // Call the existing `readCsvFile` method to process the file
                hasCsvFile = true // Mark that a CSV file was found
            }
        }

        // If no CSV files found
        if (!hasCsvFile) {
            Log.d("DashboardFragment", "No CSV files found in the internal storage directory.")
        }
    }



    data class WeeklyDrivingScore(
        val score: Double,
        val weekNumber: Int
    )

    @SuppressLint("MutatingSharedPrefs")
    @RequiresApi(Build.VERSION_CODES.O)
    private fun readCsvFile(file: File) {
        println("helllo===")

        val processedFiles: MutableSet<String> =
            sharedPreferences.getStringSet("processedFiles", mutableSetOf()) ?: mutableSetOf()

//        // Check if the file has already been processed
        if (processedFiles.contains(file.name)) {
            println("File ${file.name} has already been processed.")
            return
        }
        val filename=file.name.split(".")[0]

        // Optionally, log success
        Log.d("DashboardFragment", "File ${file.name} has been processed successfully.")
        val currentDateTime = LocalDateTime.now()
        val currentMonth = currentDateTime.month
        val currentWeekStart = getStartOfCurrentWeek()
        var lastWeekStart: LocalDateTime = getStartOfCurrentWeek()

        val currentWeekCounts = MutableList(7) { 0 }
        val previousWeekCounts = MutableList(7) { 0 }
        val currentWeekEvents = mutableMapOf<String, Int>()
        val previousWeekEvents = mutableMapOf<String, Int>()
        var currentWeekTotalSpeed = 0.0
        var currentWeekTotalDrivingTime = 0.0
        var currentWeekTotalHardAcceleration = 0
        var currentWeekTotalHardBraking = 0
        // Reset data if it's a new week
        if (!isSameWeek(currentWeekStart, lastWeekStart)) {
            previousWeekScores.clear()
            previousWeekScores.addAll(currentWeekScores)
            previousWeekCounts.clear()
            previousWeekCounts.addAll(currentWeekCounts)

            currentWeekScores.fill(0.0)
            currentWeekCounts.fill(0)
            currentWeekTotalSpeed = 0.0
            currentWeekTotalDrivingTime = 0.0
            currentWeekTotalHardAcceleration = 0
            currentWeekTotalHardBraking = 0
        }
        resetMonthlyScoresIfNeeded()

        // Track events and metrics
        var rapidAcceleration = 0
        var suddenDeparture = 0
        var rapidDeceleration = 0
        var totalDistanceDriven = 0.0
        var drivingTime = 0.0
        var totalSpeed = 0.0
        var cumulativeSpeedSum = 0.0
        var speedEntryCount = 0
        var suddenStop = 0
        var radicalCourseChange = 0
        var overtakingSpeed = 0
        var sharpTurns = 0
        var suddenUTurn = 0
        var continuousOperation = 0
        var overSpeed=0
        var longTermSpeedData=0


        // Initialize lists to store speed and RPM per minute

        loadSpeedRPMDataFromPreferences()

        try {
//            print("hi hello")
            processedFiles.add(file.name)
            sharedPreferences.edit().putStringSet("processedFiles", processedFiles).apply()
            FileInputStream(file).use { fis ->
                BufferedReader(InputStreamReader(fis)).use { reader ->
                    repeat(7) {
                        reader.readLine()
                    }
                    var line: String?
                    while (reader.readLine().also { line = it } != null) {
//                        println("Processing line: $line")
                        val values = line!!.split(",")
//                        if (values.size < 3) {
//                            println("Skipping malformed line: $line")
//                            continue
//                        }

                        val dateTime = LocalDateTime.parse(values[0].trim(), formatter)
                        val trip=filename.split("-")[3]
                        //val trip = values[1].trim()
                        //val vinCode = values[2].trim()
                        //val vehicleType = values[3].trim()
                        val speed = values[3].trim().toDoubleOrNull() ?: 0.0
                        val rpm = values[4].trim().toIntOrNull() ?: 0
                        val gpsLatitude = values[6].trim().toDoubleOrNull() ?: 0.0
                        //val latDirection = values[7].trim()
                        val gpsLongitude = values[7].trim().toDoubleOrNull() ?: 0.0
                        //val lonDirection = values[9].trim()
                        //val vehicleStatus = values[10].trim()
                        totalDistanceDriven=values[2].toDoubleOrNull()?:0.0
                        val eventType = values[11].trim().toInt()
                        val dayOfWeekIndex = dateTime.dayOfWeek.value % 7
                        val recordMonth = dateTime.month
                        val dayOfMonth = dateTime.dayOfMonth
//                        println("dateTime: $dateTime, currentWeekStart: $currentWeekStart")
                        val weekBeforeCurrentWeekStart = currentWeekStart.minusWeeks(1)

                        if (dateTime.isAfter(weekBeforeCurrentWeekStart)) {
//                            println("scorecalculation===")
                            currentWeek.add(RowData(dateTime,trip, speed, rpm, gpsLatitude, gpsLongitude))

                            totalSpeed += speed
                            if (speed > 0) drivingTime += 1

                            // Accumulate speed and RPM per minute
                            val minute = dateTime.minute
                            while (speedPerMinute.size <= minute) {
                                speedPerMinute.add(Pair(LocalDateTime.MIN, 0)) // Add default values
                                rpmPerMinute.add(Pair(LocalDateTime.MIN, 0))   // Add default values
                            }

// Update or add values for the current minute
                            speedPerMinute[minute] = Pair(dateTime, speedPerMinute[minute].second + speed.toInt())
                            rpmPerMinute[minute] = Pair(dateTime, rpmPerMinute[minute].second + rpm)

                            // Calculate cumulative speed
                            cumulativeSpeedSum += speed
                            speedEntryCount++
                            currentWeekSpeed.add(Pair(dateTime, speed)) // Ensure dateTime is used
                            currentWeekRpm.add(Pair(dateTime, rpm))
                            //println("rpm data found and storing :$dateTime - $rpm")
                            if (speedDataPerWeek.size > 4) {
                                speedDataPerWeek.removeAt(0)
                                rpmDataPerWeek.removeAt(0)
                            }
                            speedDataPerWeek.add(Pair(dateTime, speed))
                            rpmDataPerWeek.add(Pair(dateTime, rpm))
                            println("rpm data found and storing :$rpmDataPerWeek")
                            saveSpeedRPMDataToPreferences(speedDataPerWeek, rpmDataPerWeek)
                            saveCumulativeSpeedSum()
                            saveSpeedEntryCount()
//                            println("cumulativeSpeedSum====$cumulativeSpeedSum")
//                            println("speedEntryCount====$speedEntryCount")
                            if (eventType>0){
                                println("Event found ...$eventType")
                            }

                            // Increment counters for events
                            when (eventType) {
                                61 -> {
overSpeed++
                                }
                                62->{
                                    longTermSpeedData++
                                }
                                63->{
                                    rapidAcceleration++
                                }
                                64->{
                                    suddenStop++
                                }
                                65->{
                                    rapidDeceleration++
                                }
                                66->{
                                    suddenStop++
                                }
                                67->{
                                    radicalCourseChange++
                                }
                                68->{
                                    overtakingSpeed++
                                }
                                69->{
                                    sharpTurns++
                                }
                                70->{
                                    suddenUTurn++
                                }
                                71->{
                                    continuousOperation++
                                }
//                                11->{
//locationTrackingDeviceError++
//                                }
//                                12->{
//speedSensorError++
//                                }
//                                13->{
//                                    RpmSensorError++
//                                }
//                                14->{
//brakeSignalDetectionSensorError++
//                                }
//                                21->{
//sensorInputUnitError++
//                                }
//                                22->{
//                                    sensorOutputUnitError++
//                                }
//                                31->{
//dataOutputUnitError++
//                                }
//                                32->{
//communicationDeviceError++
//                                }
//                                41->{
//moreThanMilageCalculationType++
//                                }
//                                99->{
//powerSupplyError++
//                                }
//                                61->{
//overSpeed++
//                                }
//                                62->{
//longTermSpeedData++
//                                }
//                                63->{
//                                    rapidAcceleration++
//                                }
//                                64->{
//suddenDeparture++
//                                }
//                                65->{
//rapidDeceleration++
//                                }
//                                66->{
//suddenStop++
//                                }
//                                67->{
//radicalCourseChange++
//                                }
//                                68->{
//
//                                }
//                                69->{
//
//                                }
//                                70->{
//suddenUTurn++
//                                }
//                                71->{
//continuousOperation++
//                                }
//                                81->{
//
//                                }
//                                82->{
//
//                                }
//                                83->{
//
//                                }
//                                84->{
//
//                                }
//                                85->{
//
//                                }
//                                86->{
//
//                                }
//                                87->{
//
//                                }
//                                88->{
//
//                                }
//                                89->{
//
//                                }

//                                1 -> {
//                                    rapidAcceleration++
//                                    currentWeekTotalHardAcceleration++
//                                }
//                                2 -> longTermSpeeding++
//                                3 -> rapidAcceleration++
//                                4 -> suddenDeparture++
//                                5 -> rapidDeceleration++
//                                6 -> {
//                                    suddenStop++
//                                    currentWeekTotalHardBraking++
//                                }
//                                7 -> aggressiveLaneChange++
//                                8 -> outpacing++
//                                9 -> sharpTurns++
//                                10 -> suddenUTurn++
//                                11 -> continuousOperation++
//                                12 -> btoPuaD1++
//                                13 -> btoPuaD2++
//                                14 -> btoPuaD3++
//                                15 -> btoReversing++
//                                16 -> btoSpeedCam++
//                                else -> println("Unknown event type: $eventType")
                            }


                            // Calculate daily safe score
                            val safeScore = calculateSafeDrivingScore(
                                rapidAcceleration, suddenDeparture, rapidDeceleration,
                                suddenStop, radicalCourseChange, overtakingSpeed,
                                sharpTurns, suddenUTurn, continuousOperation, totalDistanceDriven
                            )
                            println("safeScore===: $dayOfWeekIndex is $safeScore")

                            currentWeekScores[dayOfWeekIndex] = (currentWeekScores[dayOfWeekIndex]+safeScore)/2
                            dailySafeScores[dayOfWeekIndex].add(safeScore)
                            currentWeekCounts[dayOfWeekIndex]++
                            if (recordMonth == currentMonth) {
                                currentMonthSafeScore += safeScore.toFloat()
                                currentMonthDaysSafeScore[dayOfMonth - 1] += safeScore.toFloat() // Add score to the corresponding day of the month
                            } else if (recordMonth == currentMonth.minus(1)) {
                                previousMonthSafeScore += safeScore.toFloat()
                                previousMonthDaysSafeScore[dayOfMonth - 1] += safeScore.toFloat() // Add score to the corresponding day of the month
                            }
                        }
                        currentWeekTotalDrivingTime++
                    }
                }
            }
//            println("speedDataPerWeek>===$speedDataPerWeek")
//            println("rpmDataPerWeek>====$rpmDataPerWeek")
            saveMonthlySafeScores()

            val gson = Gson()
            val scoresJson = gson.toJson(currentWeekScores)
            Log.d("CurrentWeekScores JSON", scoresJson)
            val dailySafeScoresJson = gson.toJson(dailySafeScores)
//            println("scoresJson===>$scoresJson")
//            println("dailySafeScoresJson===>$dailySafeScoresJson")
            var curretmonthscore = gson.toJson(currentMonthSafeScore)
            Log.d("CurrentMonthScores JSON", curretmonthscore)
            var monthlydailyscorejson = gson.toJson(currentMonthDaysSafeScore)

//            println("curretmonthscore===>$curretmonthscore")
//            println("monthlydailyscorejson===>$monthlydailyscorejson")

            with(sharedPreferences.edit()) {
                putString("current_week_scores", scoresJson)
                putString("daily_safe_scores", dailySafeScoresJson)
                apply()
            }

//            println("hi====")
            with(sharedPreferences.edit()) {
                putString("cumulativeSpeedSum", cumulativeSpeedSum.toString())
                putInt("speedEntryCount", speedEntryCount)

                apply()
            }
            val oldradicalCourseChange = sharedPreferences.getInt("radicalCourseChange", 0)
            val oldsharpturns=sharedPreferences.getInt("sharpTurns",0)
            with(sharedPreferences.edit()) {
                putInt("overspeed",overSpeed)
                putInt("longtermspeeddata",longTermSpeedData)
                //putInt("locationTrackingDeviceError",locationTrackingDeviceError)
                putInt("rapidAcceleration", rapidAcceleration)
                putInt("suddenDeparture", suddenDeparture)
                putInt("rapidDeceleration", rapidDeceleration)
                putInt("suddenStop", suddenStop)
                putInt("radicalCourseChange", radicalCourseChange+oldradicalCourseChange)
                putInt("overtakingSpeed", overtakingSpeed)
                putInt("sharpTurns", sharpTurns+oldsharpturns)
                putInt("suddenUTurn", suddenUTurn)
                putInt("continuousOperation", continuousOperation)
                apply() // Commit the changes
            }

            with(sharedPreferences.edit()) {
                putString("currentmonth_scores", curretmonthscore)
                putString("monthly_daily_safe_scores", monthlydailyscorejson)
                apply()
            }
            val averageScore = calculateWeeklyAverageScore(currentWeekScores, previousWeekScores, currentWeekCounts, previousWeekCounts)
//            println("averageScore$averageScore")
            val monthlyAverageScore = calculateMonthlyAverageScore(
                currentMonthDaysSafeScore,
                previousMonthDaysSafeScore
            )
//            println("Monthly Average Score: $monthlyAverageScore")



            // Update UI or store values as needed

            //updateUIWithAverageSpeed(averageSpeed)
            println("Before calling calculateAverageSpeed()")
            var averageSpeed = calculateAverageSpeed()
            println("After calling calculateAverageSpeed()")
            saveWeekDataToPreferences(currentWeek, "current_week")
            saveWeekDataToPreferences(previousWeek, "previous_week")
            saveAdditionalWeeklyData(currentWeekTotalDrivingTime, currentWeekTotalHardAcceleration, currentWeekTotalHardBraking, averageSpeed)
            retrieveWeeklyData()
            displayAverageSpeed()
            calculateAverageDrivingTime(currentWeek)
            updateUIWithMonthlyScore()
            updateUIWithScore()
            displayTotalDrivingTime()
            printDailySafeScores()
            getMonthlySafeScores()
            setupChart(binding.chart,true)
            setupChart(binding.chart,false)


        } catch (e: Exception) {
            Log.e("DashboardFragment", "Error reading file ${file.name}: ${e.message}")
        }
    }
    fun printDailySafeScores() {
        // Iterate through dailySafeScores and print each day with its score
//        println("dailyscore======")
        dailySafeScores.forEachIndexed { index, score ->
            val dayOfWeek = if (index == 0) 7 else index
            println("$dayOfWeek:$score")
        }
    }



    @RequiresApi(Build.VERSION_CODES.O)
    fun calculateAverageDrivingTime(events: List<RowData>) {
//        println("hi1===")
        var totalDrivingTimeInSeconds = 0L
        var totalTrips = 0

        // Calculate total driving time for all trips
        for (event in events) {
//            println("hi2===")

            val startDateTime = event.time
            val endDateTime = event.time.plusMinutes(60) // Assuming each event lasts 60 minutes (adjust based on your data)

            // Calculate the time difference in seconds
            val durationInSeconds = Duration.between(startDateTime, endDateTime).seconds
            totalDrivingTimeInSeconds += durationInSeconds
            totalTrips++
        }
//        println("hi3===")

        // Calculate the average driving time in seconds
        val averageDrivingTimeInSeconds = if (totalTrips > 0) totalDrivingTimeInSeconds / totalTrips else 0L
        println("averageDrivingTimeInSeconds===$averageDrivingTimeInSeconds")
        val currentTotalDrivingTime = sharedPreferences?.getFloat("averageDrivingTime", 0f)?.toDouble() ?: 0.0
        println("currentTotalDrivingTime === $currentTotalDrivingTime")
        println("averageDrivingTimeInSeconds === $averageDrivingTimeInSeconds")

        // Convert to hours and calculate the new average
        val averageDrivingTimeInHours = averageDrivingTimeInSeconds / 3600.0
        println("averageDrivingTimeInHours === $averageDrivingTimeInHours")

        // Calculate the new average driving time by averaging the current and the calculated values
        val newTime = (averageDrivingTimeInHours + currentTotalDrivingTime) / 2.0
        println("newTime === $newTime")

        // Save the new average driving time to SharedPreferences
        saveAverageDrivingTimeToPreferences(newTime)

        println("Average Driving Time in Hours: $averageDrivingTimeInHours")
    }

    fun saveAverageDrivingTimeToPreferences(averageDrivingTimeInHours: Double) {
        // Ensure the value is a valid number
        if (averageDrivingTimeInHours.isNaN() || averageDrivingTimeInHours.isInfinite()) {
            Log.d("Preferences", "Invalid value for averageDrivingTime: $averageDrivingTimeInHours")
            return
        }

        // Convert Double to Float
        val drivingTimeInFloat = averageDrivingTimeInHours.toFloat()

        // Save to SharedPreferences
        val editor = sharedPreferences?.edit()
        editor?.putFloat("averageDrivingTime", drivingTimeInFloat)
        editor?.apply()

        // Log the value being saved
        Log.d("Preferences", "Saved averageDrivingTime: $drivingTimeInFloat")
    }




    private fun saveAdditionalWeeklyData(
        totalDrivingTime: Double,
        totalHardAcceleration: Int,
        totalHardBraking: Int,
        averageSpeed: Double,
    ) {
        println("total drivin time is $totalDrivingTime")

        val sharedPreferences = context?.getSharedPreferences("weekly_data_prefs", Context.MODE_PRIVATE)
        val editor = sharedPreferences?.edit()
        val currentTotalHardBraking = sharedPreferences?.getInt("total_hard_braking", 0) ?: 0
        val currentTotalAcceleration = sharedPreferences?.getInt("total_hard_acceleration", 0) ?: 0
        val updatedTotalHardBraking = currentTotalHardBraking + totalHardBraking

        val updatedTotalAcceleration = currentTotalAcceleration + totalHardAcceleration

        editor?.apply {
            putFloat("total_driving_time", totalDrivingTime.toFloat())
            putInt("total_hard_acceleration", updatedTotalAcceleration)
            putInt("total_hard_braking", updatedTotalHardBraking)
            putFloat("average_speed", averageSpeed.toFloat())
            apply()
        }
    }
    private fun retrieveWeeklyData() {
        val sharedPreferences = context?.getSharedPreferences("weekly_data_prefs", Context.MODE_PRIVATE)

        // Retrieve each value from SharedPreferences
        val totalDrivingTime = sharedPreferences?.getFloat("total_driving_time", 0f) ?: 0f
        val totalHardAcceleration = sharedPreferences?.getInt("total_hard_acceleration", 0) ?: 0
        val totalHardBraking = sharedPreferences?.getInt("total_hard_braking", 0) ?: 0
        val averageSpeed = sharedPreferences?.getFloat("average_speed", 0f) ?: 0f

        // Print or use the retrieved values
        println("Total Driving Time: $totalDrivingTime")
        println("Total Hard Acceleration: $totalHardAcceleration")
        println("Total Hard Braking: $totalHardBraking")
        println("Average Speed: $averageSpeed")
    }

    fun getWeeklyData(context: Context): Pair<Int, Int> {
        val sharedPreferences = context.getSharedPreferences("weekly_data_prefs", Context.MODE_PRIVATE)
        val totalHardAcceleration = sharedPreferences.getInt("total_hard_acceleration", 0)
        val totalHardBraking = sharedPreferences.getInt("total_hard_braking", 0)

        return Pair(totalHardAcceleration, totalHardBraking)
    }

    @RequiresApi(Build.VERSION_CODES.O)
    private fun getStartOfCurrentWeek(): LocalDateTime {
        return LocalDateTime.now().with(DayOfWeek.MONDAY).toLocalDate().atStartOfDay()
    }
    @RequiresApi(Build.VERSION_CODES.O)
    private fun isSameWeek(date1: LocalDateTime, date2: LocalDateTime): Boolean {
        return date1.with(DayOfWeek.MONDAY) == date2.with(DayOfWeek.MONDAY)
    }
    private fun saveWeekDataToPreferences(newData: List<RowData>, week: String) {
        val sharedPreferences = context?.getSharedPreferences("week_data_prefs", Context.MODE_PRIVATE)
        val existingDataJson = sharedPreferences?.getString(week, null)

        // Initialize a list to hold the combined data
        val combinedData: MutableList<RowData> = mutableListOf()

        // If there is existing data, parse it and add it to combinedData
        existingDataJson?.let {
            val existingData = Gson().fromJson(it, Array<RowData>::class.java).toList()
            combinedData.addAll(existingData)
        }

        // Add the new data to the combined list
        combinedData.addAll(newData)

        // Convert the combined data to JSON and save it in shared preferences
        val updatedJsonData = Gson().toJson(combinedData)
        sharedPreferences?.edit()?.apply {
            putString(week, updatedJsonData)
            apply()
        }
    }




    // Calculate and store average speed

    // Save average speed to SharedPreferences
    private fun saveAverageSpeedToPreferences(updatedTotalSpeed: Double) {
        val editor = sharedPreferences?.edit()
        editor?.putFloat("average_speed", updatedTotalSpeed.toFloat())
        editor?.apply()
        println("Saved Average Speed to Preferences: $updatedTotalSpeed")
    }

    private fun calculateAverageSpeed(): Double {
        println("Calculating Average Speed...")
        val cumulativeSpeedSum = getCumulativeSpeedSum()
        val speedEntryCount = getSpeedEntryCount()
        println("cumulativeSpeedSum: $cumulativeSpeedSum")
        println("speedEntryCount: $speedEntryCount")

        var averageSpeed = 0.0
        if (speedEntryCount > 0) {
            averageSpeed = cumulativeSpeedSum / speedEntryCount // Calculate the average

            val currentTotalSpeed = sharedPreferences?.getFloat("average_speed", 0f) ?: 0f
            val updatedTotalSpeed = (currentTotalSpeed + averageSpeed) / 2.0 // Average with the current stored value

            println("cumulativeSpeedSum === $cumulativeSpeedSum")
            println("speedEntryCount === $speedEntryCount")
            println("averageSpeed === $updatedTotalSpeed")

            // Save updatedTotalSpeed in SharedPreferences
            saveAverageSpeedToPreferences(updatedTotalSpeed)
        } else {
            println("Skipping average speed calculation as speedEntryCount is 0")
        }
        return averageSpeed
    }
    private fun getCumulativeSpeedSum(): Double {
        val cumulativeSpeedSumString = sharedPreferences.getString("cumulativeSpeedSum", "0.0")
        return cumulativeSpeedSumString?.toDoubleOrNull() ?: 0.0
    }

    private fun getSpeedEntryCount(): Int {
        return sharedPreferences.getInt("speedEntryCount", 0)
    }

    // Display the saved average speed
    private fun displayAverageSpeed() {
        //val sharedPreferences = context?.getSharedPreferences("weekly_data_prefs", Context.MODE_PRIVATE)
        val averageSpeed = sharedPreferences?.getFloat("average_speed", 0f) ?: 0f

        println("Average Speed from Preferences: $averageSpeed")

        binding.tvSpeed.text = String.format("%.1f", averageSpeed)
        binding.pgAvgSpeed.progress = averageSpeed.coerceIn(0f, 100f)
    }


    private fun saveSpeedRPMToPreferences(context: Context, speedPerMinute: Double, rpmPerMinute: Double) {
        // Get SharedPreferences instance
        val sharedPreferences: SharedPreferences = context.getSharedPreferences("driving_data", Context.MODE_PRIVATE)

        // Edit and commit speed and RPM values
        sharedPreferences.edit().apply {
            putFloat("speedPerMinute", speedPerMinute.toFloat())
            putFloat("rpmPerMinute", rpmPerMinute.toFloat())
            apply() // Asynchronously save changes
        }
    }
    private fun getSpeedRPMFromPreferences(context: Context): Pair<Double, Double> {
        val sharedPreferences: SharedPreferences = context.getSharedPreferences("driving_data", Context.MODE_PRIVATE)
        val speedPerMinute = sharedPreferences.getFloat("speedPerMinute", 0.0f).toDouble()
        val rpmPerMinute = sharedPreferences.getFloat("rpmPerMinute", 0.0f).toDouble()
        return Pair(speedPerMinute, rpmPerMinute)
    }
    private fun saveCumulativeSpeedSum() {
        with(sharedPreferences.edit()) {
            putString("cumulativeSpeedSum", cumulativeSpeedSum.toString())
            apply()
        }
    }

    private fun saveSpeedEntryCount() {
        with(sharedPreferences.edit()) {
            putInt("speedEntryCount", speedEntryCount)
            apply()
        }
    }
    private fun calculateSafeDrivingScore(
        fastAcceleration: Int, suddenDeparture: Int, rapidDeceleration: Int,
        suddenStop: Int, radicalCourseChange: Int, overtakingSpeed: Int,
        sharpTurns: Int, suddenUTurn: Int, continuousOperation: Int,
        distanceDriven: Double
    ): Double {
        Log.d("SafeDrivingScore", "calculateSafeDrivingScore function called")

        val totalEvents = fastAcceleration + suddenDeparture + rapidDeceleration +
                suddenStop + radicalCourseChange + overtakingSpeed +
                sharpTurns + suddenUTurn + continuousOperation

        if (totalEvents == 0) return 100.0 // Return full score if no events are recorded

        val weightedSum = (fastAcceleration * WEIGHT_FAST_ACCELERATION +
                suddenDeparture * WEIGHT_SUDDEN_DEPARTURE +
                rapidDeceleration * WEIGHT_RAPID_DECELERATION +
                suddenStop * WEIGHT_SUDDEN_STOP +
                radicalCourseChange * WEIGHT_RADICAL_COURSE_CHANGE +
                overtakingSpeed * WEIGHT_OVERTAKING_SPEED +
                sharpTurns * WEIGHT_SHARP_TURNS +
                suddenUTurn * WEIGHT_SUDDEN_U_TURN +
                continuousOperation * WEIGHT_CONTINUOUS_OPERATION)
        println("weightedSum===$weightedSum")

        val adjustedDistance = Math.max(distanceDriven, 1.0)
        val scoreReduction = (weightedSum / totalEvents) * (totalEvents / adjustedDistance) * 10
        println("scoreReduction===$scoreReduction")

        val safeDrivingScore = (100 - scoreReduction).coerceIn(0.0, 100.0)
        println("safeDrivingScore===$safeDrivingScore")
        return safeDrivingScore.coerceIn(0.0, 100.0) // Ensure score stays within 0-100 range
    }

    private fun calculateWeeklyAverageScore(
        currentWeekScores: MutableList<Double>, previousWeekScores: MutableList<Double>,
        currentWeekCounts: MutableList<Int>, previousWeekCounts: MutableList<Int>
    ): Double {
        // Calculate the average score for both weeks
        val currentWeekTotalScore = currentWeekScores.zip(currentWeekCounts) { score, count -> if (count > 0) score / count else 0.0 }.sum()
        val previousWeekTotalScore = previousWeekScores.zip(previousWeekCounts) { score, count -> if (count > 0) score / count else 0.0 }.sum()

        // Calculate the average score across both weeks
        val totalScores = currentWeekTotalScore + previousWeekTotalScore
        val totalWeeks = currentWeekCounts.sum() + previousWeekCounts.sum()
        println("totalScores===$totalScores")
        println("totalWeeks===$totalWeeks")

        val averageScore = if (totalWeeks > 0) {
            (totalScores / totalWeeks).coerceIn(0.0, 100.0) // Ensure score stays within 0-100 range
        } else {
            0.0 // Return 0 if there are no valid scores
        }
        saveWeekScoreToPreferences(averageScore, "current_week_total_score")
        saveWeekScoreToPreferences(previousWeekTotalScore, "previous_week_total_score")
        return averageScore
    }
    private fun saveWeekScoreToPreferences(score: Double, key: String) {
        val sharedPreferences = context?.getSharedPreferences("week_scores_prefs", Context.MODE_PRIVATE)
        sharedPreferences?.edit()?.apply {
            putString(key, score.toString())
            apply()
        }
    }
    private fun calculateMonthlyAverageScore(
        currentMonthDaysSafeScore: MutableList<MutableList<Float>>,
        previousMonthDaysSafeScore: MutableList<MutableList<Float>>
    ): Float {
        // Flatten current and previous month scores
        val currentMonthScores = currentMonthDaysSafeScore.flatten()
        val previousMonthScores = previousMonthDaysSafeScore.flatten()

        // Calculate totals and valid day counts
        val currentMonthTotalScore = currentMonthScores.sum()
        val currentMonthDayCount = currentMonthScores.count { it > 0.0 }

        val previousMonthTotalScore = previousMonthScores.sum()
        val previousMonthDayCount = previousMonthScores.count { it > 0.0 }

        // Total scores and days
        val totalScores = currentMonthTotalScore + previousMonthTotalScore
        val totalDays = currentMonthDayCount + previousMonthDayCount

        Log.d("MonthlyAverage", "currentMonthTotalScore: $currentMonthTotalScore")
        Log.d("MonthlyAverage", "previousMonthTotalScore: $previousMonthTotalScore")
        Log.d("MonthlyAverage", "totalScores: $totalScores")
        Log.d("MonthlyAverage", "totalDays: $totalDays")

        // Calculate average score (coerce to 0-100 range)
        val averageScore = if (totalDays > 0) {
            (totalScores / totalDays).coerceIn(0.0F, 100.0F)
        } else {
            0.0F // No valid days
        }

        // Save scores to preferences
        saveMonthScoreToPreferences(averageScore, "current_month_total_score")
        saveMonthScoreToPreferences(previousMonthTotalScore, "previous_month_total_score")

        return averageScore
    }

    private fun saveMonthScoreToPreferences(score: Float, key: String) {
        with(sharedPreferences.edit()) {
            putString(key, score.toString())
            apply()
        }
    }
    private fun updateUIWithScore() {
        val gson = Gson()

        // Fetch the current week scores and convert to List<Float>
        val currentWeekScoresJson = sharedPreferences?.getString("current_week_scores", null)
        val currentWeekScores = gson.fromJson(currentWeekScoresJson, Array<Float>::class.java)?.toList() ?: emptyList()

        // Filter out zero or negative scores and calculate the average
        val validWeekScores = currentWeekScores.filter { it > 0.0 }

        val averageWeekScore = if (validWeekScores.isNotEmpty()) {
            validWeekScores.sum() / validWeekScores.size.toFloat()
        } else {
            0.0f
        }

        // Update UI with the average score
        binding.tvProgress.text = String.format("%.1f", averageWeekScore.coerceIn(0f, 100f))
        binding.pgScore.progress = averageWeekScore.coerceIn(0f, 100f)

        println("averageWeekScore===$averageWeekScore")

        // Display score message based on the average score
        binding.tvScoreMsg.text = when {
            averageWeekScore >= 80 -> "You're doing great!"
            averageWeekScore >= 50 -> "Good, but there's room for improvement."
            else -> "Please drive more cautiously."
        }
    }

    @RequiresApi(Build.VERSION_CODES.O)
    private fun updateUIWithMonthlyScore() {
        val gson = Gson()
        val currentMonthSafeScoreJson = sharedPreferences?.getString("currentmonth_scores", null)
        val currentMonthDaysSafeScoreJson = sharedPreferences?.getString("monthly_daily_safe_scores", null)

        val currentMonthSafeScore = gson.fromJson(currentMonthSafeScoreJson, Array<Float>::class.java)?.toList() ?: emptyList()
        var currentMonthDaysSafeScore = gson.fromJson(currentMonthDaysSafeScoreJson, Array<Array<Float>>::class.java)?.map { it.toList() } ?: emptyList()

        // Flatten the list of lists (days with scores) and filter out zero scores
        val validScores = currentMonthDaysSafeScore.flatten().filter { it > 0.0 }

        // Calculate average score only over valid scores
        val averageScore = if (validScores.isNotEmpty()) {
            validScores.sum() / validScores.size.toFloat()
        } else {
            0.0f
        }

        binding.tvProgress.text = String.format("%.1f", averageScore)
        binding.pgScore.progress = averageScore.coerceIn(0f, 100f)

        println("current_month_total_score>==>$averageScore")
        Log.d("Monthly Data", currentMonthSafeScore.toString())

        // Display score message based on the average score
        binding.tvScoreMsg.text = when {
            averageScore >= 80 -> "You're doing great!"
            averageScore >= 50 -> "Good, but there's room for improvement."
            else -> "Please drive more cautiously."
        }

        if (isCurrentMonthCompleted()) {
            // Transfer current month averages to previous month scores
            previousMonthSafeScore = currentMonthDaysSafeScore.map { dayScores ->
                if (dayScores.isNotEmpty()) dayScores.average().toFloat() else 0f
            }.toMutableList()

            Log.d("Previous Month Data After Transfer", previousMonthSafeScore.toString())

            // Clear current month's scores without reassigning the reference
            currentMonthDaysSafeScore = MutableList(31) { mutableListOf() }
        }
    }


    @RequiresApi(Build.VERSION_CODES.O)
    private fun getMonthlySafeScores() {
        val sharedPreferences = requireContext().getSharedPreferences("SafeScorePrefs", Context.MODE_PRIVATE)
        val daysInMonth = YearMonth.now().lengthOfMonth()

        // Retrieve the current and previous month's total scores as a float (you might need to use a default value here)
        val currentMonthScore = sharedPreferences.getFloat("current_month_safe_score", 0f)
        val previousMonthScore = sharedPreferences.getFloat("previous_month_safe_score", 0f)
        currentMonthSafeScore = MutableList(daysInMonth) { currentMonthScore } // Assuming the same score for each day, or you can adjust this logic
        previousMonthSafeScore = MutableList(daysInMonth) { previousMonthScore }

        // Retrieve daily scores for the current and previous month
        val gson = Gson()

        val currentMonthJson = sharedPreferences.getString("current_month_days_safe_score", "[]")
        currentMonthDaysSafeScore = gson.fromJson(currentMonthJson, object : TypeToken<MutableList<MutableList<Float>>>() {}.type) ?: MutableList(daysInMonth) { mutableListOf() }

        val previousMonthJson = sharedPreferences.getString("previous_month_days_safe_score", "[]")
        previousMonthDaysSafeScore = gson.fromJson(previousMonthJson, object : TypeToken<MutableList<MutableList<Float>>>() {}.type) ?: MutableList(daysInMonth) { mutableListOf() }

        println("Current Month Safe Score: $currentMonthSafeScore")
        println("Previous Month Safe Score: $previousMonthSafeScore")
        println("Current Month Daily Safe Scores: ${currentMonthDaysSafeScore.joinToString()}")
        println("Previous Month Daily Safe Scores: ${previousMonthDaysSafeScore.joinToString()}")
    }

    @RequiresApi(Build.VERSION_CODES.O)
    private fun resetMonthlyScoresIfNeeded() {
        val sharedPreferences = requireContext().getSharedPreferences("SafeScorePrefs", Context.MODE_PRIVATE)
        val lastUpdatedMonth = sharedPreferences.getInt("last_updated_month", -1)
        val currentMonth = LocalDateTime.now().monthValue
        val daysInMonth = YearMonth.now().lengthOfMonth()

        if (lastUpdatedMonth != currentMonth) {
            // Move current month score to previous month and reset current month score
            previousMonthSafeScore = currentMonthSafeScore.toMutableList() // Copy the list
            previousMonthDaysSafeScore = currentMonthDaysSafeScore.map { it.toMutableList() }.toMutableList()

            // Reset daily scores
            currentMonthDaysSafeScore = MutableList(daysInMonth) { mutableListOf() }
            currentMonthDaysSafeScore = MutableList(daysInMonth) { mutableListOf() }

            // Update the "last updated month" in SharedPreferences
            with(sharedPreferences.edit()) {
                putInt("last_updated_month", currentMonth)
                apply()
            }
        }
    }
    private fun saveMonthlySafeScores() {
        val sharedPreferences = requireContext().getSharedPreferences("SafeScorePrefs", Context.MODE_PRIVATE)
        val gson = Gson()
        with(sharedPreferences.edit()) {
            // Save the total scores
            putFloat("current_month_safe_score", currentMonthSafeScore.firstOrNull() ?: 0f)
            putFloat("previous_month_safe_score", previousMonthSafeScore.firstOrNull() ?: 0f)

            // Save daily scores as JSON strings
            putString("current_month_days_safe_score", gson.toJson(currentMonthDaysSafeScore))
            putString("previous_month_days_safe_score", gson.toJson(previousMonthDaysSafeScore))
            apply()
        }
    }


    @RequiresApi(Build.VERSION_CODES.O)
    private fun loadSpeedRPMDataFromPreferences(): Pair<List<Pair<LocalDateTime, Double>>, List<Pair<LocalDateTime, Int>>> {
        val speedDataStrings = sharedPreferences.getStringSet("speed_data", emptySet()) ?: emptySet()
        val rpmDataStrings = sharedPreferences.getStringSet("rpm_data", emptySet()) ?: emptySet()
        println("rpmdataString is :$rpmDataStrings")

        val speedData = speedDataStrings.mapNotNull {
            try {
                val parts = it.split(",")
                // Attempt to parse the datetime with the formatter
                val dateTime = LocalDateTime.parse(parts[0].trim(), formatter)
                Pair(dateTime, parts[1].toDouble())  // If successful, return a Pair
            } catch (e: DateTimeParseException) {
                // Log invalid data and skip it
                Log.e("DashboardFragment", "Invalid speed data: $it", e)
                null  // Return null if parsing fails
            }
        }

        val rpmData = rpmDataStrings.mapNotNull {
            try {
                val parts = it.split(",")
                // Attempt to parse the datetime with the formatter
                val dateTime = LocalDateTime.parse(parts[0].trim(), formatter)
                Pair(dateTime, parts[1].toInt())  // If successful, return a Pair
            } catch (e: DateTimeParseException) {
                // Log invalid data and skip it
                Log.e("DashboardFragment", "Invalid rpm data: $it", e)
                null  // Return null if parsing fails
            }
        }

        return Pair(speedData, rpmData)
    }

//    @RequiresApi(Build.VERSION_CODES.O)
//    private fun saveSpeedRPMDataToPreferences(
//        speedData: MutableList<Pair<LocalDateTime, Double>>,
//        rpmData: MutableList<Pair<LocalDateTime, Int>>
//    ) {
//        val speedDataStrings = speedData.map { pair -> "${pair.first.format(formatter)},${pair.second}" } // Use formatter to format the datetime
//        val rpmDataStrings = rpmData.map { pair -> "${pair.first.format(formatter)},${pair.second}" }
//
//        with(sharedPreferences.edit()) {
//            putStringSet("speed_data", speedDataStrings.toSet())  // Save as Set to SharedPreferences
//            putStringSet("rpm_data", rpmDataStrings.toSet())
//            apply()
//        }
//    }
    @RequiresApi(Build.VERSION_CODES.O)
    private fun saveSpeedRPMDataToPreferences(
        speedData: MutableList<Pair<LocalDateTime, Double>>,
        rpmData: MutableList<Pair<LocalDateTime, Int>>
    ) {
        // Get existing data from SharedPreferences
        val existingSpeedData = sharedPreferences.getStringSet("speed_data", emptySet()) ?: emptySet()
        val existingRPMData = sharedPreferences.getStringSet("rpm_data", emptySet()) ?: emptySet()

        // Map new data to strings
        val newSpeedDataStrings = speedData.map { pair -> "${pair.first.format(formatter)},${pair.second}" }
        val newRPMDataStrings = rpmData.map { pair -> "${pair.first.format(formatter)},${pair.second}" }

        // Combine existing data with new data
        val updatedSpeedData = existingSpeedData + newSpeedDataStrings
        val updatedRPMData = existingRPMData + newRPMDataStrings

        // Save the updated data back to SharedPreferences
        with(sharedPreferences.edit()) {
            putStringSet("speed_data", updatedSpeedData.toSet())
            putStringSet("rpm_data", updatedRPMData.toSet())
            apply()
        }
    }


    data class RowData(
        val time: LocalDateTime,
        val trip: String,
        val speed: Double,
        val rpm: Int,
        val gpsLatitude: Double,
        val gpsLongitude: Double
    )

    data class WeeklyScore(
        val weekNumber: Int,
        var score: Double
    )


//    @SuppressLint("NewApi")
//    fun createCsvFile() {
//        val downloadsDirectory = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)
//        val csvFile = File(downloadsDirectory, "526549524525222.csv")
//
//        try {
//            FileOutputStream(csvFile).use { fos ->
//                OutputStreamWriter(fos).use { writer ->
//                    // Write the header
//                    writer.append("Time,Trip,VINCODE,VehicleType,Speed,RPM,GPS_Latitude,Lat_Direction,GPS_Longitude,Lon_Direction,Heading,VehicleStatus\n")
//
//                    val now = LocalDateTime.now()
//                    val previousWeekStart = now.with(DayOfWeek.MONDAY).minusWeeks(1)
//                    val previousWeekEnd = now.with(DayOfWeek.SUNDAY).minusWeeks(1)
//                    val currentWeekStart = now.with(DayOfWeek.MONDAY)
//                    val currentWeekEnd = now.with(DayOfWeek.SUNDAY)
//
//                    val vehicleStatuses = listOf(
//                        "Fast Acceleration", "Sudden Departure", "Rapid Deceleration", "Sudden Stop",
//                        "Radical Course Change", "Overtaking Speed", "Sharp Turns",
//                        "Sudden U-Turn", "Continuous Operation"
//                    )
//                    val trip = "00134"
//                    val vinCode = "TRADITIONAL"
//                    val vehicleType = "1HGCM82633A123456"
//                    val baseLatitude = 62.75
//                    val baseLongitude = 12.0
//                    // Generate data for the current week
//                    for (i in 0 until 5) {
//                        val day = currentWeekStart.plusDays(i.toLong())
//                        val randomSeconds = (0..ChronoUnit.SECONDS.between(currentWeekStart, currentWeekEnd)).random()
//                        val randomDateTime = day.plusSeconds(randomSeconds)
//
//                        val currentLatitude = baseLatitude + (i * 0.0001)
//                        val currentLongitude = baseLongitude + (i * 0.0001)
//                        val latDirection = if (currentLatitude >= 0) "N" else "S"
//                        val lonDirection = if (currentLongitude >= 0) "E" else "W"
//                        val speed = Random.nextInt(40, 71)
//                        val rpm = Random.nextInt(800, 3501)
//                        val vehicleStatus = vehicleStatuses.random()
//
//                        writer.append(
//                            "${randomDateTime.toString()},$trip,$vinCode,$vehicleType,$speed,$rpm,$currentLatitude,$latDirection,$currentLongitude,$lonDirection,0,$vehicleStatus\n"
//                        )
//                    }
//                }
//            }
//            println("CSV file created: ${csvFile.absolutePath}")
//        } catch (e: Exception) {
//            e.printStackTrace()
//            println("Error creating CSV file: ${e.message}")
//        }
//    }
//@RequiresApi(Build.VERSION_CODES.O)
//fun createCsvFile() {
//        // Ensure you have permission to write to external storage (requires Android permission handling)
//        val downloadsDirectory = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)
//        val csvFile = File(downloadsDirectory, "52652333900006.csv")
//
//        try {
//            FileOutputStream(csvFile).use { fos ->
//                OutputStreamWriter(fos).use { writer ->
//                    // Write the header
//                    writer.append("Time,Trip,VINCODE,VehicleType,Speed,RPM,GPS_Latitude,Lat_Direction,GPS_Longitude,Lon_Direction,Heading,VehicleStatus\n")
//
//                    val now = LocalDateTime.now()
//                    val currentWeekStart = now.with(DayOfWeek.MONDAY)
//                    val currentWeekEnd = now.with(DayOfWeek.SUNDAY)
//
//                    val vehicleStatuses = listOf(
//                        "Fast Acceleration", "Sudden Departure", "Rapid Deceleration", "Sudden Stop",
//                        "Radical Course Change", "Overtaking Speed", "Sharp Turns",
//                        "Sudden U-Turn", "Continuous Operation"
//                    )
//                    val trip = "00134"
//                    val vinCode = "TRADITIONAL"
//                    val vehicleType = "1HGCM82633A123456"
//                    val baseLatitude = 62.75
//                    val baseLongitude = 12.0
//
//                    // Generate data for the current week
//                    for (i in 0 until 5) {
//                        val day = currentWeekStart.plusDays(i.toLong())
//                        val randomSeconds = (0..ChronoUnit.SECONDS.between(currentWeekStart, currentWeekEnd)).random()
//                        val randomDateTime = day.plusSeconds(randomSeconds)
//
//                        val currentLatitude = baseLatitude + (i * 0.0001)
//                        val currentLongitude = baseLongitude + (i * 0.0001)
//                        val latDirection = if (currentLatitude >= 0) "N" else "S"
//                        val lonDirection = if (currentLongitude >= 0) "E" else "W"
//                        val speed = Random.nextInt(40, 71)
//                        val rpm = Random.nextInt(800, 3501)
//                        val vehicleStatus = vehicleStatuses.random()
//
//                        // Ensure proper CSV format with quotes and commas inside data
//                        writer.append(
//                            "\"${randomDateTime.toString()}\",\"$trip\",\"$vinCode\",\"$vehicleType\",$speed,$rpm,$currentLatitude,\"$latDirection\",$currentLongitude,\"$lonDirection\",0,\"$vehicleStatus\"\n"
//                        )
//                    }
//                }
//            }
//            println("CSV file created: ${csvFile.absolutePath}")
//        } catch (e: Exception) {
//            e.printStackTrace()
//            println("Error creating CSV file: ${e.message}")
//        }
//    }

    //    @RequiresApi(Build.VERSION_CODES.O)
//    fun createCsvFile() {
//        val downloadsDirectory = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)
//        val csvFile = File(downloadsDirectory, "53399988922222222233.csv")
//
//        try {
//            FileOutputStream(csvFile).use { fos ->
//                OutputStreamWriter(fos).use { writer ->
//                    // Write the header
//                    writer.append("Time,Trip,VINCODE,VehicleType,Speed,RPM,GPS_Latitude,Lat_Direction,GPS_Longitude,Lon_Direction,Heading,VehicleStatus\n")
//
//                    val now = LocalDateTime.now()
//                    val currentWeekStart = now.with(DayOfWeek.MONDAY)
//                    val currentWeekEnd = now.with(DayOfWeek.SUNDAY)
//
//                    val vehicleStatuses = listOf(
//                        "Rapid Deceleration",
//                        "Continuous Operation"
//                    )
////                    val vehicleStatuses = listOf(
////                        "Fast Acceleration", "Sudden Departure", "Rapid Deceleration", "Sudden Stop",
////                        "Radical Course Change", "Overtaking Speed", "Sharp Turns",
////                        "Sudden U-Turn", "Continuous Operation"
////                    )
//
//                    val trip = "00134"
//                    val vinCode = "TRADITIONAL"
//                    val vehicleType = "1HGCM82633A123456333"
//
//                    // Define origin and destination coordinates
//                    val originLatitude = 11.2588  // New York
//                    val originLongitude = 75.7804
//                    val destinationLatitude = 10.7769  // Los Angeles
//                    val destinationLongitude = 76.6540
//
//                    // Calculate the bearing (heading) from origin to destination
//                    val bearing = calculateBearing(originLatitude, originLongitude, destinationLatitude, destinationLongitude)
//
//                    // Generate data for the current week (travel from origin to destination)
//                    for (i in 0 until 5) {
//                        val day = currentWeekStart.plusDays(i.toLong())
//                        val randomSeconds = (0..ChronoUnit.SECONDS.between(currentWeekStart, currentWeekEnd)).random()
//                        val randomDateTime = day.plusSeconds(randomSeconds)
//
//                        // Calculate intermediate latitude and longitude for travel
//                        val currentLatitude = originLatitude + (i * (destinationLatitude - originLatitude) / 5.0)
//                        val currentLongitude = originLongitude + (i * (destinationLongitude - originLongitude) / 5.0)
//
//                        // Determine the direction
//                        val latDirection = if (currentLatitude >= 0) "N" else "S"
//                        val lonDirection = if (currentLongitude >= 0) "E" else "W"
//
//                        val speed = Random.nextInt(40, 71)
//                        val rpm = Random.nextInt(800, 3501)
//                        val vehicleStatus = vehicleStatuses.random()
//
//                        // Append data to the CSV
//                        writer.append(
//                            "${randomDateTime.toString()},$trip,$vinCode,$vehicleType,$speed,$rpm,$currentLatitude,$latDirection,$currentLongitude,$lonDirection,$bearing,$vehicleStatus\n"
//                        )
//                    }
//                }
//            }
//            println("CSV file created: ${csvFile.absolutePath}")
//        } catch (e: Exception) {
//            e.printStackTrace()
//            println("Error creating CSV file: ${e.message}")
//        }
//    }
    @RequiresApi(Build.VERSION_CODES.O)
    fun createCsvFile() {
        val downloadsDirectory = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)
        val csvFile = File(downloadsDirectory, "042112721480.csv")

        try {
            FileOutputStream(csvFile).use { fos ->
                OutputStreamWriter(fos).use { writer ->
                    // Write the header
                    writer.append("Time,Trip,VINCODE,VehicleType,Speed,RPM,GPS_Latitude,Lat_Direction,GPS_Longitude,Lon_Direction,Heading,VehicleStatus\n")

                    val data = generateData()
                    val trip = "00134"
                    val vinCode = "TRADITIONAL"
                    val vehicleType = "1HGCM82633A123456333"
                    val vehicleStatuses = listOf(NULL, NULL, "Rapid Deceleration", "Rapid Deceleration", "Rapid Deceleration", "Rapid Deceleration", "Continuous Operation", NULL, NULL, NULL, NULL)

                    data.forEach { (timestamp, coordinates, speed) ->
                        val (latitude, longitude) = coordinates

                        // Ensure rpm is a Double
                        val rpm = (800..3500).random().toDouble()  // Convert to Double
                        // Ensure heading is a Double
                        val heading = (0..360).random().toDouble()  // Convert to Double
                        val vehicleStatus = vehicleStatuses.random()

                        val latDirection = if (latitude >= 0) "N" else "S"
                        val lonDirection = if (longitude >= 0) "E" else "W"

                        // Write row to CSV
                        writer.append("$timestamp,$trip,$vinCode,$vehicleType,$speed,$rpm,$latitude,$latDirection,$longitude,$lonDirection,$heading,$vehicleStatus\n")
                    }
                }
            }
            println("CSV file created: ${csvFile.absolutePath}")
            readCsvFiles()
        } catch (e: Exception) {
            e.printStackTrace()
            println("Error creating CSV file: ${e.message}")
        }
    }
    fun generateData(): List<Triple<String, Pair<Double, Double>, Any>> {
        return listOf(
            Triple("27-11-2024 21:46", 10.96691.toDouble() to 76.27502.toDouble(), 3.52.toDouble()),
            Triple("27-11-2024 21:46", 10.96695 to 76.27502, 4.42),
            Triple("27-11-2024 21:46", 10.96697 to 76.27503, 4.04),
            Triple("27-11-2024 21:46", 10.96699 to 76.27504, 3.96),
            Triple("27-11-2024 21:46", 10.967 to 76.27505, 2.84),
            Triple("27-11-2024 21:46", 10.96701 to 76.27505, 2.26),
            Triple("27-11-2024 21:46", 10.96701 to 76.27505, 0.57),
            Triple("27-11-2024 21:46", 10.96701 to 76.27506, 0.13),
            Triple("27-11-2024 21:46", 10.96701 to 76.27506, 0.25),
            Triple("27-11-2024 21:46", 10.96701 to 76.27505, 0.36),
            Triple("27-11-2024 21:46", 10.96702 to 76.27505, 0.78),
            Triple("27-11-2024 21:46", 10.96702 to 76.27505, 0.49),
            Triple("27-11-2024 21:46", 10.96702 to 76.27506, 0.67),
            Triple("27-11-2024 21:46", 10.96702 to 76.27506, 2.25),
            Triple("27-11-2024 21:47", 10.96702 to 76.27506, 1.93),
            Triple("27-11-2024 21:47", 10.96702 to 76.27507, 0.68),
            Triple("27-11-2024 21:47", 10.96703 to 76.27507, 0.16),
            Triple("27-11-2024 21:47", 10.96703 to 76.27507, 0.4),
            Triple("27-11-2024 21:47", 10.96703 to 76.27507, 0.73),
            Triple("27-11-2024 21:47", 10.96704 to 76.27508, 4.11),
            Triple("27-11-2024 21:47", 10.96704 to 76.2751, 8.63),
            Triple("27-11-2024 21:47", 10.96705 to 76.27512, 10.36),
            Triple("27-11-2024 21:47", 10.96706 to 76.27515, 11.06),
            Triple("27-11-2024 21:47", 10.96705 to 76.27518, 12.97),
            Triple("27-11-2024 21:47", 10.96704 to 76.27522, 14.33),
            Triple("27-11-2024 21:47", 10.96704 to 76.27525, 15.96),
            Triple("27-11-2024 21:47", 10.96703 to 76.27529, 16.05),
            Triple("27-11-2024 21:47", 10.96702 to 76.27534, 16.78),
            Triple("27-11-2024 21:47", 10.967 to 76.27538, 17.51),
            Triple("27-11-2024 21:47", 10.96699 to 76.27542, 17.97),
            Triple("27-11-2024 21:47", 10.96697 to 76.27547, 18.34),
            Triple("27-11-2024 21:47", 10.96696 to 76.27551, 17.96),
            Triple("27-11-2024 21:47", 10.96694 to 76.27554, 17.74),
            Triple("27-11-2024 21:47", 10.96693 to 76.2756, 17.62),
            Triple("27-11-2024 21:47", 10.96692 to 76.27563, 17.01),
            Triple("27-11-2024 21:47", 10.96691 to 76.27567, 16.28),
            Triple("27-11-2024 21:47", 10.96689 to 76.27572, 17.58),
            Triple("27-11-2024 21:47", 10.96688 to 76.27576, 19.0),
            Triple("27-11-2024 21:47", 10.96687 to 76.2758, 20.7),
            Triple("27-11-2024 21:47", 10.96686 to 76.27586, 22.05),
            Triple("27-11-2024 21:47", 10.96685 to 76.27592, 23.41),
            Triple("27-11-2024 21:47", 10.96684 to 76.27599, 24.49),
            Triple("27-11-2024 21:47", 10.96684 to 76.27605, 25.3),
            Triple("27-11-2024 21:47", 10.96683 to 76.27612, 26.23),
            Triple("27-11-2024 21:47", 10.96683 to 76.27618, 27.05),
            Triple("27-11-2024 21:47", 10.96682 to 76.27625, 29.44),
            Triple("27-11-2024 21:47", 10.96682 to 76.27634, 32.76),
            Triple("27-11-2024 21:47", 10.96682 to 76.27642, 35.78),
            Triple("27-11-2024 21:47", 10.96682 to 76.27653, 39.61),
            Triple("27-11-2024 21:47", 10.96684 to 76.27663, 42.51),
            Triple("27-11-2024 21:47", 10.96686 to 76.27674, 42.66),
            Triple("27-11-2024 21:47", 10.96688 to 76.27685, 43.77),
            Triple("27-11-2024 21:47", 10.9669 to 76.27695, 42.87),
            Triple("27-11-2024 21:47", 10.96692 to 76.27707, 43.57),
            Triple("27-11-2024 21:47", 10.96694 to 76.27718, 43.96),
            Triple("27-11-2024 21:47", 10.96696 to 76.27728, 43.63),
            Triple("27-11-2024 21:47", 10.96698 to 76.27739, 43.49),
            Triple("27-11-2024 21:47", 10.967 to 76.2775, 43.02),
            Triple("27-11-2024 21:47", 10.96702 to 76.2776, 42.71),
            Triple("27-11-2024 21:47", 10.96704 to 76.27771, 41.76),
            Triple("27-11-2024 21:47", 10.96705 to 76.27782, 41.53),
            Triple("27-11-2024 21:47", 10.96706 to 76.27792, 40.85),
            Triple("27-11-2024 21:47", 10.96706 to 76.27803, 40.6),
            Triple("27-11-2024 21:47", 10.96706 to 76.27813, 40.84),
            Triple("27-11-2024 21:47", 10.96706 to 76.27824, 41.66),
            Triple("27-11-2024 21:47", 10.96705 to 76.27834, 41.76),
            Triple("27-11-2024 21:47", 10.96704 to 76.27845, 42.35),
            Triple("27-11-2024 21:47", 10.96702 to 76.27856, 43.38),
            Triple("27-11-2024 21:47", 10.96701 to 76.27867, 45.49),
            Triple("27-11-2024 21:47", 10.96699 to 76.27879, 47.46),
            Triple("27-11-2024 21:47", 10.96697 to 76.27892, 49.4),
            Triple("27-11-2024 21:47", 10.96696 to 76.27904, 50.68),
            Triple("27-11-2024 21:47", 10.96694 to 76.27917, 52.33),
            Triple("27-11-2024 21:47", 10.96692 to 76.2793, 53.46),
            Triple("27-11-2024 21:48", 10.9669 to 76.27944, 54.86),
            Triple("27-11-2024 21:48", 10.96687 to 76.27957, 52.54),
            Triple("27-11-2024 21:48", 10.96685 to 76.2797, 50.13),
            Triple("27-11-2024 21:48", 10.96682 to 76.27982, 48.0),
            Triple("27-11-2024 21:48", 10.9668 to 76.27994, 45.86),
            Triple("27-11-2024 21:48", 10.96676 to 76.28004, 44.55),
            Triple("27-11-2024 21:48", 10.96673 to 76.28015, 43.57),
            Triple("27-11-2024 21:48", 10.96669 to 76.28026, 41.96),
            Triple("27-11-2024 21:48", 10.96665 to 76.28035, 38.65),
            Triple("27-11-2024 21:48", 10.96662 to 76.28043, 34.77),
            Triple("27-11-2024 21:48", 10.96658 to 76.28051, 33.4),
            Triple("27-11-2024 21:48", 10.96655 to 76.28059, 32.13),
            Triple("27-11-2024 21:48", 10.96651 to 76.28065, 30.22),
            Triple("27-11-2024 21:48", 10.96648 to 76.28072, 30.79),
            Triple("27-11-2024 21:48", 10.96644 to 76.28079, 31.19),
            Triple("27-11-2024 21:48", 10.96641 to 76.28087, 31.4),
            Triple("27-11-2024 21:48", 10.96638 to 76.28094, 31.46),
            Triple("27-11-2024 21:48", 10.96634 to 76.28101, 31.42),
            Triple("27-11-2024 21:48", 10.96631 to 76.2811, 32.32),
            Triple("27-11-2024 21:48", 10.96628 to 76.28116, 29.41),
            Triple("27-11-2024 21:48", 10.96624 to 76.28123, 28.07),
            Triple("27-11-2024 21:48", 10.96621 to 76.28128, 27.81),
            Triple("27-11-2024 21:48", 10.96618 to 76.28136, 27.77),
            Triple("27-11-2024 21:48", 10.96615 to 76.28141, 27.81),
            Triple("27-11-2024 21:48", 10.96612 to 76.28148, 28.13),
            Triple("27-11-2024 21:48", 10.96609 to 76.28154, 27.87),
            Triple("27-11-2024 21:48", 10.96606 to 76.28161, 27.65),
            Triple("27-11-2024 21:48", 10.96603 to 76.28167, 27.63),
            Triple("27-11-2024 21:48", 10.966 to 76.28173, 27.26),
            Triple("27-11-2024 21:48", 10.96597 to 76.28179, 27.21),
            Triple("27-11-2024 21:48", 10.96594 to 76.28186, 27.12),
            Triple("27-11-2024 21:48", 10.96592 to 76.28193, 26.86),
            Triple("27-11-2024 21:48", 10.96589 to 76.28198, 26.76),
            Triple("27-11-2024 21:48", 10.96587 to 76.28205, 27.13),
            Triple("27-11-2024 21:48", 10.96585 to 76.28211, 27.51),
            Triple("27-11-2024 21:48", 10.96582 to 76.28218, 27.86),
            Triple("27-11-2024 21:48", 10.9658 to 76.28224, 28.02),
            Triple("27-11-2024 21:48", 10.96577 to 76.28232, 28.53),
            Triple("27-11-2024 21:48", 10.96575 to 76.28238, 29.07),
            Triple("27-11-2024 21:48", 10.96572 to 76.28246, 30.39),
            Triple("27-11-2024 21:48", 10.9657 to 76.28252, 30.67),
            Triple("27-11-2024 21:48", 10.96568 to 76.2826, 30.94),
            Triple("27-11-2024 21:48", 10.96565 to 76.28268, 32.66),
            Triple("27-11-2024 21:48", 10.96563 to 76.28276, 34.69),
            Triple("27-11-2024 21:48", 10.96561 to 76.28284, 35.21),
            Triple("27-11-2024 21:48", 10.96558 to 76.28294, 35.22),
            Triple("27-11-2024 21:48", 10.96556 to 76.28303, 35.35),
            Triple("27-11-2024 21:48", 10.96554 to 76.28311, 35.47),
            Triple("27-11-2024 21:48", 10.96552 to 76.2832, 35.5),
            Triple("27-11-2024 21:48", 10.96549 to 76.28329, 35.93),
            Triple("27-11-2024 21:48", 10.96547 to 76.28337, 35.08),
            Triple("27-11-2024 21:48", 10.96545 to 76.28345, 33.31),
            Triple("27-11-2024 21:48", 10.96542 to 76.28354, 33.25),
            Triple("27-11-2024 21:48", 10.9654 to 76.28362, 33.51),
            Triple("27-11-2024 21:48", 10.96537 to 76.2837, 33.73),
            Triple("27-11-2024 21:48", 10.96534 to 76.28378, 34.65),
            Triple("27-11-2024 21:48", 10.9653 to 76.28386, 35.38),
            Triple("27-11-2024 21:48", 10.96527 to 76.28394, 35.9),
            Triple("27-11-2024 21:48", 10.96522 to 76.28403, 36.17),
            Triple("27-11-2024 21:48", 10.96518 to 76.2841, 36.62),
            Triple("27-11-2024 21:49", 10.96513 to 76.28419, 37.24),
            Triple("27-11-2024 21:49", 10.96509 to 76.28427, 38.07),
            Triple("27-11-2024 21:49", 10.96504 to 76.28435, 38.98),
            Triple("27-11-2024 21:49", 10.96498 to 76.28444, 39.7),
            Triple("27-11-2024 21:49", 10.96493 to 76.28452, 40.25),
            Triple("27-11-2024 21:49", 10.96487 to 76.28461, 40.19),
            Triple("27-11-2024 21:49", 10.96481 to 76.28469, 40.58),
            Triple("27-11-2024 21:49", 10.96475 to 76.28478, 41.84),
            Triple("27-11-2024 21:49", 10.96469 to 76.28487, 42.82),
            Triple("27-11-2024 21:49", 10.96463 to 76.28496, 43.3),
            Triple("27-11-2024 21:49", 10.96457 to 76.28505, 42.63),
            Triple("27-11-2024 21:49", 10.96451 to 76.28514, 42.8),
            Triple("27-11-2024 21:49", 10.96445 to 76.28523, 42.56),
            Triple("27-11-2024 21:49", 10.9644 to 76.28532, 41.74),
            Triple("27-11-2024 21:49", 10.96434 to 76.28541, 40.68),
            Triple("27-11-2024 21:49", 10.96429 to 76.2855, 40.06),
            Triple("27-11-2024 21:49", 10.96424 to 76.28559, 39.64),
            Triple("27-11-2024 21:49", 10.96419 to 76.28568, 39.38),
            Triple("27-11-2024 21:49", 10.96414 to 76.28577, 40.03),
            Triple("27-11-2024 21:49", 10.96409 to 76.28586, 40.38),
            Triple("27-11-2024 21:49", 10.96404 to 76.28594, 40.48),
            Triple("27-11-2024 21:49", 10.964 to 76.28603, 40.81),
            Triple("27-11-2024 21:49", 10.96395 to 76.28613, 41.26),
            Triple("27-11-2024 21:49", 10.96391 to 76.28622, 40.96),
            Triple("27-11-2024 21:49", 10.96387 to 76.28632, 40.67),
            Triple("27-11-2024 21:49", 10.96382 to 76.28641, 40.92),
            Triple("27-11-2024 21:49", 10.96378 to 76.28651, 40.57),
            Triple("27-11-2024 21:49", 10.96375 to 76.28661, 39.19),
            Triple("27-11-2024 21:49", 10.96372 to 76.2867, 38.14),
            Triple("27-11-2024 21:49", 10.96369 to 76.28678, 37.08),
            Triple("27-11-2024 21:49", 10.96367 to 76.28687, 36.12),
            Triple("27-11-2024 21:49", 10.96364 to 76.28696, 35.7),
            Triple("27-11-2024 21:49", 10.96361 to 76.28704, 35.23),
            Triple("27-11-2024 21:49", 10.96358 to 76.28713, 36.25),
            Triple("27-11-2024 21:49", 10.96355 to 76.28722, 37.2),
            Triple("27-11-2024 21:49", 10.96352 to 76.28732, 39.1),
            Triple("27-11-2024 21:49", 10.96349 to 76.28741, 39.57),
            Triple("27-11-2024 21:49", 10.96347 to 76.28751, 39.58),
            Triple("27-11-2024 21:49", 10.96344 to 76.28761, 39.54),
            Triple("27-11-2024 21:49", 10.96343 to 76.2877, 38.5),
            Triple("27-11-2024 21:49", 10.96341 to 76.2878, 38.76),
            Triple("27-11-2024 21:49", 10.9634 to 76.2879, 37.58),
            Triple("27-11-2024 21:49", 10.96338 to 76.28799, 36.11),
            Triple("27-11-2024 21:49", 10.96337 to 76.28809, 36.36),
            Triple("27-11-2024 21:49", 10.96335 to 76.28818, 36.4),
            Triple("27-11-2024 21:49", 10.96333 to 76.28826, 36.34),
            Triple("27-11-2024 21:49", 10.9633 to 76.28835, 35.13),
            Triple("27-11-2024 21:49", 10.96327 to 76.28844, 35.71),
            Triple("27-11-2024 21:49", 10.96323 to 76.28852, 35.95),
            Triple("27-11-2024 21:49", 10.96319 to 76.2886, 35.99),
            Triple("27-11-2024 21:49", 10.96315 to 76.28868, 36.1),
            Triple("27-11-2024 21:49", 10.9631 to 76.28876, 36.31),
            Triple("27-11-2024 21:49", 10.96306 to 76.28883, 36.32),
            Triple("27-11-2024 21:49", 10.96301 to 76.28892, 36.69),
            Triple("27-11-2024 21:49", 10.96296 to 76.28899, 36.58),
            Triple("27-11-2024 21:49", 10.96291 to 76.28907, 36.83),
            Triple("27-11-2024 21:49", 10.96286 to 76.28915, 37.7),
            Triple("27-11-2024 21:49", 10.96282 to 76.28924, 38.42),
            Triple("27-11-2024 21:49", 10.96277 to 76.28933, 38.73),
            Triple("27-11-2024 21:49", 10.96273 to 76.28942, 38.77),
            Triple("27-11-2024 21:50", 10.9627 to 76.28951, 39.2),
            Triple("27-11-2024 21:50", 10.96266 to 76.2896, 40.32),
            Triple("27-11-2024 21:50", 10.96261 to 76.2897, 41.68),
            Triple("27-11-2024 21:50", 10.96256 to 76.28979, 42.88),
            Triple("27-11-2024 21:50", 10.96251 to 76.28989, 43.31),
            Triple("27-11-2024 21:50", 10.96244 to 76.28999, 43.4),
            Triple("27-11-2024 21:50", 10.96237 to 76.29006, 43.56),
            Triple("27-11-2024 21:50", 10.96231 to 76.29015, 44),
            Triple("27-11-2024 21:50", 10.96224 to 76.29025, 44.86),
            Triple("27-11-2024 21:50", 10.96218 to 76.29034, 45.38),
            Triple("27-11-2024 21:50", 10.96211 to 76.29044, 45.7),
            Triple("27-11-2024 21:50", 10.96204 to 76.29053, 44.91)
        )
    }

    // Function to calculate the bearing (direction) from the origin to the destination
    fun calculateBearing(lat1: Double, lon1: Double, lat2: Double, lon2: Double): Double {
        val phi1 = Math.toRadians(lat1)
        val phi2 = Math.toRadians(lat2)
        val deltaLambda = Math.toRadians(lon2 - lon1)

        val y = Math.sin(deltaLambda) * Math.cos(phi2)
        val x = Math.cos(phi1) * Math.sin(phi2) - Math.sin(phi1) * Math.cos(phi2) * Math.cos(deltaLambda)

        val bearing = Math.toDegrees(Math.atan2(y, x))

        return (bearing + 360) % 360  // Normalize to a 0-360 degree range
    }
}




