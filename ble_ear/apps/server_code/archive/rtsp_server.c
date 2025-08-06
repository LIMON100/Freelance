#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <stdio.h>
#include <stdlib.h>

static gboolean timeout(GstRTSPServer *server) {
    GstRTSPSessionPool *pool = gst_rtsp_server_get_session_pool(server);
    gst_rtsp_session_pool_cleanup(pool);
    g_object_unref(pool);
    g_print("Session pool cleaned up\n");
    return TRUE;
}

static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data) {
    GstElement *element = gst_rtsp_media_get_element(media);
    g_print("Media configured, element: %p\n", element);
    g_object_unref(element);
}

static void client_connected(GstRTSPServer *server, GstRTSPClient *client, gpointer user_data) {
    g_print("Client connected: %p\n", client);
}

int main(int argc, char *argv[]) {
    GMainLoop *loop;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;

    gst_init(&argc, &argv);

    loop = g_main_loop_new(NULL, FALSE);
    if (!loop) {
        g_printerr("Failed to create main loop\n");
        return -1;
    }

    server = gst_rtsp_server_new();
    if (!server) {
        g_printerr("Failed to create RTSP server\n");
        g_main_loop_unref(loop);
        return -1;
    }

    mounts = gst_rtsp_server_get_mount_points(server);
    if (!mounts) {
        g_printerr("Failed to get mount points\n");
        g_object_unref(server);
        g_main_loop_unref(loop);
        return -1;
    }

    factory = gst_rtsp_media_factory_new();
    if (!factory) {
        g_printerr("Failed to create media factory\n");
        g_object_unref(mounts);
        g_object_unref(server);
        g_main_loop_unref(loop);
        return -1;
    }

    // Use NV12 format supported by the camera
    gst_rtsp_media_factory_set_launch(factory,
        "( v4l2src device=/dev/video1 ! video/x-raw,format=NV12,width=1280,height=720,framerate=30/1 ! videoconvert ! x264enc tune=zerolatency bitrate=500 ! rtph264pay name=pay0 pt=96 )");
    gst_rtsp_media_factory_set_shared(factory, TRUE);
    g_signal_connect(factory, "media-configure", G_CALLBACK(media_configure), NULL);

    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    g_object_unref(mounts);

    g_signal_connect(server, "client-connected", G_CALLBACK(client_connected), NULL);

    if (gst_rtsp_server_attach(server, NULL) == 0) {
        g_printerr("Failed to attach the server to default port (8554)\n");
        g_object_unref(server);
        g_main_loop_unref(loop);
        return -1;
    } else {
        g_print("RTSP server attached to default port 8554\n");
    }

    g_timeout_add_seconds(2, (GSourceFunc)timeout, server);
    g_print("Stream ready at rtsp://127.0.0.1:8554/test\n");

    g_main_loop_run(loop);

    g_object_unref(server);
    g_main_loop_unref(loop);
    return 0;
}