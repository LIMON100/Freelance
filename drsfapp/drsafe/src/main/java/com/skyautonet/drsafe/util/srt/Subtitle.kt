package com.skyautonet.drsafe.util.srt

/**
 * Created by Hussain on 26/07/24.
 */
class Subtitle {
    var id: Int = 0
    var startTime: String? = null
    var endTime: String? = null
    var text: String? = null
    var timeIn: Long = 0
    var timeOut: Long = 0
    var nextSubtitle: Subtitle? = null

    override fun toString(): String {
        return ("Subtitle [id=" + id + ", startTime=" + startTime + ", endTime=" + endTime + ", text=" + text
                + ", timeIn=" + timeIn + ", timeOut=" + timeOut + "]")
    }
}