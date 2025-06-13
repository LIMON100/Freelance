// // main.cc - Modified for chunk-by-chunk processing from a file

// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include "wav2vec2.h"
// #include "audio_utils.h" // We'll still use parts of this
// #include "easy_timer.h"
// #include <fstream>     // For std::ifstream
// #include <algorithm>   // For std::min

// // Constants from process.h (or ensure process.h is included before main)
// // We need N_SAMPLES and SAMPLE_RATE for chunking logic
// // #define VOCAB_NUM 32 // Already in process.h
// // #define SAMPLE_RATE 16000 // Already in process.h
// // #define CHUNK_LENGTH 20 // Original full chunk length model expects
// // #define OUTPUT_SIZE CHUNK_LENGTH * 50 - 1 // Already in process.h
// // #define N_SAMPLES CHUNK_LENGTH *SAMPLE_RATE // Model's expected total input samples

// #define PROCESSING_CHUNK_SECONDS 2 // Process 2 seconds at a time
// #define PROCESSING_CHUNK_SAMPLES (PROCESSING_CHUNK_SECONDS * SAMPLE_RATE)
// #define PROCESSING_CHUNK_BYTES (PROCESSING_CHUNK_SAMPLES * sizeof(int16_t)) // Assuming 16-bit samples

// int main(int argc, char **argv) {
//     if (argc != 3) {
//         printf("%s <model_path> <audio_path>\n", argv[0]);
//         return -1;
//     }

//     const char *model_path = argv[1];
//     const char *audio_path = argv[2];

//     int ret;
//     TIMER timer_total, timer_chunk;
//     rknn_app_context_t rknn_app_ctx;
//     memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));

//     // --- Initialize Wav2Vec2 Model (once at the start) ---
//     std::cout << "Initializing Wav2Vec2 model..." << std::endl;
//     timer_total.tik();
//     ret = init_wav2vec2_model(model_path, &rknn_app_ctx);
//     if (ret != 0) {
//         printf("init_wav2vec2_model fail! ret=%d model_path=%s\n", ret, model_path);
//         return -1;
//     }
//     timer_total.tok();
//     timer_total.print_time("init_wav2vec2_model");


//     // --- Open WAV file for chunked reading ---
//     std::ifstream wav_file(audio_path, std::ios::binary);
//     if (!wav_file.is_open()) {
//         printf("Failed to open WAV file: %s\n", audio_path);
//         release_wav2vec2_model(&rknn_app_ctx);
//         return -1;
//     }

//     // Basic WAV header validation and skipping
//     // This is a simplified header skip, assumes standard 16-bit mono 16kHz after header
//     char header[44];
//     wav_file.read(header, 44);
//     if (!wav_file.good() || std::string(header, header + 4) != "RIFF" || std::string(header + 8, header + 12) != "WAVE") {
//         printf("Invalid WAV file format.\n");
//         wav_file.close();
//         release_wav2vec2_model(&rknn_app_ctx);
//         return -1;
//     }
//     // You should ideally parse channels, sample rate, bits_per_sample from header
//     // and convert/resample if necessary before this loop.
//     // For this example, we assume the WAV is already 16kHz, 16-bit, mono.

//     std::cout << "\nProcessing audio file in " << PROCESSING_CHUNK_SECONDS << "-second chunks..." << std::endl;

//     int chunk_count = 0;
//     while (!wav_file.eof()) {
//         chunk_count++;
//         std::cout << "\n--- Processing Chunk " << chunk_count << " ---" << std::endl;
//         timer_chunk.tik();

//         std::vector<int16_t> raw_chunk_s16(PROCESSING_CHUNK_SAMPLES, 0); // Buffer for 16-bit samples
//         wav_file.read(reinterpret_cast<char*>(raw_chunk_s16.data()), PROCESSING_CHUNK_BYTES);
//         std::streamsize bytes_actually_read = wav_file.gcount();

//         if (bytes_actually_read == 0 && wav_file.eof()) {
//             std::cout << "End of file reached." << std::endl;
//             break;
//         }
//         if (bytes_actually_read < 0) { // Should not happen with gcount
//             std::cout << "Error reading from WAV file." << std::endl;
//             break;
//         }

//         int samples_actually_read = bytes_actually_read / sizeof(int16_t);
//         std::cout << "Read " << samples_actually_read << " samples for this chunk." << std::endl;


//         // --- Preprocess this chunk ---
//         // Convert int16_t chunk to float and pad/trim to N_SAMPLES for the model
//         std::vector<float> current_chunk_float(samples_actually_read);
//         for(int i=0; i < samples_actually_read; ++i) {
//             current_chunk_float[i] = static_cast<float>(raw_chunk_s16[i]) / 32768.0f; // Normalize
//         }

//         std::vector<float> model_input_audio_data(N_SAMPLES, 0.0f); // This is what the model expects
//         // The audio_preprocess in process.cc expects an audio_buffer_t
//         // We need to adapt or reimplement its core logic (pad_or_trim)
//         // For simplicity, let's directly use a pad_or_trim logic here:
//         int num_frames_in_chunk = current_chunk_float.size();
//         if (num_frames_in_chunk > N_SAMPLES) { // Should not happen if PROCESSING_CHUNK_SAMPLES <= N_SAMPLES
//             std::copy(current_chunk_float.begin(), current_chunk_float.begin() + N_SAMPLES, model_input_audio_data.begin());
//         } else {
//             std::copy(current_chunk_float.begin(), current_chunk_float.end(), model_input_audio_data.begin());
//             // The rest of model_input_audio_data is already 0.0f (padding)
//         }
//         // Note: The original audio_preprocess also handled audio_buffer_t.
//         // Here we are directly creating the float vector the model needs.

//         timer_chunk.tok();
//         timer_chunk.print_time("Chunk audio loading & preprocessing");

//         // --- Inference for this chunk ---
//         timer_chunk.tik();
//         std::vector<std::string> recognized_text_chunk;
//         ret = inference_wav2vec2_model(&rknn_app_ctx, model_input_audio_data, recognized_text_chunk);
//         if (ret != 0) {
//             printf("inference_wav2vec2_model fail! ret=%d\n", ret);
//             // Decide if you want to break or continue with next chunk
//             break;
//         }
//         timer_chunk.tok();
//         timer_chunk.print_time("Chunk inference_wav2vec2_model");

//         // --- Print result for this chunk ---
//         if (!recognized_text_chunk.empty()) {
//             std::cout << "Chunk " << chunk_count << " Output: ";
//             for (const auto &str : recognized_text_chunk) {
//                 std::cout << str;
//             }
//             std::cout << std::endl;
//         } else if (samples_actually_read > 0) { // Only print "no text" if there was audio
//             std::cout << "Chunk " << chunk_count << " Output: (No text recognized or silence)" << std::endl;
//         }

//         if (bytes_actually_read < PROCESSING_CHUNK_BYTES) {
//              std::cout << "Partial read at end of file, processing finished." << std::endl;
//             break; // Likely hit EOF
//         }
//     }

//     // --- Cleanup ---
//     wav_file.close();
//     ret = release_wav2vec2_model(&rknn_app_ctx);
//     if (ret != 0) {
//         printf("release_wav2vec2_model fail! ret=%d\n", ret);
//     }

//     std::cout << "\nTotal processing finished." << std::endl;
//     return 0;
// }


// main.cc - Modified for chunk-by-chunk with simple energy VAD simulation

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wav2vec2.h"
#include "audio_utils.h"
#include "easy_timer.h"
#include <fstream>
#include <algorithm>
#include <numeric> // For std::accumulate
#include <cmath>   // For std::sqrt, std::abs

// ... (Keep your existing defines for MODEL_PATH, SAMPLE_RATE, N_SAMPLES etc.) ...
// ... (Keep your inference_wav2vec2_model, init_wav2vec2_model, release_wav2vec2_model) ...
// ... (Keep your audio_preprocess from process.cc or its logic) ...

#define VAD_SUB_CHUNK_MS 200 // Process VAD in 200ms sub-chunks
#define VAD_SUB_CHUNK_SAMPLES ((VAD_SUB_CHUNK_MS * SAMPLE_RATE) / 1000)
#define VAD_SUB_CHUNK_BYTES (VAD_SUB_CHUNK_SAMPLES * sizeof(int16_t))

#define ENERGY_THRESHOLD 500.0f // Adjust this based on your microphone/audio levels!
#define SILENCE_CHUNKS_TO_END_SPEECH 5 // How many consecutive silent sub-chunks end an utterance
#define MIN_SPEECH_CHUNKS 2          // Minimum number of speech sub-chunks to form an utterance

// Function to calculate RMS energy of a 16-bit audio chunk
float calculate_rms_energy(const std::vector<int16_t>& audio_chunk, int num_samples) {
    if (num_samples == 0) return 0.0f;
    double sum_sq = 0.0;
    for (int i = 0; i < num_samples; ++i) {
        double sample = static_cast<double>(audio_chunk[i]);
        sum_sq += sample * sample;
    }
    return static_cast<float>(std::sqrt(sum_sq / num_samples));
}


int main(int argc, char **argv) {
    if (argc != 3) {
        printf("%s <model_path> <audio_path>\n", argv[0]);
        return -1;
    }
    const char *model_path = argv[1];
    const char *audio_path = argv[2];
    int ret;
    TIMER timer_total;
    rknn_app_context_t rknn_app_ctx;
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));

    std::cout << "Initializing Wav2Vec2 model..." << std::endl;
    timer_total.tik();
    ret = init_wav2vec2_model(model_path, &rknn_app_ctx);
    if (ret != 0) { /* ... error handling ... */ return -1; }
    timer_total.tok();
    timer_total.print_time("init_wav2vec2_model");

    std::ifstream wav_file(audio_path, std::ios::binary);
    if (!wav_file.is_open()) { /* ... error handling ... */ release_wav2vec2_model(&rknn_app_ctx); return -1; }
    
    char header[44]; // Skip header
    wav_file.read(header, 44);
    if (!wav_file.good()) { /* ... error handling ... */ return -1; }


    std::cout << "\nSimulating VAD and processing speech segments from file..." << std::endl;

    std::vector<int16_t> speech_segment_s16; // Accumulates speech sub-chunks
    bool currently_speaking = false;
    int consecutive_silence_chunks = 0;
    int utterance_count = 0;

    while (!wav_file.eof()) {
        std::vector<int16_t> vad_sub_chunk_s16(VAD_SUB_CHUNK_SAMPLES, 0);
        wav_file.read(reinterpret_cast<char*>(vad_sub_chunk_s16.data()), VAD_SUB_CHUNK_BYTES);
        std::streamsize bytes_actually_read = wav_file.gcount();

        if (bytes_actually_read == 0 && wav_file.eof()) break;
        if (bytes_actually_read < 0) break;
        
        int samples_in_sub_chunk = bytes_actually_read / sizeof(int16_t);
        if (samples_in_sub_chunk == 0) continue; // Should not happen if bytes_actually_read > 0

        // Resize to actual samples read for energy calculation
        vad_sub_chunk_s16.resize(samples_in_sub_chunk); 

        float energy = calculate_rms_energy(vad_sub_chunk_s16, samples_in_sub_chunk);
        // printf("Sub-chunk energy: %.2f\n", energy); // For debugging threshold

        if (energy > ENERGY_THRESHOLD) {
            // std::cout << "SPEECH DETECTED in sub-chunk" << std::endl;
            if (!currently_speaking) {
                 // std::cout << "Start of new utterance detected." << std::endl;
                speech_segment_s16.clear(); // Start accumulating new utterance
            }
            currently_speaking = true;
            consecutive_silence_chunks = 0;
            speech_segment_s16.insert(speech_segment_s16.end(), vad_sub_chunk_s16.begin(), vad_sub_chunk_s16.end());
        } else { // Silence or low energy
            if (currently_speaking) {
                consecutive_silence_chunks++;
                // Also add this silent chunk to the segment if it's short silence within speech
                speech_segment_s16.insert(speech_segment_s16.end(), vad_sub_chunk_s16.begin(), vad_sub_chunk_s16.end());

                if (consecutive_silence_chunks >= SILENCE_CHUNKS_TO_END_SPEECH) {
                    // std::cout << "End of utterance detected (silence)." << std::endl;
                    currently_speaking = false; // Stop accumulating this utterance
                    // Process the accumulated speech_segment_s16
                }
            } else {
                // Still silence, do nothing, wait for speech
            }
        }

        // Process if:
        // 1. Speech ended (currently_speaking is false AND speech_segment_s16 has data)
        // 2. We hit EOF and there's a pending speech segment
        bool end_of_file_and_pending = (wav_file.eof() && !speech_segment_s16.empty() && currently_speaking);

        if ((!currently_speaking && speech_segment_s16.size() >= VAD_SUB_CHUNK_SAMPLES * MIN_SPEECH_CHUNKS) || end_of_file_and_pending) {
            utterance_count++;
            std::cout << "\n--- Processing Utterance " << utterance_count 
                      << " (length: " << static_cast<float>(speech_segment_s16.size())/SAMPLE_RATE << "s) ---" << std::endl;

            // Convert accumulated int16_t speech segment to float
            std::vector<float> current_utterance_float(speech_segment_s16.size());
            for(size_t i=0; i < speech_segment_s16.size(); ++i) {
                current_utterance_float[i] = static_cast<float>(speech_segment_s16[i]) / 32768.0f;
            }

            // Pad this utterance to N_SAMPLES for the model
            std::vector<float> model_input_audio_data(N_SAMPLES, 0.0f);
            int num_frames_in_utterance = current_utterance_float.size();
            if (num_frames_in_utterance > N_SAMPLES) { // Trim if utterance is > 20s
                std::copy(current_utterance_float.begin(), current_utterance_float.begin() + N_SAMPLES, model_input_audio_data.begin());
            } else {
                std::copy(current_utterance_float.begin(), current_utterance_float.end(), model_input_audio_data.begin());
            }

            // --- Inference for this utterance ---
            std::vector<std::string> recognized_text_utterance;
            ret = inference_wav2vec2_model(&rknn_app_ctx, model_input_audio_data, recognized_text_utterance);
            
            if (ret == 0 && !recognized_text_utterance.empty()) {
                std::cout << "Recognized: ";
                for (const auto &str : recognized_text_utterance) {
                    std::cout << str;
                }
                std::cout << std::endl;
            } else if (ret != 0) {
                 printf("inference_wav2vec2_model failed for utterance! ret=%d\n", ret);
            } else {
                 std::cout << "Recognized: (No text or silence for this utterance)" << std::endl;
            }

            speech_segment_s16.clear(); // Clear for next utterance
            consecutive_silence_chunks = 0; // Reset silence counter
            if(end_of_file_and_pending) currently_speaking = false; // ensure it's reset
        }
        
        if (bytes_actually_read < VAD_SUB_CHUNK_BYTES && wav_file.eof()) {
             // std::cout << "End of file reached during sub-chunk read." << std::endl;
            if (!speech_segment_s16.empty() && currently_speaking) { // Process any lingering speech
                 // This logic is somewhat duplicated above, could be refined
                 // but ensures last segment is processed if EOF hit mid-speech segment
                 // The 'end_of_file_and_pending' should catch this
            }
            break;
        }
    }

    wav_file.close();
    release_wav2vec2_model(&rknn_app_ctx);
    std::cout << "\nTotal processing finished." << std::endl;
    return 0;
}