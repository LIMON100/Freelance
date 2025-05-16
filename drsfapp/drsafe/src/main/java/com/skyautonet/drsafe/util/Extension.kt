package com.skyautonet.drsafe.util

import android.app.Activity
import android.bluetooth.BluetoothGattCharacteristic
import android.content.Intent
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.RecyclerView
import java.io.UnsupportedEncodingException

/**
 * Created by Hussain on 31/07/24.
 */
inline fun <reified T : RecyclerView.ViewHolder> RecyclerView.forEachVisibleHolder(
    action: (T) -> Unit
) {
    for (i in 0 until childCount) {
        action(getChildViewHolder(getChildAt(i)) as T)
    }
}

fun Activity.start(anotherActivity : Class<*>) {
    val intent = Intent(this, anotherActivity)
    startActivity(intent)
    finish()
}

fun Fragment.start(anotherActivity: Class<*>) {
    val intent = Intent(requireActivity(), anotherActivity)
    requireActivity().startActivity(intent)
    requireActivity().finish()
}

fun String.convertStringToByte(): ByteArray? {
    return try {
        toByteArray(charset("UTF-8"))
    } catch (e: UnsupportedEncodingException) {
        null
    }
}

fun ByteArray.convertByteToString(): String {
    try {
        return String(this, charset("UTF-8"))
    } catch (e: UnsupportedEncodingException) {
        e.printStackTrace()
        return ""
    }
}

fun BluetoothGattCharacteristic.isNotifiable(): Boolean =
    containsProperty(BluetoothGattCharacteristic.PROPERTY_NOTIFY)

fun BluetoothGattCharacteristic.containsProperty(property: Int): Boolean =
    properties and property != 0