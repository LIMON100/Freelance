package com.axon.kisa10.util

import android.bluetooth.BluetoothGattCharacteristic
import android.content.Context
import android.content.res.Resources
import android.os.Environment
import android.view.View
import android.view.inputmethod.InputMethodManager
import androidx.recyclerview.widget.RecyclerView
import com.axon.kisa10.ble.AxonBLEService
import com.axon.kisa10.R

/**
 * Created by Hussain on 01/07/24.
 */

fun View.hideKeyboard() {
    val inputMethodManager = context.getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
    inputMethodManager.hideSoftInputFromWindow(windowToken, InputMethodManager.HIDE_IMPLICIT_ONLY)
}

fun Resources.getSdiNameFromType(type : Int) : String {
    return when (type) {
        0 -> {
            getString(R.string.signal_speed_camera)
        }

        1 -> {
            getString(R.string.speeding_fixed_camera)
        }

        2 -> {
            getString(R.string.section_enforcement_start_camera)
        }

        3 -> {
            getString(R.string.camera_end_section_enforcement)
        }

        4 -> {
            getString(R.string.camera_during_section_enforcement)
        }

        5 -> {
            getString(R.string.tail_biting_camera)
        }

        6 -> {
            getString(R.string.red_light_camera)
        }

        7 -> {
            getString(R.string.speeding_mobile_camera)
        }

        8 -> {
            "box type fixed camera"
        }

        9 -> {
            "Bus lane section camera"
        }

        10 -> {
            "Variable lane speed camera"
        }

        11 -> {
            "Shoulder guard point camera"
        }

        12 -> {
            "No-Interference Camera"
        }

        13 -> {
            "Traffic information collection point"
        }

        14 -> {
            "CCTV for crime prevention"
        }

        15 -> {
            "Overloaded vehicle danger zone"
        }

        16 -> {
            "Poor loading detection camera"
        }

        17 -> {
            "Parking enforcement point camera"
        }

        18 -> {
            "General traffic road"
        }

        19 -> {
            "rail crossing"
        }

        20 -> "Children's Protection Zone (School Zone Start Area)"
        21 -> "Children's protection zone (end of school zone)"
        22 -> "speed bump"
        23 -> "LPG filling station"
        24 -> "Tunnel section"
        25 -> "Rest area"
        26 -> "Tollgate"
        27 -> "Fog warning area"
        28 -> "Hazardous substances area"
        29 -> "Accident-prone area"
        30 -> "Sharp curve area section"
        31 -> "Sharp curve section"
        32 -> "Steep slope section"
        33 -> "Areas with frequent wildlife traffic accidents"
        34 -> "Poor right field of vision"
        35 -> "Poor visibility point"
        36 -> "Left visual field defect point"
        37 -> "Area with frequent signal violations"
        38 -> "Area with frequent speeding"
        39 -> "congested traffic area"
        40 -> "Directional lane selection points"
        41 -> "Frequent jaywalking accident locations"
        42 -> "Accident hotspot on the shoulder"
        43 -> "Speeding accident hotspots"
        44 -> "Drowsy accident hotspots"
        45 -> "Accident hotspot"
        46 -> "Pedestrian accident hotspots"
        47 -> "Locations where vehicle thefts frequently occur"
        48 -> "Nakseong area"
        49 -> "Ice warning area"
        50 -> "Bottleneck area"
        51 -> "merging road"
        52 -> "Fall warning area"
        53 -> "Underpass section"
        54 -> "Densely populated residential area (traffic calming area)"
        55 -> "Interchange"
        56 -> "bifurcation"
        57 -> "Rest area (LPG refill available)"
        58 -> "Bridge"
        59 -> "Brake system accident hotspots"
        60 -> "Center line violation accident frequent location"
        61 -> "Areas with a high incidence of traffic violations"
        62 -> "Directions across the destination"
        63 -> "Guide to the sleep shelter"
        64 -> "Crackdown on old diesel vehicles"
        65 -> "Enforcement of lane changes in tunnels"
        66 -> "Disabled person protection zone point of view"
        67 -> "End of the disabled protection zone"
        68 -> "Elderly protection zone point of view"
        69 -> "End of Senior Citizen Protection Zone"
        70 -> "Village resident protection zone point"
        71 -> "End of village protection zone"
        72 -> "Truck height limit"
        73 -> "Truck weight limit"
        74 -> "Truck width limit"
        75 -> "Rear speed camera"
        76 -> "Rear Traffic Signal Speed Camera"
        else -> {
            "$type"
        }
    }
}

inline fun <reified T : RecyclerView.ViewHolder> RecyclerView.forEachVisibleHolder(
    action: (T) -> Unit
) {
    for (i in 0 until childCount) {
        action(getChildViewHolder(getChildAt(i)) as T)
    }
}

fun BluetoothGattCharacteristic.isIndicatable(): Boolean =
    containsProperty(BluetoothGattCharacteristic.PROPERTY_INDICATE)

fun BluetoothGattCharacteristic.isNotifiable(): Boolean =
    containsProperty(BluetoothGattCharacteristic.PROPERTY_NOTIFY)

fun BluetoothGattCharacteristic.containsProperty(property: Int): Boolean =
    properties and property != 0

fun Context.getLogDownloadDirectory() : String {
    return getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)?.path.toString() + AppConstants.LOG_DIR
}

fun Context.getEventDownloadDirectory() : String {
    return getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)?.path.toString() + AppConstants.EVENT_DIR
}

fun AxonBLEService.getDeviceDirectory() : String {
    val device = getBluetoothGatt()
    if (device?.device != null) {
        return (device.device?.address?.replace(":","") + "/")
    } else {
        return ""
    }
}