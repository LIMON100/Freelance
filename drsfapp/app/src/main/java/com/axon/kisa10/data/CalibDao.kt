package com.axon.kisa10.data

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.Query
import com.axon.kisa10.model.distributor.calibration.CalibrationHistory

@Dao
interface CalibDao {

    @Query("SELECT * FROM Calibration")
    fun getAll() : List<CalibrationHistory>

    @Query("DELETE FROM Calibration")
    fun deleteAll()

    @Insert
    fun save(data : CalibrationHistory)
}