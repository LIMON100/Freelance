package com.example.rtest2 // Your package name

import android.content.Context
import android.view.SurfaceHolder
import android.view.View
import io.flutter.plugin.common.BinaryMessenger
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import io.flutter.plugin.platform.PlatformView
import org.freedesktop.gstreamer.GStreamer
import org.freedesktop.gstreamer.GStreamerSurfaceView // Make sure this import is correct

// Corrected class signature and constructor
internal class GStreamerView(
    context: Context,
    id: Int,
    creationParams: Map<String?, Any?>?,
    messenger: BinaryMessenger // Needs the messenger to create a MethodChannel
) : PlatformView, MethodChannel.MethodCallHandler, SurfaceHolder.Callback {

    // 1. The Native View
    private val surfaceView: GStreamerSurfaceView = GStreamerSurfaceView(context)
    private val methodChannel: MethodChannel

    // 2. GStreamer Native Variables
    private var native_custom_data: Long = 0 // Matches the C code
//    var native_custom_data: Long = 0
    private var is_playing_desired = false

    init {
        // Initialize GStreamer
        try {
            GStreamer.init(context.applicationContext)
        } catch (e: Exception) {
            // It's better to log the error
            e.printStackTrace()
        }

        // Initialize the native controller part
        nativeControllerInit()

        surfaceView.holder.addCallback(this)

        // Setup the communication channel using the provided messenger
        methodChannel = MethodChannel(messenger, "gstreamer_channel_$id")
        methodChannel.setMethodCallHandler(this)
    }

    override fun getView(): View {
        return surfaceView
    }

    override fun dispose() {
        nativeFinalize()
    }

    // 3. Handle calls from Flutter
    override fun onMethodCall(call: MethodCall, result: MethodChannel.Result) {
        when (call.method) {
            "startStream" -> {
                val url = call.argument<String>("url")
                if (url != null) {
                    val gstDesc = "rtspsrc location=$url protocols=tcp latency=50 ! decodebin3 ! videoconvert ! autovideosink"
                    nativeInit(gstDesc)
                    is_playing_desired = true
                    result.success(null)
                } else {
                    result.error("INVALID_ARGS", "URL is null", null)
                }
            }
            "play" -> {
                nativePlay()
                result.success(null)
            }
            "pause" -> {
                nativePause()
                result.success(null)
            }
            else -> result.notImplemented()
        }
    }

    // 4. SurfaceHolder Callbacks to interact with native code
    override fun surfaceCreated(holder: SurfaceHolder) {
        // This is handled by surfaceChanged
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        nativeSurfaceInit(holder.surface)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        nativeSurfaceFinalize()
    }

    // 5. Native GStreamer methods
    private external fun nativeControllerInit()
    private external fun nativeInit(gst_desc: String)
    private external fun nativeFinalize()
    private external fun nativePlay()
    private external fun nativePause()
    private external fun nativeSurfaceInit(surface: Any)
    private external fun nativeSurfaceFinalize()

    // Dummy methods to satisfy the JNI linking, can be implemented if needed
    private fun setMessage(message: String) { /* Handle messages from C */ }
    private fun onGStreamerInitialized() {
        if (is_playing_desired) {
            nativePlay()
        }
    }

    companion object {
        @JvmStatic
        private external fun nativeClassInit(): Boolean
        init {
            System.loadLibrary("gstreamer_android")
            System.loadLibrary("main") // Your C code library
            nativeClassInit()
        }
    }
}