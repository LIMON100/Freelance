#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <cstring>
#include <complex>
#include <csignal>
#include <cstdint> // For int16_t


#include "libs/onnxruntime/include/onnxruntime_cxx_api.h"
#define DR_WAV_IMPLEMENTATION
#include "libs/single_header/dr_wav.h"

#include <fftw3.h>
#include <alsa/asoundlib.h>

// ===================================================================
//  GLOBAL CONFIGURATION CONSTANTS
// ===================================================================

const int SAMPLE_RATE = 16000;
const int OUTPUT_SEQ_LEN = 64000;
const int N_FFT = 512;
const int HOP_LENGTH = 128;
const int FINAL_H = 171;
const int FINAL_W = 560;

const float ENERGY_THRESHOLD = 0.005f; 
const int SILENCE_CHUNKS_NEEDED = 20;

const unsigned int CHUNK_SIZE = 256;
const char* MIC_DEVICE = "hw:1,0";

volatile bool keep_running = true;
void int_handler(int signum) { keep_running = false; }

// ===================================================================
//  HELPER FUNCTIONS
// ===================================================================

std::vector<std::string> load_labels(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file) throw std::runtime_error("Failed to open label file: " + filepath);
    std::vector<std::string> labels;
    std::string line;
    while (std::getline(file, line)) if (!line.empty()) labels.push_back(line);
    return labels;
}

void apply_softmax(std::vector<float>& data) {
    if (data.empty()) return;
    float max_val = *std::max_element(data.begin(), data.end());
    float sum = 0.0f;
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = expf(data[i] - max_val);
        sum += data[i];
    }
    if (sum == 0) sum = 1.0f;
    for (size_t i = 0; i < data.size(); ++i) data[i] /= sum;
}

float calculate_rms(const std::vector<float>& chunk) {
    double sum_sq = 0.0;
    for (float sample : chunk) {
        sum_sq += sample * sample;
    }
    return std::sqrt(sum_sq / chunk.size());
}


// ===================================================================
//  SPECTROGRAM PRE-PROCESSING
// ===================================================================

void bilinear_resize(const std::vector<float>& input, int in_h, int in_w,
                     std::vector<float>& output, int out_h, int out_w) {
    if (in_h <= 1 || in_w <= 1) {
        for(size_t i = 0; i < output.size(); ++i) output[i] = input.empty() ? 0.0f : input[0];
        return;
    }
    float h_ratio = static_cast<float>(in_h - 1) / (out_h - 1);
    float w_ratio = static_cast<float>(in_w - 1) / (out_w - 1);
    for (int i = 0; i < out_h; ++i) {
        for (int j = 0; j < out_w; ++j) {
            float y = i * h_ratio;
            float x = j * w_ratio;
            int y_floor = static_cast<int>(y);
            int x_floor = static_cast<int>(x);
            int y_ceil = std::min(y_floor + 1, in_h - 1);
            int x_ceil = std::min(x_floor + 1, in_w - 1);
            float y_frac = y - y_floor;
            float x_frac = x - x_floor;
            float p00 = input[y_floor * in_w + x_floor];
            float p10 = input[y_floor * in_w + x_ceil];
            float p01 = input[y_ceil * in_w + x_floor];
            float p11 = input[y_ceil * in_w + x_ceil];
            float top = p00 * (1 - x_frac) + p10 * x_frac;
            float bottom = p01 * (1 - x_frac) + p11 * x_frac;
            output[i * out_w + j] = top * (1 - y_frac) + bottom * y_frac;
        }
    }
}

std::vector<float> preprocess_from_buffer(std::vector<float>& audio) {
    // 1. Pad or truncate audio
    if (audio.size() < OUTPUT_SEQ_LEN) {
        audio.resize(OUTPUT_SEQ_LEN, 0.0f);
    } else {
        audio.resize(OUTPUT_SEQ_LEN);
    }

    // 2. Compute STFT (linear spectrogram)
    int n_frames = 1 + (OUTPUT_SEQ_LEN - N_FFT) / HOP_LENGTH;
    int n_freqs = N_FFT / 2 + 1;
    std::vector<float> spectrogram_flat(n_freqs * n_frames);

    std::vector<float> window(N_FFT);
    for(int i = 0; i < N_FFT; ++i) window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (N_FFT - 1)));

    std::vector<float> frame(N_FFT);
    std::vector<std::complex<float>> fft_out(n_freqs);
    fftwf_plan plan = fftwf_plan_dft_r2c_1d(N_FFT, frame.data(), reinterpret_cast<fftwf_complex*>(fft_out.data()), FFTW_ESTIMATE);

    for (int i = 0; i < n_frames; ++i) {
        int start = i * HOP_LENGTH;
        for (int j = 0; j < N_FFT; ++j) {
            frame[j] = audio[start + j] * window[j];
        }
        fftwf_execute(plan);
        for (int j = 0; j < n_freqs; ++j) {
            spectrogram_flat[j * n_frames + i] = std::abs(fft_out[j]);
        }
    }
    fftwf_destroy_plan(plan);

    // 3. Resize the spectrogram
    std::vector<float> final_input(FINAL_H * FINAL_W);
    bilinear_resize(spectrogram_flat, n_freqs, n_frames, final_input, FINAL_H, FINAL_W);
    
    return final_input;
}


// ===================================================================
//  MAIN APPLICATION LOGIC
// ===================================================================

int main() {
    signal(SIGINT, int_handler);

    const std::string onnx_model_path = "../models/smaller_grok.onnx";
    const std::string label_path = "../models/labels.txt";

    try {
        // --- 1. Load Models and Labels ---
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "RealTimePredictor");
        Ort::SessionOptions session_options;
        Ort::Session session(env, onnx_model_path.c_str(), session_options);
        auto labels = load_labels(label_path);
        
        // --- 2. Initialize ALSA for Microphone Capture ---
        snd_pcm_t *capture_handle;
        snd_pcm_hw_params_t *hw_params;
        int err;

        if ((err = snd_pcm_open(&capture_handle, MIC_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
            throw std::runtime_error("Cannot open audio device " + std::string(MIC_DEVICE) + ": " + snd_strerror(err));
        }
        
        snd_pcm_hw_params_alloca(&hw_params);
        snd_pcm_hw_params_any(capture_handle, hw_params);
        
        snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
        
        // --- THE FIRST FIX: Request 16-bit integers, not floats ---
        snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S16_LE);
        
        unsigned int sample_rate_alsa = SAMPLE_RATE;
        snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &sample_rate_alsa, 0);
        snd_pcm_hw_params_set_channels(capture_handle, hw_params, 1);
        
        if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
            throw std::runtime_error("Cannot set hw parameters: " + std::string(snd_strerror(err)));
        }
        
        std::cout << "Microphone initialized successfully. Listening for commands... (Press Ctrl+C to exit)" << std::endl;

        // --- 3. Main Loop ---
        enum VadState { SILENCE, VOICE, WAITING_FOR_SILENCE };
        VadState state = SILENCE;
        std::vector<float> command_buffer;
        int silence_counter = 0;

        while (keep_running) {
            // --- THE SECOND FIX: Read into an integer buffer and convert to float ---
            std::vector<int16_t> chunk_buffer_int(CHUNK_SIZE);
            err = snd_pcm_readi(capture_handle, chunk_buffer_int.data(), CHUNK_SIZE);
            
            if (err == -EPIPE) {
                fprintf(stderr, "Overrun occurred\n");
                snd_pcm_prepare(capture_handle);
                continue;
            } else if (err < 0) {
                fprintf(stderr, "Error from read: %s\n", snd_strerror(err));
                continue;
            }

            // Convert S16 integer to Float (-1.0 to 1.0)
            std::vector<float> chunk_buffer_float(CHUNK_SIZE);
            for(int i = 0; i < CHUNK_SIZE; ++i) {
                chunk_buffer_float[i] = static_cast<float>(chunk_buffer_int[i]) / 32768.0f;
            }
            // --- END OF FIX ---

            // Now, the rest of the code uses the correctly converted float buffer
            float rms = calculate_rms(chunk_buffer_float);

            switch (state) {
                case SILENCE:
                    if (rms > ENERGY_THRESHOLD) {
                        std::cout << "\nVoice detected, recording..." << std::flush;
                        state = VOICE;
                        command_buffer.insert(command_buffer.end(), chunk_buffer_float.begin(), chunk_buffer_float.end());
                    }
                    break;
                
                case VOICE:
                    command_buffer.insert(command_buffer.end(), chunk_buffer_float.begin(), chunk_buffer_float.end());
                    if (rms < ENERGY_THRESHOLD) {
                        state = WAITING_FOR_SILENCE;
                        silence_counter = 1;
                    }
                    break;

                case WAITING_FOR_SILENCE:
                    command_buffer.insert(command_buffer.end(), chunk_buffer_float.begin(), chunk_buffer_float.end());
                    if (rms < ENERGY_THRESHOLD) {
                        silence_counter++;
                    } else {
                        state = VOICE;
                        silence_counter = 0;
                    }

                    if (silence_counter >= SILENCE_CHUNKS_NEEDED) {
                        std::cout << " End of speech detected. Processing..." << std::endl;

                        std::vector<float> input_spectrogram = preprocess_from_buffer(command_buffer);
                        
                        // (The rest of the ONNX inference logic is the same and correct)
                        // ...
                        
                        std::cout << "\nListening for commands..." << std::endl;
                        command_buffer.clear();
                        state = SILENCE;
                    }
                    break;
            }
        }

        std::cout << "\nExiting..." << std::endl;
        snd_pcm_close(capture_handle);

    } catch (const std::exception& e) {
        std::cerr << "A critical error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}






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

//     const std::string onnx_model_path = "smaller_grok.onnx";
//     const std::string label_path = "labels.txt";
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