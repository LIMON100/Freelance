#include <iostream>
#include <vector>
#include <string>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <stdexcept>
#include <chrono>
#include <map>

// HailoRT C++ API
#include <hailo/hailort.hpp>
#include <hailo/vstream.hpp>

// OpenCV for resizing
#include <opencv2/opencv.hpp>

// FFTW Library for spectrogram
#include <fftw3.h>

// WAV Loading (single header library)
#define DR_WAV_IMPLEMENTATION
#include "libs/dr_wav.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace hailort;

// ===================================================================
//  CONFIGURATION
// ===================================================================
namespace config {
    const int SAMPLE_RATE = 16000;
    const int OUTPUT_SEQ_LEN_SAMPLES = 64000;
    const int N_FFT = 512;
    const int HOP_LENGTH = 128;
    const int FINAL_H = 171;
    const int FINAL_W = 560;
    const float VAD_THRESHOLD = 0.015f;
    const float VAD_HPF_CUTOFF = 100.0f;

    const std::vector<std::string> CLASS_NAMES = {
        "100미터", "10미터", "10시", "11시", "12시", "1미터", "1시", "200미터", 
        "20미터", "2시", "300미터", "3미터", "3시", "400미터", "40미터", "5미터", 
        "60미터", "6시", "80미터", "9시", "경계모드", "공격모드", "느리게", 
        "대기모드", "드론 경계 모드", "매우느리게", "매우빠르게", "멈춰", "복귀", 
        "빠르게", "사격", "아래", "야간모드", "우측", "우회전", "위", "전방", 
        "전진", "정지", "정찰", "정찰모드", "조명꺼", "조명켜", "조준", "좌측", 
        "좌회전", "주행모드", "출발", "후방", "후진"
    };
}

// ===================================================================
//  HELPER, VAD, & PRE-PROCESSING FUNCTIONS (These are correct)
// ===================================================================
void normalize_audio(std::vector<float>& audio) {
    if (audio.empty()) return;
    float max_abs_val = 0.0f;
    for (float sample : audio) { max_abs_val = std::max(max_abs_val, std::abs(sample)); }
    if (max_abs_val > 1e-5) { for (float& sample : audio) { sample /= max_abs_val; } }
}

class HighPassFilter {
public:
    HighPassFilter(float cutoff, float sample_rate) {
        float rc = 1.0f / (2.0f * M_PI * cutoff);
        alpha = rc / (rc + 1.0f / sample_rate);
        y_prev = 0.0f; x_prev = 0.0f;
    }
    float process(float x) {
        float y = alpha * (y_prev + x - x_prev);
        y_prev = y; x_prev = x; return y;
    }
private: float alpha, y_prev, x_prev;
};

bool is_speech_present(const std::vector<float>& audio_chunk) {
    if (audio_chunk.empty()) return false;
    HighPassFilter hpf(config::VAD_HPF_CUTOFF, config::SAMPLE_RATE);
    double sum_sq = 0.0;
    for (float sample : audio_chunk) { sum_sq += std::pow(hpf.process(sample), 2); }
    return std::sqrt(sum_sq / audio_chunk.size()) > config::VAD_THRESHOLD;
}

size_t find_energy_center(const std::vector<float>& audio_chunk) {
    const int frame_length = 512; const int hop_length = 256;
    if (audio_chunk.size() < frame_length) return audio_chunk.size() / 2;
    float max_energy = -1.0f; size_t max_energy_index = 0;
    for (size_t i = 0; i <= audio_chunk.size() - frame_length; i += hop_length) {
        float current_energy = 0.0f;
        for (size_t j = 0; j < frame_length; ++j) { current_energy += audio_chunk[i + j] * audio_chunk[i + j]; }
        if (current_energy > max_energy) { max_energy = current_energy; max_energy_index = i; }
    } return max_energy_index + frame_length / 2;
}

std::vector<float> preprocess_audio_chunk(const std::vector<float>& audio_chunk) {
    std::vector<float> chunk_copy = audio_chunk;
    if (chunk_copy.size() < config::OUTPUT_SEQ_LEN_SAMPLES) {
        chunk_copy.resize(config::OUTPUT_SEQ_LEN_SAMPLES, 0.0f);
    }
    
    int n_frames = 1 + (config::OUTPUT_SEQ_LEN_SAMPLES - config::N_FFT) / config::HOP_LENGTH;
    int n_freqs = config::N_FFT / 2 + 1;
    cv::Mat magnitude_spectrogram(n_freqs, n_frames, CV_32F);
    
    std::vector<float> window(config::N_FFT);
    for (int i = 0; i < config::N_FFT; ++i) { window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (config::N_FFT - 1))); }
    
    std::vector<float> frame(config::N_FFT);
    std::vector<fftwf_complex> fft_out(n_freqs);
    fftwf_plan plan = fftwf_plan_dft_r2c_1d(config::N_FFT, frame.data(), fft_out.data(), FFTW_ESTIMATE);

    for (int i = 0; i < n_frames; ++i) {
        int start = i * config::HOP_LENGTH;
        for (int j = 0; j < config::N_FFT; ++j) { frame[j] = chunk_copy[start + j] * window[j]; }
        fftwf_execute(plan);
        for (int j = 0; j < n_freqs; ++j) {
            magnitude_spectrogram.at<float>(j, i) = std::sqrt(fft_out[j][0] * fft_out[j][0] + fft_out[j][1] * fft_out[j][1]);
        }
    }
    fftwf_destroy_plan(plan);

    cv::Mat resized_spectrogram;
    cv::resize(magnitude_spectrogram, resized_spectrogram, cv::Size(config::FINAL_W, config::FINAL_H), 0, 0, cv::INTER_LINEAR);

    std::vector<float> final_input(config::FINAL_H * config::FINAL_W);
    if (resized_spectrogram.isContinuous()) {
        memcpy(final_input.data(), resized_spectrogram.data, final_input.size() * sizeof(float));
    } else {
        for (int row = 0; row < resized_spectrogram.rows; ++row) {
            memcpy(final_input.data() + row * resized_spectrogram.cols, resized_spectrogram.ptr<float>(row), resized_spectrogram.cols * sizeof(float));
        }
    }
    return final_input;
}

std::pair<std::string, int> postprocess_output(const std::vector<float>& logits) {
    auto max_it = std::max_element(logits.begin(), logits.end());
    int predicted_index = std::distance(logits.begin(), max_it);
    return {config::CLASS_NAMES[predicted_index], predicted_index};
}

// ===================================================================
//  MAIN LOGIC
// ===================================================================
int main(int argc, char* argv[]) {
    // --- THIS IS THE FIX: Check for 3 arguments now ---
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <path_to_hef_file> <path_to_wav_file>" << std::endl;
        return 1;
    }
    
    // --- THIS IS THE FIX: Get paths from arguments ---
    const std::string hef_path = argv[1];
    const std::string wav_path = argv[2];
    
    std::cout << "Loaded " << config::CLASS_NAMES.size() << " class names." << std::endl;

    try {
        unsigned int channels, sr; drwav_uint64 total_frames;
        float* p_sample_data = drwav_open_file_and_read_pcm_frames_f32(wav_path.c_str(), &channels, &sr, &total_frames, nullptr);
        if (!p_sample_data) throw std::runtime_error("Failed to load WAV file");
        if (sr != config::SAMPLE_RATE) throw std::runtime_error("Sample rate mismatch!");
        std::vector<float> full_audio(p_sample_data, p_sample_data + total_frames);
        drwav_free(p_sample_data, nullptr);
        normalize_audio(full_audio);
        std::cout << "Audio file loaded and normalized: " << wav_path << " (" << full_audio.size() / (float)config::SAMPLE_RATE << "s)" << std::endl;

        auto hef = Hef::create(hef_path); // Use the argument here
        if (!hef) { std::cerr << "Failed to create HEF from path: " << hef_path << " (Error: " << hef.status() << ")" << std::endl; return 1; }

        auto vdevice = VDevice::create();
        if (!vdevice) { std::cerr << "Failed to create VDevice: " << vdevice.status() << std::endl; return 1; }

        auto configure_params = hef->create_configure_params(HAILO_STREAM_INTERFACE_PCIE);
        if (!configure_params) { std::cerr << "Failed to create configure_params: " << configure_params.status() << std::endl; return 1; }

        auto network_groups = vdevice.value()->configure(*hef, configure_params.value());
        if (!network_groups) { std::cerr << "Failed to configure network_groups: " << network_groups.status() << std::endl; return 1; }
        
        auto& net_group = *(network_groups.value().at(0));

        auto vstreams = VStreamsBuilder::create_vstreams(net_group, true, HAILO_FORMAT_TYPE_FLOAT32);
        if (!vstreams) { std::cerr << "Failed to create vstreams: " << vstreams.status() << std::endl; return 1; }
        
        auto& input_vstream = vstreams->first[0];
        auto& output_vstream = vstreams->second[0];

        std::cout << "\nHailo pipeline initialized. Starting command detection loop..." << std::endl;

        size_t vad_scan_samples = static_cast<size_t>(0.1 * config::SAMPLE_RATE);
        size_t last_detection_end_sample = 0;
        int detection_count = 0;
        size_t current_pos = 0;

        while (current_pos + vad_scan_samples < full_audio.size()) {
            if (current_pos < last_detection_end_sample) { current_pos = last_detection_end_sample; continue; }
            std::vector<float> vad_chunk(full_audio.begin() + current_pos, full_audio.begin() + current_pos + vad_scan_samples);

            if (is_speech_present(vad_chunk)) {
                detection_count++;
                long timestamp_ms = static_cast<long>(current_pos * 1000.0 / config::SAMPLE_RATE);
                std::cout << "\n[" << timestamp_ms << " ms] Speech detected! (Detection #" << detection_count << ")" << std::endl;

                size_t search_start = (current_pos > 0.5 * config::SAMPLE_RATE) ? current_pos - 0.5 * config::SAMPLE_RATE : 0;
                size_t search_end = std::min(full_audio.size(), current_pos + static_cast<size_t>(1.5 * config::SAMPLE_RATE));
                std::vector<float> search_window(full_audio.begin() + search_start, full_audio.begin() + search_end);
                size_t energy_center = search_start + find_energy_center(search_window);
                
                size_t clip_start = (energy_center > config::OUTPUT_SEQ_LEN_SAMPLES / 2) ? energy_center - config::OUTPUT_SEQ_LEN_SAMPLES / 2 : 0;
                std::vector<float> command_chunk;
                size_t copy_len = std::min((size_t)config::OUTPUT_SEQ_LEN_SAMPLES, full_audio.size() - clip_start);
                command_chunk.assign(full_audio.begin() + clip_start, full_audio.begin() + clip_start + copy_len);
                if (command_chunk.size() < config::OUTPUT_SEQ_LEN_SAMPLES) {
                    command_chunk.resize(config::OUTPUT_SEQ_LEN_SAMPLES, 0.0f);
                }

                std::vector<float> input_data = preprocess_audio_chunk(command_chunk);
                std::vector<float> output_buffer(output_vstream.get_frame_size() / sizeof(float));

                hailo_status status = input_vstream.write({input_data.data(), input_data.size() * sizeof(float)});
                if (status != HAILO_SUCCESS) { std::cerr << "  -> HailoRT input write Error: " << status << std::endl; continue; }

                status = output_vstream.read({output_buffer.data(), output_buffer.size() * sizeof(float)});
                if (status != HAILO_SUCCESS) { std::cerr << "  -> HailoRT output read Error: " << status << std::endl; continue; }

                auto result = postprocess_output(output_buffer);
                std::cout << "  -> Inference Result: '" << result.first << "' (Class index: " << result.second << ")" << std::endl;
                
                last_detection_end_sample = energy_center + static_cast<size_t>(1.5 * config::SAMPLE_RATE);
                current_pos = last_detection_end_sample;
            } else {
                current_pos += vad_scan_samples;
            }
        }
        std::cout << "\n--- Processing complete. Total commands processed: " << detection_count << " ---" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "A critical error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
