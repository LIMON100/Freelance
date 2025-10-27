// WORKABLE but only once

// #include <gst/gst.h>
// #include <gst/rtsp-server/rtsp-server.h>
// #include <iostream>
// #include <string>
// #include <thread>
// #include <atomic>
// #include <unistd.h>
// #include <sys/socket.h>
// #include <netinet/in.h>

// // Global variables
// static GstElement *input_selector = nullptr;
// static std::atomic<bool> running(true);

// // Function to switch the active camera pad
// void switch_input(const std::string &pad_name) 
// {
//     if (!input_selector) {
//         std::cerr << "Input selector not ready yet." << std::endl;
//         return;
//     }
//     GstPad *pad = gst_element_get_static_pad(input_selector, pad_name.c_str());
//     if (pad) 
//     {
//         g_object_set(G_OBJECT(input_selector), "active-pad", pad, nullptr);
//         std::cout << "SERVER: Switched to active pad: " << pad_name << std::endl;
//         gst_object_unref(pad);
//     } 
//     else 
//     {
//         std::cerr << "SERVER: Failed to get pad: " << pad_name << std::endl;
//     }
// }

// // TCP Command Server to handle remote switching
// void handle_network_commands() 
// {
//     int server_fd, client_socket;
//     struct sockaddr_in address;
//     int opt = 1;
//     int addrlen = sizeof(address);
//     char buffer[16] = {0};

//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { perror("socket failed"); return; }
//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { perror("setsockopt"); return; }
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(9999);

//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) { perror("bind failed"); return; }
//     if (listen(server_fd, 3) < 0) { perror("listen"); return; }
//     std::cout << "SERVER: Command server listening on port 9999..." << std::endl;

//     while (running) {
//         if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
//             if (!running) break;
//             perror("accept");
//             continue;
//         }
//         read(client_socket, buffer, 1);
//         char input = buffer[0];
//         if (input >= '0' && input <= '3') {
//             std::cout << "SERVER: Received command '" << input << "' from network." << std::endl;
//             switch_input("sink_" + std::string(1, input));
//         } else {
//             std::cout << "SERVER: Received unknown command: " << input << std::endl;
//         }
//         close(client_socket);
//     }
//     close(server_fd);
//     std::cout << "SERVER: Command server shut down." << std::endl;
// }

// // GStreamer Callback to get the input-selector
// static void media_configure_cb(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data)
// {
//     GstElement *pipeline = gst_rtsp_media_get_element(media);
//     input_selector = gst_bin_get_by_name(GST_BIN(pipeline), "selector");
//     if (!input_selector) {
//         g_printerr("SERVER: Could not get input-selector from pipeline\n");
//     } else {
//         std::cout << "SERVER: Successfully got input-selector from configured media." << std::endl;
//         switch_input("sink_0");
//     }
//     gst_object_unref(pipeline);
// }

// int main(int argc, char *argv[]) {
//     gst_init(&argc, &argv);

//     GMainLoop *loop = g_main_loop_new(NULL, FALSE);
//     GstRTSPServer *server = gst_rtsp_server_new();
//     GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
//     GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();

//     // --- The Final, Corrected Pipeline String ---
//     const char *pipeline_str = 
//         "( "
//         // Capture from your working camera and split the stream
//         "v4l2src device=/dev/video1 ! video/x-raw,width=640,height=480 ! tee name=t "
//         // The main processing branch
//         "input-selector name=selector ! videoconvert ! mpph264enc ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 "
//         // Tee branches feeding the selector
//         "t. ! queue ! selector.sink_0 "
//         "t. ! queue ! selector.sink_1 "
//         ")";



//     gst_rtsp_media_factory_set_launch(factory, pipeline_str);
//     gst_rtsp_media_factory_set_shared(factory, TRUE);
//     g_signal_connect(factory, "media-configure", G_CALLBACK(media_configure_cb), NULL);
//     gst_rtsp_mount_points_add_factory(mounts, "/stream", factory);
//     g_object_unref(mounts);

//     gst_rtsp_server_attach(server, NULL);
//     std::thread command_thread(handle_network_commands);
//     std::cout << "SERVER: RTSP stream ready at rtsp://<your_rk3588_ip>:8554/stream" << std::endl;
    
//     g_main_loop_run(loop);
    
//     running = false;
//     shutdown(fileno(stdin), SHUT_RD);
//     command_thread.join();
//     g_main_loop_unref(loop);
//     g_object_unref(server);
    
//     return 0;
// }





// #include <gst/gst.h>
// #include <gst/rtsp-server/rtsp-server.h>
// #include <iostream>
// #include <string>

// int main(int argc, char *argv[]) {
//     gst_init(&argc, &argv);

//     GMainLoop *loop = g_main_loop_new(NULL, FALSE);
//     GstRTSPServer *server = gst_rtsp_server_new();
//     GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

//     // --- Media Factory for Camera 0 (using your working /dev/video1) ---
//     GstRTSPMediaFactory *factory_cam0 = gst_rtsp_media_factory_new();
//     gst_rtsp_media_factory_set_launch(factory_cam0,
//         "( v4l2src device=/dev/video1 ! video/x-raw,width=640,height=480 ! "
//         "videoconvert ! mpph264enc ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
//     gst_rtsp_media_factory_set_shared(factory_cam0, TRUE);
//     gst_rtsp_mount_points_add_factory(mounts, "/cam0", factory_cam0);
//     std::cout << "SERVER: Stream ready at rtsp://<your_ip>:8554/cam0" << std::endl;

//     // --- Media Factory for Camera 1 (using /dev/video2 as an example) ---
//     GstRTSPMediaFactory *factory_cam1 = gst_rtsp_media_factory_new();
//     gst_rtsp_media_factory_set_launch(factory_cam1,
//         "( v4l2src device=/dev/video2 ! video/x-raw,width=640,height=480 ! "
//         "videoconvert ! mpph264enc ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
//     gst_rtsp_media_factory_set_shared(factory_cam1, TRUE);
//     gst_rtsp_mount_points_add_factory(mounts, "/cam1", factory_cam1);
//     std::cout << "SERVER: Stream ready at rtsp://<your_ip>:8554/cam1" << std::endl;
    
//     // You can add more factories for /cam2, /cam3, etc., using other /dev/videoX devices here.

//     g_object_unref(mounts);
//     gst_rtsp_server_attach(server, NULL);

//     std::cout << "SERVER: RTSP server is running." << std::endl;
//     g_main_loop_run(loop);
    
//     // Cleanup
//     g_main_loop_unref(loop);
//     g_object_unref(server);
    
//     return 0;
// }



#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>
#include <string>

// This simplified server provides multiple, independent stream endpoints.
// This is a much more stable architecture.

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstRTSPServer *server = gst_rtsp_server_new();
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

    // --- Media Factory for Camera 0 (using your working /dev/video1) ---
    GstRTSPMediaFactory *factory_cam0 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam0,
        "( "
        "v4l2src device=/dev/video1 ! video/x-raw,width=640,height=480 ! "
        // The queue element prevents pipeline stalls and ensures smooth video
        "videoconvert ! queue ! mpph264enc bitrate=2000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 "
        ")"
    );
    gst_rtsp_media_factory_set_shared(factory_cam0, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam0", factory_cam0);
    std::cout << "SERVER: Stream ready at rtsp://<your_ip>:8554/cam0" << std::endl;

    // --- Media Factory for Camera 1 (using /dev/video2 as an example) ---
    GstRTSPMediaFactory *factory_cam1 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam1,
        "( "
        "v4l2src device=/dev/video2 ! video/x-raw,width=640,height=480 ! "
        "videoconvert ! queue ! mpph264enc bitrate=2000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 "
        ")"
    );
    gst_rtsp_media_factory_set_shared(factory_cam1, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam1", factory_cam1);
    std::cout << "SERVER: Stream ready at rtsp://<your_ip>:8554/cam1" << std::endl;
    
    // You can add more factories for /cam2, etc. here if you have more working cameras.

    g_object_unref(mounts);
    gst_rtsp_server_attach(server, NULL);

    std::cout << "SERVER: RTSP server is running." << std::endl;
    g_main_loop_run(loop);
    
    // Cleanup
    g_main_loop_unref(loop);
    g_object_unref(server);
    
    return 0;
}