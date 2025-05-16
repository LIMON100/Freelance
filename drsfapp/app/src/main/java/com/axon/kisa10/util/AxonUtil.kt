package com.axon.kisa10.util

import android.annotation.SuppressLint
import android.os.Build
import androidx.annotation.RequiresApi
import java.text.SimpleDateFormat
import java.time.LocalDate
import java.time.format.DateTimeFormatter
import java.util.Calendar
import java.util.Date


/**
 * Created by Hussain on 17/09/24.
 */
object AxonUtil {

    @SuppressLint("NewApi")
    fun getLast7Days(): List<String> {
        val currentDate = LocalDate.now()
        val dates = mutableListOf<String>()
        val formatter = DateTimeFormatter.ofPattern("yyMMdd")
        for (i in 0..6) {
            val date = currentDate.minusDays(i.toLong())
            dates.add(date.format(formatter))
        }
        return dates
    }

    @SuppressLint("NewApi")
    fun getLast7DaysString(): String {
        val calendar = Calendar.getInstance()
        calendar.time = Date()
        val dates = mutableListOf<String>()
        val formatter = DateTimeFormatter.ofPattern("MM.dd")
        for (i in 0..6) {
            val date = calendar.time.toInstant().atZone(java.time.ZoneId.systemDefault()).toLocalDate().minusDays(i.toLong())
            dates.add(date.format(formatter))
        }
        return "${dates.first()}-${dates.last()}"
    }

    @SuppressLint("NewApi")
    fun getLast7DaysFrom(): MutableList<String> {
        val calendar = Calendar.getInstance()
        calendar.time = Date()
        val dates = mutableListOf<String>()
        val formatter = DateTimeFormatter.ofPattern("yyMMdd")
        for (i in 0..6) {
            val date = calendar.time.toInstant().atZone(java.time.ZoneId.systemDefault()).toLocalDate().minusDays(i.toLong())
            dates.add(date.format(formatter))
        }
        return dates
    }

    fun bytesToInt(bytes: ByteArray): Int {
        var result = 0
        for (i in bytes.indices) {
            result = result or (bytes[i].toInt() and 0xFF shl (8 * i))
        }
        return result
    }

    @RequiresApi(Build.VERSION_CODES.O)
    fun getLast3DaysFromToday(): MutableList<String> {
        val calendar = Calendar.getInstance()
        calendar.time = Date()
        val dates = mutableListOf<String>()
        val formatter = DateTimeFormatter.ofPattern("yyMMdd")
        for (i in 0..2) {
            val date = calendar.time.toInstant().atZone(java.time.ZoneId.systemDefault()).toLocalDate().minusDays(i.toLong())
            dates.add(date.format(formatter))
        }
        return dates
    }

    @RequiresApi(Build.VERSION_CODES.O)
    fun getLast3DaysFromToday(date : String): MutableList<String> {
        val calendar = Calendar.getInstance()
        calendar.time = Date()
        val dates = mutableListOf<String>()
        val formatter = DateTimeFormatter.ofPattern("yyMMdd")
        for (i in 0..2) {
            val date = calendar.time.toInstant().atZone(java.time.ZoneId.systemDefault()).toLocalDate().minusDays(i.toLong())
            dates.add(date.format(formatter))
        }
        return dates
    }

    @RequiresApi(Build.VERSION_CODES.O)
    fun getNext3DaysFrom(date: String): MutableList<String> {
        val calendar = Calendar.getInstance()
        calendar.time = SimpleDateFormat("yyMMdd").parse(date)
        val dates = mutableListOf<String>()
        val formatter = DateTimeFormatter.ofPattern("yyMMdd")
        for (i in 0..2) {
            val date = calendar.time.toInstant().atZone(java.time.ZoneId.systemDefault()).toLocalDate().minusDays(i.toLong())
            dates.add(date.format(formatter))
        }
        return dates
    }
}
