package com.axon.kisa10.util

import android.annotation.SuppressLint
import android.content.Context
import android.os.Environment
import android.util.Log
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.nio.ByteBuffer
import java.nio.ByteOrder

/**
 * Created by Hussain on 12/09/24.
 */
object LogParser {

    private const val TAG = "LogParser"

    @SuppressLint("DefaultLocale")
    fun parseEvent(context: Context, data: ByteArray, filename : String) {
        val fileSize = data.size

        val csvData = mutableListOf<String>()
        csvData.add("date,time,type,speed,azm,remarks\n")
        var ofs = 0
        while (ofs < fileSize - 4) {
            val eventInf = ByteBuffer.wrap(data, ofs, 11).order(ByteOrder.LITTLE_ENDIAN)
            val year = eventInf.short.toInt()
            val month = eventInf.get().toInt()
            val day = eventInf.get().toInt()
            val hour = eventInf.get().toInt()
            val minute = eventInf.get().toInt()
            val sec = eventInf.get().toInt()
            val eventType = eventInf.get().toInt()
            val speed = eventInf.get().toInt()
            val azm = eventInf.short.toInt()


            var dbgInfoStr = "None"
            if (eventType == 7) {
                val sysState = azm and 0x7F
                val opType = (azm and 0x80) shr 7
                val dbgEventId = (azm shr 8) and 0xFF

                val dbgEventData = ByteBuffer.wrap(data, ofs + 11, 4).order(ByteOrder.LITTLE_ENDIAN).int

                val sysStateName = EventConstants.SYSTEM_STATE_NAMES.getOrDefault(sysState, sysState.toString())

                var eventName = EventConstants.DBG_EVENT_NAMES.getOrDefault(dbgEventId, dbgEventId.toString())
                if (dbgEventId == 9 && dbgEventData.toLong() == 0x80000000) {
                    eventName = "FW_UPDATE"
                }

                if (dbgEventId in listOf(2, 3, 4, 255)) {
                    val zoneSt = (dbgEventData shr 8) and 0xFF
                    val zoneWarn = dbgEventData and 0xFF
                    val zoneWarnName = EventConstants.WARNING_TYPE_NAMES.getOrDefault(zoneWarn, zoneWarn.toString())
                    val zoneStateName = EventConstants.ZONE_STATE_NAMES.getOrDefault(zoneSt, zoneSt.toString())
                    dbgInfoStr = "state=$sysStateName;event=$eventName;op=${EventConstants.OP_TYPE_NAMES[opType]};data=${dbgEventData.toString(16)}($zoneWarnName,$zoneStateName)"
                } else {
                    dbgInfoStr = "state=$sysStateName;event=$eventName;op=${EventConstants.OP_TYPE_NAMES[opType]};data=${dbgEventData.toString(16)}"
                }
            }

            Log.i(TAG, "Event Type : $eventType")
            Log.i(TAG, "Event Name : ${EventConstants.EVENT_NAMES[eventType]}")

            val formattedDate = String.format("%d-%02d-%02d,%02d:%02d:%02d,%s,%d,%d,%s\n",year,month,day,hour,minute,sec,EventConstants.EVENT_NAMES[eventType] ?: "None", eventInf[7], eventInf[8], dbgInfoStr)
            csvData.add(formattedDate)
            ofs += 16
        }

        val filePath = context.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)?.path.toString() + AppConstants.EVENT_DIR
        val mainDir = File(filePath)
        if (!mainDir.exists()) {
            mainDir.mkdirs()
        }
        val csvFile = File(filePath, "$filename.csv")
        if (csvFile.exists()) {
            csvFile.delete()
            csvFile.createNewFile()
        }
        csvFile.writeText(csvData.joinToString(""))
        println("$csvFile created")
    }

    fun parseVehicleInfo(context: Context,data : ByteArray, filename : String)  {

        val fileSize = data.size

        if ((fileSize - 4) % 32 != 0) {
            println("WARNING: invalid file size")
        }

        val csvData = mutableListOf<String>()
        val line = mutableListOf<JSONArray>()
        val geojsonDataLine = JSONObject().apply {
            put("type", "FeatureCollection")
            put("name", "block_data")
            put("crs", JSONObject().apply {
                put("type", "name")
                put("properties", JSONObject().put("name", "urn:ogc:def:crs:OGC:1.3:CRS84"))
            })
            put("features", JSONArray())
        }
        val geojsonDataPoints = JSONObject().apply {
            put("type", "FeatureCollection")
            put("name", "block_data")
            put("crs", JSONObject().apply {
                put("type", "name")
                put("properties", JSONObject().put("name", "urn:ogc:def:crs:OGC:1.3:CRS84"))
            })
            put("features", JSONArray())
        }
        csvData.add("date,time,lat,lng,azm,speed,brake,rpm,imu_acc_x, imu_acc_y,daily(km),cumulative(km)\n")

        var ofs = 0
        var pointIdx = 0

        while (ofs < fileSize - 4) {
            Log.i("LOGPARSER", "$ofs")
            val buffer = ByteBuffer.wrap(data, ofs, 32).order(ByteOrder.LITTLE_ENDIAN)
            val year = buffer.short.toInt()
            val month = buffer.get().toInt()
            val day = buffer.get().toInt()
            val hour = buffer.get().toInt()
            val minute = buffer.get().toInt()
            val sec = buffer.get().toInt()
            val unused = buffer.get()
            ofs += 8

            val lat = buffer.int
            val lng = buffer.int
            ofs += 8

            val azm = buffer.short.toUShort()
            val speed = buffer.get().toInt()
            val brake = buffer.get().toInt()
            ofs += 4

            val rpm = buffer.short.toUShort()
            val imuAccX = buffer.short.toUShort()
            val imuAccY = buffer.short.toUShort()
            val dailyKm = buffer.short.toUShort()
            val cumulKm = buffer.int
            ofs += 12

            var formatted = "%d-%02d-%02d,%02d:%02d:%02d,".format(year,month,day,hour,minute,sec)
            formatted += "${lat/1e6},${lng/1e6},$azm,$speed,$brake,$rpm,$imuAccX,$imuAccY,$dailyKm,$cumulKm\n"
            Log.i("LOG_PARSER", formatted)
            csvData.add(formatted)

            if (lat > 1 && lng > 1) {
                val lineCoords = JSONArray(listOf(lng / 1e6, lat / 1e6))
                line.add(lineCoords)
                val pointProps = JSONObject().apply {
                    put("Index", pointIdx.toString())
                    put("Speed", speed.toString())
                    put("Azm", azm.toString())
                    put("Brake", brake.toString())
                    put("RPM", rpm.toString())
                }
                val geojsonFeature = JSONObject().apply {
                    put("type","Feature")
                    put("geometry", JSONObject().apply {
                        put("type", "Point")
                        put("coordinates", lineCoords)
                    })
                    put("properties", pointProps)
                }
                geojsonDataPoints.getJSONArray("features").put(geojsonFeature)
                pointIdx++
            }
        }
        Log.i("LOG_PARSER", "line size : ${line.size}")
        if (line.size > 0) {
            geojsonDataLine.getJSONArray("features").put(JSONObject().apply {
                put("type","Feature")
                put("geometry", JSONObject().apply {
                    put("type", "LineString")
                    put("coordinates", JSONArray(line))
                })
                put("properties", JSONObject())
            })
        }

        try {
            val mainDirPath = context.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)?.path.toString() + AppConstants.LOG_DIR
            val mainDir = File(mainDirPath)
            if (!mainDir.exists()) {
                mainDir.mkdirs()
            }
            val csvFile = File(mainDirPath, "${filename}_vehicle_info.csv")
            if (csvFile.exists()) {
                csvFile.delete()
                csvFile.createNewFile()
            }
            csvFile.writeText(csvData.joinToString(""))
            Log.i("LOG_PARSER","${csvFile.name} created")

            val geoJsonLineFile = File(mainDirPath, "${filename}_vehicle_info_line.geojson")
            if (geoJsonLineFile.exists()) {
                geoJsonLineFile.delete()
                geoJsonLineFile.createNewFile()
            }
            geoJsonLineFile.writeText(geojsonDataLine.toString(2))
            Log.i("LOG_PARSER","${geoJsonLineFile.name} created")

            val geoJsonFilePoints = File(mainDirPath, "${filename}_vehicle_info_points.geojson")
            if (geoJsonFilePoints.exists()) {
                geoJsonFilePoints.delete()
                geoJsonFilePoints.createNewFile()
            }
            geoJsonFilePoints.writeText(geojsonDataPoints.toString(2))
            Log.i("LOG_PARSER","${geoJsonFilePoints.name} created")
        } catch (e : Exception) {
            e.printStackTrace()
            Log.e("LOG_PARSER", "unable to save file", e.cause)
        }
    }



}