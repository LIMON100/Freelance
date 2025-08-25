//package com.example.strjk
//
//import android.content.Context
//import android.view.SurfaceHolder
//import android.view.View
//import io.flutter.plugin.common.BinaryMessenger
//import io.flutter.plugin.common.MethodCall
//import io.flutter.plugin.common.MethodChannel
//import io.flutter.plugin.platform.PlatformView
//import org.freedesktop.gstreamer.GStreamer
//import org.freedesktop.gstreamer.GStreamerSurfaceView // Make sure this import is correct
//
//// Corrected class signature and constructor
//internal class GStreamerView(
//    context: Context,
//    id: Int,
//    creationParams: Map<String?, Any?>?,
//    messenger: BinaryMessenger // Needs the messenger to create a MethodChannel
//) : PlatformView, MethodChannel.MethodCallHandler, SurfaceHolder.Callback {
//
//    // 1. The Native View
//    private val surfaceView: GStreamerSurfaceView = GStreamerSurfaceView(context)
//    private val methodChannel: MethodChannel
//
//    // 2. GStreamer Native Variables
//    private var native_custom_data: Long = 0 // Matches the C code
////    var native_custom_data: Long = 0
//    private var is_playing_desired = false
//
//    init {
//        // Initialize GStreamer
//        try {
//            GStreamer.init(context.applicationContext)
//        } catch (e: Exception) {
//            // It's better to log the error
//            e.printStackTrace()
//        }
//
//        // Initialize the native controller part
//        nativeControllerInit()
//
//        surfaceView.holder.addCallback(this)
//
//        // Setup the communication channel using the provided messenger
//        methodChannel = MethodChannel(messenger, "gstreamer_channel_$id")
//        methodChannel.setMethodCallHandler(this)
//    }
//
//    override fun getView(): View {
//        return surfaceView
//    }
//
//    override fun dispose() {
//        nativeFinalize()
//    }
//
//    // 3. Handle calls from Flutter
//    override fun onMethodCall(call: MethodCall, result: MethodChannel.Result) {
//        when (call.method) {
//            "startStream" -> {
//                val url = call.argument<String>("url")
//                if (url != null) {
//                    val gstDesc = "rtspsrc location=$url protocols=tcp latency=50 ! decodebin3 ! videoconvert ! autovideosink"
//                    nativeInit(gstDesc)
//                    is_playing_desired = true
//                    result.success(null)
//                } else {
//                    result.error("INVALID_ARGS", "URL is null", null)
//                }
//            }
//            "play" -> {
//                nativePlay()
//                result.success(null)
//            }
//            "pause" -> {
//                nativePause()
//                result.success(null)
//            }
//            else -> result.notImplemented()
//        }
//    }
//
//    // 4. SurfaceHolder Callbacks to interact with native code
//    override fun surfaceCreated(holder: SurfaceHolder) {
//        // This is handled by surfaceChanged
//    }
//
//    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
//        nativeSurfaceInit(holder.surface)
//    }
//
//    override fun surfaceDestroyed(holder: SurfaceHolder) {
//        nativeSurfaceFinalize()
//    }
//
//    // 5. Native GStreamer methods
//    private external fun nativeControllerInit()
//    private external fun nativeInit(gst_desc: String)
//    private external fun nativeFinalize()
//    private external fun nativePlay()
//    private external fun nativePause()
//    private external fun nativeSurfaceInit(surface: Any)
//    private external fun nativeSurfaceFinalize()
//
//    // Dummy methods to satisfy the JNI linking, can be implemented if needed
//    private fun setMessage(message: String) { /* Handle messages from C */ }
//    private fun onGStreamerInitialized() {
//        if (is_playing_desired) {
//            nativePlay()
//        }
//    }
//
//    companion object {
//        @JvmStatic
//        private external fun nativeClassInit(): Boolean
//        init {
//            System.loadLibrary("gstreamer_android")
//            System.loadLibrary("main") // Your C code library
//            nativeClassInit()
//        }
//    }
//}
//
//
//



package com.example.strjk

import android.content.Context
import android.view.SurfaceHolder
import android.view.View
import io.flutter.plugin.common.BinaryMessenger
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import io.flutter.plugin.platform.PlatformView
import org.freedesktop.gstreamer.GStreamer // Import GStreamer library
import org.freedesktop.gstreamer.GStreamerSurfaceView // Import custom SurfaceView

// Represents the native Android view for GStreamer playback.
// Implements PlatformView for integration with Flutter and MethodCallHandler for Dart communication.
internal class GStreamerView(
    context: Context,
    id: Int,
    creationParams: Map<String?, Any?>?, // Parameters passed from Flutter
    messenger: BinaryMessenger // Messenger for MethodChannel communication
) : PlatformView, MethodChannel.MethodCallHandler, SurfaceHolder.Callback {

    // The native GStreamer SurfaceView
    private val surfaceView: GStreamerSurfaceView = GStreamerSurfaceView(context)
    private val methodChannel: MethodChannel

    // GStreamer related variables
    private var native_custom_data: Long = 0 // Holds native GStreamer controller data pointer
    private var is_playing_desired = false // Flag to indicate if playback should start

    init {
        // Initialize GStreamer framework. This is crucial.
        try {
            // Use application context to ensure GStreamer is initialized once for the app.
            GStreamer.init(context.applicationContext)
        } catch (e: Exception) {
            // Log any initialization errors.
            e.printStackTrace()
            // Consider sending an error back to Flutter here if initialization fails critically.
        }

        // Initialize the native GStreamer controller part via JNI.
        nativeControllerInit()

        // Add this class as a callback for the SurfaceHolder to manage the lifecycle of the drawing surface.
        surfaceView.holder.addCallback(this)

        // Set up the MethodChannel to communicate with Flutter.
        // The channel name must match the one used in main.dart's AndroidView.
        methodChannel = MethodChannel(messenger, "gstreamer_channel_$id")
        methodChannel.setMethodCallHandler(this)
    }

    // Returns the native Android View to be embedded in the Flutter UI.
    override fun getView(): View {
        return surfaceView
    }

    // Called when the platform view is no longer needed. Cleans up native resources.
    override fun dispose() {
        // Stop playback and release native resources.
        nativeSurfaceFinalize() // Ensure surface resources are freed
        nativeFinalize()      // Clean up GStreamer controller resources
        methodChannel.setMethodCallHandler(null) // Remove channel handler
    }

    // Handles method calls from Flutter.
    override fun onMethodCall(call: MethodCall, result: MethodChannel.Result) {
        when (call.method) {
            "startStream" -> {
                // Expecting 'url' and potentially 'pipeline' from the arguments.
                val url = call.argument<String>("url")
                //val pipeline = call.argument<String>("pipeline") // Use this if you want to pass custom pipelines

                if (url != null) {
                    // Construct a default GStreamer pipeline if not provided.
                    // This pipeline takes an RTSP stream, decodes it, converts format, and renders.
                    // You might adjust this pipeline string for better performance or specific codecs.
                    // Example: "rtspsrc location=$url latency=200 ! decodebin ! videoconvert ! autovideosink"
                    // Using decodebin3 for potentially better decoding support.
                    val gstDesc = "rtspsrc location=$url protocols=tcp latency=50 ! decodebin3 ! videoconvert ! autovideosink"

                    // Initialize GStreamer with the pipeline description.
                    nativeInit(gstDesc)
                    is_playing_desired = true // Mark that we want playback to start
                    result.success(null) // Acknowledge successful call
                } else {
                    // If URL is missing, return an error.
                    result.error("INVALID_ARGS", "URL is null or not provided", null)
                }
            }
            "play" -> {
                nativePlay() // Start or resume playback
                result.success(null)
            }
            "pause" -> {
                nativePause() // Pause playback
                result.success(null)
            }
            "stopStream" -> { // Added a method to explicitly stop the stream
                nativePause() // Pause the stream
                is_playing_desired = false // Reset playback desire
                result.success(null)
            }
            else -> result.notImplemented() // Handle unknown method calls
        }
    }

    // --- SurfaceHolder Callbacks ---
    // Called when the surface is created. In our case, surfaceChanged handles initialization.
    override fun surfaceCreated(holder: SurfaceHolder) {
        // The actual initialization happens in surfaceChanged when dimensions are known.
    }

    // Called when the surface's properties change (format, size).
    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        // Initialize GStreamer with the native surface object.
        // This is where GStreamer gets the drawing canvas.
        nativeSurfaceInit(holder.surface)

        // If playback was desired, attempt to start it now that the surface is ready.
        if (is_playing_desired) {
            nativePlay()
        }
    }

    // Called when the surface is being destroyed.
    override fun surfaceDestroyed(holder: SurfaceHolder) {
        // Finalize GStreamer's use of the surface.
        nativeSurfaceFinalize()
        // Optionally, you might want to pause playback here too if the user navigates away.
        // nativePause(); is_playing_desired = false;
    }

    // --- Native GStreamer Methods (declared as external) ---
    // These functions are implemented in native C/C++ code and called via JNI.
    private external fun nativeControllerInit(): Boolean // Initializes the GStreamer controller part
    private external fun nativeInit(gst_desc: String): Boolean // Initializes GStreamer with a pipeline
    private external fun nativeFinalize(): Boolean // Cleans up GStreamer controller resources
    private external fun nativePlay(): Boolean // Starts or resumes playback
    private external fun nativePause(): Boolean // Pauses playback
    private external fun nativeSurfaceInit(surface: Any): Boolean // Initializes GStreamer with the drawing surface
    private external fun nativeSurfaceFinalize(): Boolean // Finalizes GStreamer's use of the surface

    // --- Companion object for JNI library loading ---
    companion object {
        // Initialize GStreamer libraries. This must be done before any native methods are called.
        @JvmStatic
        private external fun nativeClassInit(): Boolean // Static initializer for class-level JNI setup
        init {
            // Load the GStreamer core library and your custom native library.
            // Ensure these .so files are correctly placed in your Android project's JNI libraries.
            System.loadLibrary("gstreamer_android") // Required for GStreamer integration
            System.loadLibrary("main")             // Your custom native library (e.g., for GStreamer pipeline logic)
            // Perform any static native initialization.
            nativeClassInit()
        }
    }

    // Placeholder for potential callbacks from native code to Dart.
    // If your native code needs to send messages back to Dart, you would implement
    // a corresponding native function that calls this Kotlin method, which then
    // uses the MethodChannel to send data back.
    private fun setMessage(message: String) {
        // Example: methodChannel.invokeMethod("setMessage", message)
    }

    // This function might be called by native code after initialization to signal readiness.
    private fun onGStreamerInitialized() {
        if (is_playing_desired) {
            nativePlay()
        }
    }
}