package com.axon.kisa10.data

import android.content.Context
import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase
import com.axon.kisa10.model.distributor.calibration.CalibrationHistory
import com.axon.kisa10.model.distributor.calibration.DistributorCalibrationUploadRequest

@Database(entities = [CalibrationHistory::class], version = 1)
abstract class CalibrationHistoryDb : RoomDatabase() {

    abstract fun calibrationDao() : CalibDao


    companion object {

        private lateinit var calibDb : CalibrationHistoryDb

        fun getDb(context : Context) : CalibrationHistoryDb {
            if (!this::calibDb.isInitialized) {
                calibDb = Room.databaseBuilder(context, CalibrationHistoryDb::class.java, "Calibration_History")
                    .build()
            }
            return calibDb
        }

    }
}