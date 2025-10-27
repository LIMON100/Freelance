//package com.example.strjk // Your package name
//
//import io.flutter.embedding.android.FlutterActivity
//import io.flutter.embedding.engine.FlutterEngine
//
//class MainActivity: FlutterActivity() {
//    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
//        super.configureFlutterEngine(flutterEngine)
//        // Pass the messenger from the flutterEngine to the factory
//        val messenger = flutterEngine.dartExecutor.binaryMessenger
//        flutterEngine
//            .platformViewsController
//            .registry
//            .registerViewFactory("gstreamer_view", GStreamerViewFactory(messenger))
//    }
//}




package com.example.strjk

import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel
import io.flutter.plugin.platform.PlatformViewRegistry // Needed for registerViewFactory

class MainActivity: FlutterActivity() {
    // Channel for gamepad events
    private val GAMEPAD_CHANNEL = "com.yourcompany/gamepad"
    private var gamepadChannel: MethodChannel? = null

    // Channel for GStreamer view communication
    // private val GSTREAMER_CHANNEL_PREFIX = "gstreamer_channel_" // Prefix for GStreamer channels

    // Map to store the current state of all axes.
    private val axisState = mutableMapOf<Int, Float>()

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        gamepadChannel = MethodChannel(flutterEngine.dartExecutor.binaryMessenger, GAMEPAD_CHANNEL)

        // Register the GStreamer view factory
        flutterEngine
            .platformViewsController
            .registry
            .registerViewFactory(
                "gstreamer_view", // This is the identifier used in Flutter's AndroidView
                GStreamerViewFactory(flutterEngine.dartExecutor.binaryMessenger) // Pass the messenger
            )
    }

    // Handles joystick, trigger, and D-pad hat movements.
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
                axisState[axis] = 0f // Reset if within deadzone
            }
        }

        // Convert the integer keys (axis codes) to readable strings for Flutter.
        val stringAxisState = axisState.mapKeys { MotionEvent.axisToString(it.key) }

        // Send the entire map of axis values to Flutter.
        gamepadChannel?.invokeMethod("onMotionEvent", stringAxisState)

        return true // Consume the event
    }

    // Handles button presses (down).
    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        // Ignore repeats to only get the initial press.
        if (event.repeatCount > 0) {
            return true
        }

        // Check if the event source is a gamepad.
        if (event.source and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD) {
            val keyString = KeyEvent.keyCodeToString(keyCode)
            gamepadChannel?.invokeMethod("onButtonDown", mapOf("button" to keyString))
            return true // Consume the event
        }
        return super.onKeyDown(keyCode, event)
    }

    // Handles button releases (up).
    override fun onKeyUp(keyCode: Int, event: KeyEvent): Boolean {
        // Check if the event source is a gamepad.
        if (event.source and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD) {
            val keyString = KeyEvent.keyCodeToString(keyCode)
            gamepadChannel?.invokeMethod("onButtonUp", mapOf("button" to keyString))
            return true // Consume the event
        }
        return super.onKeyUp(keyCode, event)
    }
}