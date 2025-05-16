import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.preference.PreferenceManager
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.annotation.RequiresApi
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import com.skyautonet.drsafe.R
import com.skyautonet.drsafe.databinding.FragmentDrivingScoreBinding
import com.skyautonet.drsafe.ui.mydriving.DrivingScoreRepository
import org.osmdroid.tileprovider.tilesource.TileSourceFactory
import org.osmdroid.util.BoundingBox
import org.osmdroid.util.GeoPoint
import org.osmdroid.views.MapView
import org.osmdroid.views.overlay.Marker
import org.osmdroid.views.overlay.Polyline
import java.text.SimpleDateFormat
import java.util.Locale
import org.osmdroid.config.Configuration
import java.time.LocalDateTime
import java.time.format.DateTimeFormatterBuilder
import java.time.temporal.ChronoField

class DrivingScoreFragment : Fragment() {

    private lateinit var binding: FragmentDrivingScoreBinding
    private lateinit var drivingScoreViewModel: DrivingScoreViewModel

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentDrivingScoreBinding.inflate(inflater, container, false)
        return binding.root
    }

    @RequiresApi(Build.VERSION_CODES.O)
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        // Initialize ViewModel with Factory
        val repository = DrivingScoreRepository(requireContext()) // This should ideally be injected if using Dagger/Hilt
        val factory = DrivingScoreViewModelFactory(repository)
        drivingScoreViewModel = ViewModelProvider(this, factory).get(DrivingScoreViewModel::class.java)

        // Observe the driving events from the ViewModel
        drivingScoreViewModel.drivingEvents.observe(viewLifecycleOwner) { events ->
            events?.let {
                addDrivingEvents(it)
            }
        }


    }
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Set the user agent for OsmDroid
//        val ctx = requireContext().applicationContext
//        Configuration.getInstance().userAgentValue = BuildConfig.APPLICATION_ID
    }

    @RequiresApi(Build.VERSION_CODES.O)
    private fun addDrivingEvents(events: List<DrivingEvent>) {
        // Remove any existing views to avoid duplication
        binding.llEventContainer.removeAllViews()
        val inputDateFormat = SimpleDateFormat("EEE MMM dd HH:mm:ss 'GMT'Z yyyy", Locale.ENGLISH)

        val outputDateFormat = SimpleDateFormat("dd-MM-yyyy HH:mm", Locale.getDefault())


        // Dynamically add event views
        events.forEach { event ->
            val eventName = event.title.split(" - ").getOrElse(1) { event.title }
            println(eventName)
            //val dateTime = LocalDateTime.parse(event.timestamp, formatter)
            val eventDate = try {
                inputDateFormat.parse(event.timestamp) // Check timestamp format
            } catch (e: Exception) {
                Log.e("addDrivingEvents", "Invalid date format for timestamp: ${event.timestamp}", e)
                null
            }

            // Inflate the event item layout
            val eventView = LayoutInflater.from(requireContext())
                .inflate(R.layout.event_item, binding.llEventContainer, false)

            // Format the date or fallback to "Invalid Date"
            val formattedDate = eventDate?.let { outputDateFormat.format(it) } ?: "Invalid Date"

            // Find TextViews and ImageView in the inflated layout
            val tvEventTitle: TextView = eventView.findViewById(R.id.tvEventTitle)
            val tvEventTime: TextView = eventView.findViewById(R.id.tvEventTime)
            val imgDownArrow: ImageView = eventView.findViewById(R.id.imgDownArrow)
            val tvEventDescription: TextView = eventView.findViewById(R.id.tvEventDescription)
            val mapView: MapView = eventView.findViewById(R.id.mapView)

            // Set event details
            tvEventTitle.text = eventName
            tvEventTime.text = formattedDate
            imgDownArrow.setImageResource(R.drawable.ic_card_down)  // Set the down arrow image
            tvEventDescription.text = event.description

            tvEventDescription.visibility = View.GONE
            mapView.visibility = View.GONE
            mapView.setMultiTouchControls(true)

            // Toggle visibility of the description on arrow click
            imgDownArrow.setOnClickListener {
                if (tvEventDescription.visibility == View.GONE) {
                    tvEventDescription.visibility = View.VISIBLE
                    mapView.visibility = View.VISIBLE
                    displayTripOnMap(mapView, event)

                    imgDownArrow.setImageResource(R.drawable.ic_card_up) // Update icon to "up" arrow
                } else {
                    tvEventDescription.visibility = View.GONE
                    mapView.visibility = View.GONE
                    imgDownArrow.setImageResource(R.drawable.ic_card_down) // Update icon to "down" arrow
                }
            }

            // Add the event view to the container
            binding.llEventContainer.addView(eventView)
        }
    }
    private fun setupMapView(mapView: MapView) {
        mapView.apply {
            org.osmdroid.config.Configuration.getInstance().load(
                requireContext().applicationContext,
                PreferenceManager.getDefaultSharedPreferences(requireContext())
            )
            setTileSource(TileSourceFactory.MAPNIK)
            setBuiltInZoomControls(true)
            setMultiTouchControls(true)
        }
    }


    private fun displayTripOnMap(mapView: MapView, event: DrivingEvent) {
        try {

            setupMapView(mapView)
            val mapController = mapView.controller
            mapController.setZoom(20.0)  // Set the initial zoom level
            println("Displaying map...")

            val startLocation = event.startLocation
            val endLocation = event.endLocation
            val routePoints = event.route  // List of locations for the entire route

            if (startLocation == null || endLocation == null) {
                println("Error: Start or End location is null.")
                return
            }

            // Start location marker
            val startGeoPoint = GeoPoint(startLocation.latitude, startLocation.longitude)
            println("Start Location: ${startGeoPoint.latitude}, ${startGeoPoint.longitude}")

            val startMarker = Marker(mapView).apply {
                position = startGeoPoint
                icon = ContextCompat.getDrawable(requireContext(), R.drawable.ic_marker_start) // Start marker icon
                title = "Start Location"
            }
            mapView.overlays.add(startMarker)

            // End location marker
            val endGeoPoint = GeoPoint(endLocation.latitude, endLocation.longitude)
            println("End Location: ${endGeoPoint.latitude}, ${endGeoPoint.longitude}")

            val endMarker = Marker(mapView).apply {
                position = endGeoPoint
                icon = ContextCompat.getDrawable(requireContext(), R.drawable.ic_marker_end) // End marker icon
                title = "End Location"
            }
            mapView.overlays.add(endMarker)

            // Add polyline for the route
            routePoints?.let { points ->
                if (points.isNotEmpty()) {
                    // Create a polyline for the entire route
                    val polyline = Polyline().apply {
                        setPoints(points.map { GeoPoint(it.latitude, it.longitude) })
                        color = Color.BLUE
                        width = 5.0f
                    }
                    mapView.overlays.add(polyline)

                    // Add markers for events along the route
//                    points.forEachIndexed { index, point ->
//                        val routeGeoPoint = GeoPoint(point.latitude, point.longitude)
//
//                        // Create event marker
//                        val routeMarker = Marker(mapView).apply {
//                            position = routeGeoPoint
//                            icon = ContextCompat.getDrawable(context, R.drawable.marker_event) // Event marker icon
//                            title = "Event at Point $index"
//                            snippet = "Speed: ${event.description}" // Description for the event
//                        }
//                        mapView.overlays.add(routeMarker)
//                    }

                    mapView.overlays.add(startMarker)
                    mapView.controller.setCenter(startGeoPoint)
                    mapView.controller.setZoom(18.5)

                } else {
                    println("Error: Route points are empty.")
                }
            }

        } catch (e: Exception) {
            println("Error displaying map: ${e.message}")
            e.printStackTrace()
        }
    }








}