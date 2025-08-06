#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstRTSPServer *server = gst_rtsp_server_new();
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

    // --- Media Factory for Camera 0 (using your working /dev/video1) ---
    GstRTSPMediaFactory *factory_cam0 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam0,
        "( "
        // Using the corrected 'bps' property for the hardware encoder
        "v4l2src device=/dev/video1 do-timestamp=true ! video/x-raw,width=640,height=480 ! "
        "videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 "
        ")"
    );
    gst_rtsp_media_factory_set_shared(factory_cam0, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam0", factory_cam0);
    std::cout << "SERVER: Stream ready at rtsp://<your_ip>:8554/cam0" << std::endl;


    // --- Media Factory for Camera 1 (using /dev/video2 as an example) ---
    GstRTSPMediaFactory *factory_cam1 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam1,
        "( "
        "v4l2src device=/dev/video2 do-timestamp=true ! video/x-raw,width=640,height=480 ! "
        "videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 "
        ")"
    );
    gst_rtsp_media_factory_set_shared(factory_cam1, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam1", factory_cam1);
    std::cout << "SERVER: Stream ready at rtsp://<your_ip>:8554/cam1" << std::endl;
    

    g_object_unref(mounts);
    gst_rtsp_server_attach(server, NULL);

    std::cout << "SERVER: RTSP server is running." << std::endl;
    g_main_loop_run(loop);
    
    // Cleanup
    g_main_loop_unref(loop);
    g_object_unref(server);
    
    return 0;
}