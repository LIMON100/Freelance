// #include <iostream>
// #include <fstream>
// #include <vector>
// #include <vosk_api.h>

// int main(int argc, char *argv[]) {
//     // Check for correct number of arguments
//     if (argc != 3) {
//         std::cerr << "Usage: " << argv[0] << " <model_path> <wav_file>" << std::endl;
//         return 1;
//     }

//     // Initialize Vosk
//     VoskModel *model = vosk_model_new(argv[1]);
//     if (!model) {
//         std::cerr << "Failed to load model from: " << argv[1] << std::endl;
//         return 1;
//     }

//     // Initialize recognizer (16000 Hz sample rate)
//     VoskRecognizer *recognizer = vosk_recognizer_new(model, 16000.0);
//     if (!recognizer) {
//         std::cerr << "Failed to create recognizer" << std::endl;
//         vosk_model_free(model);
//         return 1;
//     }

//     // Open WAV file
//     std::ifstream wav_file(argv[2], std::ios::binary);
//     if (!wav_file.is_open()) {
//         std::cerr << "Failed to open WAV file: " << argv[2] << std::endl;
//         vosk_recognizer_free(recognizer);
//         vosk_model_free(model);
//         return 1;
//     }

//     // Skip WAV header (44 bytes for standard WAV)
//     wav_file.seekg(44);

//     // Read and process audio in chunks
//     std::vector<char> buffer(4000);
//     while (!wav_file.eof()) {
//         wav_file.read(buffer.data(), buffer.size());
//         std::streamsize bytes_read = wav_file.gcount();
//         if (bytes_read > 0) {
//             if (vosk_recognizer_accept_waveform(recognizer, buffer.data(), bytes_read)) {
//                 // Full result when silence is detected
//                 const char *result = vosk_recognizer_result(recognizer);
//                 std::cout << "Result: " << result << std::endl;
//             } else {
//                 // Partial result
//                 const char *partial = vosk_recognizer_partial_result(recognizer);
//                 std::cout << "Partial: " << partial << std::endl;
//             }
//         }
//     }

//     // Get final result
//     const char *final_result = vosk_recognizer_final_result(recognizer);
//     std::cout << "Final Result: " << final_result << std::endl;

//     // Clean up
//     wav_file.close();
//     vosk_recognizer_free(recognizer);
//     vosk_model_free(model);

//     return 0;
// }


#include <iostream>
#include <vector>
#include <string>
#include <vosk_api.h>
#include <SDL2/SDL.h> // For live audio capture
#include <atomic>    // For std::atomic_bool
#include <csignal>   // For signal handling (Ctrl+C)

#define MODEL_PATH "vosk-model-small-en-us-0.15" // MODIFY: Path to your Vosk model
#define SAMPLE_RATE 16000.0f
#define AUDIO_FORMAT AUDIO_S16LSB // 16-bit signed little-endian (common for mics)
#define CHANNELS 1                // Mono
#define SAMPLES_PER_CALLBACK 4096 // Number of samples per SDL callback (adjust for desired chunk size)
                                  // For 16kHz, 16-bit mono: 4096 samples = 8192 bytes
                                  // This gives chunks of 4096/16000 = 0.256 seconds

// Global flag to signal termination (for Ctrl+C)
std::atomic_bool g_quit_flag(false);

// Vosk recognizer - make it accessible to the callback
VoskRecognizer *g_recognizer = nullptr;

// Audio callback function - SDL will call this when it has new audio data
void audio_callback(void *userdata, Uint8 *stream, int len) {
    if (g_quit_flag || !g_recognizer) {
        return;
    }

    // Feed the raw audio bytes directly to Vosk
    // vosk_recognizer_accept_waveform expects char* for data and int for length
    if (vosk_recognizer_accept_waveform(g_recognizer, (const char*)stream, len)) {
        // Full result (silence detected or end of utterance)
        const char *result = vosk_recognizer_result(g_recognizer);
        std::cout << "\nResult: " << result << std::endl;
        std::cout << "Listening... (Ctrl+C to exit)" << std::endl; // Prompt again
    } else {
        // Partial result
        // const char *partial = vosk_recognizer_partial_result(g_recognizer);
        // std::cout << "\rPartial: " << partial << std::flush; // Use \r and flush for real-time update
    }
}

// Signal handler for Ctrl+C
void signal_handler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received." << std::endl;
    g_quit_flag = true;
}

int main(int argc, char *argv[]) {
    // --- Argument Parsing (Simplified for this example) ---
    const char* model_path;
    if (argc == 2) {
        model_path = argv[1];
    } else if (argc == 1) {
        model_path = MODEL_PATH; // Use default if no path provided
        std::cout << "Using default model path: " << MODEL_PATH << std::endl;
        std::cout << "You can specify a model path as an argument: " << argv[0] << " <model_path>" << std::endl;
    }
     else {
        std::cerr << "Usage: " << argv[0] << " [model_path]" << std::endl;
        return 1;
    }

    // --- Initialize Vosk ---
    std::cout << "Initializing Vosk..." << std::endl;
    VoskModel *model = vosk_model_new(model_path);
    if (!model) {
        std::cerr << "Failed to load model from: " << model_path << std::endl;
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
    vosk_recognizer_set_partial_words(g_recognizer, 1); // Enable partial results

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
    desired_spec.samples = SAMPLES_PER_CALLBACK; // Size of buffer in samples
    desired_spec.callback = audio_callback;
    desired_spec.userdata = nullptr; // Not used in this simple example

    // Open the default audio capture device
    SDL_AudioDeviceID dev_id = SDL_OpenAudioDevice(NULL, 1, &desired_spec, &obtained_spec, 0);
    if (dev_id == 0) {
        std::cerr << "SDL_OpenAudioDevice failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        vosk_recognizer_free(g_recognizer);
        vosk_model_free(model);
        return 1;
    }

    std::cout << "SDL Audio initialized." << std::endl;
    std::cout << "Obtained audio spec: Freq=" << obtained_spec.freq
              << " Format=" << obtained_spec.format
              << " Channels=" << (int)obtained_spec.channels
              << " Samples=" << obtained_spec.samples << std::endl;

    // --- Start Audio Capture ---
    SDL_PauseAudioDevice(dev_id, 0); // 0 to unpause (start audio)
    std::cout << "Listening... (Ctrl+C to exit)" << std::endl;

    // --- Register Signal Handler for Ctrl+C ---
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);


    // --- Main Loop (Keep program running while listening) ---
    while (!g_quit_flag) {
        SDL_Delay(100); // Sleep for a short duration to reduce CPU usage
                        // The actual audio processing happens in the callback
    }

    // --- Stop Audio and Clean Up ---
    std::cout << "\nStopping audio capture..." << std::endl;
    SDL_PauseAudioDevice(dev_id, 1); // 1 to pause
    SDL_CloseAudioDevice(dev_id);
    SDL_Quit();
    std::cout << "SDL Audio stopped and cleaned up." << std::endl;

    // Get final result just in case there's pending audio
    const char *final_result = vosk_recognizer_final_result(g_recognizer);
    if (final_result && strlen(final_result) > 0) { // Check if not null and not empty JSON
        std::cout << "Final pending result: " << final_result << std::endl;
    }


    std::cout << "Cleaning up Vosk resources..." << std::endl;
    vosk_recognizer_free(g_recognizer);
    vosk_model_free(model);
    std::cout << "Cleanup complete. Exiting." << std::endl;

    return 0;
}