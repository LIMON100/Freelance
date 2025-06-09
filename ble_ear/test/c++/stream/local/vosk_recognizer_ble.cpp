#include <iostream>
#include <vector>
#include <string>
#include <vosk_api.h>
#include <SDL2/SDL.h>    // For live audio capture
#include <atomic>       // For std::atomic_bool
#include <csignal>      // For signal handling (Ctrl+C)
#include <cstdio>       // For popen, pclose, fgets
#include <array>        // For std::array
#include <thread>       // For std::this_thread::sleep_for
#include <chrono>       // For std::chrono::seconds
#include <algorithm>    // For std::remove, std::replace
#include <cstring>      // For strstr, strlen

#define DEFAULT_MODEL_PATH "vosk-model-small-en-us-0.15" // Default Vosk model
#define DEFAULT_TARGET_BLE_ADDRESS "2C:BE:EB:C5:B1:41" // Your earphone's MAC ADDRESS
#define HEADSET_NAME_HINT "CMF Neckband Pro"           // Part of your headset's name as SDL might see it

#define SAMPLE_RATE 16000.0f
#define AUDIO_FORMAT AUDIO_S16LSB // 16-bit signed little-endian
#define CHANNELS 1                // Mono
#define SAMPLES_PER_CALLBACK 4096 // Samples per SDL callback chunk

// Global flag to signal termination (for Ctrl+C)
std::atomic_bool g_quit_flag(false);

// Vosk recognizer - make it accessible to the callback
VoskRecognizer *g_recognizer = nullptr;
SDL_AudioDeviceID g_dev_id = 0; // Global SDL audio device ID

// --- Utility function to execute shell commands and get output ---
std::string exec_shell_command(const char* cmd) {
    std::array<char, 256> buffer; // Increased buffer size
    std::string result;
    // Redirect stderr to stdout to capture all output from bluetoothctl
    std::string cmd_with_stderr_redirect = std::string(cmd) + " 2>&1";
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd_with_stderr_redirect.c_str(), "r"), pclose);
    if (!pipe) {
        std::cerr << "popen() failed for command: " << cmd_with_stderr_redirect << std::endl;
        return "ERROR_POPEN";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// --- BLE Connection Logic (Simplified using bluetoothctl) ---
bool connect_ble_device(const std::string& device_address) {
    std::cout << "Attempting to connect to BLE device: " << device_address << std::endl;

    exec_shell_command("bluetoothctl power on");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::string connect_status_cmd = "bluetoothctl info " + device_address;
    std::string status_output = exec_shell_command(connect_status_cmd.c_str());

    if (status_output.find("Connected: yes") != std::string::npos) {
        std::cout << "Device " << device_address << " is already connected." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Give audio profiles time
        return true;
    }
    if (status_output.find("Device " + device_address + " not available") != std::string::npos &&
        status_output.find("No default controller available") == std::string::npos) { // Don't attempt if no controller
        std::cout << "Device " << device_address << " not found by 'bluetoothctl info'. Attempting to scan and connect." << std::endl;
    } else if (status_output.find("No default controller available") != std::string::npos) {
        std::cerr << "Bluetoothctl reports: No default controller available. Cannot proceed with BLE connection." << std::endl;
        return false;
    }


    FILE *bt_pipe = popen("bluetoothctl", "w");
    if (!bt_pipe) {
        std::cerr << "Failed to open pipe to bluetoothctl" << std::endl;
        return false;
    }

    // Try to remove device first to ensure clean state (optional, can cause issues if not paired)
    // fprintf(bt_pipe, "remove %s\n", device_address.c_str());
    // fflush(bt_pipe);
    // std::this_thread::sleep_for(std::chrono::seconds(3));

    // Trust the device (important for auto-reconnect and profile activation)
    // Do this BEFORE connecting if it's not already trusted.
    // Manual pairing/trusting via GUI or bluetoothctl directly is often more reliable first.
    // fprintf(bt_pipe, "trust %s\n", device_address.c_str());
    // fflush(bt_pipe);
    // std::this_thread::sleep_for(std::chrono::seconds(3));

    fprintf(bt_pipe, "connect %s\n", device_address.c_str());
    fflush(bt_pipe); // Make sure command is sent
    std::cout << "Connection command sent to bluetoothctl. Waiting for connection (approx 10-20 seconds)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(15)); // Increased wait time

    fprintf(bt_pipe, "exit\n");
    int close_status = pclose(bt_pipe);
    if (close_status == -1) {
        std::cerr << "pclose() failed for bluetoothctl pipe." << std::endl;
    }


    // Check connection status again
    std::cout << "Checking connection status after attempt..." << std::endl;
    status_output = exec_shell_command(connect_status_cmd.c_str());
    std::cout << "--- bluetoothctl info " << device_address << " ---" << std::endl;
    std::cout << status_output << std::endl;
    std::cout << "------------------------------------" << std::endl;


    if (status_output.find("Connected: yes") != std::string::npos) {
        std::cout << "Successfully connected to " << device_address << " (according to bluetoothctl)." << std::endl;
        std::cout << "Wait for audio profiles to activate..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Crucial: Give more time for HFP/A2DP to register with PulseAudio/PipeWire
        return true;
    } else {
        std::cerr << "Failed to connect to " << device_address << "." << std::endl;
        std::cerr << "Please ensure device is on, discoverable/pairable, and try pairing/trusting manually via 'bluetoothctl' first if issues persist." << std::endl;
        return false;
    }
}

// Audio callback function - SDL will call this when it has new audio data
void audio_callback(void *userdata, Uint8 *stream, int len) {
    if (g_quit_flag.load() || !g_recognizer) {
        return;
    }

    if (len > 0) { // Process only if there's data
        if (vosk_recognizer_accept_waveform(g_recognizer, (const char*)stream, len)) {
            const char *result_json = vosk_recognizer_result(g_recognizer);
            std::string res_str(result_json);
            size_t text_pos = res_str.find("\"text\" : \"");
            if (text_pos != std::string::npos) {
                text_pos += 10; // Length of "\"text\" : \""
                size_t end_pos = res_str.find("\"", text_pos);
                if (end_pos != std::string::npos) {
                    std::string recognized = res_str.substr(text_pos, end_pos - text_pos);
                    if (!recognized.empty()) {
                        std::cout << "\nRecognized: " << recognized << std::endl;
                        std::cout << "Listening... " << std::flush; // Prompt again
                    }
                }
            }
        } else {
            const char *partial_json = vosk_recognizer_partial_result(g_recognizer);
            std::string partial_str(partial_json);
            size_t text_pos = partial_str.find("\"partial\" : \"");
            if (text_pos != std::string::npos) {
                text_pos += 13; // Length of "\"partial\" : \""
                size_t end_pos = partial_str.find("\"", text_pos);
                if (end_pos != std::string::npos) {
                    std::string recognized_partial = partial_str.substr(text_pos, end_pos - text_pos);
                    if (!recognized_partial.empty()) {
                        std::replace(recognized_partial.begin(), recognized_partial.end(), '\n', ' ');
                        std::cout << "\rPartial: " << recognized_partial << std::string(20, ' ') << std::flush; // Padding to clear previous
                    }
                }
            }
        }
    }
}

// Signal handler for Ctrl+C
void signal_handler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down..." << std::endl;
    g_quit_flag = true;
}

int main(int argc, char *argv[]) {
    const char* model_path_arg = nullptr;
    std::string target_ble_addr_arg = DEFAULT_TARGET_BLE_ADDRESS;

    if (argc == 2) {
        model_path_arg = argv[1];
        std::cout << "Using specified model path: " << model_path_arg << std::endl;
        std::cout << "Using default BLE address: " << target_ble_addr_arg << std::endl;
    } else if (argc == 3) {
        model_path_arg = argv[1];
        target_ble_addr_arg = argv[2];
        std::cout << "Using specified model path: " << model_path_arg << std::endl;
        std::cout << "Using specified BLE address: " << target_ble_addr_arg << std::endl;
    } else if (argc == 1) {
        model_path_arg = DEFAULT_MODEL_PATH;
        std::cout << "Using default model path: " << model_path_arg << std::endl;
        std::cout << "Using default BLE address: " << target_ble_addr_arg << std::endl;
    } else {
        std::cerr << "Usage: " << argv[0] << " [model_path] [target_ble_address]" << std::endl;
        return 1;
    }

    // --- Attempt to Connect to BLE Device ---
    if (!connect_ble_device(target_ble_addr_arg)) {
        std::cout << "Warning: Could not connect to BLE device " << target_ble_addr_arg
                  << ". Will attempt to use default system microphone." << std::endl;
        // Do not exit, let SDL try to use default mic
    }

    // --- Initialize Vosk ---
    std::cout << "Initializing Vosk..." << std::endl;
    vosk_set_log_level(0); // Enable some Vosk logging
    VoskModel *model = vosk_model_new(model_path_arg);
    if (!model) {
        std::cerr << "Failed to load model from: " << model_path_arg << std::endl;
        return 1;
    }
    std::cout << "Vosk model loaded successfully." << std::endl;

    g_recognizer = vosk_recognizer_new(model, SAMPLE_RATE);
    if (!g_recognizer) {
        std::cerr << "Failed to create recognizer" << std::endl;
        vosk_model_free(model);
        return 1;
    }
    std::cout << "Vosk recognizer created." << std::endl;
    vosk_recognizer_set_partial_words(g_recognizer, 1);

    // --- Initialize SDL Audio ---
    std::cout << "Initializing SDL Audio..." << std::endl;
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        vosk_recognizer_free(g_recognizer);
        vosk_model_free(model);
        return 1;
    }

    SDL_AudioSpec desired_spec, obtained_spec;
    SDL_zero(desired_spec);
    desired_spec.freq = (int)SAMPLE_RATE;
    desired_spec.format = AUDIO_FORMAT;
    desired_spec.channels = CHANNELS;
    desired_spec.samples = SAMPLES_PER_CALLBACK;
    desired_spec.callback = audio_callback;
    desired_spec.userdata = nullptr;

    // --- List available audio devices and try to select the headset ---
    std::cout << "Available audio capture devices:" << std::endl;
    const char* selected_device_name_sdl = nullptr;
    int num_capture_devices = SDL_GetNumAudioDevices(1); // 1 for capture devices
    if (num_capture_devices < 1) {
        std::cerr << "No audio capture devices found by SDL." << std::endl;
    }

    for (int i = 0; i < num_capture_devices; ++i) {
        const char* current_device_name = SDL_GetAudioDeviceName(i, 1);
        std::cout << "  Device " << i << ": " << (current_device_name ? current_device_name : "Unnamed Device") << std::endl;
        if (current_device_name && strstr(current_device_name, HEADSET_NAME_HINT) != NULL) {
            selected_device_name_sdl = current_device_name;
            std::cout << "    -> Found potential headset: " << selected_device_name_sdl << std::endl;
        }
    }

    if (selected_device_name_sdl) {
        std::cout << "Attempting to open specific audio device: " << selected_device_name_sdl << std::endl;
    } else {
        std::cout << "Headset '" << HEADSET_NAME_HINT << "' not found in SDL list by name. Attempting to open default audio device." << std::endl;
        // selected_device_name_sdl will be NULL, so SDL_OpenAudioDevice opens default
    }

    g_dev_id = SDL_OpenAudioDevice(selected_device_name_sdl, 1, &desired_spec, &obtained_spec, 0);
    if (g_dev_id == 0) {
        std::cerr << "SDL_OpenAudioDevice failed for '"
                  << (selected_device_name_sdl ? selected_device_name_sdl : "Default Device")
                  << "': " << SDL_GetError() << std::endl;
        SDL_Quit();
        vosk_recognizer_free(g_recognizer);
        vosk_model_free(model);
        return 1;
    }

    std::cout << "SDL Audio initialized successfully." << std::endl;
    if (selected_device_name_sdl) {
        std::cout << "Attempted to open specific device: " << selected_device_name_sdl << " -> SDL reports success (Device ID: " << g_dev_id << ")" << std::endl;
    } else {
        std::cout << "Opened default audio device -> SDL reports success (Device ID: " << g_dev_id << ")" << std::endl;
        if (num_capture_devices > 0) {
            const char* default_name_check = SDL_GetAudioDeviceName(0,1); // Name of the first capture device
            std::cout << "    (SDL's first listed capture device is: " << (default_name_check ? default_name_check : "Unknown") << ")" << std::endl;
        }
    }
    std::cout << "Obtained audio spec: Freq=" << obtained_spec.freq
              << " Format=" << obtained_spec.format // You might want to map this int to a string
              << " Channels=" << (int)obtained_spec.channels
              << " Samples per callback=" << obtained_spec.samples << std::endl;


    // --- Start Audio Capture ---
    SDL_PauseAudioDevice(g_dev_id, 0); // 0 to unpause (start audio)
    std::cout << "Listening... (Ctrl+C to exit)" << std::endl;

    // --- Register Signal Handler for Ctrl+C ---
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // --- Main Loop ---
    while (!g_quit_flag.load()) {
        SDL_Delay(100);
    }

    // --- Stop Audio and Clean Up ---
    std::cout << "\nStopping audio capture..." << std::endl;
    if (g_dev_id > 0) { // Check if device was successfully opened
        SDL_PauseAudioDevice(g_dev_id, 1); // 1 to pause
        SDL_CloseAudioDevice(g_dev_id);
    }
    SDL_Quit();
    std::cout << "SDL Audio stopped and cleaned up." << std::endl;

    // Get final result just in case there's pending audio
    // Check if g_recognizer is still valid before using it.
    if (g_recognizer) {
        const char *final_result = vosk_recognizer_final_result(g_recognizer);
        if (final_result) {
            std::string final_res_str(final_result);
             if (final_res_str.length() > 15 && final_res_str.find("\"text\" : \"\"") == std::string::npos) { // Avoid empty text results
                std::cout << "Final pending result: " << final_result << std::endl;
            }
        }
    }

    std::cout << "Cleaning up Vosk resources..." << std::endl;
    if(g_recognizer) vosk_recognizer_free(g_recognizer);
    if(model) vosk_model_free(model);
    std::cout << "Cleanup complete. Exiting." << std::endl;

    return 0;
}

// #include <iostream>
// #include <vector>
// #include <string>
// #include <vosk_api.h>
// #include <SDL2/SDL.h>
// #include <atomic>
// #include <csignal>
// #include <cstdio>
// #include <array>
// #include <thread>
// #include <chrono>
// #include <algorithm>
// #include <cstring>

// #define DEFAULT_MODEL_PATH "vosk-model-small-en-us-0.15"
// #define DEFAULT_TARGET_BLE_ADDRESS "2C:BE:EB:C5:B1:41"
// #define HEADSET_NAME_HINT "CMF Neckband Pro"

// #define SAMPLE_RATE 16000.0f
// #define AUDIO_FORMAT AUDIO_S16LSB
// #define CHANNELS 1
// #define SAMPLES_PER_CALLBACK 4096

// std::atomic_bool g_quit_flag(false);
// VoskRecognizer *g_recognizer = nullptr;
// SDL_AudioDeviceID g_dev_id = 0;

// // Utility function to execute shell commands and get output
// std::string exec_shell_command(const char* cmd) {
//     std::array<char, 256> buffer;
//     std::string result;
//     std::string cmd_with_stderr = std::string(cmd) + " 2>&1";
//     std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd_with_stderr.c_str(), "r"), pclose);
//     if (!pipe) {
//         std::cerr << "popen() failed for command: " << cmd_with_stderr << std::endl;
//         return "ERROR_POPEN";
//     }
//     while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
//         result += buffer.data();
//     }
//     return result;
// }

// // BLE connection logic
// bool connect_ble_device(const std::string& device_address) {
//     std::cout << "Attempting to connect to BLE device: " << device_address << std::endl;

//     // Ensure Bluetooth is powered on
//     exec_shell_command("bluetoothctl power on");
//     exec_shell_command("bluetoothctl agent on");
//     exec_shell_command("bluetoothctl default-agent");
//     std::this_thread::sleep_for(std::chrono::seconds(2));

//     // Check initial connection status
//     std::string connect_status_cmd = "bluetoothctl info " + device_address;
//     std::string status_output = exec_shell_command(connect_status_cmd.c_str());
//     if (status_output.find("Connected: yes") != std::string::npos) {
//         std::cout << "Device " << device_address << " is already connected." << std::endl;
//         std::this_thread::sleep_for(std::chrono::seconds(2));
//         return true;
//     }
//     if (status_output.find("No default controller available") != std::string::npos) {
//         std::cerr << "No Bluetooth controller available. Check if Bluetooth is enabled." << std::endl;
//         return false;
//     }

//     // Trust and pair the device
//     exec_shell_command(("bluetoothctl trust " + device_address).c_str());
//     exec_shell_command(("bluetoothctl pair " + device_address).c_str());
//     std::this_thread::sleep_for(std::chrono::seconds(3));

//     // Connect using bluetoothctl
//     FILE *bt_pipe = popen("bluetoothctl", "w");
//     if (!bt_pipe) {
//         std::cerr << "Failed to open pipe to bluetoothctl" << std::endl;
//         return false;
//     }
//     fprintf(bt_pipe, "connect %s\n", device_address.c_str());
//     fprintf(bt_pipe, "exit\n");
//     fflush(bt_pipe);
//     std::cout << "Connection command sent. Waiting for connection..." << std::endl;
//     std::this_thread::sleep_for(std::chrono::seconds(15));
//     pclose(bt_pipe);

//     // Verify connection
//     status_output = exec_shell_command(connect_status_cmd.c_str());
//     std::cout << "--- bluetoothctl info " << device_address << " ---\n" << status_output << "\n------------------------------------\n";
//     if (status_output.find("Connected: yes") == std::string::npos) {
//         std::cerr << "Failed to connect to " << device_address << ". Ensure device is on and pairable." << std::endl;
//         return false;
//     }

//     // Ensure HFP profile is active
//     std::string card_name = "bluez_card." + device_address;
//     std::replace(card_name.begin(), card_name.end(), ':', '_');
//     exec_shell_command(("pactl set-card-profile " + card_name + " headset_head_unit").c_str());
//     std::this_thread::sleep_for(std::chrono::seconds(5));
//     std::cout << "Successfully connected to " << device_address << std::endl;
//     return true;
// }

// // Find Bluetooth audio device
// std::string find_bluetooth_audio_device(const std::string& headset_hint, const std::string& ble_address) {
//     std::cout << "Scanning for audio capture devices..." << std::endl;
//     int num_devices = SDL_GetNumAudioDevices(1);
//     if (num_devices < 1) {
//         std::cerr << "No audio capture devices found by SDL." << std::endl;
//         return "";
//     }

//     std::string ble_address_clean = ble_address;
//     std::replace(ble_address_clean.begin(), ble_address_clean.end(), ':', '_');
//     std::string target_source = "bluez_source." + ble_address_clean + ".handsfree_head_unit";

//     for (int i = 0; i < num_devices; ++i) {
//         const char* device_name = SDL_GetAudioDeviceName(i, 1);
//         if (!device_name) continue;
//         std::string name(device_name);
//         std::cout << "Device " << i << ": " << name << std::endl;

//         // Check for Bluetooth device
//         if (name.find(headset_hint) != std::string::npos ||
//             name.find("bluez") != std::string::npos ||
//             name.find(ble_address_clean) != std::string::npos ||
//             name.find("Headset") != std::string::npos ||
//             name.find("handsfree") != std::string::npos) {
//             std::cout << "Selected potential Bluetooth audio device: " << name << std::endl;
//             return name;
//         }
//     }

//     // Try to set Bluetooth device as default source
//     exec_shell_command(("pactl set-default-source " + target_source).c_str());
//     std::this_thread::sleep_for(std::chrono::seconds(2));

//     // Re-check devices after setting default
//     for (int i = 0; i < num_devices; ++i) {
//         const char* device_name = SDL_GetAudioDeviceName(i, 1);
//         if (!device_name) continue;
//         std::string name(device_name);
//         std::cout << "Device " << i << " (post-default): " << name << std::endl;
//         if (name.find(target_source) != std::string::npos) {
//             std::cout << "Selected Bluetooth audio device after setting default: " << name << std::endl;
//             return name;
//         }
//     }

//     std::cerr << "No Bluetooth audio device found matching '" << headset_hint << "' or '" << ble_address << "'." << std::endl;
//     return "";
// }

// // Audio callback function
// void audio_callback(void *userdata, Uint8 *stream, int len) {
//     if (g_quit_flag.load() || !g_recognizer || len <= 0) return;

//     if (vosk_recognizer_accept_waveform(g_recognizer, (const char*)stream, len)) {
//         const char *result_json = vosk_recognizer_result(g_recognizer);
//         std::string res_str(result_json);
//         size_t text_pos = res_str.find("\"text\" : \"");
//         if (text_pos != std::string::npos) {
//             text_pos += 10;
//             size_t end_pos = res_str.find("\"", text_pos);
//             if (end_pos != std::string::npos) {
//                 std::string recognized = res_str.substr(text_pos, end_pos - text_pos);
//                 if (!recognized.empty()) {
//                     std::cout << "\nRecognized: " << recognized << std::endl;
//                     std::cout << "Listening... " << std::flush;
//                 }
//             }
//         }
//     } else {
//         const char *partial_json = vosk_recognizer_partial_result(g_recognizer);
//         std::string partial_str(partial_json);
//         size_t text_pos = partial_str.find("\"partial\" : \"");
//         if (text_pos != std::string::npos) {
//             text_pos += 13;
//             size_t end_pos = partial_str.find("\"", text_pos);
//             if (end_pos != std::string::npos) {
//                 std::string recognized_partial = partial_str.substr(text_pos, end_pos - text_pos);
//                 if (!recognized_partial.empty()) {
//                     std::replace(recognized_partial.begin(), recognized_partial.end(), '\n', ' ');
//                     std::cout << "\rPartial: " << recognized_partial << std::string(20, ' ') << std::flush;
//                 }
//             }
//         }
//     }
// }

// // Signal handler
// void signal_handler(int signum) {
//     std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down..." << std::endl;
//     g_quit_flag = true;
// }

// int main(int argc, char *argv[]) {
//     const char* model_path = DEFAULT_MODEL_PATH;
//     std::string target_ble_addr = DEFAULT_TARGET_BLE_ADDRESS;

//     if (argc == 2) {
//         model_path = argv[1];
//     } else if (argc == 3) {
//         model_path = argv[1];
//         target_ble_addr = argv[2];
//     } else if (argc > 3) {
//         std::cerr << "Usage: " << argv[0] << " [model_path] [target_ble_address]" << std::endl;
//         return 1;
//     }

//     std::cout << "Using model path: " << model_path << std::endl;
//     std::cout << "Using BLE address: " << target_ble_addr << std::endl;

//     // Connect to BLE device
//     if (!connect_ble_device(target_ble_addr)) {
//         std::cout << "Warning: Could not connect to BLE device. Falling back to default microphone." << std::endl;
//     } else {
//         std::cout << "Please ensure '" << HEADSET_NAME_HINT << "' is set as the default audio input in 'pavucontrol' or system settings." << std::endl;
//     }

//     // Initialize Vosk
//     std::cout << "Initializing Vosk..." << std::endl;
//     vosk_set_log_level(0);
//     VoskModel *model = vosk_model_new(model_path);
//     if (!model) {
//         std::cerr << "Failed to load model from: " << model_path << std::endl;
//         return 1;
//     }
//     g_recognizer = vosk_recognizer_new(model, SAMPLE_RATE);
//     if (!g_recognizer) {
//         std::cerr << "Failed to create recognizer" << std::endl;
//         vosk_model_free(model);
//         return 1;
//     }
//     vosk_recognizer_set_partial_words(g_recognizer, 1);
//     std::cout << "Vosk initialized." << std::endl;

//     // Initialize SDL Audio
//     std::cout << "Initializing SDL Audio..." << std::endl;
//     if (SDL_Init(SDL_INIT_AUDIO) < 0) {
//         std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
//         vosk_recognizer_free(g_recognizer);
//         vosk_model_free(model);
//         return 1;
//     }

//     // Find Bluetooth audio device
//     std::string selected_device_name = find_bluetooth_audio_device(HEADSET_NAME_HINT, target_ble_addr);

//     SDL_AudioSpec desired_spec, obtained_spec;
//     SDL_zero(desired_spec);
//     desired_spec.freq = (int)SAMPLE_RATE;
//     desired_spec.format = AUDIO_FORMAT;
//     desired_spec.channels = CHANNELS;
//     desired_spec.samples = SAMPLES_PER_CALLBACK;
//     desired_spec.callback = audio_callback;
//     desired_spec.userdata = nullptr;

//     // Open audio device
//     g_dev_id = SDL_OpenAudioDevice(selected_device_name.empty() ? nullptr : selected_device_name.c_str(), 1, &desired_spec, &obtained_spec, 0);
//     if (g_dev_id == 0) {
//         std::cerr << "SDL_OpenAudioDevice failed for '" << (selected_device_name.empty() ? "Default Device" : selected_device_name) << "': " << SDL_GetError() << std::endl;
//         SDL_Quit();
//         vosk_recognizer_free(g_recognizer);
//         vosk_model_free(model);
//         return 1;
//     }

//     std::cout << "SDL Audio initialized. Using device: " << (selected_device_name.empty() ? "Default" : selected_device_name) << " (ID: " << g_dev_id << ")" << std::endl;
//     std::cout << "Obtained audio spec: Freq=" << obtained_spec.freq
//               << " Format=" << obtained_spec.format
//               << " Channels=" << (int)obtained_spec.channels
//               << " Samples=" << obtained_spec.samples << std::endl;

//     // Start audio capture
//     SDL_PauseAudioDevice(g_dev_id, 0);
//     std::cout << "Listening... (Ctrl+C to exit)" << std::endl;

//     // Register signal handler
//     signal(SIGINT, signal_handler);
//     signal(SIGTERM, signal_handler);

//     // Main loop
//     while (!g_quit_flag.load()) {
//         SDL_Delay(100);
//     }

//     // Cleanup
//     std::cout << "\nStopping audio capture..." << std::endl;
//     if (g_dev_id > 0) {
//         SDL_PauseAudioDevice(g_dev_id, 1);
//         SDL_CloseAudioDevice(g_dev_id);
//     }
//     SDL_Quit();

//     if (g_recognizer) {
//         const char *final_result = vosk_recognizer_final_result(g_recognizer);
//         if (final_result && strlen(final_result) > 15 && strstr(final_result, "\"text\" : \"\"") == nullptr) {
//             std::cout << "Final pending result: " << final_result << std::endl;
//         }
//         vosk_recognizer_free(g_recognizer);
//     }
//     if (model) {
//         vosk_model_free(model);
//     }

//     // Disconnect Bluetooth device
//     std::cout << "Disconnecting Bluetooth device..." << std::endl;
//     exec_shell_command(("bluetoothctl disconnect " + target_ble_addr).c_str());
//     std::cout << "Cleanup complete. Exiting." << std::endl;

//     return 0;
// }