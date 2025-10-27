#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>

// --- Configuration ---
#define PORT 65432 // Must match the port in the Flutter app

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces (0.0.0.0)
    address.sin_port = htons(PORT);
    
    // Bind the socket to the network address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "--- C++ Robot Command Server ---" << std::endl;
    std::cout << "Listening for commands on port " << PORT << std::endl;
    
    while (true) {
        std::cout << "\nWaiting for a connection..." << std::endl;
        // Accept an incoming connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue; // Continue to the next iteration to wait for a new connection
        }
        
        std::cout << "Connection accepted." << std::endl;

        // Buffer to store received data
        std::vector<char> buffer(1024);
        int valread;

        // Read data from the client
        while((valread = read(new_socket, buffer.data(), buffer.size())) > 0) {
            std::string command(buffer.begin(), buffer.begin() + valread);
            
            // --- THIS IS WHERE YOU CHECK THE COMMAND ---
            std::cout << "Received Command: " << command << std::endl;

            // --- TODO: Add your robot's logic here ---
            if (command == "CMD_FORWARD") {
                // Call your function to move the robot forward
            } else if (command == "CMD_STOP") {
                // Call your function to stop the robot
            }
            // ... and so on for all other commands
        }
        
        if (valread == 0) {
            std::cout << "Client disconnected." << std::endl;
        } else {
            perror("read");
        }
        
        close(new_socket); // Close the client socket
    }
    
    return 0;
}