package com.example.rtest1

//import io.flutter.embedding.android.FlutterActivity
//
//class MainActivity: FlutterActivity()

import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel

class MainActivity: FlutterActivity() {
    // This is the "bridge" to communicate with Flutter.
    private val CHANNEL = "com.yourcompany/gamepad"
    private var channel: MethodChannel? = null

    // This map will hold the current state of all axes.
    private val axisState = mutableMapOf<Int, Float>()

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        channel = MethodChannel(flutterEngine.dartExecutor.binaryMessenger, CHANNEL)
    }

    // This is the core function that handles joystick, trigger, and D-pad hat movements.
    override fun onGenericMotionEvent(event: MotionEvent): Boolean {
        // We only care about joystick or gamepad sources.
        if (event.source and InputDevice.SOURCE_JOYSTICK != InputDevice.SOURCE_JOYSTICK &&
            event.source and InputDevice.SOURCE_GAMEPAD != InputDevice.SOURCE_GAMEPAD) {
            return super.onGenericMotionEvent(event)
        }

        // Iterate through all the motion ranges (axes) the controller has.
        event.device.motionRanges.forEach { range ->
            val axis = range.axis
            val value = event.getAxisValue(axis)
            // If the value is inside the "flat" deadzone, treat it as 0.
            if (Math.abs(value) > range.flat) {
                axisState[axis] = value
            } else {
                axisState[axis] = 0f
            }
        }

        // Convert the integer keys to readable strings for Flutter.
        val stringAxisState = axisState.mapKeys { MotionEvent.axisToString(it.key) }

        // Send the entire map of axis values to Flutter.
        channel?.invokeMethod("onMotionEvent", stringAxisState)

        return true
    }

    // This handles button presses (down).
    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        // Ignore repeats to only get the initial press.
        if (event.repeatCount > 0) {
            return true
        }

        if (event.source and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD) {
            val keyString = KeyEvent.keyCodeToString(keyCode)
            channel?.invokeMethod("onButtonDown", mapOf("button" to keyString))
            return true
        }
        return super.onKeyDown(keyCode, event)
    }

    // This handles button releases (up).
    override fun onKeyUp(keyCode: Int, event: KeyEvent): Boolean {
        if (event.source and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD) {
            val keyString = KeyEvent.keyCodeToString(keyCode)
            channel?.invokeMethod("onButtonUp", mapOf("button" to keyString))
            return true
        }
        return super.onKeyUp(keyCode, event)
    }
}