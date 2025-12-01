#include <string.h>
#include <stdint.h>
#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <pthread.h>
#include <unistd.h>

GST_DEBUG_CATEGORY_STATIC (debug_category);
#define GST_CAT_DEFAULT debug_category

#if GLIB_SIZEOF_VOID_P == 8
# define GET_CUSTOM_DATA(env, thiz, fieldID) (CustomData *)(*env)->GetLongField (env, thiz, fieldID)
# define SET_CUSTOM_DATA(env, thiz, fieldID, data) (*env)->SetLongField (env, thiz, fieldID, (jlong)data)
#else
# define GET_CUSTOM_DATA(env, thiz, fieldID) (CustomData *)(jint)(*env)->GetLongField (env, thiz, fieldID)
# define SET_CUSTOM_DATA(env, thiz, fieldID, data) (*env)->SetLongField (env, thiz, fieldID, (jlong)(jint)data)
#endif

typedef struct _CustomData {
    jobject app;
    GstElement *pipeline;
    GMainContext *context;
    GMainLoop *main_loop;
    gboolean initialized;
    GstElement *video_sink;
    ANativeWindow *native_window;
    gchar *gst_desc;
    gboolean main_loop_quit;
    pthread_t gst_app_thread; // <-- MOVE THE THREAD HANDLE HERE
} CustomData;

//static pthread_t gst_app_thread;
static pthread_key_t current_jni_env;
static JavaVM *java_vm;
static jfieldID custom_data_field_id;
static jmethodID set_message_method_id;
static jmethodID on_gstreamer_initialized_method_id;
static jmethodID on_stream_ready_method_id;
static jmethodID on_stream_error_method_id;

static JNIEnv * attach_current_thread(void) {
    JNIEnv *env;
    JavaVMAttachArgs args;
    GST_DEBUG ("Attaching thread %p", g_thread_self());
    args.version = JNI_VERSION_1_6;
    args.name = NULL;
    args.group = NULL;
    if ((*java_vm)->AttachCurrentThread(java_vm, &env, &args) < 0) {
        GST_ERROR ("Failed to attach current thread");
        return NULL;
    }
    return env;
}

static void detach_current_thread(void *env) {
    GST_DEBUG ("Detaching thread %p", g_thread_self());
    (*java_vm)->DetachCurrentThread(java_vm);
}

static JNIEnv * get_jni_env(void) {
    JNIEnv *env;
    if ((env = pthread_getspecific(current_jni_env)) == NULL) {
        env = attach_current_thread();
        pthread_setspecific(current_jni_env, env);
    }
    return env;
}

static void set_ui_message(const gchar *message, CustomData *data) {
    JNIEnv *env = get_jni_env();
    GST_DEBUG ("Setting message to: %s", message);
    jstring jmessage = (*env)->NewStringUTF(env, message);
    (*env)->CallVoidMethod(env, data->app, set_message_method_id, jmessage);
    if ((*env)->ExceptionCheck(env)) {
        GST_ERROR ("Failed to call Java method");
        (*env)->ExceptionClear(env);
    }
    (*env)->DeleteLocalRef(env, jmessage);
}

static void error_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
    GError *err;
    gchar *debug_info;
    gchar *message_string;
    JNIEnv *env = get_jni_env();

    gst_message_parse_error(msg, &err, &debug_info);
    message_string = g_strdup_printf("Error received from element %s: %s", GST_OBJECT_NAME (msg->src), err->message);
    g_clear_error(&err);
    g_free(debug_info);

    jstring jmessage = (*env)->NewStringUTF(env, message_string);
    (*env)->CallVoidMethod(env, data->app, on_stream_error_method_id, jmessage);
    if ((*env)->ExceptionCheck(env)) {
        GST_ERROR ("Failed to call Java method onStreamError");
        (*env)->ExceptionClear(env);
    }
    (*env)->DeleteLocalRef(env, jmessage);
    g_free(message_string);
    if (data && data->pipeline) {
        gst_element_set_state(data->pipeline, GST_STATE_NULL);
    }
}

static void state_changed_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
    GstState old_state, new_state, pending_state;
    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
    if (data && GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
        gchar *message = g_strdup_printf("State changed from %s to %s", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
        set_ui_message(message, data);
        g_free(message);

        if (new_state == GST_STATE_PLAYING) {
            JNIEnv *env = get_jni_env();
            (*env)->CallVoidMethod(env, data->app, on_stream_ready_method_id);
            if ((*env)->ExceptionCheck(env)) {
                GST_ERROR ("Failed to call Java method onStreamReady");
                (*env)->ExceptionClear(env);
            }
        }
    }
}

static void check_initialization_complete(CustomData *data) {
    JNIEnv *env = get_jni_env();
    if (!data->initialized && data->native_window && data->main_loop) {
        GST_DEBUG("Initialization complete, notifying application. native_window:%p main_loop:%p", data->native_window, data->main_loop);
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY (data->video_sink), (guintptr) data->native_window);
        (*env)->CallVoidMethod(env, data->app, on_gstreamer_initialized_method_id);
        if ((*env)->ExceptionCheck(env)) {
            GST_ERROR ("Failed to call Java method");
            (*env)->ExceptionClear(env);
        }
        data->initialized = TRUE;
    }
}

static void * app_function(void *userdata) {
    GstBus *bus;
    CustomData *data = (CustomData *) userdata;
    GSource *bus_source;
    GError *error = NULL;

    GST_DEBUG ("Creating pipeline in CustomData at %p", data);
    data->context = g_main_context_new();
    g_main_context_push_thread_default(data->context);

    data->pipeline = gst_parse_launch(data->gst_desc, &error);
    if (error) {
        gchar *message = g_strdup_printf("Unable to build pipeline: %s", error->message);
        g_clear_error(&error);
        JNIEnv *env = get_jni_env();
        jstring jmessage = (*env)->NewStringUTF(env, message);
        (*env)->CallVoidMethod(env, data->app, on_stream_error_method_id, jmessage);
        (*env)->DeleteLocalRef(env, jmessage);
        g_free(message);
        return NULL;
    }

    gst_element_set_state(data->pipeline, GST_STATE_READY);
    data->video_sink = gst_bin_get_by_interface(GST_BIN (data->pipeline), GST_TYPE_VIDEO_OVERLAY);
    if (!data->video_sink) {
        GST_ERROR ("Could not retrieve video sink");
        return NULL;
    }

    bus = gst_element_get_bus(data->pipeline);
    bus_source = gst_bus_create_watch(bus);
    g_source_set_callback(bus_source, (GSourceFunc) gst_bus_async_signal_func, NULL, NULL);
    g_source_attach(bus_source, data->context);
    g_source_unref(bus_source);
    g_signal_connect (G_OBJECT(bus), "message::error", (GCallback) error_cb, data);
    g_signal_connect (G_OBJECT(bus), "message::state-changed", (GCallback) state_changed_cb, data);
    gst_object_unref(bus);

    GST_DEBUG ("Entering main loop... (CustomData:%p)", data);
    data->main_loop = g_main_loop_new(data->context, FALSE);
    check_initialization_complete(data);
    g_main_loop_run(data->main_loop);
    GST_DEBUG ("Exited main loop");
    g_main_loop_unref(data->main_loop);
    data->main_loop = NULL;

    g_main_context_pop_thread_default(data->context);
    g_main_context_unref(data->context);
    gst_element_set_state(data->pipeline, GST_STATE_NULL);
    if(data->video_sink) gst_object_unref(data->video_sink);
    if(data->pipeline) gst_object_unref(data->pipeline);
    data->pipeline = NULL;
    data->video_sink = NULL;

    return NULL;
}

static void gst_controller_init(JNIEnv *env, jobject thiz) {
    CustomData *data = g_new0 (CustomData, 1);
    SET_CUSTOM_DATA (env, thiz, custom_data_field_id, data);
    GST_DEBUG_CATEGORY_INIT (debug_category, "SkyAutoNet", 0, "SkyAutoNet");
    gst_debug_set_threshold_for_name("SkyAutoNet", GST_LEVEL_DEBUG);
    GST_DEBUG ("Created CustomData at %p", data);
    data->app = (*env)->NewGlobalRef(env, thiz);
    GST_DEBUG ("Created GlobalRef for app object at %p", data->app);
}

static void
gst_native_init(JNIEnv *env, jobject thiz, jstring gst_desc) {
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data) return;

    if (data->gst_app_thread != 0) {
        GST_WARNING("This instance already has a GStreamer thread running. Finalize it first.");
        return;
    }

    const gchar *char_gst_desc = (*env)->GetStringUTFChars(env, gst_desc, NULL);
    data->gst_desc = g_strdup(char_gst_desc);
    GST_DEBUG ("Setting pipeline description to %s", char_gst_desc);
    (*env)->ReleaseStringUTFChars(env, gst_desc, char_gst_desc);

    pthread_create(&data->gst_app_thread, NULL, &app_function, data);
}

static void gst_native_finalize(JNIEnv *env, jobject thiz) {
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data) return;

    GST_DEBUG ("Finalize called for instance %p. Quitting main loop...", data);

    // 1. Signal the GStreamer thread to exit its main loop.
    if (data->main_loop && g_main_loop_is_running(data->main_loop)) {
        g_main_loop_quit(data->main_loop);
    }

    // 2. Wait for the GStreamer thread to finish its cleanup.
    if (data->gst_app_thread != 0) {
        GST_DEBUG ("Waiting for instance thread %lu to finish...", (unsigned long)data->gst_app_thread);
        pthread_join(data->gst_app_thread, NULL);
        GST_DEBUG ("Instance thread %lu finished.", (unsigned long)data->gst_app_thread);
        data->gst_app_thread = 0;
    }

    // 3. Clean up the remaining Java and C objects.
    if (data->gst_desc) {
        g_free(data->gst_desc);
        data->gst_desc = NULL;
    }
    if (data->app) {
        (*env)->DeleteGlobalRef(env, data->app);
        data->app = NULL;
    }

    GST_DEBUG ("Freeing CustomData at %p", data);
    g_free(data);
    SET_CUSTOM_DATA (env, thiz, custom_data_field_id, NULL);
    GST_DEBUG ("Done finalizing instance.");
}

static void gst_native_play(JNIEnv *env, jobject thiz) {
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data || !data->pipeline) return;
    GST_DEBUG ("Setting state to PLAYING");
    gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
}

static void gst_native_pause(JNIEnv *env, jobject thiz) {
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data || !data->pipeline) return;
    GST_DEBUG ("Setting state to PAUSED");
    gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
}

static jboolean gst_native_class_init(JNIEnv *env, jclass klass) {
    custom_data_field_id = (*env)->GetFieldID(env, klass, "native_custom_data", "J");
    set_message_method_id = (*env)->GetMethodID(env, klass, "setMessage", "(Ljava/lang/String;)V");
    on_gstreamer_initialized_method_id = (*env)->GetMethodID(env, klass, "onGStreamerInitialized", "()V");
    on_stream_ready_method_id = (*env)->GetMethodID(env, klass, "onStreamReady", "()V");
    on_stream_error_method_id = (*env)->GetMethodID(env, klass, "onStreamError", "(Ljava/lang/String;)V");

    if (!custom_data_field_id || !set_message_method_id || !on_gstreamer_initialized_method_id || !on_stream_ready_method_id || !on_stream_error_method_id) {
        __android_log_print(ANDROID_LOG_ERROR, "SkyAutoNet", "The calling class does not implement all necessary interface methods");
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

static void gst_native_surface_init(JNIEnv *env, jobject thiz, jobject surface) {
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data) return;
    ANativeWindow *new_native_window = ANativeWindow_fromSurface(env, surface);
    GST_DEBUG ("Received surface %p (native window %p)", surface, new_native_window);

    if (data->native_window) {
        ANativeWindow_release(data->native_window);
        if (data->native_window == new_native_window) {
            GST_DEBUG ("New native window is the same as the previous one %p", data->native_window);
            if (data->video_sink) {
                gst_video_overlay_expose(GST_VIDEO_OVERLAY (data->video_sink));
                gst_video_overlay_expose(GST_VIDEO_OVERLAY (data->video_sink));
            }
            ANativeWindow_release(new_native_window);
            return;
        } else {
            GST_DEBUG ("Released previous native window %p", data->native_window);
            data->initialized = FALSE;
        }
    }
    data->native_window = new_native_window;
    check_initialization_complete(data);
}

static void gst_native_surface_finalize(JNIEnv *env, jobject thiz) {
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data) return;

    GST_DEBUG ("Surface is being destroyed. Informing GStreamer thread.");
    if (data->video_sink) {
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY (data->video_sink), (guintptr) NULL);
    }

    if (data->native_window) {
        ANativeWindow_release(data->native_window);
        data->native_window = NULL;
    }
    data->initialized = FALSE;
    GST_DEBUG ("Native window detached successfully.");
}


static JNINativeMethod native_methods[] = {
        {"nativeInit", "(Ljava/lang/String;)V", (void *) gst_native_init},
        {"nativeControllerInit", "()V", (void *) gst_controller_init},
        {"nativeFinalize", "()V", (void *) gst_native_finalize},
        {"nativePlay", "()V", (void *) gst_native_play},
        {"nativePause", "()V", (void *) gst_native_pause},
        {"nativeSurfaceInit", "(Ljava/lang/Object;)V", (void *) gst_native_surface_init},
        {"nativeSurfaceFinalize", "()V", (void *) gst_native_surface_finalize},
        {"nativeClassInit", "()Z", (void *) gst_native_class_init},
};

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    java_vm = vm;
    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "SkyAutoNet", "Could not retrieve JNIEnv");
        return 0;
    }
    jclass klass = (*env)->FindClass(env, "com/example/strjk/GStreamerView");
    (*env)->RegisterNatives(env, klass, native_methods, G_N_ELEMENTS (native_methods));
    pthread_key_create(&current_jni_env, detach_current_thread);
    return JNI_VERSION_1_6;
}
