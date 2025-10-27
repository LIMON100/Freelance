// #include <gst/gst.h>
// #include <gst/rtsp-server/rtsp-server.h>
// #include <iostream>
// #include <string>
// #include <vector>
// #include <thread>
// #include <mutex>
// #include <queue>
// #include <unistd.h>
// #include <cstdint>
// #include <sys/socket.h>
// #include <netinet/in.h>

// // --- Configuration: ONLY ONE PORT NEEDED NOW ---
// #define COMMAND_PORT 65432

// #pragma pack(push, 1)
// // --- NEW: Unified 13-byte Command Struct ---
// struct UnifiedCommand {
//     uint8_t command_id;
//     int8_t  move_front_back_speed;
//     int8_t  turn_left_right_angle;
//     uint8_t gun_trigger;
//     uint8_t gun_trigger_permission;
//     float   touch_x; // Normalized X position
//     float   touch_y; // Normalized Y position
// };
// #pragma pack(pop)

// std::queue<UnifiedCommand> command_queue;
// std::mutex command_queue_mutex;

// // =========================================================================
// //  COMMAND SERVER THREAD (Now handles the single unified packet)
// // =========================================================================
// void commandServerThread() {
//     int server_fd, new_socket;
//     struct sockaddr_in address;
//     int opt = 1;
//     socklen_t addrlen = sizeof(address);
    
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
//     if (listen(server_fd, 5) < 0) {
//         perror("[Command Thread] listen"); return;
//     }

//     std::cout << "[Command Thread] Listening for unified commands on port " << COMMAND_PORT << std::endl;

//     while (true) {
//         if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
//             perror("[Command Thread] accept"); continue;
//         }
        
//         UnifiedCommand received_command;
//         // Read exactly the size of our new, larger struct
//         int valread = read(new_socket, &received_command, sizeof(UnifiedCommand));
        
//         if (valread == sizeof(UnifiedCommand)) {
//             std::lock_guard<std::mutex> lock(command_queue_mutex);
//             command_queue.push(received_command);
//         } else if (valread > 0) {
//              std::cerr << "[Command Thread] Warning: Received incomplete packet. Expected "
//                        << sizeof(UnifiedCommand) << " bytes, but got " << valread << std::endl;
//         }
        
//         close(new_socket);
//     }
// }

// // =========================================================================
// //  GSTREAMER CALLBACK (Now processes the single unified packet)
// // =========================================================================
// static gboolean check_for_commands(gpointer user_data) {
//     UnifiedCommand command;
//     bool command_found = false;
//     {
//         std::lock_guard<std::mutex> lock(command_queue_mutex);
//         if (!command_queue.empty()) {
//             command = command_queue.front();
//             command_queue.pop();
//             command_found = true;
//         }
//     }

//     if (command_found) {
//         std::cout << "--- Processing Unified Command Packet ---" << std::endl;
//         std::cout << "  Command ID: " << (int)command.command_id << std::endl;
//         std::cout << "  Speed: " << (int)command.move_front_back_speed << std::endl;
//         std::cout << "  Angle: " << (int)command.turn_left_right_angle << std::endl;
//         std::cout << "  Gun Trigger: " << (int)command.gun_trigger << std::endl;
//         std::cout << "  Gun Permission: " << (int)command.gun_trigger_permission << std::endl;
//         // Only print touch coordinates if they are valid (not the default -1.0)
//         if (command.touch_x >= 0.0) {
//             std::cout << "  Touch X: " << command.touch_x << std::endl;
//             std::cout << "  Touch Y: " << command.touch_y << std::endl;
//         }
//         std::cout << "---------------------------------------" << std::endl;

//     }
//     return TRUE;
// }

// // =========================================================================
// //  MAIN APPLICATION (Now only launches one command thread)
// // =========================================================================
// int main(int argc, char *argv[]) {
//     gst_init(&argc, &argv);

//     // Launch ONLY the single command server thread
//     std::thread cmd_thread(commandServerThread);

//     GMainLoop *loop = g_main_loop_new(NULL, FALSE);
//     GstRTSPServer *server = gst_rtsp_server_new();
//     GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

//     // --- Media Factory for /cam0 ---
//     GstRTSPMediaFactory *factory_cam0 = gst_rtsp_media_factory_new();
//     gst_rtsp_media_factory_set_launch(factory_cam0, "( v4l2src device=/dev/video1 do-timestamp=true ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
//     gst_rtsp_media_factory_set_shared(factory_cam0, TRUE);
//     gst_rtsp_mount_points_add_factory(mounts, "/cam0", factory_cam0);

//     // --- Media Factory for /cam1 ---
//     GstRTSPMediaFactory *factory_cam1 = gst_rtsp_media_factory_new();
//     gst_rtsp_media_factory_set_launch(factory_cam1, "( v4l2src device=/dev/video0 do-timestamp=true ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
//     gst_rtsp_media_factory_set_shared(factory_cam1, TRUE);
//     gst_rtsp_mount_points_add_factory(mounts, "/cam1", factory_cam1);

//     // --- Media Factory for /cam2 ---
//     GstRTSPMediaFactory *factory_cam2 = gst_rtsp_media_factory_new();
//     gst_rtsp_media_factory_set_launch(factory_cam2, "( v4l2src device=/dev/video2 do-timestamp=true ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
//     gst_rtsp_media_factory_set_shared(factory_cam2, TRUE);
//     gst_rtsp_mount_points_add_factory(mounts, "/cam2", factory_cam2);

//     g_object_unref(mounts);

//     gst_rtsp_server_attach(server, NULL);
//     g_timeout_add(100, (GSourceFunc)check_for_commands, NULL);

//     std::cout << "[Main Thread] RTSP server is running." << std::endl;
//     g_main_loop_run(loop);
    
//     // Cleanup
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
#include <thread>
#include <mutex>
#include <queue>
#include <unistd.h>
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <chrono> 

// --- Configuration ---
#define COMMAND_PORT 65432
#define TOUCH_PORT 65433

#pragma pack(push, 1)
// Unified Command Struct (for TCP)
struct UnifiedCommand {
    uint8_t command_id;
    int8_t  move_front_back_speed;
    int8_t  turn_left_right_angle;
    uint8_t gun_trigger;
    uint8_t gun_trigger_permission;
    float   touch_x;
    float   touch_y;
};

// Struct for touch coordinates (for UDP)
struct TouchCoordinate {
    float x;
    float y;
};
#pragma pack(pop)

// Global queue to hold all incoming commands (from both TCP and UDP)
std::queue<UnifiedCommand> command_queue;
std::mutex command_queue_mutex;

// =========================================================================
//  DEDICATED COMMAND PROCESSING THREAD
// =========================================================================
void commandProcessingThread() {
    std::cout << "[Processing Thread] Worker thread started." << std::endl;
    while (true) {
        UnifiedCommand command;
        bool command_found = false;

        // Briefly lock the mutex to check for and grab a command
        {
            std::lock_guard<std::mutex> lock(command_queue_mutex);
            if (!command_queue.empty()) {
                command = command_queue.front();
                command_queue.pop();
                command_found = true;
            }
        } // Mutex is released here

        if (command_found) {
            // Process the command. This is where you would put your motor control logic.
            // Printing here won't block the network threads.
            if (command.command_id == 99) {
                std::cout << "--- Processing Touch Coordinate Packet (UDP) ---" << std::endl;
                std::cout << "  Touch X: " << command.touch_x << std::endl;
                std::cout << "  Touch Y: " << command.touch_y << std::endl;
                std::cout << "----------------------------------------------" << std::endl;
            } else {
                std::cout << "--- Processing Unified Command Packet (TCP) ---" << std::endl;
                std::cout << "  Command ID: " << (int)command.command_id << std::endl;
                std::cout << "  Speed: " << (int)command.move_front_back_speed << std::endl;
                std::cout << "  Angle: " << (int)command.turn_left_right_angle << std::endl;
                std::cout << "  Gun Trigger: " << (int)command.gun_trigger << std::endl;
                std::cout << "  Gun Permission: " << (int)command.gun_trigger_permission << std::endl;
                std::cout << "---------------------------------------------" << std::endl;
            }
        } else {
            // If the queue is empty, sleep briefly to prevent 100% CPU usage.
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}


// =========================================================================
//  TOUCH SERVER THREAD (UDP)
// =========================================================================
void touchServerThread() {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("[Touch UDP Thread] socket failed"); return;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(TOUCH_PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[Touch UDP Thread] bind failed"); return;
    }
    std::cout << "[Touch UDP Thread] Listening for touch coordinates on port " << TOUCH_PORT << std::endl;
    while (true) {
        TouchCoordinate received_touch;
        int valread = recvfrom(server_fd, &received_touch, sizeof(TouchCoordinate), 0, NULL, NULL);
        if (valread == sizeof(TouchCoordinate)) {
            // Wrap the touch data in a UnifiedCommand and add to the queue
            UnifiedCommand touch_command = {0};
            touch_command.command_id = 99; // Special ID for touch data
            touch_command.touch_x = received_touch.x;
            touch_command.touch_y = received_touch.y;
            
            std::lock_guard<std::mutex> lock(command_queue_mutex);
            command_queue.push(touch_command);
        } else if (valread > 0) {
            std::cerr << "[Touch UDP Thread] Warning: Received incomplete packet. Expected "
                      << sizeof(TouchCoordinate) << " bytes, but got " << valread << std::endl;
        }
    }
    close(server_fd);
}


// =========================================================================
//  COMMAND SERVER THREAD (TCP)
// =========================================================================
void commandServerThread() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[Command TCP Thread] socket failed"); return;
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("[Command TCP Thread] setsockopt"); return;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(COMMAND_PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[Command TCP Thread] bind failed"); return;
    }
    if (listen(server_fd, 5) < 0) {
        perror("[Command TCP Thread] listen"); return;
    }
    std::cout << "[Command TCP Thread] Listening for unified commands on port " << COMMAND_PORT << std::endl;
    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            perror("[Command TCP Thread] accept"); continue;
        }
        UnifiedCommand received_command;
        int valread = read(new_socket, &received_command, sizeof(UnifiedCommand));
        if (valread == sizeof(UnifiedCommand)) {
            std::lock_guard<std::mutex> lock(command_queue_mutex);
            command_queue.push(received_command);
        } else if (valread > 0) {
             std::cerr << "[Command TCP Thread] Warning: Received incomplete packet. Expected "
                       << sizeof(UnifiedCommand) << " bytes, but got " << valread << std::endl;
        }
        close(new_socket);
    }
}

// =========================================================================
//  MAIN APPLICATION
// =========================================================================
int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    // Start all three background threads
    std::thread cmd_thread(commandServerThread);
    std::thread touch_thread(touchServerThread);
    std::thread worker_thread(commandProcessingThread); 

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstRTSPServer *server = gst_rtsp_server_new();
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

    // --- RKNN GStreamer pipelines using v4l2src and mpph264enc for hardware acceleration ---
    
    // --- Media Factory for /cam0 ---
    GstRTSPMediaFactory *factory_cam0 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam0, "( v4l2src device=/dev/video1 ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
    gst_rtsp_media_factory_set_shared(factory_cam0, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam0", factory_cam0);
    std::cout << "[Main Thread] Stream ready at rtsp://<your_ip>:8554/cam0" << std::endl;

    // --- Media Factory for /cam1 ---
    GstRTSPMediaFactory *factory_cam1 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam1, "( v4l2src device=/dev/video0 ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
    gst_rtsp_media_factory_set_shared(factory_cam1, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam1", factory_cam1);
    std::cout << "[Main Thread] Stream ready at rtsp://<your_ip>:8554/cam1" << std::endl;

    // --- Media Factory for /cam2 ---
    GstRTSPMediaFactory *factory_cam2 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam2, "( v4l2src device=/dev/video2 ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
    gst_rtsp_media_factory_set_shared(factory_cam2, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam2", factory_cam2);
    std::cout << "[Main Thread] Stream ready at rtsp://<your_ip>:8554/cam2" << std::endl;

    g_object_unref(mounts);
    gst_rtsp_server_attach(server, NULL);

    std::cout << "[Main Thread] RTSP server is running." << std::endl;
    g_main_loop_run(loop);
    
    // --- Cleanup ---
    // In a real application, you'd need a way to signal these threads to exit gracefully.
    // For this example, they will run until the program is terminated.
    cmd_thread.join();
    touch_thread.join();
    worker_thread.join();
    
    g_main_loop_unref(loop);
    g_object_unref(server);
    
    return 0;
}
