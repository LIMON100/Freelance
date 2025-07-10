package com.example.rtest1 // Your package name

import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine

class MainActivity: FlutterActivity() {
    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        // Pass the messenger from the flutterEngine to the factory
        val messenger = flutterEngine.dartExecutor.binaryMessenger
        flutterEngine
            .platformViewsController
            .registry
            .registerViewFactory("gstreamer_view", GStreamerViewFactory(messenger))
    }
}