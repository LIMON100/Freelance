package com.skyautonet.drsafe.ui.mydriving.trips

import android.content.Context
import android.content.SharedPreferences
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.drawable.BitmapDrawable
import android.location.Address
import android.location.Geocoder
import android.os.Build
import android.os.Environment
import android.os.FileObserver
import android.preference.PreferenceManager
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import org.osmdroid.views.MapView
import android.view.ViewGroup
import android.widget.TextView
import androidx.annotation.RequiresApi
import androidx.core.content.ContentProviderCompat.requireContext
import androidx.core.content.ContextCompat
import androidx.core.content.res.ResourcesCompat
import androidx.databinding.DataBindingUtil
import androidx.recyclerview.widget.RecyclerView
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.ItemTripDetailsBinding
import org.osmdroid.tileprovider.tilesource.TileSourceFactory
import org.osmdroid.util.GeoPoint
import org.osmdroid.views.overlay.Marker
import org.osmdroid.views.overlay.Polyline
import java.io.BufferedReader
import java.io.File
import java.io.FileReader
import java.text.SimpleDateFormat
import java.util.Locale

@RequiresApi(Build.VERSION_CODES.O)
class TripsAdapter(
    private val context: Context,
    private val sharedPreferences: SharedPreferences
) : RecyclerView.Adapter<TripsAdapter.TripsViewHolder>() {

    private val tripList = mutableListOf<Trip>()
    //private val downloadsDirectory: File =File(context.filesDir, "axon/data")

    private lateinit var fileObserver: FileObserver
    private val tripLocationDataMap = mutableMapOf<String, MutableList<LocationData>>()
    // A map to hold location data for each trip (CSV file)
    val locationDataForFile = mutableListOf<LocationData>()

    private val processedFiles = mutableSetOf<String>()
    private var lastProcessedFile: String? = null
    private var lastProcessedTime: Long = 0
    private var locationData = mutableListOf<LocationData>()
    var previousGpsLatitude :Double = 0.0
    var previousGpsLongitude: Double = 0.0
    private lateinit var marker: Marker
    private lateinit var mapView: MapView
    private var binding : ItemTripDetailsBinding? = null

    init {
        tripList.clear()
        //processedFiles=
        processedFiles.clear()
        val success = sharedPreferences.edit().remove("saved_trip").commit()
        //observeCsvFiles()
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): TripsViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        val binding = ItemTripDetailsBinding.inflate(inflater, parent, false)
        return TripsViewHolder(binding)
    }

    override fun getItemCount(): Int = tripList.size

    override fun onBindViewHolder(holder: TripsViewHolder, position: Int) {
        val trip = tripList[position]
        holder.bind(trip)

        holder.itemView.setOnClickListener {
            // Toggle the isExpanded state for the clicked trip
            tripList[position] = trip.copy(isExpanded = !trip.isExpanded)
            notifyItemChanged(position)  // Notify the adapter that the item has changed
        }

        // Conditionally show or hide the map based on isExpanded state
        if (trip.isExpanded) {
            holder.binding.llDetails.visibility = View.VISIBLE
            holder.binding.mapView.visibility = View.VISIBLE
            setupMapView(holder.binding, trip.tripName)


        } else {
            holder.binding.llDetails.visibility = View.GONE
            holder.binding.mapView.visibility = View.GONE
        }
    }

    @RequiresApi(Build.VERSION_CODES.O)
    fun observeCsvFiles() {
        // Define the path to the "axon data" folder inside the app's internal storage
        val axonDataFolder = File(context.filesDir, "axon/data")
        println("current processed files list is $processedFiles")

        // Check if the folder exists
        if (!axonDataFolder.exists()) {
            Log.e("TripsAdapter", "Axon data folder does not exist: ${axonDataFolder.absolutePath}")
            return
        }

        Log.d("TripsAdapter", "Observing CSV files in directory: ${axonDataFolder.absolutePath}")

        // Read the initial files in the directory
        readCsvFiles()
        val file=File(axonDataFolder.absolutePath)
        // Create a FileObserver for the internal storage directory
        fileObserver = object : FileObserver(file) {
            override fun onEvent(event: Int, path: String?) {
                val currentTime = System.currentTimeMillis()

                // Check if a new CSV file is created and ensure it is within the "axon data" folder
                if (event == FileObserver.CREATE && path?.endsWith(".csv") == true) {
                    // Prevent processing the same file multiple times in a short span
                    if (path == lastProcessedFile && currentTime - lastProcessedTime < 1000) {
                        Log.d("TripsAdapter", "Duplicate event for $path ignored.")
                        return
                    }
                    lastProcessedFile = path
                    lastProcessedTime = currentTime

                    Log.d("TripsAdapter", "New CSV file detected: $path")
                    readCsvFiles() // Read the new CSV file
                }
            }
        }

        // Start observing the folder
        fileObserver.startWatching()
    }

    fun printTrips() {
        println("------- Trip List Debugging -------")
        tripList.forEachIndexed { index, trip ->
            println("Index: $index")
            println("Trip Name: ${trip.tripName}")
            println("Start Location: ${trip.startLocation}")
            println("End Location: ${trip.endLocation}")
            println("Start Time: ${trip.startTime}")
            println("End Time: ${trip.endTime}")
            println("Distance: ${trip.distance}")
            println("Average Speed: ${trip.averageSpeed}")
            println("----------------------------------")
        }
        println("Total Trips: ${tripList.size}")
    }


    fun clearData() {
        tripList.clear()  // Clear the list
        notifyDataSetChanged()  // Notify RecyclerView that the data has changed
        val success = sharedPreferences.edit().remove("saved_trip").commit()
        Log.d("TripsAdapter", "Cleared saved trips. Operation successful: $success")
    }
    @RequiresApi(Build.VERSION_CODES.O)
    private fun readCsvFiles() {
        File(context.filesDir, "axon/data").listFiles()
            ?.filter { it.isFile && it.extension == "csv" }
            ?.sortedByDescending { it.lastModified() }
            ?.take(8)
            ?.forEach {
                if (!processedFiles.contains(it.name)) {
                    Log.d("TripsAdapter", "Processing new file: ${it.name}")
                    processedFiles.add(it.name)
                    readCsvFile(it)
                } else {
                    Log.d("TripsAdapter", "File already processed: ${it.name}")
                }
            }
    }


    @RequiresApi(Build.VERSION_CODES.O)
    private fun readCsvFile(file: File) {
        try {
            var firstLine: String? = null
            var lastLine: String? = null
            val locationData = mutableListOf<LocationData>()
            BufferedReader(FileReader(file)).use { reader ->
                repeat(7) {
                    reader.readLine()
                } // Skip header

                reader.lineSequence().forEachIndexed { index, line ->
                    val data = line.split(",")

                    // Parse GPS data
                    val gpsLatitude = data.getOrNull(7)?.takeIf { it != "NULL" }?.toDoubleOrNull()?.let { it / 1_000_000.0 } ?: previousGpsLatitude
//                    val latDir = data.getOrNull(7)?.takeIf { it.isNotBlank() }
                    val gpsLongitude = data.getOrNull(6)?.takeIf { it != "NULL" }?.toDoubleOrNull()?.let { it / 1_000_000.0 } ?: previousGpsLongitude
//                    val longDir = data.getOrNull(9)?.takeIf { it.isNotBlank() }

                    Log.d("TripsAdapter","gpsLatitude=$gpsLatitude, gpsLongitude=$gpsLongitude, ")

                    // Update global variables
                    if (gpsLatitude != previousGpsLatitude) previousGpsLatitude = gpsLatitude
                    if (gpsLongitude != previousGpsLongitude) previousGpsLongitude = gpsLongitude

                    // Add to location data
                    locationData.add(
                        LocationData(
                            gpsLatitude,
                            gpsLongitude,

                        )
                    )

                    //binding?.let { setupMapView(it) }

                    // Update first and last lines
                    if (index == 0) firstLine = line
                    lastLine = line
                }
            }

            tripLocationDataMap[file.nameWithoutExtension.split("-")[3]] = locationData
            println("Data for file: ${file.nameWithoutExtension}")
//            locationData.forEachIndexed { index, location ->
//                println("Entry $index: Latitude=${location.latitude}, Longitude=${location.longitude}, " )
//            }

            // Process trip if file contains data
            if (firstLine != null && lastLine != null) {
                processTripFromLines(file.nameWithoutExtension.split("-")[3], firstLine!!, lastLine!!)
            } else {
                Log.w("TripsAdapter", "File ${file.name} is empty or malformed.")
            }
        } catch (e: Exception) {
            Log.e("TripsAdapter", "Error reading file ${file.name}: ${e.message}", e)
        }
    }

    data class LocationData(
        val latitude: Double,
        val longitude: Double,

    )
    fun decimalToDMS(decimal: Double): Triple<Double, Double, Double> {
        // Extract degrees as the integer part of the decimal
        val degrees = decimal.toInt().toDouble()

        // Calculate fractional part for minutes
        val fractionalDegrees = decimal - degrees
        val minutes = fractionalDegrees * 60

        // Calculate fractional part for seconds
        val fractionalMinutes = minutes - minutes.toInt()
        val seconds = fractionalMinutes * 60

        return Triple(degrees, minutes, seconds) // Return as a Triple (degrees, minutes, seconds with floating-point precision)
    }
    private fun processTripFromLines(tripName: String, firstLine: String, lastLine: String) {
        val firstValues = firstLine.split(",")
        val lastValues = lastLine.split(",")
        println("First Value Size is ${firstValues.size}")
        println("First Vlaue is $firstLine")
        println("Trip name is : $tripName")
        if (firstValues.size >= 12 && lastValues.size >= 12) {

            val startLatitude = firstValues[7].toLongOrNull()?.let { it / 1_000_000.0 } ?: 0.0
            val startLongitude = firstValues[6].toLongOrNull()?.let { it / 1_000_000.0 } ?: 0.0
            val endLatitude = lastValues[7].toLongOrNull()?.let { it / 1_000_000.0 } ?: 0.0
            val endLongitude = lastValues[6].toLongOrNull()?.let { it / 1_000_000.0 } ?: 0.0

            // Parse the start and end times
            val startTime = firstValues[0]
            val endTime = lastValues[0]
            println("start time is $startTime")
            val dateFormat = SimpleDateFormat("dd-MM-yyyy HH:mm", Locale.getDefault())
            val startDate = dateFormat.parse(startTime)
            val endDate = dateFormat.parse(endTime)
            val timeDifferenceInSeconds = ((endDate.time - startDate.time) / 1000).toDouble()

            // Calculate distance using Haversine formula
            val distance = calculateDistance(
                startLatitude, startLongitude,
                endLatitude, endLongitude
            )

            // Calculate average speed (distance in km, time in hours)
            val timeInHours = timeDifferenceInSeconds / 3600
            val averageSpeed = if (timeInHours > 0) distance / timeInHours else 0.0


            val startLocation = getLocationName(startLatitude, startLongitude)
            val endLocation = getLocationName(endLatitude, endLongitude)

            // You can also use getGeoPointFromAddress to obtain the GeoPoint for the start and end locations
            val startGeoPoint = getGeoPointFromAddress(context, startLocation)
            val endGeoPoint = getGeoPointFromAddress(context, endLocation)


            val trip = Trip(
                tripName,
                getLocationName(startLatitude, startLongitude),
                getLocationName(endLatitude, endLongitude),
                startTime,
                endTime,
                distance,
                averageSpeed
            )
//            val updatedTrips = loadSavedTrips().toMutableList().apply {
//                add(0, trip)
//            }
            val updatedTrips = loadSavedTrips().toMutableList().apply {
                add(0, trip)
            }.sortedByDescending { it.startTime }
                .take(7)
            saveTrips(updatedTrips)
            setTrips(updatedTrips)
        } else {
            Log.w("TripsAdapter", "Malformed data in trip lines: $firstLine | $lastLine")
        }
    }
    fun setTrips(updatedTrips: List<Trip>) {
        tripList.clear()          // Clear the current list of trips
        tripList.addAll(updatedTrips)  // Add the new list of trips
        notifyDataSetChanged()    // Notify the adapter that the data has changed
    }
    private fun loadSavedTrips(): List<Trip> {
        val tripsJson = sharedPreferences.getString("saved_trip", "[]")
        return Gson().fromJson(tripsJson, object : TypeToken<List<Trip>>() {}.type)
    }

    private fun saveTrips(trips: List<Trip>) {
        val tripsJson = Gson().toJson(trips)
        Log.d("TripsAdapter", "Saving trips: $tripsJson")
        val success = sharedPreferences.edit().putString("saved_trip", tripsJson).commit()
        Log.d("TripsAdapter", "Save operation successful: $success")
    }

    private fun getLocationName(latitude: Double, longitude: Double): String {
        return try {
            val geocoder = Geocoder(context, Locale.getDefault())
            val address: Address? = geocoder.getFromLocation(latitude, longitude, 1)?.firstOrNull()

            // Extract locality (or subLocality) and postal code
            val locality = address?.locality ?: address?.subLocality
            val postalCode = address?.postalCode

            // Combine them, or show a fallback if both are unavailable
            when {
                locality != null && postalCode != null -> "$locality, $postalCode"
                locality != null -> locality
                postalCode != null -> postalCode
                else -> "Unknown Location"
            }
        } catch (e: Exception) {
            Log.e("TripsAdapter", "Error in reverse geocoding: ${e.message}", e)
            "Unknown Location"
        }
    }

    fun getGeoPointFromAddress(context: Context, address: String): GeoPoint? {
        val geocoder = Geocoder(context, Locale.getDefault())
        try {
            val addressList: MutableList<Address>? = geocoder.getFromLocationName(address, 1)
            if (addressList != null) {
                if (addressList.isNotEmpty()) {
                    val location = addressList?.get(0)
                    if (location != null) {
                        return GeoPoint(location.latitude, location.longitude)
                    }
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
        return null
    }
    fun dmsToDecimal(degree: Double, minute: Double, second: Double, direction: Char): Double {
        var decimal = degree + minute / 60.0 + second / 3600.0
        // Adjust for direction
        return if (direction == 'S' || direction == 'W') -decimal else decimal
    }



//private fun setupMapView(binding: ItemTripDetailsBinding,tripName: String) {
//    val locationDataForTrip = tripLocationDataMap[tripName]
//    if (locationDataForTrip == null || locationDataForTrip.isEmpty()) {
//        Log.w("TripsAdapter", "No location data available for the selected trip: $tripName")
//        return
//    }
//    val startPoint = locationData[0]
//    val endPoint = locationData[1]
//    mapView = binding.mapView
//    mapView.setTileSource(TileSourceFactory.MAPNIK)
//    mapView.setBuiltInZoomControls(true)
//    mapView.setMultiTouchControls(true)
//    mapView.controller.setZoom(15.0)
//
//    // Clear existing overlays
//    mapView.overlays.clear()
//
//    // Add a polyline for the route
//    val polyline = Polyline()
//    polyline.color = Color.RED
//    polyline.width = 5f
//
//    val geoPoints = locationData.map {
//        GeoPoint(it.latitude, it.longitude)
//    }
//    polyline.setPoints(geoPoints)
//
//    // Add markers for start and end points
//    if (geoPoints.isNotEmpty()) {
//        val startMarker = Marker(mapView).apply {
//            position = geoPoints.first()
//            setAnchor(Marker.ANCHOR_CENTER, Marker.ANCHOR_BOTTOM)
//            //icon = ResourcesCompat.getDrawable(context.resources, R.drawable.ic_start_marker, null)
//        }
//
//        val endMarker = Marker(mapView).apply {
//            position = geoPoints.last()
//            setAnchor(Marker.ANCHOR_CENTER, Marker.ANCHOR_BOTTOM)
//            //icon = ResourcesCompat.getDrawable(context.resources, R.drawable.ic_end_marker, null)
//        }
//
//        mapView.overlays.addAll(listOf(startMarker, endMarker))
//    }
//
//    mapView.overlays.add(polyline)
//    if (geoPoints.isNotEmpty()) mapView.controller.setCenter(geoPoints.first())
//    mapView.invalidate()
//}
private fun initiateMapView(mapView: MapView) {
    mapView.apply {
        val config = org.osmdroid.config.Configuration.getInstance()

        // Set the user agent
        config.userAgentValue = "com.skyautonet.drsafe/1.0"

        // Load configuration
        config.load(
            context.applicationContext,
            PreferenceManager.getDefaultSharedPreferences(context)
        )

        setTileSource(TileSourceFactory.MAPNIK)
        setBuiltInZoomControls(true)
        setMultiTouchControls(true)
    }
}

private fun setupMapView(binding: ItemTripDetailsBinding, tripName: String) {
println("Setting up the map view for trip")
    // Clear previous overlays to avoid adding duplicates
    binding.mapView.overlays.clear()
    initiateMapView(binding.mapView)

    // Retrieve the location data for the selected trip
    val locationData = tripLocationDataMap[tripName]

    if (locationData != null && locationData.isNotEmpty()) {
        // Create a Polyline to connect all points
        val polyline = Polyline()

        // Iterate through the location data points
        for (i in locationData.indices) {
            val point = locationData[i]
            val geoPoint = GeoPoint(point.latitude, point.longitude)
            println("current point is : ${point.latitude} and ${point.longitude}")

            // Start point - Green marker with custom icon
            if (i == 0) {
                val startMarker = Marker(binding.mapView).apply {
                    position = geoPoint
                    // Use custom marker icon for the start point
                    icon = ContextCompat.getDrawable(context, R.drawable.ic_marker_start)
                    title = "Start Point: ${getLocationName(point.latitude, point.longitude)}"
                }
                binding.mapView.overlays.add(startMarker)
            }
            // End point - Red marker with custom icon
            else if (i == locationData.size - 1) {
                val endMarker = Marker(binding.mapView).apply {
                    position = geoPoint
                    // Use custom marker icon for the end point
                    icon = ContextCompat.getDrawable(context, R.drawable.ic_marker_end)
                    title = "End Point: ${getLocationName(point.latitude, point.longitude)}"
                }
                binding.mapView.overlays.add(endMarker)
            }

            // Add point to the polyline
            polyline.addPoint(geoPoint)
            polyline.addPoint(geoPoint)
            binding.mapView.controller.setCenter(geoPoint)
            binding.mapView.controller.setZoom(15.5)
        }

        // Set the polyline color (optional)
        polyline.color = Color.RED

        // Add the polyline to the map
        binding.mapView.overlays.add(polyline)

        // Optionally, adjust zoom to fit the whole route

    }
}

    private fun createMarkerIcon(color: Int): BitmapDrawable {
        val markerIcon = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(markerIcon)
        val paint = Paint()
        paint.color = color
        paint.isAntiAlias = true
        paint.style = Paint.Style.FILL

        // Draw a circle for the marker
        canvas.drawCircle(50f, 50f, 40f, paint)

        return BitmapDrawable(context.resources, markerIcon)
    }
    private fun boundingBoxForRoute(locationData: List<LocationData>): org.osmdroid.util.BoundingBox {
        val padding = 0.01
        val minLat = locationData.minOf { it.latitude } - padding
        val maxLat = locationData.maxOf { it.latitude }+ padding
        val minLon = locationData.minOf { it.longitude }- padding
        val maxLon = locationData.maxOf { it.longitude }+ padding
        return org.osmdroid.util.BoundingBox(maxLat, maxLon, minLat, minLon)
    }

    private fun calculateDistance(
        lat1: Double, lon1: Double, lat2: Double, lon2: Double
    ): Double {
        val R = 6371.0 // Radius of Earth in kilometers
        val dLat = Math.toRadians(lat2 - lat1)
        val dLon = Math.toRadians(lon2 - lon1)
        val a = Math.sin(dLat / 2) * Math.sin(dLat / 2) +
                Math.cos(Math.toRadians(lat1)) * Math.cos(Math.toRadians(lat2)) *
                Math.sin(dLon / 2) * Math.sin(dLon / 2)
        val c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a))
        return R * c
    }

    inner class TripsViewHolder(val binding: ItemTripDetailsBinding) : RecyclerView.ViewHolder(binding.root) {
        fun bind(trip: Trip) {
            binding.tvTripName.text = trip.tripName
            binding.tvStartLocation.text = trip.startLocation
            binding.tvEndLocation.text = trip.endLocation
            binding.tvStartTime.text = formatTime(trip.startTime)
            binding.tvEndTime.text = formatTime(trip.endTime)
            binding.tvStartTimeValue.text = "${formatDate(trip.startTime)} | ${formatTime(trip.startTime)}"
            binding.tvEndTimeValue.text = "${formatDate(trip.endTime)} | ${formatTime(trip.endTime)}"
            val formattedDistance = String.format("%.2f km", trip.distance)
            val formattedAverageSpeed = String.format("%.2f km/h", trip.averageSpeed)

            binding.tvDistance.text = formattedDistance
            binding.tvAverageSpeed.text = formattedAverageSpeed


        }


        private fun formatTime(dateString: String): String {
            if (dateString.isNullOrEmpty() || dateString == "0") {
                return "Invalid Time"  // Or you can return a default value like "N/A"
            }
            val formats = listOf(
                SimpleDateFormat("dd-MM-yyyy HH:mm", Locale.getDefault()), // Handle 'T' separator in time
                SimpleDateFormat("dd-MM-yyyy HH:mm", Locale.getDefault())
            )
            formats.forEach {
                try {
                    return it.parse(dateString)?.let { date ->
                        SimpleDateFormat("HH:mm", Locale.getDefault()).format(date)
                    } ?: "Invalid Time"
                } catch (e: Exception) {
                    Log.e("TripsAdapter", "Error parsing time: ${e.message}", e)
                }
            }
            return "Invalid Time"
        }
        private fun formatDate(dateString: String): String {
            if (dateString.isNullOrEmpty() || dateString == "0") {
                return "Invalid Time"  // Or you can return a default value like "N/A"
            }
            val formats = listOf(
                SimpleDateFormat("dd-MM-yyyy HH:mm", Locale.getDefault()), // Handle 'T' separator in time
                SimpleDateFormat("dd-MM-yyyy HH:mm", Locale.getDefault())
            )
            formats.forEach {
                try {
                    return it.parse(dateString)?.let { date ->
                        SimpleDateFormat("dd-MM-yyyy", Locale.getDefault()).format(date)
                    } ?: "Invalid Time"
                } catch (e: Exception) {
                    Log.e("TripsAdapter", "Error parsing time: ${e.message}", e)
                }
            }
            return "Invalid Time"
        }
    }
}

data class Trip(
    val tripName: String,
    val startLocation: String,
    val endLocation: String,
    val startTime: String,
    val endTime: String,
    val distance: Double,
    val averageSpeed: Double,
    val isExpanded: Boolean = false,


)
//    val isExpanded: Boolean = false