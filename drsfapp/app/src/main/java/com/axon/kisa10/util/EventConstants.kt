package com.axon.kisa10.util

object EventConstants {
    val EVENT_NAMES = mapOf(
        1 to "OVER_SPEED",
        2 to "RAPID_ACCEL",
        3 to "QUICK_DECEL",
        4 to "SUDDEN_STOP",
        5 to "AGGRESSIVE_LANE_CHANGE",
        6 to "SHARP_TURN",
        7 to "DEBUG"
    )

    val SYSTEM_STATE_NAMES = mapOf(
        0 to "NONE",
        1 to "NORMAL",
        2 to "UNCALIBRATED",
        3 to "ERROR",
        4 to "UPDATING",
        6 to "PAUSE",
        7 to "CALIBRATING",
        8 to "PUA",
        9 to "BUTTON",
        10 to "SPEED_BTO"
    )

    val DBG_EVENT_NAMES = mapOf(
        0 to "PAUSE",
        1 to "PUA",
        2 to "ZONE_BTO",3 to "ZONE_SPEED",
        4 to "ZONE_IN",
        5 to "CALIBRATING",
        6 to "BUTTON",
        7 to "GPS",
        8 to "BTO_ONOFF",
        9 to "DB_UPDATE",
        10 to "ERROR",
        11 to "SPEED_BTO"
    )

    val OP_TYPE_NAMES = listOf("UNSET", "SET")

    val ZONE_STATE_NAMES = mapOf(
        0 to "SCHOOLZONE_NONE",
        1 to "SCHOOLZONE_IN",
        2 to "SCHOOLZONE_ENTERING",
        3 to "SCHOOLZONE_LEAVING"
    )

    val WARNING_TYPE_NAMES = mapOf(
        0 to "SCHOOLZONE_NONE",
        1 to "SCHOOLZONE_IN",
        2 to "SCHOOLZONE_ENTER",
        3 to "SCHOOLZONE_OUT",
        4 to "SCHOOLZONE_LEAVING",
        5 to "SCHOOLZONE_SPEED",
        6 to "SCHOOLZONE_BTO",
        7 to "SPEEDCAM_NONE",
        8 to "SPEEDCAM_AHEAD",
        9 to "SPEEDCAM_SPEED",
        10 to "SPEEDCAM_BTO",
        11 to "SPEEDCAM_OUT",
        12 to "DUMMY"
    )
}