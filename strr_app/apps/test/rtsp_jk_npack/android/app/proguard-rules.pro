# --- Keep Rules for GStreamer Integration ---

# Rule for your GStreamerView class with the NEW package name.
-keep class com.example.strjk.GStreamerView { *; }

# Specifically keep the field that JNI needs to access.
# Make sure this matches the new package name.
-keepclassmembers class com.example.strjk.GStreamerView {
    public long native_custom_data;
}

# Keep the methods that are called from the native C code.
# Make sure this matches the new package name.
-keepclassmembers class com.example.strjk.GStreamerView {
    public void setMessage(java.lang.String);
    public void onGStreamerInitialized();
}

# General rule for all GStreamer packages. This rule does not change
# as it refers to the GStreamer library's own package name.
-keep class org.freedesktop.gstreamer.** { *; }

-keep class com.longdo.zingswift.vlc.** { *; }
-dontwarn com.longdo.zingswift.vlc.**
-keep class org.videolan.libvlc.** { *; }
-dontwarn org.videolan.libvlc.**