//package com.example.strjk
//
//import android.content.Context
//import io.flutter.plugin.common.BinaryMessenger
//import io.flutter.plugin.common.StandardMessageCodec
//import io.flutter.plugin.platform.PlatformView
//import io.flutter.plugin.platform.PlatformViewFactory
//
//// Add a constructor to receive the BinaryMessenger
//class GStreamerViewFactory(private val messenger: BinaryMessenger) : PlatformViewFactory(StandardMessageCodec.INSTANCE) {
//    override fun create(context: Context, viewId: Int, args: Any?): PlatformView {
//        val creationParams = args as Map<String?, Any?>?
//        // Pass the messenger to the GStreamerView
//        return GStreamerView(context, viewId, creationParams, messenger)
//    }
//}


package com.example.strjk

import android.content.Context
import io.flutter.plugin.common.BinaryMessenger
import io.flutter.plugin.common.StandardMessageCodec
import io.flutter.plugin.platform.PlatformView
import io.flutter.plugin.platform.PlatformViewFactory

// Factory for creating GStreamerView instances.
// It needs the BinaryMessenger to establish communication with Flutter.
class GStreamerViewFactory(private val messenger: BinaryMessenger) : PlatformViewFactory(StandardMessageCodec.INSTANCE) {

    // This method is called by Flutter when it needs to create a new instance of the platform view.
    override fun create(context: Context, viewId: Int, args: Any?): PlatformView {
        // Arguments passed from Flutter can be anything, but we expect a Map here.
        // Cast arguments to the expected type.
        val creationParams = args as Map<String?, Any?>?

        // Create and return a new instance of GStreamerView, passing all necessary parameters.
        return GStreamerView(context, viewId, creationParams, messenger)
    }
}