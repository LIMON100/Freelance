#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>      // For std::thread
#include <mutex>       // For std::mutex
#include <queue>       // For std::queue

// Include socket headers for the command server
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// --- Configuration for Command Server ---
#define COMMAND_PORT 65432 // Must match the port in the Flutter app

// --- Global Shared Data for Thread Communication ---
std::queue<std::string> command_queue;
std::mutex queue_mutex;


// =========================================================================
//  COMMAND SERVER THREAD FUNCTION
//  This function runs in a separate thread and handles all networking.
// =========================================================================
void commandServerThread() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[Command Thread] socket failed"); return;
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("[Command Thread] setsockopt"); return;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces
    address.sin_port = htons(COMMAND_PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[Command Thread] bind failed"); return;
    }
    if (listen(server_fd, 3) < 0) {
        perror("[Command Thread] listen"); return;
    }

    std::cout << "[Command Thread] Listening for commands on port " << COMMAND_PORT << std::endl;

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("[Command Thread] accept"); continue;
        }
        
        std::vector<char> buffer(1024);
        int valread = read(new_socket, buffer.data(), buffer.size());
        if (valread > 0) {
            std::string command(buffer.begin(), buffer.begin() + valread);
            std::cout << "[Command Thread] Received command: " << command << std::endl;

            // Safely add the received command to the shared queue
            std::lock_guard<std::mutex> lock(queue_mutex);
            command_queue.push(command);
        }
        close(new_socket);
    }
}


// =========================================================================
//  GSTREAMER CALLBACK FOR PROCESSING COMMANDS
//  This function is called by the main GStreamer loop to check the queue.
// =========================================================================
static gboolean check_for_commands(gpointer user_data) {
    std::string command;
    bool command_found = false;

    // Safely check if there is a command in the queue
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (!command_queue.empty()) {
            command = command_queue.front();
            command_queue.pop();
            command_found = true;
        }
    }

    if (command_found) {
        std::cout << "[Main Thread] Processing command: " << command << std::endl;

        // based on the command string.
        if (command == "CMD_FORWARD") {
            // e.g., robot_move_forward();
        } else if (command == "CMD_STOP") {
            // e.g., robot_stop_all_motors();
        } else if (command == "CMD_ATTACK") {
            // e.g., robot_activate_tool();
        }
    }

    return TRUE; // Return TRUE to ensure this callback is called again
}


// =========================================================================
//  MAIN APPLICATION
//  Sets up GStreamer, launches the command thread, and runs the main loop.
// =========================================================================
int main(int argc, char *argv[]) {
    // 1. Initialize GStreamer
    gst_init(&argc, &argv);

    // 2. Launch the command server in a separate thread
    std::thread cmd_thread(commandServerThread);

    // 3. Create the GStreamer Main Loop and RTSP Server
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstRTSPServer *server = gst_rtsp_server_new();
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

    // 4. Set up the RTSP media factories for each camera
    // --- Media Factory for /cam0 (using /dev/video1) ---
    GstRTSPMediaFactory *factory_cam0 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam0,
        "( "
        "v4l2src device=/dev/video1 do-timestamp=true ! video/x-raw,width=640,height=480 ! "
        "videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 "
        ")"
    );
    gst_rtsp_media_factory_set_shared(factory_cam0, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam0", factory_cam0);
    std::cout << "[Main Thread] Stream ready at rtsp://<your_ip>:8554/cam0" << std::endl;

    // --- Media Factory for /cam1 (using /dev/video0) ---
    GstRTSPMediaFactory *factory_cam1 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam1,
        "( "
        "v4l2src device=/dev/video0 do-timestamp=true ! video/x-raw,width=640,height=480 ! "
        "videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 "
        ")"
    );
    gst_rtsp_media_factory_set_shared(factory_cam1, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam1", factory_cam1);
    std::cout << "[Main Thread] Stream ready at rtsp://<your_ip>:8554/cam1" << std::endl;

    // --- Media Factory for /cam2 (using /dev/video2) ---
    GstRTSPMediaFactory *factory_cam2 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam2,
        "( "
        "v4l2src device=/dev/video2 do-timestamp=true ! video/x-raw,width=640,height=480 ! "
        "videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 "
        ")"
    );
    gst_rtsp_media_factory_set_shared(factory_cam2, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam2", factory_cam2);
    std::cout << "[Main Thread] Stream ready at rtsp://<your_ip>:8554/cam2" << std::endl;

    g_object_unref(mounts);

    // 5. Attach the server and schedule the command checker
    gst_rtsp_server_attach(server, NULL);
    g_timeout_add(100, (GSourceFunc)check_for_commands, NULL); // Check for commands every 100ms

    // 6. Start the main loop
    std::cout << "[Main Thread] RTSP server is running." << std::endl;
    g_main_loop_run(loop);
    
    // 7. Cleanup (code will not reach here in this simple setup)
    std::cout << "SERVER: Main loop stopped. Cleaning up." << std::endl;
    cmd_thread.join(); // Wait for command thread to finish
    g_main_loop_unref(loop);
    g_object_unref(server);
    
    return 0;
}