#include <iostream>
#include <vector>
#include <string>
#include <vosk_api.h>
#include <alsa/asoundlib.h> // ALSA header
#include <atomic>
#include <csignal>
#include <cstdio>
#include <array>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cstring>

#define DEFAULT_MODEL_PATH "vosk-model-small-en-us-0.15"
#define DEFAULT_TARGET_BLE_ADDRESS "2C:BE:EB:C5:B1:41"
#define ALSA_PCM_DEVICE_HINT "plughw:Headset" // Try "default", "plughw:0,0", or find your headset's ALSA name
                                           // You can try "plughw:CARD=Headset,DEV=0" if you know card name
                                           // Use `arecord -L` to list devices

#define SAMPLE_RATE 16000
#define ALSA_FORMAT SND_PCM_FORMAT_S16_LE // 16-bit signed little-endian
#define CHANNELS 1
#define FRAMES_PER_PERIOD 2048 // Number of frames per ALSA period (chunk size)
                               // For 16kHz, 16-bit mono: 2048 frames = 4096 bytes
                               // This gives chunks of 2048/16000 = 0.128 seconds

std::atomic_bool g_quit_flag(false);
VoskRecognizer *g_recognizer = nullptr;
snd_pcm_t *g_alsa_handle = nullptr; // ALSA PCM handle
std::thread g_audio_thread;

// --- BLE Connection Logic (same as your previous version) ---
std::string exec_shell_command(const char* cmd) {
    // ... (same implementation) ...
    std::array<char, 256> buffer; 
    std::string result;
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

bool connect_ble_device(const std::string& device_address) {
    // ... (same implementation, ensure it gives enough time for audio profiles to settle) ...
    std::cout << "Attempting to connect to BLE device: " << device_address << std::endl;
    exec_shell_command("bluetoothctl power on");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::string connect_status_cmd = "bluetoothctl info " + device_address;
    std::string status_output = exec_shell_command(connect_status_cmd.c_str());
    if (status_output.find("Connected: yes") != std::string::npos) {
        std::cout << "Device " << device_address << " is already connected." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5)); // More time for audio
        return true;
    }
    // ... rest of your connect_ble_device logic ...
    // Ensure you have a good delay at the end if connection is successful:
    // std::this_thread::sleep_for(std::chrono::seconds(5)); // Crucial
    FILE *bt_pipe = popen("bluetoothctl", "w");
    if (!bt_pipe) {
        std::cerr << "Failed to open pipe to bluetoothctl" << std::endl;
        return false;
    }
    fprintf(bt_pipe, "connect %s\n", device_address.c_str());
    fflush(bt_pipe);
    std::cout << "Connection command sent to bluetoothctl. Waiting for connection (approx 10-20 seconds)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(15));
    fprintf(bt_pipe, "exit\n");
    pclose(bt_pipe);
    std::cout << "Checking connection status after attempt..." << std::endl;
    status_output = exec_shell_command(connect_status_cmd.c_str());
    std::cout << "--- bluetoothctl info " << device_address << " ---" << std::endl;
    std::cout << status_output << std::endl;
    std::cout << "------------------------------------" << std::endl;
    if (status_output.find("Connected: yes") != std::string::npos) {
        std::cout << "Successfully connected to " << device_address << " (according to bluetoothctl)." << std::endl;
        std::cout << "Wait for audio profiles to activate..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5)); 
        return true;
    } else {
        std::cerr << "Failed to connect to " << device_address << "." << std::endl;
        return false;
    }
}


// --- ALSA Audio Capture Thread Function ---
void audio_capture_thread_func() {
    int err;
    std::vector<char> buffer(FRAMES_PER_PERIOD * CHANNELS * 2); // 2 bytes per sample for S16_LE

    std::cout << "[Audio Thread] Started." << std::endl;

    while (!g_quit_flag.load()) {
        if (!g_alsa_handle || !g_recognizer) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Read PCM data from ALSA
        snd_pcm_sframes_t frames_read = snd_pcm_readi(g_alsa_handle, buffer.data(), FRAMES_PER_PERIOD);

        if (frames_read < 0) {
            // Attempt to recover from ALSA errors (e.g., overrun)
            frames_read = snd_pcm_recover(g_alsa_handle, frames_read, 0);
        }
        if (frames_read < 0) {
            std::cerr << "[Audio Thread] ALSA read error: " << snd_strerror(frames_read) << std::endl;
            // Potentially signal main thread to stop or try to reinitialize ALSA
            g_quit_flag = true; // Simple error handling: quit
            break;
        }

        if (frames_read > 0) {
            // Data read is in frames, Vosk wants bytes
            int bytes_read = frames_read * CHANNELS * 2; // 2 bytes for S16_LE
            if (vosk_recognizer_accept_waveform(g_recognizer, buffer.data(), bytes_read)) {
                const char *result_json = vosk_recognizer_result(g_recognizer);
                // ... (your JSON parsing and printing logic from previous example) ...
                std::string res_str(result_json);
                size_t text_pos = res_str.find("\"text\" : \"");
                if (text_pos != std::string::npos) {
                    text_pos += 10; 
                    size_t end_pos = res_str.find("\"", text_pos);
                    if (end_pos != std::string::npos) {
                        std::string recognized = res_str.substr(text_pos, end_pos - text_pos);
                        if (!recognized.empty()) {
                            std::cout << "\nRecognized: " << recognized << std::endl;
                            std::cout << "Listening... " << std::flush;
                        }
                    }
                }
            } else {
                const char *partial_json = vosk_recognizer_partial_result(g_recognizer);
                // ... (your JSON parsing and printing logic for partial results) ...
                std::string partial_str(partial_json);
                size_t text_pos = partial_str.find("\"partial\" : \"");
                if (text_pos != std::string::npos) {
                    text_pos += 13; 
                    size_t end_pos = partial_str.find("\"", text_pos);
                    if (end_pos != std::string::npos) {
                        std::string recognized_partial = partial_str.substr(text_pos, end_pos - text_pos);
                        if (!recognized_partial.empty()) {
                            std::replace(recognized_partial.begin(), recognized_partial.end(), '\n', ' ');
                            std::cout << "\rPartial: " << recognized_partial << std::string(20, ' ') << std::flush; 
                        }
                    }
                }
            }
        }
    }
    std::cout << "[Audio Thread] Exiting." << std::endl;
}

// Signal handler for Ctrl+C
void signal_handler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down..." << std::endl;
    g_quit_flag = true;
}

// --- ALSA Initialization ---
bool init_alsa(const char* pcm_device_name) {
    int err;
    unsigned int actual_sample_rate = SAMPLE_RATE;
    int dir; // dummy for direction

    // Open PCM device for recording (capture).
    if ((err = snd_pcm_open(&g_alsa_handle, pcm_device_name, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        std::cerr << "Cannot open ALSA audio device " << pcm_device_name << ": " << snd_strerror(err) << std::endl;
        return false;
    }
    std::cout << "ALSA device " << pcm_device_name << " opened." << std::endl;

    // Allocate parameters object
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params); // Allocate on stack

    // Fill it with default values
    if ((err = snd_pcm_hw_params_any(g_alsa_handle, hw_params)) < 0) {
        std::cerr << "Cannot initialize ALSA hardware parameter structure: " << snd_strerror(err) << std::endl;
        snd_pcm_close(g_alsa_handle); g_alsa_handle = nullptr;
        return false;
    }

    // Set desired hardware parameters.
    // Interleaved mode
    if ((err = snd_pcm_hw_params_set_access(g_alsa_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        std::cerr << "Cannot set ALSA access type: " << snd_strerror(err) << std::endl;
        snd_pcm_close(g_alsa_handle); g_alsa_handle = nullptr;
        return false;
    }

    // Signed 16-bit little-endian format
    if ((err = snd_pcm_hw_params_set_format(g_alsa_handle, hw_params, ALSA_FORMAT)) < 0) {
        std::cerr << "Cannot set ALSA sample format: " << snd_strerror(err) << std::endl;
        snd_pcm_close(g_alsa_handle); g_alsa_handle = nullptr;
        return false;
    }

    // Set sample rate
    if ((err = snd_pcm_hw_params_set_rate_near(g_alsa_handle, hw_params, &actual_sample_rate, &dir)) < 0) {
        std::cerr << "Cannot set ALSA sample rate: " << snd_strerror(err) << std::endl;
        snd_pcm_close(g_alsa_handle); g_alsa_handle = nullptr;
        return false;
    }
    if (actual_sample_rate != SAMPLE_RATE) {
        std::cerr << "Warning: ALSA sample rate " << actual_sample_rate << " does not match desired " << SAMPLE_RATE << std::endl;
    }

    // Set channel count
    if ((err = snd_pcm_hw_params_set_channels(g_alsa_handle, hw_params, CHANNELS)) < 0) {
        std::cerr << "Cannot set ALSA channel count: " << snd_strerror(err) << std::endl;
        snd_pcm_close(g_alsa_handle); g_alsa_handle = nullptr;
        return false;
    }

    // Set period size
    snd_pcm_uframes_t period_size = FRAMES_PER_PERIOD;
    if ((err = snd_pcm_hw_params_set_period_size_near(g_alsa_handle, hw_params, &period_size, &dir)) < 0) {
        std::cerr << "Cannot set ALSA period size: " << snd_strerror(err) << std::endl;
        snd_pcm_close(g_alsa_handle); g_alsa_handle = nullptr;
        return false;
    }
    std::cout << "ALSA Period size set to " << period_size << " frames." << std::endl;


    // Write parameters to the driver
    if ((err = snd_pcm_hw_params(g_alsa_handle, hw_params)) < 0) {
        std::cerr << "Cannot set ALSA hardware parameters: " << snd_strerror(err) << std::endl;
        snd_pcm_close(g_alsa_handle); g_alsa_handle = nullptr;
        return false;
    }

    // Prepare the ALSA interface for use
    if ((err = snd_pcm_prepare(g_alsa_handle)) < 0) {
        std::cerr << "Cannot prepare ALSA audio interface for use: " << snd_strerror(err) << std::endl;
        snd_pcm_close(g_alsa_handle); g_alsa_handle = nullptr;
        return false;
    }
    std::cout << "ALSA initialized successfully." << std::endl;
    return true;
}


int main(int argc, char *argv[]) {
    // ... (Argument parsing logic from your previous main, simplified) ...
    const char* model_path_arg = DEFAULT_MODEL_PATH;
    std::string target_ble_addr_arg = DEFAULT_TARGET_BLE_ADDRESS;
    std::string alsa_device_name = ALSA_PCM_DEVICE_HINT; // Default ALSA device

    if (argc >= 2) model_path_arg = argv[1];
    if (argc >= 3) target_ble_addr_arg = argv[2];
    if (argc >= 4) alsa_device_name = argv[3]; // Allow specifying ALSA device

    std::cout << "Using Model: " << model_path_arg << std::endl;
    std::cout << "Using BLE Address: " << target_ble_addr_arg << std::endl;
    std::cout << "Using ALSA Device: " << alsa_device_name << std::endl;


    // --- Attempt to Connect to BLE Device ---
    if (!connect_ble_device(target_ble_addr_arg)) {
        std::cout << "Warning: Could not connect to BLE device " << target_ble_addr_arg
                  << ". Will attempt to use ALSA device: " << alsa_device_name << std::endl;
    } else {
        std::cout << "BLE device connected. Attempting to use it as audio source via ALSA." << std::endl;
        // After successful BLE connection, the headset should appear as an ALSA device.
        // The ALSA_PCM_DEVICE_HINT might need to be general "default" or specific
        // to how PulseAudio/PipeWire names Bluetooth headsets (e.g., bluealsa devices).
        // It's often better to let the user select or use 'default' and configure default in system.
    }

    // --- Initialize Vosk ---
    std::cout << "Initializing Vosk..." << std::endl;
    vosk_set_log_level(0);
    VoskModel *model = vosk_model_new(model_path_arg);
    if (!model) { /* ... error ... */ return 1; }
    g_recognizer = vosk_recognizer_new(model, SAMPLE_RATE);
    if (!g_recognizer) { /* ... error ... */ vosk_model_free(model); return 1; }
    vosk_recognizer_set_partial_words(g_recognizer, 1);
    std::cout << "Vosk initialized." << std::endl;

    // --- Initialize ALSA Audio ---
    if (!init_alsa(alsa_device_name.c_str())) {
        vosk_recognizer_free(g_recognizer);
        vosk_model_free(model);
        return 1;
    }

    // --- Register Signal Handler ---
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // --- Start Audio Capture Thread ---
    std::cout << "Starting audio capture thread..." << std::endl;
    g_audio_thread = std::thread(audio_capture_thread_func);
    std::cout << "Listening... (Ctrl+C to exit)" << std::endl;

    // --- Main Loop (Wait for quit signal) ---
    while (!g_quit_flag.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // --- Stop Audio and Clean Up ---
    std::cout << "\nSignaling audio thread to stop..." << std::endl;
    if (g_audio_thread.joinable()) {
        g_audio_thread.join(); // Wait for audio thread to finish
    }
    std::cout << "Audio thread joined." << std::endl;

    if (g_alsa_handle) {
        snd_pcm_drain(g_alsa_handle); // Wait for all pending frames to be transmitted
        snd_pcm_close(g_alsa_handle);
        g_alsa_handle = nullptr;
        std::cout << "ALSA device closed." << std::endl;
    }

    if (g_recognizer) {
        const char *final_result = vosk_recognizer_final_result(g_recognizer);
        if (final_result && strlen(final_result) > 15 ) {
            std::cout << "Final pending result: " << final_result << std::endl;
        }
        vosk_recognizer_free(g_recognizer);
        g_recognizer = nullptr;
    }
    if (model) vosk_model_free(model);
    std::cout << "Vosk resources freed. Exiting." << std::endl;

    return 0;
}