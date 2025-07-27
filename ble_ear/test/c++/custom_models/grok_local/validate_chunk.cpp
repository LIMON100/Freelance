#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <complex>

#include "libs/onnxruntime/include/onnxruntime_cxx_api.h"
#define DR_WAV_IMPLEMENTATION
#include "libs/single_header/dr_wav.h"

// FFTW Library
#include <fftw3.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ===================================================================
//  CONFIGURATION, HELPERS, and PRE-PROCESSING (All Unchanged)
// ===================================================================
const int SAMPLE_RATE = 16000;
const int OUTPUT_SEQ_LEN = 64000;
const int N_FFT = 512;
const int HOP_LENGTH = 128;
const int FINAL_H = 171;
const int FINAL_W = 560;

std::vector<std::string> load_labels(const std::string& filepath) {
    std::ifstream file(filepath); if (!file) throw std::runtime_error("Failed to open label file: " + filepath);
    std::vector<std::string> labels; std::string line;
    while (std::getline(file, line)) { if (!line.empty()) labels.push_back(line); }
    return labels;
}
void apply_softmax(std::vector<float>& data) {
    if (data.empty()) return; float max_val = *std::max_element(data.begin(), data.end()); float sum = 0.0f;
    for (size_t i = 0; i < data.size(); ++i) { data[i] = expf(data[i] - max_val); sum += data[i]; }
    for (size_t i = 0; i < data.size(); ++i) { data[i] /= sum > 1e-9 ? sum : 1.0f; }
}
void bilinear_resize(const std::vector<float>& input, int in_h, int in_w, std::vector<float>& output, int out_h, int out_w) {
    if (in_h <= 1 || in_w <= 1) { for(int i = 0; i < out_h * out_w; ++i) output[i] = input.empty() ? 0.0f : input[0]; return; }
    float h_ratio = static_cast<float>(in_h - 1) / (out_h - 1); float w_ratio = static_cast<float>(in_w - 1) / (out_w - 1);
    for (int i = 0; i < out_h; ++i) { for (int j = 0; j < out_w; ++j) {
        float y = i * h_ratio; float x = j * w_ratio; int y_floor = static_cast<int>(y); int x_floor = static_cast<int>(x);
        float y_frac = y - y_floor; float x_frac = x - x_floor;
        float p00 = input[y_floor * in_w + x_floor]; float p10 = input[y_floor * in_w + std::min(x_floor + 1, in_w - 1)];
        float p01 = input[std::min(y_floor + 1, in_h - 1) * in_w + x_floor];
        float p11 = input[std::min(y_floor + 1, in_h - 1) * in_w + std::min(x_floor + 1, in_w - 1)];
        float top = p00 * (1 - x_frac) + p10 * x_frac; float bottom = p01 * (1 - x_frac) + p11 * x_frac;
        output[i * out_w + j] = top * (1 - y_frac) + bottom * y_frac;
    }}
}
std::vector<float> preprocess_single_wav(const std::string& wav_path) {
    unsigned int channels, sr; drwav_uint64 total_frames;
    float* p_sample_data = drwav_open_file_and_read_pcm_frames_f32(wav_path.c_str(), &channels, &sr, &total_frames, nullptr);
    if (!p_sample_data) throw std::runtime_error("Failed to load WAV file: " + wav_path);
    if (sr != SAMPLE_RATE) throw std::runtime_error("Sample rate mismatch. Must be 16kHz.");
    std::vector<float> audio(p_sample_data, p_sample_data + total_frames);
    drwav_free(p_sample_data, nullptr);
    if (audio.size() < OUTPUT_SEQ_LEN) { audio.resize(OUTPUT_SEQ_LEN, 0.0f); } else { audio.resize(OUTPUT_SEQ_LEN); }
    int n_frames = 1 + (OUTPUT_SEQ_LEN - N_FFT) / HOP_LENGTH; int n_freqs = N_FFT / 2 + 1;
    std::vector<float> spectrogram_flat(n_freqs * n_frames); std::vector<float> window(N_FFT);
    for(int i = 0; i < N_FFT; ++i) window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (N_FFT - 1)));
    fftwf_plan plan; std::vector<float> frame(N_FFT); std::vector<std::complex<float>> fft_out(n_freqs);
    plan = fftwf_plan_dft_r2c_1d(N_FFT, frame.data(), reinterpret_cast<fftwf_complex*>(fft_out.data()), FFTW_ESTIMATE);
    for (int i = 0; i < n_frames; ++i) {
        int start = i * HOP_LENGTH;
        for (int j = 0; j < N_FFT; ++j) { frame[j] = audio[start + j] * window[j]; }
        fftwf_execute(plan);
        for (int j = 0; j < n_freqs; ++j) { spectrogram_flat[j * n_frames + i] = std::abs(fft_out[j]); }
    }
    fftwf_destroy_plan(plan);
    std::vector<float> final_input(FINAL_H * FINAL_W);
    bilinear_resize(spectrogram_flat, n_freqs, n_frames, final_input, FINAL_H, FINAL_W);
    return final_input;
}


// ===================================================================
//  VALIDATION MAIN LOGIC (UPDATED TO TAKE FOLDER PATH)
// ===================================================================
int main(int argc, char* argv[]) {
    // --- UPDATED: Check for command-line arguments ---
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_folder_with_wav_chunks>" << std::endl;
        std::cerr << "Example: " << argv[0] << " ." << std::endl;
        std::cerr << "Example: " << argv[0] << " /path/to/my/chunks" << std::endl;
        return 1;
    }
    
    // The folder path is the first argument
    const std::string chunk_folder_path = argv[1];

    const std::string onnx_model_path = "../models/groknewmodel.onnx";
    const std::string label_path = "../models/labels.txt";
    const int num_chunks_to_test = 19; // We still assume 19 chunks

    try {
        // --- Initialize ONNX and Labels ---
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "Chunk-Validator");
        Ort::SessionOptions session_options;
        Ort::Session session(env, onnx_model_path.c_str(), session_options);
        auto labels = load_labels(label_path);
        std::cout << "ONNX model and " << labels.size() << " labels loaded." << std::endl;

        std::cout << "\n--- Starting validation of " << num_chunks_to_test << " chunks from folder: " << chunk_folder_path << " ---" << std::endl;

        // --- Loop through each saved chunk ---
        for (int i = 1; i <= num_chunks_to_test; ++i) {
            // Construct the full path to the WAV file
            std::string chunk_filename = chunk_folder_path + "/detected_chunk_" + std::to_string(i) + ".wav";
            
            std::cout << "\nProcessing file: " << chunk_filename << std::endl;

            // 1. Pre-process the audio file
            std::vector<float> input_data;
            try {
                input_data = preprocess_single_wav(chunk_filename);
            } catch (const std::exception& e) {
                std::cerr << "  -> Error processing this file: " << e.what() << std::endl;
                continue; // Skip to the next file
            }
            
            // 2. Prepare tensor for ONNX
            Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
            const int64_t input_shape[] = {1, 1, FINAL_H, FINAL_W};
            Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
                memory_info, input_data.data(), input_data.size(), input_shape, 4
            );
            
            const char* input_names[] = {"input"}; 
            const char* output_names[] = {"output"};

            // 3. Run inference
            auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);

            // 4. Get and process results
            float* logits = output_tensors[0].GetTensorMutableData<float>();
            std::vector<float> probabilities(logits, logits + labels.size());
            apply_softmax(probabilities);
            
            int predicted_id = std::distance(probabilities.begin(), std::max_element(probabilities.begin(), probabilities.end()));
            float confidence = probabilities[predicted_id];

            // 5. Print the result for this chunk
            std::cout << "  -> Prediction: '" << labels[predicted_id] << "' (Confidence: " 
                      << std::fixed << std::setprecision(2) << confidence * 100.0f << "%)" << std::endl;
        }

        std::cout << "\n--- Validation complete ---" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "A critical error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}







// ORIGINAL GROK

// #include <iostream>
// #include <vector>
// #include <string>
// #include <fstream>
// #include <stdexcept>
// #include <numeric>
// #include <algorithm>
// #include <cmath>
// #include <iomanip>
// #include <cstring>
// #include <complex>
// #include <filesystem>
// #include <regex>

// // ONNX Runtime API
// #include "libs/onnxruntime/include/onnxruntime_cxx_api.h"

// // WAV Loading
// #define DR_WAV_IMPLEMENTATION
// #include "libs/single_header/dr_wav.h"

// // FFTW Library
// #include <fftw3.h>

// // ===================================================================
// //  GENERAL HELPER FUNCTIONS
// // ===================================================================

// std::vector<std::string> load_labels(const std::string& filepath) {
//     std::ifstream file(filepath);
//     if (!file) throw std::runtime_error("Failed to open label file: " + filepath);
//     std::vector<std::string> labels;
//     std::string line;
//     while (std::getline(file, line)) {
//         if (!line.empty()) labels.push_back(line);
//     }
//     return labels;
// }

// void apply_softmax(std::vector<float>& data) {
//     if (data.empty()) return;
//     float max_val = *std::max_element(data.begin(), data.end());
//     float sum = 0.0f;
//     for (size_t i = 0; i < data.size(); ++i) {
//         data[i] = expf(data[i] - max_val);
//         sum += data[i];
//     }
//     if (sum == 0) sum = 1.0f; // Avoid division by zero
//     for (size_t i = 0; i < data.size(); ++i) {
//         data[i] /= sum;
//     }
// }

// // ===================================================================
// //  SPECTROGRAM PRE-PROCESSING
// // ===================================================================

// // --- Configuration (MUST match your Python script) ---
// const int SAMPLE_RATE = 16000;
// const int OUTPUT_SEQ_LEN = 64000;
// const int N_FFT = 512;
// const int HOP_LENGTH = 128;
// const int FINAL_H = 171;
// const int FINAL_W = 560;

// // Bilinear Interpolation function
// void bilinear_resize(const std::vector<float>& input, int in_h, int in_w,
//                      std::vector<float>& output, int out_h, int out_w) {
//     if (in_h <= 1 || in_w <= 1) { // Avoid division by zero for single-pixel dimensions
//         for(int i = 0; i < out_h * out_w; ++i) output[i] = input[0];
//         return;
//     }

//     float h_ratio = static_cast<float>(in_h - 1) / (out_h - 1);
//     float w_ratio = static_cast<float>(in_w - 1) / (out_w - 1);

//     for (int i = 0; i < out_h; ++i) {
//         for (int j = 0; j < out_w; ++j) {
//             float y = i * h_ratio;
//             float x = j * w_ratio;

//             int y_floor = static_cast<int>(y);
//             int x_floor = static_cast<int>(x);
//             int y_ceil = std::min(y_floor + 1, in_h - 1);
//             int x_ceil = std::min(x_floor + 1, in_w - 1);

//             float y_frac = y - y_floor;
//             float x_frac = x - x_floor;

//             float p00 = input[y_floor * in_w + x_floor];
//             float p10 = input[y_floor * in_w + x_ceil];
//             float p01 = input[y_ceil * in_w + x_floor];
//             float p11 = input[y_ceil * in_w + x_ceil];

//             float top = p00 * (1 - x_frac) + p10 * x_frac;
//             float bottom = p01 * (1 - x_frac) + p11 * x_frac;

//             output[i * out_w + j] = top * (1 - y_frac) + bottom * y_frac;
//         }
//     }
// }

// std::vector<float> preprocess_audio(const std::string& wav_path) {
//     // 1. Load audio
//     unsigned int channels, sr;
//     drwav_uint64 total_frames;
//     float* p_sample_data = drwav_open_file_and_read_pcm_frames_f32(wav_path.c_str(), &channels, &sr, &total_frames, nullptr);
//     if (!p_sample_data) throw std::runtime_error("Failed to load WAV file: " + wav_path);
//     std::vector<float> audio(p_sample_data, p_sample_data + total_frames);
//     drwav_free(p_sample_data, nullptr);
    
//     if (sr != SAMPLE_RATE) {
//          std::cerr << "Warning: Sample rate mismatch for " << wav_path << ". Expected " << SAMPLE_RATE << " but got " << sr << ". Please ensure input audio is 16kHz." << std::endl;
//     }

//     // 2. Pad or truncate audio
//     if (audio.size() < OUTPUT_SEQ_LEN) {
//         audio.resize(OUTPUT_SEQ_LEN, 0.0f);
//     } else {
//         audio.resize(OUTPUT_SEQ_LEN);
//     }

//     // 3. Compute STFT (linear spectrogram)
//     int n_frames = 1 + (OUTPUT_SEQ_LEN - N_FFT) / HOP_LENGTH;
//     int n_freqs = N_FFT / 2 + 1;
//     std::vector<float> spectrogram_flat(n_freqs * n_frames);

//     std::vector<float> window(N_FFT);
//     for(int i = 0; i < N_FFT; ++i) window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (N_FFT - 1))); // Hann window

//     std::vector<float> frame(N_FFT);
//     std::vector<std::complex<float>> fft_out(n_freqs);
//     fftwf_plan plan = fftwf_plan_dft_r2c_1d(N_FFT, frame.data(), reinterpret_cast<fftwf_complex*>(fft_out.data()), FFTW_ESTIMATE);

//     for (int i = 0; i < n_frames; ++i) {
//         int start = i * HOP_LENGTH;
//         for (int j = 0; j < N_FFT; ++j) {
//             frame[j] = audio[start + j] * window[j];
//         }
//         fftwf_execute(plan);
//         for (int j = 0; j < n_freqs; ++j) {
//             spectrogram_flat[j * n_frames + i] = std::abs(fft_out[j]);
//         }
//     }
//     fftwf_destroy_plan(plan);

//     // 4. Resize the spectrogram using bilinear interpolation
//     std::vector<float> final_input(FINAL_H * FINAL_W);
//     bilinear_resize(spectrogram_flat, n_freqs, n_frames, final_input, FINAL_H, FINAL_W);
    
//     return final_input;
// }

// // ===================================================================
// //  MAIN INFERENCE LOGIC
// // ===================================================================
// int main(int argc, char* argv[]) {
//     if (argc != 2) {
//         std::cerr << "Usage: " << argv[0] << " <path_to_wav_folder>" << std::endl;
//         return 1;
//     }

//     const std::string onnx_model_path = "../models/groknewmodel.onnx";
//     const std::string label_path = "../models/labels.txt";
//     const std::string folder_path = argv[1];

//     try {
//         // Check if folder exists
//         if (!std::filesystem::exists(folder_path) || !std::filesystem::is_directory(folder_path)) {
//             throw std::runtime_error("Invalid folder path: " + folder_path);
//         }

//         // Collect all WAV files
//         std::vector<std::string> wav_files;
//         for (const auto& entry : std::filesystem::directory_iterator(folder_path)) {
//             if (entry.path().extension() == ".wav") {
//                 wav_files.push_back(entry.path().string());
//             }
//         }

//         // Sort files by chunk number
//         std::sort(wav_files.begin(), wav_files.end(), [](const std::string& a, const std::string& b) {
//             std::regex chunk_regex("detected_chunk_(\\d+)\\.wav");
//             std::smatch match_a, match_b;
//             int num_a = 0, num_b = 0;
//             if (std::regex_search(a, match_a, chunk_regex)) {
//                 num_a = std::stoi(match_a[1].str());
//             }
//             if (std::regex_search(b, match_b, chunk_regex)) {
//                 num_b = std::stoi(match_b[1].str());
//             }
//             return num_a < num_b;
//         });

//         if (wav_files.empty()) {
//             throw std::runtime_error("No WAV files found in folder: " + folder_path);
//         }

//         Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "Grok3OnnxPredictor");
//         Ort::SessionOptions session_options;
//         Ort::Session session(env, onnx_model_path.c_str(), session_options);
//         auto labels = load_labels(label_path);

//         // Process each WAV file in sorted order
//         for (const auto& wav_path : wav_files) {
//             std::cout << "\nProcessing audio file: " << wav_path << std::endl;
            
//             try {
//                 std::vector<float> input_data = preprocess_audio(wav_path);
//                 std::cout << "Audio pre-processing complete. Running ONNX inference..." << std::endl;

//                 Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
//                 const int64_t input_shape[] = {1, 1, FINAL_H, FINAL_W};
                
//                 Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
//                     memory_info, input_data.data(), input_data.size(), input_shape, 4
//                 );
                
//                 const char* input_names[] = {"input"}; 
//                 const char* output_names[] = {"output"};

//                 auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);

//                 float* logits = output_tensors[0].GetTensorMutableData<float>();
//                 size_t num_classes = labels.size();

//                 std::vector<float> probabilities(logits, logits + num_classes);
//                 apply_softmax(probabilities);

//                 int predicted_id = std::distance(probabilities.begin(), std::max_element(probabilities.begin(), probabilities.end()));
//                 float confidence = probabilities[predicted_id];

//                 std::cout << "\n--- ONNX Prediction Result for " << wav_path << " ---" << std::endl;
//                 std::cout << "Predicted Command: '" << labels[predicted_id] << "'" << std::endl;
//                 std::cout << "Confidence: " << std::fixed << std::setprecision(2) << confidence * 100.0f << "%" << std::endl;
//                 std::cout << "------------------------------" << std::endl;
//             } catch (const std::exception& e) {
//                 std::cerr << "Error processing " << wav_path << ": " << e.what() << std::endl;
//                 continue; // Continue with next file
//             }
//         }

//     } catch (const std::exception& e) {
//         std::cerr << "An error occurred: " << e.what() << std::endl;
//         return 1;
//     }

//     return 0;
// }