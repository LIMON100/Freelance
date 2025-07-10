package com.example.rtest2 // Your package name

import android.content.Context
import io.flutter.plugin.common.BinaryMessenger
import io.flutter.plugin.common.StandardMessageCodec
import io.flutter.plugin.platform.PlatformView
import io.flutter.plugin.platform.PlatformViewFactory

// Add a constructor to receive the BinaryMessenger
class GStreamerViewFactory(private val messenger: BinaryMessenger) : PlatformViewFactory(StandardMessageCodec.INSTANCE) {
    override fun create(context: Context, viewId: Int, args: Any?): PlatformView {
        val creationParams = args as Map<String?, Any?>?
        // Pass the messenger to the GStreamerView
        return GStreamerView(context, viewId, creationParams, messenger)
    }
}