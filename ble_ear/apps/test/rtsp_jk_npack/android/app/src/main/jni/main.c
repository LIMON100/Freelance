//#include <string.h>
//#include <string.h>
//#include <stdint.h>
//#include <jni.h>
//#include <android/log.h>
//#include <android/native_window.h>
//#include <android/native_window_jni.h>
//#include <gst/gst.h>
//#include <gst/video/video.h>
//#include <pthread.h>
//
//
//GST_DEBUG_CATEGORY_STATIC (debug_category);
//#define GST_CAT_DEFAULT debug_category
//
///*
// * These macros provide a way to store the native pointer to CustomData, which might be 32 or 64 bits, into
// * a jlong, which is always 64 bits, without warnings.
// */
//#if GLIB_SIZEOF_VOID_P == 8
//# define GET_CUSTOM_DATA(env, thiz, fieldID) (CustomData *)(*env)->GetLongField (env, thiz, fieldID)
//# define SET_CUSTOM_DATA(env, thiz, fieldID, data) (*env)->SetLongField (env, thiz, fieldID, (jlong)data)
//#else
//# define GET_CUSTOM_DATA(env, thiz, fieldID) (CustomData *)(jint)(*env)->GetLongField (env, thiz, fieldID)
//# define SET_CUSTOM_DATA(env, thiz, fieldID, data) (*env)->SetLongField (env, thiz, fieldID, (jlong)(jint)data)
//#endif
//
///* Structure to contain all our information, so we can pass it to callbacks */
//typedef struct _CustomData {
//    jobject app;                  /* Application instance, used to call its methods. A global reference is kept. */
//    GstElement *pipeline;         /* The running pipeline */
//    GMainContext *context;        /* GLib context used to run the main loop */
//    GMainLoop *main_loop;         /* GLib main loop */
//    gboolean initialized;         /* To avoid informing the UI multiple times about the initialization */
//    GstElement *video_sink;       /* The video sink element which receives XOverlay commands */
//    ANativeWindow *native_window; /* The Android native window where video will be rendered */
//    gchar *gst_desc;
//} CustomData;
//
///* These global variables cache values which are not changing during execution */
//static pthread_t gst_app_thread;
//static pthread_key_t current_jni_env;
//static JavaVM *java_vm;
//static jfieldID custom_data_field_id;
//static jmethodID set_message_method_id;
//static jmethodID on_gstreamer_initialized_method_id;
//
///*
// * Private methods
// */
//
///* Register this thread with the VM */
//static JNIEnv *
//attach_current_thread(void) {
//    JNIEnv *env;
//    JavaVMAttachArgs args;
//
//    GST_DEBUG ("Attaching thread %p", g_thread_self());
//    args.version = JNI_VERSION_1_6;
//    args.name = NULL;
//    args.group = NULL;
//
//    if ((*java_vm)->AttachCurrentThread(java_vm, &env, &args) < 0) {
//        GST_ERROR ("Failed to attach current thread");
//        return NULL;
//    }
//
//    return env;
//}
//
///* Unregister this thread from the VM */
//static void
//detach_current_thread(void *env) {
//    GST_DEBUG ("Detaching thread %p", g_thread_self());
//    (*java_vm)->DetachCurrentThread(java_vm);
//}
//
///* Retrieve the JNI environment for this thread */
//static JNIEnv *
//get_jni_env(void) {
//    JNIEnv *env;
//
//    if ((env = pthread_getspecific(current_jni_env)) == NULL) {
//        env = attach_current_thread();
//        pthread_setspecific(current_jni_env, env);
//    }
//
//    return env;
//}
//
///* Change the content of the UI's TextView */
//static void
//set_ui_message(const gchar *message, CustomData *data) {
//    JNIEnv *env = get_jni_env();
//    GST_DEBUG ("Setting message to: %s", message);
//    jstring jmessage = (*env)->NewStringUTF(env, message);
//    (*env)->CallVoidMethod(env, data->app, set_message_method_id, jmessage);
//    if ((*env)->ExceptionCheck(env)) {
//        GST_ERROR ("Failed to call Java method");
//        (*env)->ExceptionClear(env);
//    }
//    (*env)->DeleteLocalRef(env, jmessage);
//}
//
///* Retrieve errors from the bus and show them on the UI */
//static void
//error_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
//    GError *err;
//    gchar *debug_info;
//    gchar *message_string;
//
//    gst_message_parse_error(msg, &err, &debug_info);
//    message_string =
//            g_strdup_printf("Error received from element %s: %s",
//                            GST_OBJECT_NAME (msg->src), err->message);
//    g_clear_error(&err);
//    g_free(debug_info);
//    set_ui_message(message_string, data);
//    g_free(message_string);
//    gst_element_set_state(data->pipeline, GST_STATE_NULL);
//}
//
///* Notify UI about pipeline state changes */
//static void
//state_changed_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
//    GstState old_state, new_state, pending_state;
//    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
//    /* Only pay attention to messages coming from the pipeline, not its children */
//    if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
//        gchar *message = g_strdup_printf("State changed to %s",
//                                         gst_element_state_get_name(new_state));
//        set_ui_message(message, data);
//        g_free(message);
//    }
//}
//
///* Check if all conditions are met to report GStreamer as initialized.
// * These conditions will change depending on the application */
//static void
//check_initialization_complete(CustomData *data) {
//    JNIEnv *env = get_jni_env();
//    if (!data->initialized && data->native_window && data->main_loop) {
//        GST_DEBUG
//        ("Initialization complete, notifying application. native_window:%p main_loop:%p",
//         data->native_window, data->main_loop);
//
//        /* The main loop is running and we received a native window, inform the sink about it */
//        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY (data->video_sink),
//                                            (guintptr) data->native_window);
//
//        (*env)->CallVoidMethod(env, data->app, on_gstreamer_initialized_method_id);
//        if ((*env)->ExceptionCheck(env)) {
//            GST_ERROR ("Failed to call Java method");
//            (*env)->ExceptionClear(env);
//        }
//        data->initialized = TRUE;
//    }
//}
//
///* Main method for the native code. This is executed on its own thread. */
//static void *
//app_function(void *userdata) {
//    JavaVMAttachArgs args;
//    GstBus *bus;
//    CustomData *data = (CustomData *) userdata;
//    GSource *bus_source;
//    GError *error = NULL;
//
//    GST_DEBUG ("Creating pipeline in CustomData at %p", data);
//
//    /* Create our own GLib Main Context and make it the default one */
//    data->context = g_main_context_new();
//    g_main_context_push_thread_default(data->context);
//
//    /* Build pipeline */
//    data->pipeline =
//            //gst_parse_launch ("videotestsrc ! warptv ! videoconvert ! autovideosink",
//            gst_parse_launch(data->gst_desc,
//                             &error);
//    if (error) {
//        gchar *message =
//                g_strdup_printf("Unable to build pipeline: %s", error->message);
//        g_clear_error(&error);
//        set_ui_message(message, data);
//        g_free(message);
//        return NULL;
//    }
//
//    /* Set the pipeline to READY, so it can already accept a window handle, if we have one */
//    gst_element_set_state(data->pipeline, GST_STATE_READY);
//
//    data->video_sink =
//            gst_bin_get_by_interface(GST_BIN (data->pipeline),
//                                     GST_TYPE_VIDEO_OVERLAY);
//    if (!data->video_sink) {
//        GST_ERROR ("Could not retrieve video sink");
//        return NULL;
//    }
//
//    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
//    bus = gst_element_get_bus(data->pipeline);
//    bus_source = gst_bus_create_watch(bus);
//    g_source_set_callback(bus_source, (GSourceFunc) gst_bus_async_signal_func,
//                          NULL, NULL);
//    g_source_attach(bus_source, data->context);
//    g_source_unref(bus_source);
//    g_signal_connect (G_OBJECT(bus), "message::error", (GCallback) error_cb,
//                      data);
//    g_signal_connect (G_OBJECT(bus), "message::state-changed",
//                      (GCallback) state_changed_cb, data);
//    gst_object_unref(bus);
//
//    /* Create a GLib Main Loop and set it to run */
//    GST_DEBUG ("Entering main loop... (CustomData:%p)", data);
//    data->main_loop = g_main_loop_new(data->context, FALSE);
//    check_initialization_complete(data);
//    g_main_loop_run(data->main_loop);
//    GST_DEBUG ("Exited main loop");
//    g_main_loop_unref(data->main_loop);
//    data->main_loop = NULL;
//
//    /* Free resources */
//    g_main_context_pop_thread_default(data->context);
//    g_main_context_unref(data->context);
//    gst_element_set_state(data->pipeline, GST_STATE_NULL);
//    gst_object_unref(data->video_sink);
//    gst_object_unref(data->pipeline);
//
//    return NULL;
//}
//
///*
// * Java Bindings
// */
//
//static void gst_controller_init(JNIEnv *env, jobject thiz) {
//    CustomData *data = g_new0 (CustomData, 1);
//    SET_CUSTOM_DATA (env, thiz, custom_data_field_id, data);
//    GST_DEBUG_CATEGORY_INIT (debug_category, "SkyAutoNet", 0,
//                             "SkyAutoNet");
//    gst_debug_set_threshold_for_name("SkyAutoNet", GST_LEVEL_DEBUG);
//    GST_DEBUG ("Created CustomData at %p", data);
//    data->app = (*env)->NewGlobalRef(env, thiz);
//    GST_DEBUG ("Created GlobalRef for app object at %p", data->app);
//}
//
///* Instruct the native code to create its internal data structure, pipeline and thread */
//static void
//gst_native_init(JNIEnv *env, jobject thiz, jstring gst_desc) {
//    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
//    const gchar *char_gst_desc = (*env)->GetStringUTFChars(env, gst_desc, NULL);
//    data->gst_desc = g_strconcat(char_gst_desc, NULL);
//    GST_DEBUG ("Setting pipeline description to %s", char_gst_desc);
//    (*env)->ReleaseStringUTFChars(env, gst_desc, char_gst_desc);
//    pthread_create(&gst_app_thread, NULL, &app_function, data);
//    check_initialization_complete(data);
//}
//
///* Quit the main loop, remove the native thread and free resources */
//static void
//gst_native_finalize(JNIEnv *env, jobject thiz) {
//    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
//    if (!data)
//        return;
//    GST_DEBUG ("Quitting main loop...");
//    g_main_loop_quit(data->main_loop);
//    GST_DEBUG ("Waiting for thread to finish...");
//    pthread_join(gst_app_thread, NULL);
//    g_free(data->gst_desc);
//    GST_DEBUG ("Deleting GlobalRef for app object at %p", data->app);
//    (*env)->DeleteGlobalRef(env, data->app);
//    GST_DEBUG ("Freeing CustomData at %p", data);
//    g_free(data);
//    SET_CUSTOM_DATA (env, thiz, custom_data_field_id, NULL);
//    GST_DEBUG ("Done finalizing");
//}
//
///* Set pipeline to PLAYING state */
//static void
//gst_native_play(JNIEnv *env, jobject thiz) {
//    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
//    if (!data)
//        return;
//    GST_DEBUG ("Setting state to PLAYING");
//    gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
//}
//
///* Set pipeline to PAUSED state */
//static void
//gst_native_pause(JNIEnv *env, jobject thiz) {
//    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
//    if (!data)
//        return;
//    GST_DEBUG ("Setting state to PAUSED");
//    gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
//}
//
///* Static class initializer: retrieve method and field IDs */
//static jboolean
//gst_native_class_init(JNIEnv *env, jclass klass) {
//    custom_data_field_id =
//            (*env)->GetFieldID(env, klass, "native_custom_data", "J");
//    set_message_method_id =
//            (*env)->GetMethodID(env, klass, "setMessage", "(Ljava/lang/String;)V");
//    on_gstreamer_initialized_method_id =
//            (*env)->GetMethodID(env, klass, "onGStreamerInitialized", "()V");
//
//    if (!custom_data_field_id || !set_message_method_id
//        || !on_gstreamer_initialized_method_id) {
//        /* We emit this message through the Android log instead of the GStreamer log because the later
//         * has not been initialized yet.
//         */
//        __android_log_print(ANDROID_LOG_ERROR, "tutorial-3",
//                            "The calling class does not implement all necessary interface methods");
//        return JNI_FALSE;
//    }
//    return JNI_TRUE;
//}
//
//static void
//gst_native_surface_init(JNIEnv *env, jobject thiz, jobject surface) {
//    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
//    if (!data)
//        return;
//    ANativeWindow *new_native_window = ANativeWindow_fromSurface(env, surface);
//    GST_DEBUG ("Received surface %p (native window %p)", surface,
//               new_native_window);
//
//    if (data->native_window) {
//        ANativeWindow_release(data->native_window);
//        if (data->native_window == new_native_window) {
//            GST_DEBUG ("New native window is the same as the previous one %p",
//                       data->native_window);
//            if (data->video_sink) {
//                gst_video_overlay_expose(GST_VIDEO_OVERLAY (data->video_sink));
//                gst_video_overlay_expose(GST_VIDEO_OVERLAY (data->video_sink));
//            }
//            return;
//        } else {
//            GST_DEBUG ("Released previous native window %p", data->native_window);
//            data->initialized = FALSE;
//        }
//    }
//    data->native_window = new_native_window;
//
//    check_initialization_complete(data);
//}
//
//static void
//gst_native_surface_finalize(JNIEnv *env, jobject thiz) {
//    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
//    if (!data)
//        return;
//    GST_DEBUG ("Releasing Native Window %p", data->native_window);
//
//    if (data->video_sink) {
//        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY (data->video_sink),
//                                            (guintptr) NULL);
//        gst_element_set_state(data->pipeline, GST_STATE_READY);
//    }
//
//    ANativeWindow_release(data->native_window);
//    data->native_window = NULL;
//    data->initialized = FALSE;
//}
//
///* List of implemented native methods */
//static JNINativeMethod native_methods[] = {
//        {"nativeInit",            "(Ljava/lang/String;)V", (void *) gst_native_init},
//        {"nativeControllerInit", "()V", (void *) gst_controller_init},
//        {"nativeFinalize",        "()V",                   (void *) gst_native_finalize},
//        {"nativePlay",            "()V",                   (void *) gst_native_play},
//        {"nativePause",           "()V",                   (void *) gst_native_pause},
//        {"nativeSurfaceInit",     "(Ljava/lang/Object;)V",
//                                                           (void *) gst_native_surface_init},
//        {"nativeSurfaceFinalize", "()V",                   (void *) gst_native_surface_finalize},
//        {"nativeClassInit",       "()Z",                   (void *) gst_native_class_init},
//};
//
///* Library initializer */
//jint
//JNI_OnLoad(JavaVM *vm, void *reserved) {
//    JNIEnv *env = NULL;
//
//    java_vm = vm;
//
//    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
//        __android_log_print(ANDROID_LOG_ERROR, "tutorial-3",
//                            "Could not retrieve JNIEnv");
//        return 0;
//    }
//    jclass klass = (*env)->FindClass(env,
//                                     "com/example/strjk/GStreamerView");
//    (*env)->RegisterNatives(env, klass, native_methods,
//                            G_N_ELEMENTS (native_methods));
//
//    pthread_key_create(&current_jni_env, detach_current_thread);
//
//    return JNI_VERSION_1_6;
//}



#include <string.h>
#include <stdint.h>
#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <pthread.h>

#include <glib.h> // Required for g_strdup, G_N_ELEMENTS, etc.

GST_DEBUG_CATEGORY_STATIC (debug_category);
#define GST_CAT_DEFAULT debug_category

/*
 * These macros provide a way to store the native pointer to CustomData, which might be 32 or 64 bits, into
 * a jlong, which is always 64 bits, without warnings.
 */
#if GLIB_SIZEOF_VOID_P == 8
# define GET_CUSTOM_DATA(env, thiz, fieldID) (CustomData *)(*env)->GetLongField (env, thiz, fieldID)
# define SET_CUSTOM_DATA(env, thiz, fieldID, data) (*env)->SetLongField (env, thiz, fieldID, (jlong)data)
#else
# define GET_CUSTOM_DATA(env, thiz, fieldID) (CustomData *)(jint)(*env)->GetLongField (env, thiz, fieldID)
# define SET_CUSTOM_DATA(env, thiz, fieldID, data) (*env)->SetLongField (env, thiz, fieldID, (jlong)(jint)data)
#endif

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
    jobject app;                  /* Application instance, used to call its methods. A global reference is kept. */
    GstElement *pipeline;         /* The running pipeline */
    GMainContext *context;        /* GLib context used to run the main loop */
    GMainLoop *main_loop;         /* GLib main loop */
    gboolean initialized;         /* To avoid informing the UI multiple times about the initialization */
    GstElement *video_sink;       /* The video sink element which receives XOverlay commands */
    ANativeWindow *native_window; /* The Android native window where video will be rendered */
    gchar *gst_desc;              /* Pipeline description string */
} CustomData;

/* These global variables cache values which are not changing during execution */
static pthread_t gst_app_thread;
static pthread_key_t current_jni_env;
static JavaVM *java_vm;
static jfieldID custom_data_field_id;
static jmethodID set_message_method_id;
static jmethodID on_gstreamer_initialized_method_id;

/*
 * Private methods
 */

/* Register this thread with the VM */
static JNIEnv *
attach_current_thread(void) {
    JNIEnv *env;
    JavaVMAttachArgs args;

    GST_DEBUG ("Attaching thread %p", g_thread_self());
    args.version = JNI_VERSION_1_6;
    args.name = NULL; // Use default thread name
    args.group = NULL;

    if ((*java_vm)->AttachCurrentThread(java_vm, &env, &args) < 0) {
        GST_ERROR ("Failed to attach current thread");
        return NULL;
    }

    return env;
}

/* Unregister this thread from the VM */
static void
detach_current_thread(void *env) {
    GST_DEBUG ("Detaching thread %p", g_thread_self());
    (*java_vm)->DetachCurrentThread(java_vm);
}

/* Retrieve the JNI environment for this thread */
static JNIEnv *
get_jni_env(void) {
    JNIEnv *env;

    if ((env = pthread_getspecific(current_jni_env)) == NULL) {
        env = attach_current_thread();
        if (env) {
            pthread_setspecific(current_jni_env, env);
        }
    }

    return env;
}

/* Change the content of the UI's TextView (assuming this method exists in GStreamerView) */
static void
set_ui_message(const gchar *message, CustomData *data) {
    JNIEnv *env = get_jni_env();
    if (!env || !data) {
        __android_log_print(ANDROID_LOG_ERROR, "SkyAutoNet_JNI", "set_ui_message: JNIEnv or CustomData is NULL");
        return;
    }
    GST_DEBUG ("Setting message to: %s", message);
    jstring jmessage = (*env)->NewStringUTF(env, message);
    if (!jmessage) {
        __android_log_print(ANDROID_LOG_ERROR, "SkyAutoNet_JNI", "Failed to create jstring");
        return;
    }
    (*env)->CallVoidMethod(env, data->app, set_message_method_id, jmessage);
    if ((*env)->ExceptionCheck(env)) {
        GST_ERROR ("Failed to call Java method 'setMessage'");
        (*env)->ExceptionClear(env);
    }
    (*env)->DeleteLocalRef(env, jmessage);
}

/* Retrieve errors from the bus and show them on the UI */
static void
error_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
    GError *err = NULL;
    gchar *debug_info = NULL;
    gchar *message_string = NULL;

    gst_message_parse_error(msg, &err, &debug_info);
    message_string =
            g_strdup_printf("Error received from element %s: %s",
                            GST_OBJECT_NAME (msg->src), err ? err->message : "Unknown error");
    if (err) g_clear_error(&err);
    if (debug_info) g_free(debug_info);
    set_ui_message(message_string, data);
    g_free(message_string);
    if (data && data->pipeline) {
        gst_element_set_state(data->pipeline, GST_STATE_NULL);
    }
}

/* Notify UI about pipeline state changes */
static void
state_changed_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
    GstState old_state, new_state, pending_state;
    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
    /* Only pay attention to messages coming from the pipeline, not its children */
    if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
        gchar *message = g_strdup_printf("State changed to %s",
                                         gst_element_state_get_name(new_state));
        set_ui_message(message, data);
        g_free(message);
    }
}

/* Check if all conditions are met to report GStreamer as initialized.
 * These conditions will change depending on the application */
static void
check_initialization_complete(CustomData *data) {
    JNIEnv *env = get_jni_env();
    if (!env) {
        __android_log_print(ANDROID_LOG_ERROR, "SkyAutoNet_JNI", "check_initialization_complete: JNIEnv is NULL");
        return;
    }
    if (!data->initialized && data->native_window && data->main_loop) {
        GST_DEBUG
                ("Initialization complete, notifying application. native_window:%p main_loop:%p",
                 data->native_window, data->main_loop);

        /* The main loop is running and we received a native window, inform the sink about it */
        if (data->video_sink) {
            gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY (data->video_sink),
                                                (guintptr) data->native_window);
        } else {
            GST_ERROR("Video sink is NULL, cannot set window handle.");
        }

        (*env)->CallVoidMethod(env, data->app, on_gstreamer_initialized_method_id);
        if ((*env)->ExceptionCheck(env)) {
            GST_ERROR ("Failed to call Java method 'onGStreamerInitialized'");
            (*env)->ExceptionClear(env);
        }
        data->initialized = TRUE;
    }
}

/* Main method for the native code. This is executed on its own thread. */
static void *
app_function(void *userdata) {
    GstBus *bus;
    CustomData *data = (CustomData *) userdata;
    GSource *bus_source;
    GError *error = NULL;

    GST_DEBUG ("Creating pipeline in CustomData at %p", data);

    /* Create our own GLib Main Context and make it the default one */
    data->context = g_main_context_new();
    g_main_context_push_thread_default(data->context);

    /* Build pipeline */
    if (!data->gst_desc) {
        GST_ERROR("Pipeline description is NULL.");
        // Clean up and return if description is missing
        g_main_context_pop_thread_default(data->context);
        g_main_context_unref(data->context);
        return NULL;
    }

    data->pipeline = gst_parse_launch(data->gst_desc, &error);
    if (error) {
        gchar *message = g_strdup_printf("Unable to build pipeline: %s", error->message);
        g_clear_error(&error);
        set_ui_message(message, data);
        g_free(message);
        g_main_context_pop_thread_default(data->context);
        g_main_context_unref(data->context);
        return NULL;
    }

    /* Set the pipeline to READY, so it can already accept a window handle, if we have one */
    gst_element_set_state(data->pipeline, GST_STATE_READY);

    data->video_sink = gst_bin_get_by_interface(GST_BIN (data->pipeline), GST_TYPE_VIDEO_OVERLAY);
    if (!data->video_sink) {
        GST_ERROR ("Could not retrieve video sink");
        // Clean up and return if sink is not found
        gst_element_set_state(data->pipeline, GST_STATE_NULL);
        gst_object_unref(data->pipeline);
        g_main_context_pop_thread_default(data->context);
        g_main_context_unref(data->context);
        return NULL;
    }

    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    bus = gst_element_get_bus(data->pipeline);
    bus_source = gst_bus_create_watch(bus);
    // Use gst_bus_async_signal_func for asynchronous signal handling
    g_source_set_callback(bus_source, (GSourceFunc) gst_bus_async_signal_func, NULL, NULL);
    g_source_attach(bus_source, data->context);
    g_source_unref(bus_source);
    g_signal_connect (G_OBJECT(bus), "message::error", (GCallback) error_cb, data);
    g_signal_connect (G_OBJECT(bus), "message::state-changed", (GCallback) state_changed_cb, data);
    gst_object_unref(bus);

    /* Create a GLib Main Loop and set it to run */
    GST_DEBUG ("Entering main loop... (CustomData:%p)", data);
    data->main_loop = g_main_loop_new(data->context, FALSE);
    check_initialization_complete(data); // Check initialization status
    g_main_loop_run(data->main_loop);
    GST_DEBUG ("Exited main loop");

    /* Free resources */
    if (data->main_loop) {
        g_main_loop_unref(data->main_loop);
        data->main_loop = NULL;
    }
    g_main_context_pop_thread_default(data->context);
    g_main_context_unref(data->context);
    gst_element_set_state(data->pipeline, GST_STATE_NULL);
    gst_object_unref(data->video_sink);
    gst_object_unref(data->pipeline);
    data->pipeline = NULL;
    data->video_sink = NULL;
    data->native_window = NULL; // The native window is released by finalize

    return NULL;
}

/*
 * Java Bindings
 */

/* Initialize the native data structure */
static void gst_controller_init(JNIEnv *env, jobject thiz) {
    CustomData *data = g_new0 (CustomData, 1);
    SET_CUSTOM_DATA (env, thiz, custom_data_field_id, data);
    GST_DEBUG_CATEGORY_INIT (debug_category, "SkyAutoNet", 0, "SkyAutoNet");
    gst_debug_set_threshold_for_name("SkyAutoNet", GST_LEVEL_DEBUG);
    GST_DEBUG ("Created CustomData at %p", data);
    data->app = (*env)->NewGlobalRef(env, thiz); // Keep a global reference to the Java object
    if (!data->app) {
        GST_ERROR("Failed to create global reference for Java object.");
        g_free(data); // Clean up if global ref fails
        SET_CUSTOM_DATA (env, thiz, custom_data_field_id, NULL);
        return;
    }
    GST_DEBUG ("Created GlobalRef for app object at %p", data->app);
}

/*
 * Native method: Initialize GStreamer with the provided pipeline description.
 * This function is called from the Kotlin/Java side.
 */
JNIEXPORT jboolean JNICALL Java_com_example_strjk_GStreamerView_nativeInit
        (JNIEnv* env, jobject thiz, jstring gst_desc) {

    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data) {
        __android_log_print(ANDROID_LOG_ERROR, "SkyAutoNet_JNI", "nativeInit: CustomData is NULL");
        return JNI_FALSE;
    }

    // Convert jstring to C-style string
    const char* c_gst_desc = (*env)->GetStringUTFChars(env, gst_desc, NULL);
    if (!c_gst_desc) {
        __android_log_print(ANDROID_LOG_ERROR, "SkyAutoNet_JNI", "nativeInit: Failed to get UTFChars for gst_desc");
        return JNI_FALSE;
    }

    // Allocate memory for the pipeline description and copy it. g_strdup allocates memory.
    data->gst_desc = g_strdup(c_gst_desc);
    GST_DEBUG ("Pipeline description set to: %s", data->gst_desc);

    // Release the jstring as we have copied it.
    (*env)->ReleaseStringUTFChars(env, gst_desc, c_gst_desc);

    // Create and run the GStreamer pipeline in a separate thread.
    if (pthread_create(&gst_app_thread, NULL, &app_function, data) != 0) {
        GST_ERROR("Failed to create GStreamer thread.");
        g_free(data->gst_desc); // Clean up allocated description
        return JNI_FALSE;
    }

    // Important: Check initialization AFTER starting the thread.
    // The actual completion happens inside app_function when main_loop is ready.
    // This call ensures that if a window is already available, it's passed.
    check_initialization_complete(data);

    return JNI_TRUE; // Indicate success
}

/* Quit the main loop, wait for the thread to finish, and free all resources */
static void
gst_native_finalize(JNIEnv *env, jobject thiz) {
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data) {
        GST_DEBUG("CustomData is NULL in finalize, nothing to do.");
        return;
    }
    GST_DEBUG ("Quitting main loop...");

    // Signal the main loop to exit
    if (data->main_loop) {
        g_main_loop_quit(data->main_loop);
    }

    GST_DEBUG ("Waiting for GStreamer thread to finish...");
    // Wait for the GStreamer thread to complete
    if (pthread_join(gst_app_thread, NULL) != 0) {
        GST_ERROR("Failed to join GStreamer thread.");
    }

    GST_DEBUG ("Deleting GlobalRef for app object at %p", data->app);
    if (data->app) {
        (*env)->DeleteGlobalRef(env, data->app);
        data->app = NULL; // Nullify after deleting
    }
    GST_DEBUG ("Freeing CustomData at %p", data);
    g_free(data->gst_desc); // Free the pipeline description string
    g_free(data);
    SET_CUSTOM_DATA (env, thiz, custom_data_field_id, NULL); // Clear the field
    GST_DEBUG ("Done finalizing");
}

/* Set pipeline to PLAYING state */
static void
gst_native_play(JNIEnv *env, jobject thiz) {
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data || !data->pipeline) {
        GST_DEBUG("Pipeline is NULL, cannot play.");
        return;
    }
    GST_DEBUG ("Setting state to PLAYING");
    gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
}

/* Set pipeline to PAUSED state */
static void
gst_native_pause(JNIEnv *env, jobject thiz) {
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data || !data->pipeline) {
        GST_DEBUG("Pipeline is NULL, cannot pause.");
        return;
    }
    GST_DEBUG ("Setting state to PAUSED");
    gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
}

/* Static class initializer: retrieve method and field IDs */
static jboolean
gst_native_class_init(JNIEnv *env, jclass klass) {
    custom_data_field_id = (*env)->GetFieldID(env, klass, "native_custom_data", "J");
    set_message_method_id = (*env)->GetMethodID(env, klass, "setMessage", "(Ljava/lang/String;)V");
    on_gstreamer_initialized_method_id = (*env)->GetMethodID(env, klass, "onGStreamerInitialized", "()V");

    if (!custom_data_field_id) {
        __android_log_print(ANDROID_LOG_ERROR, "SkyAutoNet_JNI", "Failed to get field ID for native_custom_data");
        return JNI_FALSE;
    }
    if (!set_message_method_id) {
        __android_log_print(ANDROID_LOG_ERROR, "SkyAutoNet_JNI", "Failed to get method ID for setMessage");
        return JNI_FALSE;
    }
    if (!on_gstreamer_initialized_method_id) {
        __android_log_print(ANDROID_LOG_ERROR, "SkyAutoNet_JNI", "Failed to get method ID for onGStreamerInitialized");
        return JNI_FALSE;
    }
    GST_DEBUG("Successfully retrieved JNI field and method IDs.");
    return JNI_TRUE;
}

/* Called when the native surface is created or its properties change */
static void
gst_native_surface_init(JNIEnv *env, jobject thiz, jobject surface) {
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data) {
        GST_DEBUG("CustomData is NULL in surface_init.");
        return;
    }

    ANativeWindow *new_native_window = ANativeWindow_fromSurface(env, surface);
    if (!new_native_window) {
        GST_ERROR("Failed to get ANativeWindow from Surface.");
        return;
    }
    GST_DEBUG ("Received surface %p (native window %p)", surface, new_native_window);

    // If we already have a native window, release the old one.
    if (data->native_window) {
        // Check if it's the same window to avoid unnecessary work.
        if (data->native_window == new_native_window) {
            GST_DEBUG ("New native window is the same as the previous one %p", data->native_window);
            // If it's the same, just re-expose if needed.
            if (data->video_sink) {
                gst_video_overlay_expose(GST_VIDEO_OVERLAY (data->video_sink));
                gst_video_overlay_expose(GST_VIDEO_OVERLAY (data->video_sink)); // Sometimes requires two calls
            }
            ANativeWindow_release(new_native_window); // Release the duplicate reference
            return;
        } else {
            // Different window, release the old one and mark as not initialized.
            GST_DEBUG ("Releasing previous native window %p", data->native_window);
            ANativeWindow_release(data->native_window);
            data->initialized = FALSE; // Reset initialization flag
        }
    }

    data->native_window = new_native_window;

    // Try to complete initialization now that we have the native window.
    check_initialization_complete(data);
}

/* Called when the native surface is finalized (destroyed) */
static void
gst_native_surface_finalize(JNIEnv *env, jobject thiz) {
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data) {
        GST_DEBUG("CustomData is NULL in surface_finalize.");
        return;
    }
    GST_DEBUG ("Releasing Native Window %p", data->native_window);

    // Set the window handle to NULL in the GStreamer sink and reset the pipeline state to READY.
    if (data->video_sink) {
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY (data->video_sink), (guintptr) NULL);
        gst_element_set_state(data->pipeline, GST_STATE_READY);
    }

    // Release the ANativeWindow reference.
    if (data->native_window) {
        ANativeWindow_release(data->native_window);
        data->native_window = NULL;
    }
    data->initialized = FALSE; // Reset initialization flag
}

/* List of implemented native methods */
// This array maps the Java/Kotlin method names to the C JNI function pointers.
// The signature strings "(Ljava/lang/String;)V" are JNI type signatures.
static JNINativeMethod native_methods[] = {
        {"nativeInit",            "(Ljava/lang/String;)V", (void *) Java_com_example_strjk_GStreamerView_nativeInit},
        {"nativeControllerInit", "()V",                   (void *) gst_controller_init},
        {"nativeFinalize",        "()V",                   (void *) gst_native_finalize},
        {"nativePlay",            "()V",                   (void *) gst_native_play},
        {"nativePause",           "()V",                   (void *) gst_native_pause},
        {"nativeSurfaceInit",     "(Ljava/lang/Object;)V", (void *) gst_native_surface_init},
        {"nativeSurfaceFinalize", "()V",                   (void *) gst_native_surface_finalize},
        {"nativeClassInit",       "()Z",                   (void *) gst_native_class_init}
};

/* Library initializer called by the Android system when the library is loaded */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;

    java_vm = vm; // Store the JavaVM pointer globally

    // Get the JNI environment for this thread
    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "SkyAutoNet_JNI", "Could not retrieve JNIEnv");
        return JNI_ERR; // Return error code
    }

    // Find the Java class corresponding to our GStreamerView
    jclass klass = (*env)->FindClass(env, "com/example/strjk/GStreamerView");
    if (!klass) {
        __android_log_print(ANDROID_LOG_ERROR, "SkyAutoNet_JNI", "Failed to find Java class com.example.strjk.GStreamerView");
        return JNI_ERR;
    }

    // Register the native methods with the Java class
    if ((*env)->RegisterNatives(env, klass, native_methods, G_N_ELEMENTS (native_methods)) < 0) {
        __android_log_print(ANDROID_LOG_ERROR, "SkyAutoNet_JNI", "Failed to register native methods");
        return JNI_ERR;
    }

    // Create a pthread key for managing JNIEnv per thread
    if (pthread_key_create(&current_jni_env, detach_current_thread) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, "SkyAutoNet_JNI", "Failed to create pthread key");
        return JNI_ERR;
    }

    GST_DEBUG("JNI_OnLoad successful");
    return JNI_VERSION_1_6; // Return the JNI version
}