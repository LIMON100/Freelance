// gemini
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wav2vec2.h"
#include "audio_utils.h"
#include "easy_timer.h"
#include <fstream>
#include <algorithm>
#include <vector>

// --- Parameters for the Sliding Window ---
#define LENGTH_MS 5000 // Total audio window size (matches your model's 20s requirement)
#define STEP_MS   2000  // How often we process the audio (5s refresh rate)
#define KEEP_MS   (LENGTH_MS - STEP_MS) // Audio to keep from previous window

// Convert MS to sample count (at 16kHz)
#define SAMPLE_RATE 16000
#define N_SAMPLES_LENGTH ( (LENGTH_MS / 1000) * SAMPLE_RATE ) // 320,000 samples for 20s
#define N_SAMPLES_STEP   ( (STEP_MS   / 1000) * SAMPLE_RATE ) //  80,000 samples for 5s
#define N_SAMPLES_KEEP   ( (KEEP_MS   / 1000) * SAMPLE_RATE ) // 240,000 samples for 15s

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("%s <model_path> <audio_path>\n", argv[0]);
        return -1;
    }
    const char *model_path = argv[1];
    const char *audio_path = argv[2];
    int ret;
    rknn_app_context_t rknn_app_ctx;
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));

    // --- Model Initialization (Unchanged) ---
    printf("Initializing Wav2Vec2 model...\n");
    ret = init_wav2vec2_model(model_path, &rknn_app_ctx);
    if (ret != 0) {
        printf("init_wav2vec2_model failed!\n");
        return -1;
    }

    // --- Audio File Setup (Treating file as a microphone stream) ---
    std::ifstream wav_file(audio_path, std::ios::binary);
    if (!wav_file.is_open()) {
        printf("Error: Could not open audio file %s\n", audio_path);
        release_wav2vec2_model(&rknn_app_ctx);
        return -1;
    }

    char header[44]; // Skip WAV header
    wav_file.read(header, 44);
    if (!wav_file.good()) {
        printf("Error: Invalid WAV file or failed to read header.\n");
        return -1;
    }

    // --- Sliding Window Buffers ---
    // This buffer holds the audio from the previous 20s window.
    std::vector<int16_t> pcm_buffer_old(N_SAMPLES_LENGTH, 0);
    // This buffer holds the new 5s chunk of audio from the "mic".
    std::vector<int16_t> pcm_buffer_new(N_SAMPLES_STEP, 0);
    // This is the final 20s buffer we will send to the model.
    std::vector<float> model_input_buffer(N_SAMPLES_LENGTH, 0.0f);


    printf("\nStarting real-time transcription simulation (Sliding Window)...\n");
    printf("Window: %dms, Step: %dms, Keep: %dms\n", LENGTH_MS, STEP_MS, KEEP_MS);

    int n_iter = 0;
    while (true) {
        // 1. Get STEP_MS of new audio from the "microphone" (our file)
        wav_file.read(reinterpret_cast<char*>(pcm_buffer_new.data()), N_SAMPLES_STEP * sizeof(int16_t));
        std::streamsize bytes_read = wav_file.gcount();

        // If we don't have enough new audio, we've reached the end of the stream.
        if (bytes_read < N_SAMPLES_STEP * sizeof(int16_t)) {
            printf("\nEnd of audio stream.\n");
            break;
        }

        // 2. Construct the new 20s window
        // First, copy the last 15s of audio from the *previous* window.
        std::copy(pcm_buffer_old.begin() + N_SAMPLES_STEP, pcm_buffer_old.end(), pcm_buffer_old.begin());
        // Second, append the new 5s of audio to the end.
        std::copy(pcm_buffer_new.begin(), pcm_buffer_new.end(), pcm_buffer_old.begin() + N_SAMPLES_KEEP);

        // 3. Convert the prepared 16-bit integer buffer to a 32-bit float buffer for the model
        for(size_t i=0; i < N_SAMPLES_LENGTH; ++i) {
            model_input_buffer[i] = static_cast<float>(pcm_buffer_old[i]) / 32768.0f;
        }

        // 4. Run inference on the 20s window
        std::vector<std::string> recognized_text;
        TIMER timer_inference;
        timer_inference.tik();
        ret = inference_wav2vec2_model(&rknn_app_ctx, model_input_buffer, recognized_text);
        timer_inference.tok();

        if (ret != 0) {
            printf("inference_wav2vec2_model failed! ret=%d\n", ret);
            continue;
        }

        // 5. Print the result
        // Use ANSI escape codes to clear the line and print the new result, like whisper.cpp
        printf("\33[2K\r");

        // --- THIS IS THE CORRECTED LINE ---
        printf("[%.2fs] ", timer_inference.get_time() / 1000.0);

        for (const auto &str : recognized_text) {
            printf("%s", str.c_str());
        }
        fflush(stdout); // Important for real-time display

        n_iter++;
    }
    printf("\n");

    // --- Cleanup ---
    wav_file.close();
    release_wav2vec2_model(&rknn_app_ctx);
    printf("Total processing finished.\n");
    return 0;
}