package com.skyautonet.drsafe.ui.mydriving

import DrivingEvent
import LatLng
import android.content.Context
import android.os.Environment
import java.io.BufferedReader
import java.io.File
import java.io.FileInputStream
import java.io.InputStreamReader
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class DrivingScoreRepository(val context: Context) {
    // Define all possible event names
    private val eventNames = listOf(
        "Fast Acceleration",
        "Sudden Departure",
        "Rapid Deceleration",
        "Sudden Stop",
        "Radical Course Change",
        "Overtaking Speed",
        "Sharp Turns",
        "Sudden U-Turn",
        "Continuous Operation",
        "locationTrackingDeviceError"
    )

    fun readRecentCsvEvents(): List<DrivingEvent> {
        val MIN_EVENT_DURATION = 5000L // 5 seconds
        var downloadsDirectory = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)
        downloadsDirectory = File(context.filesDir, "axon/data")
        val csvFiles = downloadsDirectory.listFiles { file -> file.extension == "csv" }?.sortedByDescending { it.lastModified() }

        val events = mutableListOf<DrivingEvent>()
        var currentEvent: String? = null
        var eventStartTime: Date? = null
        val locations = mutableListOf<LatLng>()
        val tripLocations = mutableListOf<LatLng>()
        var speed = ""
        val eventTypeMap = mapOf(
            61 to "Over Speed",
            62 to "Long-Term Speed Data",
            63 to "Rapid Acceleration",
            64 to "Sudden Stop",
            65 to "Rapid Deceleration",
            66 to "Sudden Stop",
            67 to "Radical Course Change",
            68 to "Overtaking Speed",
            69 to "Sharp Turns",
            70 to "Sudden U-Turn",
            71 to "Continuous Operation"
        )
        csvFiles?.take(3)?.forEach { csvFile ->
            try {
                val reader = BufferedReader(InputStreamReader(FileInputStream(csvFile)))
                repeat(7) {
                    reader.readLine()
                }

                reader.forEachLine { line ->
                    val values = line.split(",")
                    val time = values[0] // Time column
                    val trip = csvFile.name.takeLast(2)
                    val eventType = values[11].toIntOrNull() // Event ID column

                    val eventName = eventTypeMap[eventType] ?: "Unknown Event"
                    speed = values[3]
                    val latitude = values[7].toLongOrNull()?.let { it / 1_000_000.0 } ?: 0.0
                    val longitude = values[6].toLongOrNull()?.let { it / 1_000_000.0 } ?: 0.0

                    val dateFormat = SimpleDateFormat("MM-dd-yyyy HH:mm", Locale.getDefault())
                    val timestamp = dateFormat.parse(time)

                    if (eventName in eventNames) {
                        if (currentEvent == null) {
                            // Start a new event
                            currentEvent = eventName
                            eventStartTime = timestamp
                            locations.clear()
                        } else if (eventName != currentEvent) {
                            // End the current event and start a new one
                            if (locations.isNotEmpty() && eventStartTime != null) {
                                val startLocation = locations.first()
                                val endLocation = locations.last()
                                events.add(
                                    DrivingEvent(
                                        title = "Event: $currentEvent",
                                        description = "Location: $startLocation\n" +
                                                " Vehicle Status: $currentEvent ended. \n" +
                                                " Speed: $speed km/h",
                                        timestamp = eventStartTime.toString(),
                                        startLocation = startLocation,
                                        endLocation = endLocation,
                                        route = locations.toList()
                                    )
                                )
                            }
                            currentEvent = eventName
                            eventStartTime = timestamp
                            locations.clear()
                        }
                    }

                    if (currentEvent != null) {
                        // Add location to the ongoing event
                        locations.add(
                            LatLng(
                                latitude = latitude,
                                longitude = longitude
                            )
                        )
                    }
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }

        // Finalize the last event after processing all files
        if (currentEvent != null && locations.isNotEmpty() && eventStartTime != null) {
            val startLocation = locations.first()
            val endLocation = locations.last()
            events.add(
                DrivingEvent(
                    title = "Event: $currentEvent",
                    description = "Location: $startLocation\n" +
                            " Vehicle Status: $currentEvent ended. \n" +
                            " Speed: $speed km/h",
                    timestamp = eventStartTime.toString(),
                    startLocation = startLocation,
                    endLocation = endLocation,
                    route = locations.toList()
                )
            )
        }

        return events
    }

}
