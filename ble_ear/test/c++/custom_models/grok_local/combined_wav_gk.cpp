// #include <iostream>
// #include <vector>
// #include <string>
// #include <fstream>
// #include <stdexcept>
// #include <numeric>
// #include <algorithm>
// #include <cmath>
// #include <iomanip>
// #include <complex>

// #include "libs/onnxruntime/include/onnxruntime_cxx_api.h"
// // WAV Loading
// #define DR_WAV_IMPLEMENTATION
// #include "libs/single_header/dr_wav.h"

// // FFTW Library
// #include <fftw3.h>

// #ifndef M_PI
// #define M_PI 3.14159265358979323846
// #endif

// // ===================================================================
// //  CONFIGURATION
// // ===================================================================
// const int SAMPLE_RATE = 16000;
// const int OUTPUT_SEQ_LEN = 64000;
// const int N_FFT = 512;
// const int HOP_LENGTH = 128;
// const int FINAL_H = 171;
// const int FINAL_W = 560;
// const float VAD_THRESHOLD = 0.015f;
// const float VAD_HPF_CUTOFF = 100.0f;

// // ===================================================================
// //  HELPER FUNCTIONS (All bodies fully included)
// // ===================================================================

// // Normalizes the audio to be in the [-1.0, 1.0] range
// void normalize_audio(std::vector<float>& audio) {
//     if (audio.empty()) return;
//     float max_abs_val = 0.0f;
//     for (float sample : audio) {
//         if (std::abs(sample) > max_abs_val) {
//             max_abs_val = std::abs(sample);
//         }
//     }
//     if (max_abs_val > 1e-5) { // Avoid division by zero
//         for (float& sample : audio) {
//             sample /= max_abs_val;
//         }
//     }
// }

// std::vector<std::string> load_labels(const std::string& filepath) {
//     std::ifstream file(filepath); if (!file) throw std::runtime_error("Failed to open label file: " + filepath);
//     std::vector<std::string> labels; std::string line;
//     while (std::getline(file, line)) { if (!line.empty()) labels.push_back(line); }
//     return labels;
// }

// void apply_softmax(std::vector<float>& data) {
//     if (data.empty()) return; float max_val = *std::max_element(data.begin(), data.end()); float sum = 0.0f;
//     for (size_t i = 0; i < data.size(); ++i) { data[i] = expf(data[i] - max_val); sum += data[i]; }
//     for (size_t i = 0; i < data.size(); ++i) { data[i] /= sum > 1e-9 ? sum : 1.0f; }
// }

// void save_chunk_to_wav(const std::string& filename, const std::vector<float>& audio_chunk) {
//     drwav_data_format format; format.container = drwav_container_riff; format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
//     format.channels = 1; format.sampleRate = SAMPLE_RATE; format.bitsPerSample = 32;
//     drwav wav; if (!drwav_init_file_write(&wav, filename.c_str(), &format, nullptr)) { std::cerr << "Warning: Failed to init WAV " << filename << std::endl; return; }
//     drwav_write_pcm_frames(&wav, audio_chunk.size(), audio_chunk.data());
//     drwav_uninit(&wav);
//     std::cout << "  -> Saved detected audio chunk to: " << filename << std::endl;
// }

// void bilinear_resize(const std::vector<float>& input, int in_h, int in_w, std::vector<float>& output, int out_h, int out_w) {
//     if (in_h <= 1 || in_w <= 1) { for(int i = 0; i < out_h * out_w; ++i) output[i] = input.empty() ? 0.0f : input[0]; return; }
//     float h_ratio = static_cast<float>(in_h - 1) / (out_h - 1); float w_ratio = static_cast<float>(in_w - 1) / (out_w - 1);
//     for (int i = 0; i < out_h; ++i) { for (int j = 0; j < out_w; ++j) {
//         float y = i * h_ratio; float x = j * w_ratio; int y_floor = static_cast<int>(y); int x_floor = static_cast<int>(x);
//         float y_frac = y - y_floor; float x_frac = x - x_floor;
//         float p00 = input[y_floor * in_w + x_floor]; float p10 = input[y_floor * in_w + std::min(x_floor + 1, in_w - 1)];
//         float p01 = input[std::min(y_floor + 1, in_h - 1) * in_w + x_floor];
//         float p11 = input[std::min(y_floor + 1, in_h - 1) * in_w + std::min(x_floor + 1, in_w - 1)];
//         float top = p00 * (1 - x_frac) + p10 * x_frac; float bottom = p01 * (1 - x_frac) + p11 * x_frac;
//         output[i * out_w + j] = top * (1 - y_frac) + bottom * y_frac;
//     }}
// }

// std::vector<float> preprocess_audio_chunk(const std::vector<float>& audio_chunk) {
//     std::vector<float> audio = audio_chunk;
//     if (audio.size() < OUTPUT_SEQ_LEN) { audio.resize(OUTPUT_SEQ_LEN, 0.0f); } else { audio.resize(OUTPUT_SEQ_LEN); }
//     int n_frames = 1 + (OUTPUT_SEQ_LEN - N_FFT) / HOP_LENGTH; int n_freqs = N_FFT / 2 + 1;
//     std::vector<float> spectrogram_flat(n_freqs * n_frames); std::vector<float> window(N_FFT);
//     for(int i = 0; i < N_FFT; ++i) window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (N_FFT - 1)));
//     fftwf_plan plan; std::vector<float> frame(N_FFT); std::vector<std::complex<float>> fft_out(n_freqs);
//     plan = fftwf_plan_dft_r2c_1d(N_FFT, frame.data(), reinterpret_cast<fftwf_complex*>(fft_out.data()), FFTW_ESTIMATE);
//     for (int i = 0; i < n_frames; ++i) {
//         int start = i * HOP_LENGTH;
//         for (int j = 0; j < N_FFT; ++j) { frame[j] = audio[start + j] * window[j]; }
//         fftwf_execute(plan);
//         for (int j = 0; j < n_freqs; ++j) { spectrogram_flat[j * n_frames + i] = std::abs(fft_out[j]); }
//     }
//     fftwf_destroy_plan(plan);
//     std::vector<float> final_input(FINAL_H * FINAL_W);
//     bilinear_resize(spectrogram_flat, n_freqs, n_frames, final_input, FINAL_H, FINAL_W);
//     return final_input;
// }

// class HighPassFilter {
// public:
//     HighPassFilter(float cutoff, float sample_rate) { float rc = 1.0f / (2.0f * M_PI * cutoff); alpha = rc / (rc + 1.0f / sample_rate); y_prev = 0.0f; x_prev = 0.0f; }
//     float process(float x) { float y = alpha * (y_prev + x - x_prev); y_prev = y; x_prev = x; return y; }
// private: float alpha, y_prev, x_prev;
// };

// bool is_speech_present(const std::vector<float>& audio_chunk, HighPassFilter& hpf, float threshold) {
//     if (audio_chunk.empty()) return false; double sum_sq = 0.0;
//     for (float sample : audio_chunk) { float filtered_sample = hpf.process(sample); sum_sq += filtered_sample * filtered_sample; }
//     return sqrt(sum_sq / audio_chunk.size()) > threshold;
// }

// size_t find_energy_center(const std::vector<float>& audio_chunk) {
//     const int frame_length = 512; const int hop_length = 256; if (audio_chunk.size() < frame_length) return audio_chunk.size() / 2;
//     float max_energy = -1.0f; size_t max_energy_index = 0;
//     for (size_t i = 0; i <= audio_chunk.size() - frame_length; i += hop_length) {
//         float current_energy = 0.0f; for (size_t j = 0; j < frame_length; ++j) { current_energy += audio_chunk[i + j] * audio_chunk[i + j]; }
//         if (current_energy > max_energy) { max_energy = current_energy; max_energy_index = i; }
//     } return max_energy_index + frame_length / 2;
// }

// // ===================================================================
// //  MAIN LOGIC
// // ===================================================================
// int main(int argc, char* argv[]) {
//     if (argc != 2) {
//         std::cerr << "Usage: " << argv[0] << " <path_to_multi_command_wav_file>" << std::endl;
//         return 1;
//     }

//     const std::string onnx_model_path = "../models/smaller_grok.onnx";
//     const std::string label_path = "../models/labels.txt";
//     const std::string wav_path = argv[1];

//     try {
//         Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "GrokCommandDetector");
//         Ort::SessionOptions session_options;
//         Ort::Session session(env, onnx_model_path.c_str(), session_options);
//         auto labels = load_labels(label_path);
        
//         unsigned int channels, sr; drwav_uint64 total_frames;
//         float* p_sample_data = drwav_open_file_and_read_pcm_frames_f32(wav_path.c_str(), &channels, &sr, &total_frames, nullptr);
//         if (!p_sample_data) throw std::runtime_error("Failed to load WAV file: " + wav_path);
//         if (sr != SAMPLE_RATE) throw std::runtime_error("Sample rate mismatch. Must be 16kHz.");
        
//         std::vector<float> full_audio(total_frames);
//         if (channels > 1) {
//              for (drwav_uint64 i = 0; i < total_frames; ++i) {
//                 float mono_sample = 0.0f;
//                 for (unsigned int c = 0; c < channels; ++c) mono_sample += p_sample_data[i * channels + c];
//                 full_audio[i] = mono_sample / channels;
//             }
//         } else {
//             std::copy(p_sample_data, p_sample_data + total_frames, full_audio.begin());
//         }
//         drwav_free(p_sample_data, nullptr);

//         // NORMALIZE THE ENTIRE AUDIO FILE
//         normalize_audio(full_audio);
//         std::cout << "Audio file loaded and normalized: " << wav_path << " (" << full_audio.size() / (float)SAMPLE_RATE << "s)" << std::endl;
        
//         std::cout << "\n--- Starting command detection loop (with stateless VAD & normalization) ---" << std::endl;
        
//         size_t vad_scan_samples = static_cast<size_t>(0.1 * SAMPLE_RATE);
//         size_t last_detection_end_sample = 0;
//         int detection_count = 0;

//         for (size_t current_pos = 0; current_pos + vad_scan_samples < full_audio.size(); ) {
//             if (current_pos < last_detection_end_sample) {
//                 current_pos = last_detection_end_sample;
//                 continue;
//             }

//             std::vector<float> vad_chunk(&full_audio[current_pos], &full_audio[current_pos + vad_scan_samples]);
//             HighPassFilter hpf(VAD_HPF_CUTOFF, SAMPLE_RATE); 
            
//             if (is_speech_present(vad_chunk, hpf, VAD_THRESHOLD)) {
//                 detection_count++;
//                 long timestamp_ms = static_cast<long>(current_pos * 1000.0 / SAMPLE_RATE);
                
//                 size_t search_start = (current_pos > 0.5 * SAMPLE_RATE) ? current_pos - 0.5 * SAMPLE_RATE : 0;
//                 size_t search_end = std::min(full_audio.size(), current_pos + (size_t)(1.5 * SAMPLE_RATE));
//                 std::vector<float> search_window(&full_audio[search_start], &full_audio[search_end]);
                
//                 size_t energy_center_in_window = find_energy_center(search_window);
//                 size_t absolute_energy_center = search_start + energy_center_in_window;

//                 size_t clip_start = (absolute_energy_center > OUTPUT_SEQ_LEN / 2) ? absolute_energy_center - OUTPUT_SEQ_LEN / 2 : 0;
//                 std::vector<float> command_chunk(&full_audio[clip_start], &full_audio[clip_start + OUTPUT_SEQ_LEN]);

//                 std::string chunk_filename = "detected_chunk_" + std::to_string(detection_count) + ".wav";
//                 save_chunk_to_wav(chunk_filename, command_chunk);
                
//                 std::vector<float> input_data = preprocess_audio_chunk(command_chunk);

//                 Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
//                 const int64_t input_shape[] = {1, 1, FINAL_H, FINAL_W};
//                 Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_data.data(), input_data.size(), input_shape, 4);
                
//                 const char* input_names[] = {"input"}; 
//                 const char* output_names[] = {"output"};
//                 auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);

//                 float* logits = output_tensors[0].GetTensorMutableData<float>();
//                 std::vector<float> probabilities(logits, logits + labels.size());
//                 apply_softmax(probabilities);
                
//                 int predicted_id = std::distance(probabilities.begin(), std::max_element(probabilities.begin(), probabilities.end()));
//                 float confidence = probabilities[predicted_id];

//                 std::cout << "\n[" << timestamp_ms << " ms] Detection #" << detection_count << ": '" << labels[predicted_id] << "' (Confidence: " 
//                           << std::fixed << std::setprecision(2) << confidence * 100.0f << "%)" << std::endl;

//                 last_detection_end_sample = absolute_energy_center + static_cast<size_t>(1.5 * SAMPLE_RATE);
//                 current_pos = last_detection_end_sample;
//             } else {
//                 current_pos += vad_scan_samples;
//             }
//         }
//         std::cout << "\n--- Processing complete. Total commands detected: " << detection_count << " ---" << std::endl;

//     } catch (const std::exception& e) {
//         std::cerr << "An error occurred: " << e.what() << std::endl;
//         return 1;
//     }
//     return 0;
// }





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
// WAV Loading
#define DR_WAV_IMPLEMENTATION
#include "libs/single_header/dr_wav.h"

// FFTW Library
#include <fftw3.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ===================================================================
//  CONFIGURATION
// ===================================================================
const int SAMPLE_RATE = 16000;
const int OUTPUT_SEQ_LEN = 64000; // 4 seconds
const int N_FFT = 512;
const int HOP_LENGTH = 128;
const int FINAL_H = 171;
const int FINAL_W = 560;
const float VAD_THRESHOLD = 0.015f;
const float VAD_HPF_CUTOFF = 100.0f;



void normalize_audio(std::vector<float>& audio) {
    if (audio.empty()) return; float max_abs_val = 0.0f; for (float sample : audio) { if (std::abs(sample) > max_abs_val) { max_abs_val = std::abs(sample); } }
    if (max_abs_val > 1e-5) { for (float& sample : audio) { sample /= max_abs_val; } }
}
std::vector<std::string> load_labels(const std::string& filepath) {
    std::ifstream file(filepath); if (!file) throw std::runtime_error("Failed to open label file: " + filepath);
    std::vector<std::string> labels; std::string line; while (std::getline(file, line)) { if (!line.empty()) labels.push_back(line); } return labels;
}
void apply_softmax(std::vector<float>& data) {
    if (data.empty()) return; float max_val = *std::max_element(data.begin(), data.end()); float sum = 0.0f;
    for (size_t i = 0; i < data.size(); ++i) { data[i] = expf(data[i] - max_val); sum += data[i]; }
    for (size_t i = 0; i < data.size(); ++i) { data[i] /= sum > 1e-9 ? sum : 1.0f; }
}
void save_chunk_to_wav(const std::string& filename, const std::vector<float>& audio_chunk) {
    drwav_data_format format; format.container = drwav_container_riff; format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = 1; format.sampleRate = SAMPLE_RATE; format.bitsPerSample = 32;
    drwav wav; if (!drwav_init_file_write(&wav, filename.c_str(), &format, nullptr)) { std::cerr << "Warning: Failed to init WAV " << filename << std::endl; return; }
    drwav_write_pcm_frames(&wav, audio_chunk.size(), audio_chunk.data());
    drwav_uninit(&wav); std::cout << "  -> Saved detected audio chunk to: " << filename << std::endl;
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
std::vector<float> preprocess_audio_chunk(const std::vector<float>& audio_chunk) {
    std::vector<float> audio = audio_chunk;
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
class HighPassFilter {
public:
    HighPassFilter(float cutoff, float sample_rate) { float rc = 1.0f / (2.0f * M_PI * cutoff); alpha = rc / (rc + 1.0f / sample_rate); y_prev = 0.0f; x_prev = 0.0f; }
    float process(float x) { float y = alpha * (y_prev + x - x_prev); y_prev = y; x_prev = x; return y; }
private: float alpha, y_prev, x_prev;
};
bool is_speech_present(const std::vector<float>& audio_chunk, HighPassFilter& hpf, float threshold) {
    if (audio_chunk.empty()) return false; double sum_sq = 0.0;
    for (float sample : audio_chunk) { float filtered_sample = hpf.process(sample); sum_sq += filtered_sample * filtered_sample; }
    return sqrt(sum_sq / audio_chunk.size()) > threshold;
}
size_t find_energy_center(const std::vector<float>& audio_chunk) {
    const int frame_length = 512; const int hop_length = 256; if (audio_chunk.size() < frame_length) return audio_chunk.size() / 2;
    float max_energy = -1.0f; size_t max_energy_index = 0;
    for (size_t i = 0; i <= audio_chunk.size() - frame_length; i += hop_length) {
        float current_energy = 0.0f; for (size_t j = 0; j < frame_length; ++j) { current_energy += audio_chunk[i + j] * audio_chunk[i + j]; }
        if (current_energy > max_energy) { max_energy = current_energy; max_energy_index = i; }
    } return max_energy_index + frame_length / 2;
}

// ===================================================================
//  MAIN LOGIC with DUAL-MODE HEURISTIC
// ===================================================================
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_wav_file>" << std::endl;
        return 1;
    }

    const std::string onnx_model_path = "../models/smaller_grok.onnx";
    const std::string label_path = "../models/labels.txt";
    const std::string wav_path = argv[1];

    try {
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "GrokCommandDetector");
        Ort::SessionOptions session_options;
        Ort::Session session(env, onnx_model_path.c_str(), session_options);
        auto labels = load_labels(label_path);
        
        unsigned int channels, sr; drwav_uint64 total_frames;
        float* p_sample_data = drwav_open_file_and_read_pcm_frames_f32(wav_path.c_str(), &channels, &sr, &total_frames, nullptr);
        if (!p_sample_data) throw std::runtime_error("Failed to load WAV file: " + wav_path);
        if (sr != SAMPLE_RATE) throw std::runtime_error("Sample rate mismatch. Must be 16kHz.");
        
        std::vector<float> full_audio(total_frames);
        if (channels > 1) { /* ... mono mixdown ... */ } else { std::copy(p_sample_data, p_sample_data + total_frames, full_audio.begin()); }
        drwav_free(p_sample_data, nullptr);

        normalize_audio(full_audio);
        std::cout << "Audio file loaded and normalized: " << wav_path << " (" << full_audio.size() / (float)SAMPLE_RATE << "s)" << std::endl;

        // =================================================================
        // DUAL-MODE LOGIC STARTS HERE
        // =================================================================
        if (full_audio.size() <= OUTPUT_SEQ_LEN) {
            // --- SINGLE FILE MODE ---
            std::cout << "\n--- Short audio file detected. Running in single-file mode. ---" << std::endl;

            // Manually center the audio in a 4-second buffer
            std::vector<float> command_chunk(OUTPUT_SEQ_LEN, 0.0f);
            size_t start_pos = (OUTPUT_SEQ_LEN - full_audio.size()) / 2;
            std::copy(full_audio.begin(), full_audio.end(), command_chunk.begin() + start_pos);

            save_chunk_to_wav("detected_chunk_single.wav", command_chunk);

            // Run prediction once
            std::vector<float> input_data = preprocess_audio_chunk(command_chunk);
            Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
            const int64_t input_shape[] = {1, 1, FINAL_H, FINAL_W};
            Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_data.data(), input_data.size(), input_shape, 4);
            const char* input_names[] = {"input"}; 
            const char* output_names[] = {"output"};
            auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);

            float* logits = output_tensors[0].GetTensorMutableData<float>();
            std::vector<float> probabilities(logits, logits + labels.size());
            apply_softmax(probabilities);
            
            int predicted_id = std::distance(probabilities.begin(), std::max_element(probabilities.begin(), probabilities.end()));
            float confidence = probabilities[predicted_id];

            std::cout << "\n[Single File] Prediction: '" << labels[predicted_id] << "' (Confidence: " 
                      << std::fixed << std::setprecision(2) << confidence * 100.0f << "%)" << std::endl;

        } else {
            // --- COMBINED FILE MODE ---
            std::cout << "\n--- Long audio file detected. Running in VAD-scan mode. ---" << std::endl;
            
            size_t vad_scan_samples = static_cast<size_t>(0.1 * SAMPLE_RATE);
            size_t last_detection_end_sample = 0;
            int detection_count = 0;

            for (size_t current_pos = 0; current_pos + vad_scan_samples < full_audio.size(); ) {
                if (current_pos < last_detection_end_sample) {
                    current_pos = last_detection_end_sample;
                    continue;
                }
                std::vector<float> vad_chunk(&full_audio[current_pos], &full_audio[current_pos + vad_scan_samples]);
                HighPassFilter hpf(VAD_HPF_CUTOFF, SAMPLE_RATE); 
                
                if (is_speech_present(vad_chunk, hpf, VAD_THRESHOLD)) {
                    // ... (This entire block is the same as your working version) ...
                    detection_count++;
                    long timestamp_ms = static_cast<long>(current_pos * 1000.0 / SAMPLE_RATE);
                    size_t search_start = (current_pos > 0.5 * SAMPLE_RATE) ? current_pos - 0.5 * SAMPLE_RATE : 0;
                    size_t search_end = std::min(full_audio.size(), current_pos + (size_t)(1.5 * SAMPLE_RATE));
                    std::vector<float> search_window(&full_audio[search_start], &full_audio[search_end]);
                    size_t energy_center = search_start + find_energy_center(search_window);
                    size_t clip_start = (energy_center > OUTPUT_SEQ_LEN / 2) ? energy_center - OUTPUT_SEQ_LEN / 2 : 0;
                    
                    // This padding logic is important for clips near the end of the file
                    std::vector<float> command_chunk(OUTPUT_SEQ_LEN, 0.0f);
                    size_t copy_len = std::min((size_t)OUTPUT_SEQ_LEN, full_audio.size() - clip_start);
                    std::copy(&full_audio[clip_start], &full_audio[clip_start + copy_len], command_chunk.begin());

                    save_chunk_to_wav("detected_chunk_" + std::to_string(detection_count) + ".wav", command_chunk);
                    std::vector<float> input_data = preprocess_audio_chunk(command_chunk);
                    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
                    const int64_t input_shape[] = {1, 1, FINAL_H, FINAL_W};
                    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_data.data(), input_data.size(), input_shape, 4);
                    const char* input_names[] = {"input"}; 
                    const char* output_names[] = {"output"};
                    auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);
                    float* logits = output_tensors[0].GetTensorMutableData<float>();
                    std::vector<float> probabilities(logits, logits + labels.size());
                    apply_softmax(probabilities);
                    int predicted_id = std::distance(probabilities.begin(), std::max_element(probabilities.begin(), probabilities.end()));
                    float confidence = probabilities[predicted_id];

                    std::cout << "\n[" << timestamp_ms << " ms] Detection #" << detection_count << ": '" << labels[predicted_id] << "' (Confidence: " 
                              << std::fixed << std::setprecision(2) << confidence * 100.0f << "%)" << std::endl;

                    last_detection_end_sample = energy_center + static_cast<size_t>(1.5 * SAMPLE_RATE);
                    current_pos = last_detection_end_sample;
                } else {
                    current_pos += vad_scan_samples;
                }
            }
        }
        std::cout << "\n--- Processing complete. ---" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}