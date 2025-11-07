// // 09_29
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
// #include <chrono>
// #include <atomic>
// #include <csignal>
// #include <fcntl.h>

// // --- Configuration ---
// #define COMMAND_PORT 65432
// #define TOUCH_PORT 65433
// #define DRIVING_PORT 65434
// #define STATUS_PORT 65435

// #pragma pack(push, 1)
// struct StateCommand {
//     uint8_t command_id;
//     uint8_t attack_permission;
//     int8_t  pan_speed;
//     int8_t  tilt_speed;
//     int8_t  zoom_command;
//     float   lateral_wind_speed; // ADDED: Wind speed from app
//     // REMOVED: touch_x and touch_y are gone
// };
// struct DrivingCommand {
//     int8_t move_speed;
//     int8_t turn_angle;
// };
// struct TouchCoordinate {
//     float x;
//     float y;
// };
// struct StatusPacket {
//     uint8_t rtsp_server_status;
//     uint8_t active_mode_id;
//     // float   lateral_wind_speed;
//     // uint8_t wind_direction_index;
//     uint8_t permission_request_active;
//     float   crosshair_x; 
//     float   crosshair_y; 
// };
// #pragma pack(pop)

// std::atomic<float> g_crosshair_x(-1.0f);
// std::atomic<float> g_crosshair_y(-1.0f);

// // --- Global State Management ---
// enum CommandType { STATE_CHANGE, TOUCH_INPUT, DRIVING_INPUT };
// struct GenericCommand {
//     CommandType type;
//     union { StateCommand state; TouchCoordinate touch; DrivingCommand drive; } data;
// };
// std::queue<GenericCommand> command_queue;
// std::mutex command_queue_mutex;

// std::atomic<uint8_t> g_rtsp_server_status(0);
// std::atomic<uint8_t> g_active_mode_id(0);

// GMainLoop *g_main_loop = nullptr;
// std::atomic<bool> g_shutdown_request(false);

// void signalHandler(int signum) {
//     std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down." << std::endl;
//     g_shutdown_request.store(true);
//     if (g_main_loop != nullptr && g_main_loop_is_running(g_main_loop)) {
//         g_main_loop_quit(g_main_loop);
//     }
// }

// // =========================================================================
// //  HELPER FUNCTION FOR ROBUST TCP READ
// // =========================================================================
// bool read_full(int socket, void* buffer, size_t size) {
//     char* ptr = static_cast<char*>(buffer);
//     size_t bytes_left = size;
//     while (bytes_left > 0) {
//         ssize_t bytes_read = read(socket, ptr, bytes_left);
//         if (bytes_read <= 0) {
//             // Error or connection closed
//             return false;
//         }
//         ptr += bytes_read;
//         bytes_left -= bytes_read;
//     }
//     return true;
// }


// // =========================================================================
// //  THREAD 4: STATE COMMAND SERVER (TCP, App -> Server) - CORRECTED
// // =========================================================================
// void commandServerThread() {
//     int server_fd;
//     struct sockaddr_in address;
//     int opt = 1;
//     socklen_t addrlen = sizeof(address);
//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         perror("[Command TCP Thread] socket failed"); return;
//     }
//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
//         perror("[Command TCP Thread] setsockopt"); return;
//     }
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(COMMAND_PORT);
//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         perror("[Command TCP Thread] bind failed"); close(server_fd); return;
//     }
//     if (listen(server_fd, 5) < 0) {
//         perror("[Command TCP Thread] listen"); close(server_fd); return;
//     }
//     fcntl(server_fd, F_SETFL, O_NONBLOCK); // Make the listening socket non-blocking

//     std::cout << "[Command TCP Thread] Listening for state commands on port " << COMMAND_PORT << std::endl;
//     while (!g_shutdown_request.load()) {
//         int new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
//         if (new_socket < 0) {
//             std::this_thread::sleep_for(std::chrono::milliseconds(100));
//             continue;
//         }
        
//         // --- THIS IS THE FIX ---
//         // Use a robust read function to ensure the full packet is received
//         StateCommand received_state;
//         if (read_full(new_socket, &received_state, sizeof(StateCommand))) {
//             GenericCommand cmd = {STATE_CHANGE};
//             cmd.data.state = received_state;
//             std::lock_guard<std::mutex> lock(command_queue_mutex);
//             command_queue.push(cmd);
//         } else {
//             // This can happen if the client disconnects while sending
//             std::cerr << "[Command TCP Thread] Failed to read full packet." << std::endl;
//         }
//         close(new_socket);
//     }
//     close(server_fd);
//     std::cout << "[Command TCP Thread] Shut down." << std::endl;
// }


// // =========================================================================
// //  THREAD 1: STATUS SERVER (TCP, Server -> App)
// // =========================================================================
// void statusServerThread() {
//     int server_fd, client_socket = -1;
//     struct sockaddr_in address;
//     int opt = 1;
//     socklen_t addrlen = sizeof(address);

//     // --- Socket Setup (No changes here) ---
//     server_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (server_fd == 0) {
//         perror("[Status TCP Thread] socket failed");
//         return;
//     }
//     setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(STATUS_PORT);
//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         perror("[Status TCP Thread] bind failed");
//         close(server_fd);
//         return;
//     }
//     if (listen(server_fd, 1) < 0) {
//         perror("[Status TCP Thread] listen failed");
//         close(server_fd);
//         return;
//     }
//     // Make the listening socket non-blocking so it doesn't halt the loop
//     fcntl(server_fd, F_SETFL, O_NONBLOCK);

//     std::cout << "[Status TCP Thread] Listening for app connections on port " << STATUS_PORT << std::endl;

//     // --- Main Loop ---
//     while (!g_shutdown_request.load()) {
//         // If no client is connected, try to accept a new one
//         if (client_socket < 0) {
//             client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
//             if (client_socket < 0) {
//                 // No pending connection, just wait and try again
//                 std::this_thread::sleep_for(std::chrono::milliseconds(500));
//                 continue;
//             }
//             std::cout << "[Status TCP Thread] App connected." << std::endl;
//         }

//         // --- PACKET CREATION LOGIC (This is the updated part) ---

//         // 1. Get the current active mode ID from the global atomic variable.
//         uint8_t current_mode = g_active_mode_id.load();

//         // 2. Determine if the current mode is a "combat mode" that requires permission.
//         bool is_combat_mode = (current_mode >= 2 && current_mode <= 5);
        
//         // 3. Set the permission_request_active flag.
//         uint8_t permission_active_flag = is_combat_mode ? 1 : 0;

//         // 4. Determine the crosshair coordinates to send.
//         float crosshair_x_to_send = -1.0f;
//         float crosshair_y_to_send = -1.0f;
//         bool is_attack_mode = (current_mode == 3 || current_mode == 4);
//         if (is_attack_mode) {
//             crosshair_x_to_send = g_crosshair_x.load();
//             crosshair_y_to_send = g_crosshair_y.load();
//         }

//         // 5. Assemble the new, smaller status packet.
//         //    Note that the wind data fields have been removed.
//         StatusPacket packet_to_send = { 
//             g_rtsp_server_status.load(), 
//             current_mode, 
//             permission_active_flag,
//             crosshair_x_to_send,
//             crosshair_y_to_send
//         };

//         // --- SENDING LOGIC (No changes here) ---
//         if (send(client_socket, &packet_to_send, sizeof(StatusPacket), MSG_NOSIGNAL) < 0) {
//             std::cout << "[Status TCP Thread] App disconnected." << std::endl;
//             close(client_socket);
//             client_socket = -1; // Set to -1 to look for a new connection
//         }

//         // Wait before sending the next update.
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }
    
//     // --- Cleanup ---
//     if (client_socket >= 0) {
//         close(client_socket);
//     }
//     close(server_fd);
//     std::cout << "[Status TCP Thread] Shut down." << std::endl;
// }


// // =========================================================================
// //  THREAD 2: DRIVING SERVER (UDP, App -> Server)
// // =========================================================================
// void drivingServerThread() {
//     int server_fd;
//     struct sockaddr_in address;
//     server_fd = socket(AF_INET, SOCK_DGRAM, 0);
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(DRIVING_PORT);
//     bind(server_fd, (struct sockaddr *)&address, sizeof(address));
//     std::cout << "[Driving UDP Thread] Listening on port " << DRIVING_PORT << std::endl;

//     struct timeval read_timeout;
//     read_timeout.tv_sec = 1;
//     read_timeout.tv_usec = 0;
//     setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));

//     while (!g_shutdown_request.load()) {
//         DrivingCommand received_drive;
//         if (recvfrom(server_fd, &received_drive, sizeof(DrivingCommand), 0, NULL, NULL) > 0) {
//             GenericCommand cmd = {DRIVING_INPUT};
//             cmd.data.drive = received_drive;
//             std::lock_guard<std::mutex> lock(command_queue_mutex);
//             command_queue.push(cmd);
//         }
//     }
//     close(server_fd);
//     std::cout << "[Driving UDP Thread] Shut down." << std::endl;
// }

// // =========================================================================
// //  THREAD 3: TOUCH SERVER (UDP, App -> Server)
// // =========================================================================
// void touchServerThread() {
//     int server_fd;
//     struct sockaddr_in address;
//     server_fd = socket(AF_INET, SOCK_DGRAM, 0);
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(TOUCH_PORT);
//     bind(server_fd, (struct sockaddr *)&address, sizeof(address));
//     std::cout << "[Touch UDP Thread] Listening on port " << TOUCH_PORT << std::endl;
    
//     struct timeval read_timeout;
//     read_timeout.tv_sec = 1;
//     read_timeout.tv_usec = 0;
//     setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));

//     while (!g_shutdown_request.load()) {
//         TouchCoordinate received_touch;
//         if (recvfrom(server_fd, &received_touch, sizeof(TouchCoordinate), 0, NULL, NULL) > 0) {
//             GenericCommand cmd = {TOUCH_INPUT};
//             cmd.data.touch = received_touch;
//             std::lock_guard<std::mutex> lock(command_queue_mutex);
//             command_queue.push(cmd);
//         }
//     }
//     close(server_fd);
//     std::cout << "[Touch UDP Thread] Shut down." << std::endl;
// }

// // =========================================================================
// //  THREAD 5: COMMAND PROCESSING WORKER
// // =========================================================================
// void commandProcessingThread() {
//     std::cout << "[Processing Thread] Worker thread started." << std::endl;
//     while (!g_shutdown_request.load()) {
//         GenericCommand command;
//         bool command_found = false;
//         {
//             std::lock_guard<std::mutex> lock(command_queue_mutex);
//             if (!command_queue.empty()) {
//                 command = command_queue.front();
//                 command_queue.pop();
//                 command_found = true;
//             }
//         }

//         if (command_found) {
//             switch (command.type) {
//                 case STATE_CHANGE:
//                     g_active_mode_id.store(command.data.state.command_id);
//                     std::cout << "--- Processing State Command Packet (TCP) ---" << std::endl;
//                     std::cout << "  Command ID:       " << (int)command.data.state.command_id << std::endl;
//                     std::cout << "  Attack Permission:" << (int)command.data.state.attack_permission << std::endl;
//                     std::cout << "  Pan Speed:        " << (int)command.data.state.pan_speed << std::endl;
//                     std::cout << "  Tilt Speed:       " << (int)command.data.state.tilt_speed << std::endl;
//                     std::cout << "  Zoom Command:     " << (int)command.data.state.zoom_command << std::endl;
//                     std::cout << "  Lateral Wind:     " << command.data.state.lateral_wind_speed << std::endl;
//                     std::cout << "---------------------------------------------" << std::endl;
//                     break;
//                 case TOUCH_INPUT:
//                     std::cout << "--- Processing Touch Coordinate Packet (UDP) ---" << std::endl;
//                     std::cout << "  Touch X: " << command.data.touch.x << std::endl;
//                     std::cout << "  Touch Y: " << command.data.touch.y << std::endl;
//                     std::cout << "----------------------------------------------" << std::endl;
//                     break;
//                 case DRIVING_INPUT:
//                     std::cout << "--- Processing Driving Packet (UDP) ---" << std::endl;
//                     std::cout << "  Move Speed: " << (int)command.data.drive.move_speed << std::endl;
//                     std::cout << "  Turn Angle: " << (int)command.data.drive.turn_angle << std::endl;
//                     std::cout << "---------------------------------------" << std::endl;
//                     break;
//             }
//         } else {
//             std::this_thread::sleep_for(std::chrono::milliseconds(10));
//         }
//     }
//     std::cout << "[Processing Thread] Shut down." << std::endl;
// }

// // =========================================================================
// //  THREAD 6: RTSP SERVER
// // =========================================================================
// void rtspServerThread() {
//     g_main_loop = g_main_loop_new(NULL, FALSE);
//     GstRTSPServer *server = gst_rtsp_server_new();
//     GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

//     GstRTSPMediaFactory *factory_cam0 = gst_rtsp_media_factory_new();
//     gst_rtsp_media_factory_set_launch(factory_cam0, "( v4l2src device=/dev/video1 ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
//     gst_rtsp_media_factory_set_shared(factory_cam0, TRUE);
//     gst_rtsp_mount_points_add_factory(mounts, "/cam0", factory_cam0);

//     GstRTSPMediaFactory *factory_cam1 = gst_rtsp_media_factory_new();
//     gst_rtsp_media_factory_set_launch(factory_cam1, "( v4l2src device=/dev/video0 ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
//     gst_rtsp_media_factory_set_shared(factory_cam1, TRUE);
//     gst_rtsp_mount_points_add_factory(mounts, "/cam1", factory_cam1);

//     g_object_unref(mounts);
//     gst_rtsp_server_attach(server, NULL);
//     g_rtsp_server_status.store(1);
//     std::cout << "[RTSP Thread] RTSP server is running." << std::endl;
    
//     g_main_loop_run(g_main_loop);
    
//     std::cout << "[RTSP Thread] Shutting down." << std::endl;
//     g_rtsp_server_status.store(0);
//     g_object_unref(server);
//     g_main_loop_unref(g_main_loop);
// }

// // =========================================================================
// //  MAIN APPLICATION
// // =========================================================================
// int main(int argc, char *argv[]) {
//     signal(SIGINT, signalHandler);
//     gst_init(&argc, &argv);
//     srand(time(0));

//     std::cout << "[Main Thread] Starting all service threads..." << std::endl;
//     std::vector<std::thread> threads;
//     threads.emplace_back(commandServerThread);
//     threads.emplace_back(touchServerThread);
//     threads.emplace_back(drivingServerThread);
//     threads.emplace_back(statusServerThread);
//     threads.emplace_back(commandProcessingThread);
//     threads.emplace_back(rtspServerThread);

//     std::cout << "[Main Thread] All threads started. Application is running. Press Ctrl+C to exit." << std::endl;

//     for (auto& t : threads) {
//         if (t.joinable()) {
//             t.join();
//         }
//     }
    
//     std::cout << "[Main Thread] All threads have been joined. Exiting." << std::endl;
//     return 0;
// }




//11_07
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
#include <atomic>
#include <csignal>
#include <fcntl.h>
#include <sys/select.h>

// --- Configuration ---
#define COMMAND_PORT 65432
#define TOUCH_PORT 65433
#define DRIVING_PORT 65434
#define STATUS_PORT 65435

#pragma pack(push, 1)
struct StateCommand {
    uint8_t command_id;
    uint8_t attack_permission;
    int8_t  pan_speed;
    int8_t  tilt_speed;
    int8_t  zoom_command;
    float   lateral_wind_speed;
};
struct DrivingCommand {
    int8_t move_speed;
    int8_t turn_angle;
};
struct TouchCoordinate {
    float x;
    float y;
};
struct StatusPacket {
    uint8_t rtsp_server_status;
    uint8_t active_mode_id;
    uint8_t permission_request_active;
    float   crosshair_x; 
    float   crosshair_y; 
};
#pragma pack(pop)

// --- Global State Management ---
std::atomic<float> g_crosshair_x(-1.0f);
std::atomic<float> g_crosshair_y(-1.0f);

enum CommandType { STATE_CHANGE, TOUCH_INPUT, DRIVING_INPUT };
struct GenericCommand {
    CommandType type;
    union { StateCommand state; TouchCoordinate touch; DrivingCommand drive; } data;
};
std::queue<GenericCommand> command_queue;
std::mutex command_queue_mutex;

std::atomic<uint8_t> g_rtsp_server_status(0);
std::atomic<uint8_t> g_active_mode_id(0);

// Shutdown and GStreamer Loop Management ---
GMainLoop *g_main_loop = nullptr;
std::atomic<bool> g_shutdown_request(false);
int g_shutdown_pipe[2]; // Self-pipe for clean signal handling

void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received. Notifying threads..." << std::endl;
    // Write a byte to the pipe to wake up any blocking select() calls
    char a = 1;
    if (write(g_shutdown_pipe[1], &a, 1) < 0) {
        perror("write to shutdown pipe");
    }
    g_shutdown_request.store(true);
    if (g_main_loop != nullptr && g_main_loop_is_running(g_main_loop)) {
        g_main_loop_quit(g_main_loop);
    }
}


// =========================================================================
//  HELPER FUNCTION FOR ROBUST TCP READ
// =========================================================================

bool read_full(int socket, void* buffer, size_t size) {
    char* ptr = static_cast<char*>(buffer);
    size_t bytes_left = size;
    while (bytes_left > 0) {
        ssize_t bytes_read = read(socket, ptr, bytes_left);
        if (bytes_read <= 0) return false;
        ptr += bytes_read;
        bytes_left -= bytes_read;
    }
    return true;
}

// =========================================================================
//  THREAD 4: STATE COMMAND SERVER (TCP, App -> Server) - CORRECTED
// =========================================================================
void commandServerThread() {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { perror("Command TCP: socket"); return; }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    address.sin_family = AF_INET; address.sin_addr.s_addr = INADDR_ANY; address.sin_port = htons(COMMAND_PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) { perror("Command TCP: bind"); close(server_fd); return; }
    if (listen(server_fd, 1) < 0) { perror("Command TCP: listen"); close(server_fd); return; }

    std::cout << "[Command TCP Thread] Listening on port " << COMMAND_PORT << std::endl;

    while (!g_shutdown_request.load()) {
        // --- FIX: This outer loop now waits for a new client to connect ---
        fd_set accept_fds;
        FD_ZERO(&accept_fds);
        FD_SET(server_fd, &accept_fds);
        FD_SET(g_shutdown_pipe[0], &accept_fds);
        int max_fd = std::max(server_fd, g_shutdown_pipe[0]);
        struct timeval timeout = {1, 0}; // 1 second timeout

        int activity = select(max_fd + 1, &accept_fds, NULL, NULL, &timeout);

        if (activity <= 0) { // Timeout or error
            continue;
        }

        if (FD_ISSET(g_shutdown_pipe[0], &accept_fds)) {
            // Shutdown signal received
            break;
        }

        int client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (client_socket < 0) {
            continue; // Error on accept
        }

        std::cout << "[Command TCP Thread] Client connected." << std::endl;

        // --- FIX: This new inner loop handles the connected client ---
        while (!g_shutdown_request.load()) {
            StateCommand received_state;
            // read_full will return false if the client disconnects or there's an error
            if (read_full(client_socket, &received_state, sizeof(StateCommand))) {
                GenericCommand cmd = {STATE_CHANGE, {.state = received_state}};
                std::lock_guard<std::mutex> lock(command_queue_mutex);
                command_queue.push(cmd);
            } else {
                // Client disconnected, break the inner loop to wait for a new client
                std::cout << "[Command TCP Thread] Client disconnected." << std::endl;
                break;
            }
        }
        close(client_socket);
    }

    close(server_fd);
    std::cout << "[Command TCP Thread] Shut down." << std::endl;
}

// // =========================================================================
// //  THREAD 1: STATUS SERVER (TCP, Server -> App)
// // =========================================================================
void statusServerThread() {
    int server_fd, client_socket = -1;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { perror("Status TCP: socket"); return; }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    address.sin_family = AF_INET; address.sin_addr.s_addr = INADDR_ANY; address.sin_port = htons(STATUS_PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) { perror("Status TCP: bind"); close(server_fd); return; }
    if (listen(server_fd, 1) < 0) { perror("Status TCP: listen"); close(server_fd); return; }

    std::cout << "[Status TCP Thread] Listening on port " << STATUS_PORT << std::endl;
    while (!g_shutdown_request.load()) {
        if (client_socket < 0) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(server_fd, &read_fds);
            FD_SET(g_shutdown_pipe[0], &read_fds);
            int max_fd = std::max(server_fd, g_shutdown_pipe[0]);
            struct timeval timeout = {1, 0};

            if (select(max_fd + 1, &read_fds, NULL, NULL, &timeout) > 0 && FD_ISSET(server_fd, &read_fds)) {
                client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
                if (client_socket >= 0) std::cout << "[Status TCP Thread] App connected." << std::endl;
            }
            if (g_shutdown_request.load()) break;
            if (client_socket < 0) continue;
        }

        uint8_t current_mode = g_active_mode_id.load();
        uint8_t permission_active_flag = (current_mode >= 2 && current_mode <= 5) ? 1 : 0;
        float crosshair_x_to_send = (current_mode == 3 || current_mode == 4) ? g_crosshair_x.load() : -1.0f;
        float crosshair_y_to_send = (current_mode == 3 || current_mode == 4) ? g_crosshair_y.load() : -1.0f;
        StatusPacket packet_to_send = { g_rtsp_server_status.load(), current_mode, permission_active_flag, crosshair_x_to_send, crosshair_y_to_send };

        if (send(client_socket, &packet_to_send, sizeof(StatusPacket), MSG_NOSIGNAL) < 0) {
            std::cout << "[Status TCP Thread] App disconnected." << std::endl;
            close(client_socket);
            client_socket = -1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (client_socket >= 0) close(client_socket);
    close(server_fd);
    std::cout << "[Status TCP Thread] Shut down." << std::endl;
}


// // =========================================================================
// //  THREAD 2: DRIVING SERVER (UDP, App -> Server)
// // =========================================================================
void drivingServerThread() {
    int server_fd;
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { perror("Driving UDP: socket"); return; }
    struct sockaddr_in address;
    address.sin_family = AF_INET; address.sin_addr.s_addr = INADDR_ANY; address.sin_port = htons(DRIVING_PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) { perror("Driving UDP: bind"); close(server_fd); return; }

    std::cout << "[Driving UDP Thread] Listening on port " << DRIVING_PORT << std::endl;
    while (!g_shutdown_request.load()) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        FD_SET(g_shutdown_pipe[0], &read_fds);
        int max_fd = std::max(server_fd, g_shutdown_pipe[0]);
        struct timeval timeout = {1, 0};

        if (select(max_fd + 1, &read_fds, NULL, NULL, &timeout) > 0 && FD_ISSET(server_fd, &read_fds)) {
            DrivingCommand received_drive;
            if (recvfrom(server_fd, &received_drive, sizeof(DrivingCommand), 0, NULL, NULL) > 0) {
                GenericCommand cmd = {DRIVING_INPUT, {.drive = received_drive}};
                std::lock_guard<std::mutex> lock(command_queue_mutex);
                command_queue.push(cmd);
            }
        }
    }
    close(server_fd);
    std::cout << "[Driving UDP Thread] Shut down." << std::endl;
}


// // =========================================================================
// //  THREAD 3: TOUCH SERVER (UDP, App -> Server)
// // =========================================================================
void touchServerThread() {
    int server_fd;
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { perror("Touch UDP: socket"); return; }
    struct sockaddr_in address;
    address.sin_family = AF_INET; address.sin_addr.s_addr = INADDR_ANY; address.sin_port = htons(TOUCH_PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) { perror("Touch UDP: bind"); close(server_fd); return; }

    std::cout << "[Touch UDP Thread] Listening on port " << TOUCH_PORT << std::endl;
    while (!g_shutdown_request.load()) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        FD_SET(g_shutdown_pipe[0], &read_fds);
        int max_fd = std::max(server_fd, g_shutdown_pipe[0]);
        struct timeval timeout = {1, 0};

        if (select(max_fd + 1, &read_fds, NULL, NULL, &timeout) > 0 && FD_ISSET(server_fd, &read_fds)) {
            TouchCoordinate received_touch;
            if (recvfrom(server_fd, &received_touch, sizeof(TouchCoordinate), 0, NULL, NULL) > 0) {
                GenericCommand cmd = {TOUCH_INPUT, {.touch = received_touch}};
                std::lock_guard<std::mutex> lock(command_queue_mutex);
                command_queue.push(cmd);
            }
        }
    }
    close(server_fd);
    std::cout << "[Touch UDP Thread] Shut down." << std::endl;
}


// // =========================================================================
// //  THREAD 5: COMMAND PROCESSING WORKER
// // =========================================================================
void commandProcessingThread() {
    std::cout << "[Processing Thread] Worker thread started." << std::endl;
    while (!g_shutdown_request.load()) {
        GenericCommand command;
        bool command_found = false;
        {
            std::lock_guard<std::mutex> lock(command_queue_mutex);
            if (!command_queue.empty()) {
                command = command_queue.front();
                command_queue.pop();
                command_found = true;
            }
        }
        if (command_found) {
            switch (command.type) {
                case STATE_CHANGE:
                    g_active_mode_id.store(command.data.state.command_id);
                    std::cout << "--- Processing State Command Packet (TCP) ---\n"
                              << "  Command ID:       " << (int)command.data.state.command_id << "\n"
                              << "  Attack Permission:" << (int)command.data.state.attack_permission << "\n"
                              << "  Pan Speed:        " << (int)command.data.state.pan_speed << "\n"
                              << "  Tilt Speed:       " << (int)command.data.state.tilt_speed << "\n"
                              << "  Zoom Command:     " << (int)command.data.state.zoom_command << "\n"
                              << "  Lateral Wind:     " << command.data.state.lateral_wind_speed << "\n"
                              << "---------------------------------------------" << std::endl;
                    break;
                case TOUCH_INPUT:
                    std::cout << "--- Processing Touch Coordinate Packet (UDP) ---\n"
                              << "  Touch X: " << command.data.touch.x << "\n"
                              << "  Touch Y: " << command.data.touch.y << "\n"
                              << "----------------------------------------------" << std::endl;
                    break;
                case DRIVING_INPUT:
                    std::cout << "--- Processing Driving Packet (UDP) ---\n"
                              << "  Move Speed: " << (int)command.data.drive.move_speed << "\n"
                              << "  Turn Angle: " << (int)command.data.drive.turn_angle << "\n"
                              << "---------------------------------------" << std::endl;
                    break;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    std::cout << "[Processing Thread] Shut down." << std::endl;
}


// // =========================================================================
// //  THREAD 6: RTSP SERVER
// // =========================================================================
void rtspServerThread() {
    g_main_loop = g_main_loop_new(NULL, FALSE);
    GstRTSPServer *server = gst_rtsp_server_new();
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

    GstRTSPMediaFactory *factory_cam0 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam0, "( v4l2src device=/dev/video1 ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
    gst_rtsp_media_factory_set_shared(factory_cam0, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam0", factory_cam0);
    std::cout << "[RTSP Thread] Stream ready at rtsp://<your_ip>:8554/cam0" << std::endl;

    GstRTSPMediaFactory *factory_cam1 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam1, "( v4l2src device=/dev/video0 ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
    gst_rtsp_media_factory_set_shared(factory_cam1, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam1", factory_cam1);
    std::cout << "[RTSP Thread] Stream ready at rtsp://<your_ip>:8554/cam1" << std::endl;

    g_object_unref(mounts);
    gst_rtsp_server_attach(server, NULL);
    g_rtsp_server_status.store(1);
    std::cout << "[RTSP Thread] RTSP server is running." << std::endl;
    
    g_main_loop_run(g_main_loop);
    
    std::cout << "[RTSP Thread] Shutting down." << std::endl;
    g_rtsp_server_status.store(0);
    g_object_unref(server);
    g_main_loop_unref(g_main_loop);
    g_main_loop = nullptr;
}

// =========================================================================
//  MAIN APPLICATION
// =========================================================================
int main(int argc, char *argv[]) {
    if (pipe(g_shutdown_pipe) == -1) {
        perror("pipe");
        return 1;
    }
    fcntl(g_shutdown_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(g_shutdown_pipe[1], F_SETFL, O_NONBLOCK);

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    gst_init(&argc, &argv);

    std::cout << "[Main Thread] Starting all service threads..." << std::endl;
    
    std::vector<std::thread> threads;
    threads.emplace_back(rtspServerThread);
    threads.emplace_back(commandServerThread);
    threads.emplace_back(touchServerThread);
    threads.emplace_back(drivingServerThread);
    threads.emplace_back(statusServerThread);
    threads.emplace_back(commandProcessingThread);

    std::cout << "[Main Thread] All threads started. Application is running. Press Ctrl+C to exit." << std::endl;

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    close(g_shutdown_pipe[0]);
    close(g_shutdown_pipe[1]);
    std::cout << "[Main Thread] All threads have been joined. Exiting." << std::endl;
    return 0;
}

