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
#include <deque>
#include <chrono>
#include <thread>

// ONNX Runtime API
#include "libs/onnxruntime/include/onnxruntime_cxx_api.h"
// FFTW Library
#include <fftw3.h>

// ===================================================================
//  CONFIGURATION CONSTANTS (TUNE THESE)
// ===================================================================
const double ENERGY_THRESHOLD = 1000000.0; 
const float SILENCE_DURATION_S = 0.8f; 
const float PRE_SPEECH_BUFFER_S = 0.25f; 
const float MIN_UTTERANCE_S = 0.3f;
const int CHUNK_DURATION_MS = 20;         
const int SAMPLE_RATE = 16000;
const int OUTPUT_SEQ_LEN = 64000;
const int N_FFT = 512;
const int HOP_LENGTH = 128;
const int FINAL_H = 171;
const int FINAL_W = 560;

// ===================================================================
//  WAV FILE & VAD HELPERS
// ===================================================================
struct WavHeader {
    char riff[4]; uint32_t file_size; char wave[4]; char fmt[4];
    uint32_t fmt_size; uint16_t audio_format; uint16_t num_channels;
    uint32_t sample_rate; uint32_t byte_rate; uint16_t block_align;
    uint16_t bits_per_sample; char data[4]; uint32_t data_size;
};
bool read_wav_header(std::ifstream& file, WavHeader& header) {
    file.seekg(0);
    return file.read(reinterpret_cast<char*>(&header), 44).good();
}
double compute_frame_energy(const std::vector<int16_t>& frame) {
    double energy = 0.0;
    if (frame.empty()) return 0.0;
    for (const auto& sample : frame) {
        energy += static_cast<double>(sample) * sample;
    }
    return energy / frame.size();
}

// ===================================================================
//  ALL HELPER & PRE-PROCESSING FUNCTIONS
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

void bilinear_resize(const std::vector<float>& input, int in_h, int in_w,
                     std::vector<float>& output, int out_h, int out_w) {
    if (in_h <= 1 || in_w <= 1) {
        for (size_t i = 0; i < output.size(); ++i) output[i] = input.empty() ? 0.0f : input[0];
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

std::vector<float> preprocess_audio(std::vector<float>& audio) {
    if (audio.size() < OUTPUT_SEQ_LEN) {
        audio.resize(OUTPUT_SEQ_LEN, 0.0f);
    } else {
        audio.resize(OUTPUT_SEQ_LEN);
    }
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
    std::vector<float> final_input(FINAL_H * FINAL_W);
    bilinear_resize(spectrogram_flat, n_freqs, n_frames, final_input, FINAL_H, FINAL_W);
    return final_input;
}


// ===================================================================
//  MAIN INFERENCE LOGIC (WITH TRUE REAL-TIME BEHAVIOR)
// ===================================================================
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_merged_wav_file>" << std::endl;
        return 1;
    }
    const std::string onnx_model_path = "../models/grok3.onnx";
    const std::string label_path = "../models/labels.txt";
    const std::string wav_path = argv[1];

    try {
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "StreamingPredictor");
        Ort::SessionOptions session_options;
        Ort::Session session(env, onnx_model_path.c_str(), session_options);
        auto labels = load_labels(label_path);
        
        std::ifstream audio_file(wav_path, std::ios::binary);
        if (!audio_file.is_open()) throw std::runtime_error("Failed to open WAV file");
        WavHeader header;
        if (!read_wav_header(audio_file, header) || header.sample_rate != SAMPLE_RATE || header.bits_per_sample != 16) {
            throw std::runtime_error("WAV must be 16-bit, 16kHz Mono PCM");
        }
        std::cout << "Opened " << wav_path << " for real-time simulation." << std::endl;

        const size_t CHUNK_SAMPLES = SAMPLE_RATE * CHUNK_DURATION_MS / 1000;
        const int SILENCE_CHUNKS_NEEDED = static_cast<int>(SILENCE_DURATION_S * 1000 / CHUNK_DURATION_MS);
        const size_t PRE_SPEECH_SAMPLES = static_cast<size_t>(PRE_SPEECH_BUFFER_S * SAMPLE_RATE);

        enum State { LISTENING, SPEAKING };
        State current_state = LISTENING;
        
        int silence_chunk_counter = 0;
        int silence_periods_detected = 0;
        int detected_utterances = 0;
        int listening_heartbeat_counter = 0;

        std::vector<int16_t> speech_buffer_int16;
        std::deque<int16_t> history_buffer; 

        std::cout << "\n--- Starting Real-Time Continuous Detection ---" << std::endl;
        std::cout << "Listening..." << std::flush;

        auto target_chunk_duration = std::chrono::milliseconds(CHUNK_DURATION_MS);
        std::vector<int16_t> current_chunk(CHUNK_SAMPLES);
        while (audio_file.read(reinterpret_cast<char*>(current_chunk.data()), CHUNK_SAMPLES * sizeof(int16_t))) {
            auto loop_start_time = std::chrono::high_resolution_clock::now();
            double energy = compute_frame_energy(current_chunk);

            if (current_state == LISTENING) {
                history_buffer.insert(history_buffer.end(), current_chunk.begin(), current_chunk.end());
                while(history_buffer.size() > PRE_SPEECH_SAMPLES) {
                    history_buffer.pop_front();
                }
                if (energy > ENERGY_THRESHOLD) {
                    silence_periods_detected++;
                    std::cout << "\nSpeech started..." << std::flush;
                    speech_buffer_int16.assign(history_buffer.begin(), history_buffer.end());
                    current_state = SPEAKING;
                    silence_chunk_counter = 0;
                } else {
                    // This is the "heartbeat" to show it's alive during silence
                    listening_heartbeat_counter++;
                    if (listening_heartbeat_counter * CHUNK_DURATION_MS >= 1000) {
                        std::cout << "." << std::flush;
                        listening_heartbeat_counter = 0;
                    }
                }
            } else if (current_state == SPEAKING) {
                speech_buffer_int16.insert(speech_buffer_int16.end(), current_chunk.begin(), current_chunk.end());
                if (energy < ENERGY_THRESHOLD) {
                    silence_chunk_counter++;
                } else {
                    silence_chunk_counter = 0;
                }

                if (silence_chunk_counter >= SILENCE_CHUNKS_NEEDED) {
                    float utterance_duration = (float)speech_buffer_int16.size() / SAMPLE_RATE;
                    if (utterance_duration >= MIN_UTTERANCE_S) {
                        detected_utterances++;
                        std::cout << "\nSpeech ended. Processing utterance #" << detected_utterances 
                                  << " (" << std::fixed << std::setprecision(2) << utterance_duration << "s)..." << std::flush;
                        
                        std::vector<float> utterance_to_process;
                        utterance_to_process.reserve(speech_buffer_int16.size());
                        for (int16_t sample : speech_buffer_int16) {
                            utterance_to_process.push_back(static_cast<float>(sample) / 32768.0f);
                        }
                        std::vector<float> input_data = preprocess_audio(utterance_to_process);
                        
                        Ort::MemoryInfo mem_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
                        const int64_t input_shape[] = {1, 1, FINAL_H, FINAL_W};
                        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(mem_info, input_data.data(), input_data.size(), input_shape, 4);
                        const char* input_names[] = {"input"}; 
                        const char* output_names[] = {"output"};
                        auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);
                        
                        float* logits = output_tensors[0].GetTensorMutableData<float>();
                        std::vector<float> probabilities(logits, logits + labels.size());
                        apply_softmax(probabilities);
                        int pred_id = std::distance(probabilities.begin(), std::max_element(probabilities.begin(), probabilities.end()));
                        
                        std::cout << "\n>>> Predicted Command: '" << labels[pred_id] << "' (Confidence: " 
                                  << std::fixed << std::setprecision(2) << probabilities[pred_id] * 100.0f << "%)" << std::endl;
                    } else {
                         std::cout << "\nSpeech ended, but was too short (" << utterance_duration << "s). Ignoring." << std::endl;
                    }
                    current_state = LISTENING;
                    speech_buffer_int16.clear();
                    std::cout << "\nListening..." << std::flush;
                    listening_heartbeat_counter = 0;
                }
            }
            
            auto loop_end_time = std::chrono::high_resolution_clock::now();
            auto processing_duration = std::chrono::duration_cast<std::chrono::milliseconds>(loop_end_time - loop_start_time);
            if (processing_duration < target_chunk_duration) {
                std::this_thread::sleep_for(target_chunk_duration - processing_duration);
            }
        }
        std::cout << "\n--- End of Audio Stream ---" << std::endl;
        std::cout << "Summary:" << std::endl;
        std::cout << "  - Total Utterances Processed: " << detected_utterances << std::endl;
        std::cout << "  - Total Silence Periods Detected: " << silence_periods_detected << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
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
// #include <cstring>
// #include <complex>
// #include <deque>
// #include <chrono>
// #include <thread>
// #include <map>

// // ONNX Runtime API
// #include "libs/onnxruntime/include/onnxruntime_cxx_api.h"
// // WAV Loading
// #define DR_WAV_IMPLEMENTATION
// #include "libs/single_header/dr_wav.h"
// // FFTW Library
// #include <fftw3.h>

// // Adjusted VAD parameters
// const double ENERGY_THRESHOLD = 1200000.0; // Increased to reduce false positives
// const int VAD_WINDOW_SIZE = 5;
// const int VAD_VOICED_REQUIRED = 4; // Stricter: require 4/5 chunks voiced
// const int VAD_SILENT_REQUIRED = 4; // Stricter: require 4/5 chunks silent
// const float MIN_UTTERANCE_S = 0.5f; // Increased to filter short utterances
// const float PRE_SPEECH_BUFFER_S = 0.25f;
// const int CHUNK_DURATION_MS = 20;
// const float CONFIDENCE_THRESHOLD = 0.3f; // Minimum confidence for predictions

// // Spectrogram & Model Parameters
// const int SAMPLE_RATE = 16000;
// const int OUTPUT_SEQ_LEN = 64000;
// const int N_FFT = 512;
// const int HOP_LENGTH = 128;
// const int FINAL_H = 171;
// const int FINAL_W = 560;

// // Forward Declarations
// std::vector<std::string> load_labels(const std::string& filepath);
// void apply_softmax(std::vector<float>& data);
// void bilinear_resize(const std::vector<float>& input, int in_h, int in_w, std::vector<float>& output, int out_h, int out_w);
// std::vector<float> preprocess_audio(std::vector<float>& audio);

// struct Utterance {
//     size_t start_sample;
//     size_t end_sample;
// };

// struct WavHeader {
//     char riff[4]; uint32_t file_size; char wave[4]; char fmt[4];
//     uint32_t fmt_size; uint16_t audio_format; uint16_t num_channels;
//     uint32_t sample_rate; uint32_t byte_rate; uint16_t block_align;
//     uint16_t bits_per_sample; char data[4]; uint32_t data_size;
// };
// bool read_wav_header(std::ifstream& file, WavHeader& header) { return file.read(header.riff, 44).good(); }
// double compute_frame_energy(const std::vector<int16_t>& frame) {
//     double energy = 0.0;
//     for (const auto& sample : frame) energy += static_cast<double>(sample) * sample;
//     return frame.empty() ? 0.0 : energy / frame.size();
// }

// int main(int argc, char* argv[]) {
//     if (argc != 2) {
//         std::cerr << "Usage: " << argv[0] << " <path_to_merged_wav_file>" << std::endl;
//         return 1;
//     }
//     const std::string onnx_model_path = "../models/groknewmodel.onnx";
//     const std::string label_path = "../models/labels.txt";
//     const std::string wav_path = argv[1];

//     // Expected labels for comparison
//     std::vector<std::string> expected_labels = {
//         "100미터", "10미터", "12시", "200미터", "20미터", "300미터", "40미터", "60미터", "6시", "80미터",
//         "9시", "IR_켜짐", "공격_모드", "드론", "매우_빠름", "아래에", "야간_모드", "운전_모드", "정찰_모드"
//     };

//     try {
//         Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "StreamingPredictor");
//         Ort::SessionOptions session_options;
//         Ort::Session session(env, onnx_model_path.c_str(), session_options);
//         auto labels = load_labels(label_path);

//         std::ifstream audio_file(wav_path, std::ios::binary);
//         if (!audio_file.is_open()) throw std::runtime_error("Failed to open WAV file");
//         WavHeader header;
//         if (!read_wav_header(audio_file, header) || header.sample_rate != SAMPLE_RATE || header.bits_per_sample != 16) {
//             throw std::runtime_error("WAV must be 16-bit, 16kHz Mono PCM");
//         }
//         std::cout << "Opened " << wav_path << " for real-time simulation." << std::endl;

//         const size_t CHUNK_SAMPLES = SAMPLE_RATE * CHUNK_DURATION_MS / 1000;
//         const size_t PRE_SPEECH_SAMPLES = static_cast<size_t>(PRE_SPEECH_BUFFER_S * SAMPLE_RATE);

//         enum State { LISTENING, SPEAKING };
//         State current_state = LISTENING;

//         int detected_utterances = 0;
//         int silence_periods_detected = 0;
//         std::vector<std::string> detected_labels;

//         std::vector<int16_t> speech_buffer_int16;
//         std::deque<int16_t> history_buffer;
//         std::deque<bool> vad_window;

//         std::cout << "\n--- Starting Real-Time Continuous Detection ---" << std::endl;
//         auto target_chunk_duration = std::chrono::milliseconds(CHUNK_DURATION_MS);

//         std::vector<int16_t> current_chunk(CHUNK_SAMPLES);
//         while (audio_file.read(reinterpret_cast<char*>(current_chunk.data()), CHUNK_SAMPLES * sizeof(int16_t))) {
//             auto loop_start_time = std::chrono::high_resolution_clock::now();

//             double energy = compute_frame_energy(current_chunk);
//             bool is_voiced = energy > ENERGY_THRESHOLD;

//             vad_window.push_back(is_voiced);
//             if (vad_window.size() > VAD_WINDOW_SIZE) {
//                 vad_window.pop_front();
//             }

//             int voiced_count = 0;
//             for (bool v : vad_window) {
//                 if (v) voiced_count++;
//             }
//             int silent_count = VAD_WINDOW_SIZE - voiced_count;

//             if (current_state == LISTENING) {
//                 for (int16_t sample : current_chunk) history_buffer.push_back(sample);
//                 while (history_buffer.size() > PRE_SPEECH_SAMPLES) {
//                     history_buffer.pop_front();
//                 }

//                 if (voiced_count >= VAD_VOICED_REQUIRED) {
//                     silence_periods_detected++;
//                     std::cout << "\n[Silence #" << silence_periods_detected << "] --> Speech started..." << std::endl;

//                     speech_buffer_int16.assign(history_buffer.begin(), history_buffer.end());
//                     current_state = SPEAKING;
//                 }
//             } else if (current_state == SPEAKING) {
//                 speech_buffer_int16.insert(speech_buffer_int16.end(), current_chunk.begin(), current_chunk.end());

//                 if (silent_count >= VAD_SILENT_REQUIRED) {
//                     float utterance_duration = (float)speech_buffer_int16.size() / SAMPLE_RATE;
//                     if (utterance_duration >= MIN_UTTERANCE_S) {
//                         detected_utterances++;
//                         std::cout << "Speech ended. Processing utterance #" << detected_utterances
//                                   << " (" << std::fixed << std::setprecision(2) << utterance_duration << "s)..." << std::endl;

//                         // Normalize audio
//                         std::vector<float> utterance_to_process;
//                         utterance_to_process.reserve(speech_buffer_int16.size());
//                         float max_amplitude = 0.0f;
//                         for (int16_t sample : speech_buffer_int16) {
//                             float val = static_cast<float>(sample) / 32768.0f;
//                             max_amplitude = std::max(max_amplitude, std::abs(val));
//                             utterance_to_process.push_back(val);
//                         }
//                         if (max_amplitude > 0.0f) {
//                             for (auto& sample : utterance_to_process) {
//                                 sample /= max_amplitude;
//                             }
//                         }

//                         std::vector<float> input_data = preprocess_audio(utterance_to_process);

//                         Ort::MemoryInfo mem_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
//                         const int64_t input_shape[] = {1, 1, FINAL_H, FINAL_W};
//                         Ort::Value input_tensor = Ort::Value::CreateTensor<float>(mem_info, input_data.data(), input_data.size(), input_shape, 4);
//                         const char* input_names[] = {"input"};
//                         const char* output_names[] = {"output"};
//                         auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);

//                         float* logits = output_tensors[0].GetTensorMutableData<float>();
//                         std::vector<float> probabilities(logits, logits + labels.size());
//                         apply_softmax(probabilities);
//                         int pred_id = std::distance(probabilities.begin(), std::max_element(probabilities.begin(), probabilities.end()));

//                         if (probabilities[pred_id] >= CONFIDENCE_THRESHOLD) {
//                             detected_labels.push_back(labels[pred_id]);
//                             std::cout << ">>> Predicted Command: '" << labels[pred_id] << "' (Confidence: "
//                                       << std::fixed << std::setprecision(2) << probabilities[pred_id] * 100.0f << "%)" << std::endl;
//                         } else {
//                             detected_labels.push_back("Low Confidence");
//                             std::cout << ">>> Prediction ignored due to low confidence: '" << labels[pred_id]
//                                       << "' (Confidence: " << std::fixed << std::setprecision(2) << probabilities[pred_id] * 100.0f << "%)" << std::endl;
//                         }
//                     } else {
//                         std::cout << "Speech ended, but utterance was too short (" << utterance_duration << "s). Ignoring." << std::endl;
//                     }

//                     current_state = LISTENING;
//                     speech_buffer_int16.clear();
//                 }
//             }

//             auto loop_end_time = std::chrono::high_resolution_clock::now();
//             auto processing_duration = std::chrono::duration_cast<std::chrono::milliseconds>(loop_end_time - loop_start_time);
//             if (processing_duration < target_chunk_duration) {
//                 std::this_thread::sleep_for(target_chunk_duration - processing_duration);
//             }
//         }

//         // Compare detected labels with expected labels
//         std::cout << "\n--- Label Comparison ---" << std::endl;
//         std::cout << "Expected Labels: " << expected_labels.size() << ", Detected Labels: " << detected_labels.size() << std::endl;
//         for (size_t i = 0; i < std::min(detected_labels.size(), expected_labels.size()); ++i) {
//             if (detected_labels[i] != expected_labels[i]) {
//                 std::cout << "Mismatch at #" << i + 1 << ": Expected '" << expected_labels[i]
//                           << "', Got '" << detected_labels[i] << "'" << std::endl;
//             }
//         }
//         if (detected_labels.size() > expected_labels.size()) {
//             std::cout << "Extra detections: ";
//             for (size_t i = expected_labels.size(); i < detected_labels.size(); ++i) {
//                 std::cout << "'" << detected_labels[i] << "' ";
//             }
//             std::cout << std::endl;
//         }

//         std::cout << "\n--- End of Audio Stream ---" << std::endl;
//         std::cout << "Summary:" << std::endl;
//         std::cout << "  - Total Utterances Processed: " << detected_utterances << std::endl;
//         std::cout << "  - Total Silence Periods Detected: " << silence_periods_detected << std::endl;
//     } catch (const std::exception& e) {
//         std::cerr << "An error occurred: " << e.what() << std::endl;
//         return 1;
//     }
//     return 0;
// }

// std::vector<std::string> load_labels(const std::string& filepath) {
//     std::ifstream file(filepath);
//     if (!file) throw std::runtime_error("Failed to open label file: " + filepath);
//     std::vector<std::string> labels;
//     std::string line;
//     while (std::getline(file, line)) if (!line.empty()) labels.push_back(line);
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
//     if (sum == 0) sum = 1.0f;
//     for (size_t i = 0; i < data.size(); ++i) data[i] /= sum;
// }

// void bilinear_resize(const std::vector<float>& input, int in_h, int in_w,
//                      std::vector<float>& output, int out_h, int out_w) {
//     if (in_h <= 1 || in_w <= 1) {
//         for (size_t i = 0; i < output.size(); ++i) output[i] = input.empty() ? 0.0f : input[0];
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

// std::vector<float> preprocess_audio(std::vector<float>& audio) {
//     if (audio.size() < OUTPUT_SEQ_LEN) {
//         audio.resize(OUTPUT_SEQ_LEN, 0.0f);
//     } else {
//         audio.resize(OUTPUT_SEQ_LEN);
//     }
//     int n_frames = 1 + (OUTPUT_SEQ_LEN - N_FFT) / HOP_LENGTH;
//     int n_freqs = N_FFT / 2 + 1;
//     std::vector<float> spectrogram_flat(n_freqs * n_frames);
//     std::vector<float> window(N_FFT);
//     for (int i = 0; i < N_FFT; ++i) window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (N_FFT - 1)));
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
//     std::vector<float> final_input(FINAL_H * FINAL_W);
//     bilinear_resize(spectrogram_flat, n_freqs, n_frames, final_input, FINAL_H, FINAL_W);
//     return final_input;
// }