package com.axon.kisa10.fragment.log

import android.os.Bundle
import android.os.Environment
import android.preference.PreferenceManager
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import com.axon.kisa10.util.AppConstants
import com.axon.kisa10.R
import com.axon.kisa10.databinding.FragmentLogMapviewBinding
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.json.JSONObject
import org.osmdroid.bonuspack.routing.OSRMRoadManager
import org.osmdroid.config.Configuration
import org.osmdroid.tileprovider.tilesource.TileSourceFactory
import org.osmdroid.util.GeoPoint
import org.osmdroid.views.overlay.Marker
import org.osmdroid.views.overlay.Polyline
import java.io.File

/**
 * Created by Hussain on 20/09/24.
 */
class LogMapViewFragment : Fragment() {

    private val TAG = "LogMapViewFragment"

    private var logFileName : String? = null

    private lateinit var binding : FragmentLogMapviewBinding

    private val ioScope = CoroutineScope(Dispatchers.IO)

    private var coordinates = ArrayList<GeoPoint>()

    private val roadManager by lazy {
        OSRMRoadManager(requireContext(), getString(R.string.app_name))
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentLogMapviewBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        logFileName = arguments?.getString(AppConstants.KEY_LOG_FILE)
        binding.mapView.apply {
            Configuration.getInstance().load(
                requireActivity().applicationContext,
                PreferenceManager.getDefaultSharedPreferences(requireActivity().applicationContext)
            )
            setTileSource(TileSourceFactory.MAPNIK)
            setBuiltInZoomControls(false)
            setMultiTouchControls(true)
            val center = GeoPoint(37.5519, 126.9918)
            controller.setCenter(center)
            controller.setZoom(15.5)
        }

        if (!logFileName.isNullOrBlank()) {
            readLogData()
        }
    }

    override fun onPause() {
        super.onPause()
        Configuration.getInstance().save(
            requireActivity().applicationContext,
            PreferenceManager.getDefaultSharedPreferences(requireActivity().applicationContext)
        )
    }

    override fun onResume() {
        super.onResume()
        Configuration.getInstance().load(
            requireActivity().applicationContext,
            PreferenceManager.getDefaultSharedPreferences(requireActivity().applicationContext)
        )
    }

    private fun readLogData() {
        ioScope.launch {
            val geoJsonFilename = logFileName!!.split(".")[0] + "_vehicle_info_line.geojson"
            Log.i(TAG, "openFile: $geoJsonFilename")
            val filePath = requireContext().getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)?.path.toString() + AppConstants.LOG_DIR + "/" + geoJsonFilename
            val file = File(filePath)
            if (file.exists()) {
                val data = file.readText()
                val jsonData = JSONObject(data)
                val coords = jsonData.getJSONArray("features").getJSONObject(0).getJSONObject("geometry").getJSONArray("coordinates")
                if (coords.length() > 0) {
                    for (i in 0 until coords.length()) {
                        val coordinate = coords.getJSONArray(i)
                        coordinates.add(GeoPoint(coordinate.getDouble(1), coordinate.getDouble(0)))
                    }
                }
                Log.i(TAG, "readLogData: ${coordinates.size}")
                withContext(Dispatchers.Main) {
                    configureMap()
                }
            } else {
                Log.d(TAG, "readLogData: file doesn't exist")
            }
        }
    }

    private fun configureMap()  {
        val polyLine = Polyline()
        polyLine.color = resources.getColor(R.color.bg_main,null)
        polyLine.width = 15.0f
        polyLine.setPoints(coordinates)
        binding.mapView.overlays.add(polyLine)
        binding.mapView.invalidate()
        binding.mapView.controller.animateTo(coordinates.first())

        val startMarker = Marker(binding.mapView)
        startMarker.position = coordinates.first()
        startMarker.icon = ContextCompat.getDrawable(requireContext(), R.drawable.ic_location)
        startMarker.setAnchor(Marker.ANCHOR_CENTER, Marker.ANCHOR_BOTTOM)
        binding.mapView.overlays.add(startMarker)

        val endMarker = Marker(binding.mapView)
        endMarker.position = coordinates.last()
        endMarker.icon = ContextCompat.getDrawable(requireContext(), R.drawable.ic_location)
        endMarker.setAnchor(Marker.ANCHOR_CENTER, Marker.ANCHOR_BOTTOM)
        binding.mapView.overlays.add(endMarker)

        binding.mapView.invalidate()
    }

}