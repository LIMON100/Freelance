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
#include <atomic> // For thread-safe status flags

// --- Configuration ---
#define COMMAND_PORT 65432  // For incoming TCP state commands
#define TOUCH_PORT 65433    // For incoming UDP touch commands
#define DRIVING_PORT 65434  // For incoming UDP driving commands
#define STATUS_PORT 65435   // For outgoing TCP status updates

#pragma pack(push, 1)
// --- Data Structures for Communication ---

// FROM App TO Server (TCP)
struct StateCommand {
    uint8_t command_id;
    uint8_t attack_permission;
    int8_t  pan_speed;
    int8_t  tilt_speed;
    int8_t  zoom_command;
    float   touch_x;
    float   touch_y;
};

// FROM App TO Server (UDP)
struct DrivingCommand {
    int8_t move_speed;
    int8_t turn_angle;
};

// FROM App TO Server (UDP)
struct TouchCoordinate {
    float x;
    float y;
};

// FROM Server TO App (TCP)
struct StatusPacket {
    uint8_t rtsp_server_status;   // 1 byte
    uint8_t active_mode_id;       // 1 byte
    float   lateral_wind_speed;   // 4 bytes
    uint8_t wind_direction_index; // 1 byte
}; // Total size: 7 bytes
#pragma pack(pop)


// --- Global State Management ---
enum CommandType { STATE_CHANGE, TOUCH_INPUT, DRIVING_INPUT };
struct GenericCommand {
    CommandType type;
    union { StateCommand state; TouchCoordinate touch; DrivingCommand drive; } data;
};
std::queue<GenericCommand> command_queue;
std::mutex command_queue_mutex;

// Thread-safe global variables holding the server's current state
std::atomic<uint8_t> g_rtsp_server_status(0); // 0=Down, 1=Running
std::atomic<uint8_t> g_active_mode_id(0);     // 0=IDLE
std::atomic<float>   g_lateral_wind_speed(-2.3f); // Example value
std::atomic<uint8_t> g_wind_direction_index(0);   // 0=North


// =========================================================================
//  THREAD 1: STATUS SERVER (TCP, Server -> App)
// =========================================================================
void statusServerThread() {
    int server_fd, client_socket = -1;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[Status TCP Thread] socket failed"); return;
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("[Status TCP Thread] setsockopt"); return;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(STATUS_PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[Status TCP Thread] bind failed"); return;
    }
    if (listen(server_fd, 1) < 0) {
        perror("[Status TCP Thread] listen"); return;
    }
    std::cout << "[Status TCP Thread] Listening for app connection on port " << STATUS_PORT << std::endl;

    while (true) {
        if (client_socket < 0) {
            if ((client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
                perror("[Status TCP Thread] accept");
                continue;
            }
            std::cout << "[Status TCP Thread] App connected." << std::endl;
        }

        StatusPacket packet;
        packet.rtsp_server_status = g_rtsp_server_status.load();
        packet.active_mode_id = g_active_mode_id.load();
        packet.lateral_wind_speed = g_lateral_wind_speed.load();
        packet.wind_direction_index = g_wind_direction_index.load();

        if (send(client_socket, &packet, sizeof(StatusPacket), MSG_NOSIGNAL) < 0) {
            std::cout << "[Status TCP Thread] App disconnected. Waiting for new connection." << std::endl;
            close(client_socket);
            client_socket = -1;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    close(server_fd);
}


// =========================================================================
//  THREAD 2: DRIVING SERVER (UDP, App -> Server)
// =========================================================================
void drivingServerThread() {
    int server_fd;
    struct sockaddr_in address;
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("[Driving UDP Thread] socket failed"); return;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(DRIVING_PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[Driving UDP Thread] bind failed"); return;
    }
    std::cout << "[Driving UDP Thread] Listening for driving commands on port " << DRIVING_PORT << std::endl;

    while (true) {
        DrivingCommand received_drive;
        if (recvfrom(server_fd, &received_drive, sizeof(DrivingCommand), 0, NULL, NULL) == sizeof(DrivingCommand)) {
            GenericCommand cmd;
            cmd.type = DRIVING_INPUT;
            cmd.data.drive = received_drive;
            std::lock_guard<std::mutex> lock(command_queue_mutex);
            command_queue.push(cmd);
        }
    }
    close(server_fd);
}


// =========================================================================
//  THREAD 3: TOUCH SERVER (UDP, App -> Server)
// =========================================================================
void touchServerThread() {
    int server_fd;
    struct sockaddr_in address;
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
        if (recvfrom(server_fd, &received_touch, sizeof(TouchCoordinate), 0, NULL, NULL) == sizeof(TouchCoordinate)) {
            GenericCommand cmd;
            cmd.type = TOUCH_INPUT;
            cmd.data.touch = received_touch;
            std::lock_guard<std::mutex> lock(command_queue_mutex);
            command_queue.push(cmd);
        }
    }
    close(server_fd);
}


// =========================================================================
//  THREAD 4: STATE COMMAND SERVER (TCP, App -> Server)
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
    std::cout << "[Command TCP Thread] Listening for state commands on port " << COMMAND_PORT << std::endl;
    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            perror("[Command TCP Thread] accept"); continue;
        }
        StateCommand received_state;
        int valread = read(new_socket, &received_state, sizeof(StateCommand));
        if (valread == sizeof(StateCommand)) {
            GenericCommand cmd;
            cmd.type = STATE_CHANGE;
            cmd.data.state = received_state;
            std::lock_guard<std::mutex> lock(command_queue_mutex);
            command_queue.push(cmd);
        }
        close(new_socket);
    }
}

// =========================================================================
//  THREAD 5: COMMAND PROCESSING WORKER
// =========================================================================
void commandProcessingThread() {
    std::cout << "[Processing Thread] Worker thread started." << std::endl;
    while (true) {
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
                    std::cout << "--- Processing State Command Packet (TCP) ---" << std::endl;
                    std::cout << "  Command ID:       " << (int)command.data.state.command_id << " -> SETTING GLOBAL STATE" << std::endl;
                    std::cout << "  Attack Permission:" << (int)command.data.state.attack_permission << std::endl;
                    std::cout << "  Pan Speed:        " << (int)command.data.state.pan_speed << std::endl;
                    std::cout << "  Tilt Speed:       " << (int)command.data.state.tilt_speed << std::endl;
                    std::cout << "  Zoom Command:     " << (int)command.data.state.zoom_command << std::endl;
                    std::cout << "---------------------------------------------" << std::endl;
                    break;
                case TOUCH_INPUT:
                    std::cout << "--- Processing Touch Coordinate Packet (UDP) ---" << std::endl;
                    std::cout << "  Touch X: " << command.data.touch.x << std::endl;
                    std::cout << "  Touch Y: " << command.data.touch.y << std::endl;
                    std::cout << "----------------------------------------------" << std::endl;
                    break;
                case DRIVING_INPUT:
                    std::cout << "--- Processing Driving Packet (UDP) ---" << std::endl;
                    std::cout << "  Move Speed: " << (int)command.data.drive.move_speed << std::endl;
                    std::cout << "  Turn Angle: " << (int)command.data.drive.turn_angle << std::endl;
                    std::cout << "---------------------------------------" << std::endl;
                    break;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

// =========================================================================
//  MAIN APPLICATION
// =========================================================================
int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    std::thread cmd_thread(commandServerThread);
    std::thread touch_thread(touchServerThread);
    std::thread drive_thread(drivingServerThread);
    std::thread status_thread(statusServerThread);
    std::thread worker_thread(commandProcessingThread); 

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstRTSPServer *server = gst_rtsp_server_new();
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

    GstRTSPMediaFactory *factory_cam0 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam0, "( v4l2src device=/dev/video1 ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
    gst_rtsp_media_factory_set_shared(factory_cam0, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam0", factory_cam0);
    std::cout << "[Main Thread] Stream ready at rtsp://<your_ip>:8554/cam0" << std::endl;

    GstRTSPMediaFactory *factory_cam1 = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory_cam1, "( v4l2src device=/dev/video0 ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! mpph264enc bps=2000000 ! h264parse config-interval=-1 ! rtph264pay name=pay0 pt=96 )");
    gst_rtsp_media_factory_set_shared(factory_cam1, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/cam1", factory_cam1);
    std::cout << "[Main Thread] Stream ready at rtsp://<your_ip>:8554/cam1" << std::endl;

    g_object_unref(mounts);
    gst_rtsp_server_attach(server, NULL);

    g_rtsp_server_status.store(1);
    std::cout << "[Main Thread] RTSP server is running." << std::endl;
    
    g_main_loop_run(loop);
    
    g_rtsp_server_status.store(0);

    // Cleanup
    cmd_thread.join();
    touch_thread.join();
    drive_thread.join();
    status_thread.join();
    worker_thread.join();
    
    g_main_loop_unref(loop);
    g_object_unref(server);
    
    return 0;
}