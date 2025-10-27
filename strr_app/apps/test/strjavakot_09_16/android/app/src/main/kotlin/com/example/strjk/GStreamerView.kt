package com.example.strjk

import android.content.Context
import android.util.Log
import android.view.SurfaceHolder
import android.view.View
import io.flutter.plugin.common.BinaryMessenger
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import io.flutter.plugin.platform.PlatformView
import org.freedesktop.gstreamer.GStreamer
import org.freedesktop.gstreamer.GStreamerSurfaceView

internal class GStreamerView(
    context: Context,
    id: Int,
    creationParams: Map<String?, Any?>?,
    messenger: BinaryMessenger
) : PlatformView, MethodChannel.MethodCallHandler, SurfaceHolder.Callback {

    private val surfaceView: GStreamerSurfaceView = GStreamerSurfaceView(context)
    private val methodChannel: MethodChannel
    private val TAG = "GStreamerView"

    private var native_custom_data: Long = 0
    private var is_playing_desired = false

    init {
        try {
            GStreamer.init(context.applicationContext)
        } catch (e: Exception) {
            Log.e(TAG, "GStreamer initialization failed: ${e.message}")
            e.printStackTrace()
        }

        nativeControllerInit()
        surfaceView.holder.addCallback(this)

        methodChannel = MethodChannel(messenger, "gstreamer_channel_$id")
        methodChannel.setMethodCallHandler(this)
    }

    override fun getView(): View {
        return surfaceView
    }

    override fun dispose() {
        Log.d(TAG, "Dispose called. Finalizing native resources.")
        is_playing_desired = false
        nativeFinalize()
    }

    override fun onMethodCall(call: MethodCall, result: MethodChannel.Result) {
        when (call.method) {
            "startStream" -> {
                val url = call.argument<String>("url")
                if (url != null) {
                    val gstDesc = "rtspsrc location=$url protocols=tcp latency=50 ! decodebin3 ! videoconvert ! autovideosink"
                    Log.d(TAG, "Initializing native with pipeline: $gstDesc")
                    nativeInit(gstDesc)
                    is_playing_desired = true
                    result.success(null)
                } else {
                    result.error("INVALID_ARGS", "URL is null", null)
                }
            }
            "stopStream" -> {
                Log.d(TAG, "Stopping stream.")
                is_playing_desired = false
                nativePause()
                nativeFinalize() // Finalize to clean up the old pipeline
                nativeControllerInit() // Re-initialize for the next stream
                result.success(null)
            }
            "isStreamActive" -> {
                result.success(is_playing_desired)
            }
            else -> result.notImplemented()
        }
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        Log.d(TAG, "Surface created: ${holder.surface}")
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        Log.d(TAG, "Surface changed to format $format, width $width, height $height")
        nativeSurfaceInit(holder.surface)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        Log.d(TAG, "Surface destroyed")
        nativeSurfaceFinalize()
    }

    // --- Native GStreamer Methods ---
    private external fun nativeControllerInit()
    private external fun nativeInit(gst_desc: String)
    private external fun nativeFinalize()
    private external fun nativePlay()
    private external fun nativePause()
    private external fun nativeSurfaceInit(surface: Any)
    private external fun nativeSurfaceFinalize()

    // --- Methods Called FROM C Code (JNI Callbacks) ---

    private fun setMessage(message: String) {
        Log.d(TAG, "Native Message: $message")
    }

    private fun onGStreamerInitialized() {
        Log.d(TAG, "GStreamer Initialized.")
        if (is_playing_desired) {
            Log.d(TAG, "Playing desired, calling nativePlay()")
            nativePlay()
        }
    }

    // Called from C when pipeline state changes to PLAYING
    fun onStreamReady() {
        Log.i(TAG, "Stream is ready and playing. Notifying Flutter.")
        // Ensure we're on the main thread to call Flutter UI methods
        surfaceView.post {
            methodChannel.invokeMethod("onStreamReady", null)
        }
    }

    // Called from C when the GStreamer bus posts an error
    fun onStreamError(errorMessage: String) {
        Log.e(TAG, "Native error received: $errorMessage. Notifying Flutter.")
        val errorDetails = mapOf("error" to errorMessage)
        surfaceView.post {
            methodChannel.invokeMethod("onStreamError", errorDetails)
        }
    }

    companion object {
        @JvmStatic
        private external fun nativeClassInit(): Boolean
        init {
            System.loadLibrary("gstreamer_android")
            System.loadLibrary("main")
            nativeClassInit()
        }
    }
}