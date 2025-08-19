// #include <gst/gst.h>
// #include <gst/rtsp-server/rtsp-server.h>
// #include <iostream>
// #include <string>
// #include <vector>
// #include <thread>      // For std::thread
// #include <mutex>       // For std::mutex
// #include <queue>       // For std::queue
// #include <unistd.h>    // For write and sleep
// #include <cstdint>     // Required for uint8_t, int8_t

// // Include socket headers for the command server
// #include <sys/socket.h>
// #include <netinet/in.h>

// // --- Configuration for Command Server ---
// #define COMMAND_PORT 65432 // Must match the port in the Flutter app

// #pragma pack(push, 1)
// struct UserCommand {
//     uint8_t command_id;
//     int8_t  move_front_back_speed; // Signed 8-bit int for -100 to 100
//     int8_t  turn_left_right_angle;   // Signed 8-bit int for -100 to 100
//     uint8_t gun_trigger;            // Using uint8_t for bool (0 or 1)
//     uint8_t gun_trigger_permission;
// };
// #pragma pack(pop)


// std::queue<UserCommand> command_queue;
// std::mutex queue_mutex;


// // =========================================================================
// //  COMMAND SERVER THREAD FUNCTION (UPDATED)
// // =========================================================================
// void commandServerThread() {
//     int server_fd, new_socket;
//     struct sockaddr_in address;
//     int opt = 1;
//     socklen_t addrlen = sizeof(address);
    
//     // --- Standard Socket Setup (same as before) ---
//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         perror("[Command Thread] socket failed"); return;
//     }
//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
//         perror("[Command Thread] setsockopt"); return;
//     }
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(COMMAND_PORT);
//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         perror("[Command Thread] bind failed"); return;
//     }
//     if (listen(server_fd, 3) < 0) {
//         perror("[Command Thread] listen"); return;
//     }

//     std::cout << "[Command Thread] Listening for binary commands on port " << COMMAND_PORT << std::endl;

//     while (true) {
//         if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
//             perror("[Command Thread] accept"); continue;
//         }
        
//         UserCommand received_command;
//         // Read exactly the size of our struct (5 bytes)
//         int valread = read(new_socket, &received_command, sizeof(UserCommand));
        
//         if (valread == sizeof(UserCommand)) {
//             // Safely add the entire received command struct to the shared queue
//             std::lock_guard<std::mutex> lock(queue_mutex);
//             command_queue.push(received_command);
//         } else if (valread > 0) {
//             std::cerr << "[Command Thread] Warning: Received incomplete packet. Expected "
//                       << sizeof(UserCommand) << " bytes, but got " << valread << std::endl;
//         }
        
//         close(new_socket);
//     }
// }

// // =========================================================================
// //  GSTREAMER CALLBACK FOR PROCESSING COMMANDS (UPDATED)
// // =========================================================================
// static gboolean check_for_commands(gpointer user_data) {
//     UserCommand command;
//     bool command_found = false;

//     // Safely check if there is a command in the queue
//     {
//         std::lock_guard<std::mutex> lock(queue_mutex);
//         if (!command_queue.empty()) {
//             command = command_queue.front();
//             command_queue.pop();
//             command_found = true;
//         }
//     }

//     if (command_found) {
//         // --- PROCESS THE STRUCTURED COMMAND HERE ---
//         // You now have access to all 5 fields.
//         std::cout << "--- Processing Command Packet ---" << std::endl;
//         std::cout << "  Command ID: " << (int)command.command_id << std::endl;
//         std::cout << "  Speed: " << (int)command.move_front_back_speed << std::endl;
//         std::cout << "  Angle: " << (int)command.turn_left_right_angle << std::endl;
//         std::cout << "  Gun Trigger: " << (int)command.gun_trigger << std::endl;
//         std::cout << "  Gun Permission: " << (int)command.gun_trigger_permission << std::endl;
//         std::cout << "---------------------------------------------" << std::endl;

//     }

//     return TRUE; // Return TRUE to ensure this callback is called again
// }

// // =========================================================================
// //  MAIN APPLICATION
// // =========================================================================
// int main(int argc, char *argv[]) {
//     // 1. Initialize GStreamer
//     gst_init(&argc, &argv);

//     // 2. Launch the command server in a separate thread
//     std::thread cmd_thread(commandServerThread);

//     // 3. Create the GStreamer Main Loop and RTSP Server
//     GMainLoop *loop = g_main_loop_new(NULL, FALSE);
//     GstRTSPServer *server = gst_rtsp_server_new();
//     GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

//     // 4. Set up the RTSP media factories for each camera
//     GstRTSPMediaFactory *factory_cam0 = gst_rtsp_media_factory_new();
//     gst_rtsp_media_factory_set_launch(factory_cam0, "( v4l2src device=/dev/video1 do-timestamp=true ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
//     gst_rtsp_media_factory_set_shared(factory_cam0, TRUE);
//     gst_rtsp_mount_points_add_factory(mounts, "/cam0", factory_cam0);

//     GstRTSPMediaFactory *factory_cam1 = gst_rtsp_media_factory_new();
//     gst_rtsp_media_factory_set_launch(factory_cam1, "( v4l2src device=/dev/video0 do-timestamp=true ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
//     gst_rtsp_media_factory_set_shared(factory_cam1, TRUE);
//     gst_rtsp_mount_points_add_factory(mounts, "/cam1", factory_cam1);

//     GstRTSPMediaFactory *factory_cam2 = gst_rtsp_media_factory_new();
//     gst_rtsp_media_factory_set_launch(factory_cam2, "( v4l2src device=/dev/video2 do-timestamp=true ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
//     gst_rtsp_media_factory_set_shared(factory_cam2, TRUE);
//     gst_rtsp_mount_points_add_factory(mounts, "/cam2", factory_cam2);

//     g_object_unref(mounts);

//     // 5. Attach the server and schedule the command checker
//     gst_rtsp_server_attach(server, NULL);
//     g_timeout_add(100, (GSourceFunc)check_for_commands, NULL);

//     // 6. Start the main loop
//     std::cout << "[Main Thread] RTSP server is running." << std::endl;
//     g_main_loop_run(loop);
    
//     // 7. Cleanup
//     cmd_thread.join();
//     g_main_loop_unref(loop);
//     g_object_unref(server);
    
//     return 0;
// }





#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>      // For std::thread
#include <mutex>       // For std::mutex
#include <queue>       // For std::queue
#include <unistd.h>    // For write and sleep
#include <cstdint>     // Required for uint8_t, int8_t

// Include socket headers for the command server
#include <sys/socket.h>
#include <netinet/in.h>

// --- Configuration for Command Server ---
#define COMMAND_PORT 65432 // Must match the port in the Flutter app

#pragma pack(push, 1)
struct UserCommand {
    uint8_t command_id;
    int8_t  move_front_back_speed; // Signed 8-bit int for -100 to 100
    int8_t  turn_left_right_angle;   // Signed 8-bit int for -100 to 100
    uint8_t gun_trigger;            // Using uint8_t for bool (0 or 1)
    uint8_t gun_trigger_permission;
};
#pragma pack(pop)


std::queue<UserCommand> command_queue;
std::mutex queue_mutex;


// =========================================================================
//  COMMAND SERVER THREAD FUNCTION (UPDATED)
// =========================================================================
void commandServerThread() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    
    // --- Standard Socket Setup (same as before) ---
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[Command Thread] socket failed"); return;
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("[Command Thread] setsockopt"); return;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(COMMAND_PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[Command Thread] bind failed"); return;
    }
    if (listen(server_fd, 3) < 0) {
        perror("[Command Thread] listen"); return;
    }

    std::cout << "[Command Thread] Listening for binary commands on port " << COMMAND_PORT << std::endl;

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            perror("[Command Thread] accept"); continue;
        }
        
        UserCommand received_command;
        // Read exactly the size of our struct (5 bytes)
        int valread = read(new_socket, &received_command, sizeof(UserCommand));
        
        if (valread == sizeof(UserCommand)) {
            // Safely add the entire received command struct to the shared queue
            std::lock_guard<std::mutex> lock(queue_mutex);
            command_queue.push(received_command);
        } else if (valread > 0) {
            std::cerr << "[Command Thread] Warning: Received incomplete packet. Expected "
                      << sizeof(UserCommand) << " bytes, but got " << valread << std::endl;
        }
        
        close(new_socket);
    }
}

// =========================================================================
//  GSTREAMER CALLBACK FOR PROCESSING COMMANDS (UPDATED)
// =========================================================================
static gboolean check_for_commands(gpointer user_data) {
    UserCommand command;
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
        // --- PROCESS THE STRUCTURED COMMAND HERE ---
        // You now have access to all 5 fields.
        std::cout << "--- Processing Command Packet ---" << std::endl;
        std::cout << "  Command ID: " << (int)command.command_id << std::endl;
        std::cout << "  Speed: " << (int)command.move_front_back_speed << std::endl;
        std::cout << "  Angle: " << (int)command.turn_left_right_angle << std::endl;
        std::cout << "  Gun Trigger: " << (int)command.gun_trigger << std::endl;
        std::cout << "  Gun Permission: " << (int)command.gun_trigger_permission << std::endl;
        std::cout << "---------------------------------------------" << std::endl;

    }

    return TRUE; // Return TRUE to ensure this callback is called again
}

// =========================================================================
//  MAIN APPLICATION
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
    GstRTSPMediaFactory *factory_cam0 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam0, "( v4l2src device=/dev/video1 do-timestamp=true ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
    gst_rtsp_media_factory_set_shared(factory_cam0, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam0", factory_cam0);

    GstRTSPMediaFactory *factory_cam1 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam1, "( v4l2src device=/dev/video0 do-timestamp=true ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
    gst_rtsp_media_factory_set_shared(factory_cam1, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam1", factory_cam1);

    GstRTSPMediaFactory *factory_cam2 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam2, "( v4l2src device=/dev/video2 do-timestamp=true ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
    gst_rtsp_media_factory_set_shared(factory_cam2, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam2", factory_cam2);

    g_object_unref(mounts);

    // 5. Attach the server and schedule the command checker
    gst_rtsp_server_attach(server, NULL);
    g_timeout_add(100, (GSourceFunc)check_for_commands, NULL);

    // 6. Start the main loop
    std::cout << "[Main Thread] RTSP server is running." << std::endl;
    g_main_loop_run(loop);
    
    // 7. Cleanup
    cmd_thread.join();
    g_main_loop_unref(loop);
    g_object_unref(server);
    
    return 0;
}