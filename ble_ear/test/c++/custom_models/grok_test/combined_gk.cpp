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

// RKNN API
#include "libs/rknn_api/include/rknn_api.h"

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
const int OUTPUT_SEQ_LEN = 64000; // 4 seconds, required by this model's preprocessing
const int N_FFT = 512;
const int HOP_LENGTH = 128;
const int FINAL_H = 171;
const int FINAL_W = 560;

// VAD Parameters
const float VAD_THRESHOLD = 0.015f; 
const float VAD_HPF_CUTOFF = 100.0f;

// ===================================================================
//  HELPER & VAD FUNCTIONS
// ===================================================================
std::vector<std::string> load_labels(const std::string& filepath) {
    std::ifstream file(filepath); if (!file) throw std::runtime_error("Failed to open label file: " + filepath);
    std::vector<std::string> labels; std::string line;
    while (std::getline(file, line)) if (!line.empty()) labels.push_back(line);
    return labels;
}

void apply_softmax(float* data, int size) {
    if (size <= 0) return; float max_val = data[0];
    for (int i = 1; i < size; i++) { if (data[i] > max_val) max_val = data[i]; }
    float sum = 0.0f;
    for (int i = 0; i < size; i++) { data[i] = exp(data[i] - max_val); sum += data[i]; }
    for (int i = 0; i < size; i++) { data[i] /= (sum > 1e-9 ? sum : 1.0f); }
}

static unsigned char* load_rknn_model(const char* filename, int* size) {
    FILE* fp = fopen(filename, "rb"); if (!fp) { std::cerr << "fopen " << filename << " fail!\n"; return nullptr; }
    fseek(fp, 0, SEEK_END); *size = ftell(fp); fseek(fp, 0, SEEK_SET);
    unsigned char* data = (unsigned char*)malloc(*size); if (!data) { fclose(fp); return nullptr; }
    fread(data, 1, *size, fp); fclose(fp);
    return data;
}

void save_chunk_to_wav(const std::string& filename, const std::vector<float>& audio_chunk) {
    drwav_data_format format; format.container = drwav_container_riff; format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = 1; format.sampleRate = SAMPLE_RATE; format.bitsPerSample = 32;
    drwav wav; if (!drwav_init_file_write(&wav, filename.c_str(), &format, nullptr)) { std::cerr << "Warning: Failed to init WAV " << filename << std::endl; return; }
    drwav_write_pcm_frames(&wav, audio_chunk.size(), audio_chunk.data());
    drwav_uninit(&wav);
    std::cout << "  -> Saved detected audio chunk to: " << filename << std::endl;
}

class HighPassFilter {
public:
    HighPassFilter(float cutoff, float sample_rate) { float rc = 1.0f / (2.0f * M_PI * cutoff); alpha = rc / (rc + 1.0f / sample_rate); y_prev = 0.0f; x_prev = 0.0f; }
    float process(float x) { float y = alpha * (y_prev + x - x_prev); y_prev = y; x_prev = x; return y; }
private: float alpha, y_prev, x_prev;
};

bool is_speech_present(const std::vector<float>& audio_chunk, float threshold) {
    if (audio_chunk.empty()) return false;
    HighPassFilter hpf(VAD_HPF_CUTOFF, SAMPLE_RATE);
    double sum_sq = 0.0;
    for (float sample : audio_chunk) {
        float filtered_sample = hpf.process(sample);
        sum_sq += filtered_sample * filtered_sample;
    }
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
//  SPECTROGRAM PRE-PROCESSING
// ===================================================================

void bilinear_resize(const std::vector<float>& input, int in_h, int in_w, std::vector<float>& output, int out_h, int out_w); // Fwd declare
std::vector<float> preprocess_audio_chunk(const std::vector<float>& audio_chunk); // Fwd declare

// ===================================================================
//  MAIN LOGIC
// ===================================================================

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_multi_command_wav_file>" << std::endl;
        return 1;
    }

    const char* rknn_model_path = "../models/srg.rknn";
    const std::string label_path = "../models/labels.txt";
    const std::string wav_path = argv[1];

    rknn_context ctx = 0;
    try {
        // --- 1. Init RKNN and Load Model ---
        int model_size = 0;
        unsigned char* model_data = load_rknn_model(rknn_model_path, &model_size);
        if (!model_data) throw std::runtime_error("Failed to load RKNN model file");
        
        int ret = rknn_init(&ctx, model_data, model_size, 0, nullptr);
        free(model_data);
        if (ret < 0) throw std::runtime_error("rknn_init failed");

        auto labels = load_labels(label_path);
        std::cout << "RKNN 'Grok' model and labels loaded." << std::endl;
        
        // --- 2. Load Full Audio File ---
        unsigned int sr, channels;
        drwav_uint64 total_frames;
        float* p_sample_data = drwav_open_file_and_read_pcm_frames_f32(wav_path.c_str(), &channels, &sr, &total_frames, nullptr);
        if (!p_sample_data) throw std::runtime_error("Failed to load WAV file");
        std::vector<float> full_audio(total_frames);
        if (channels > 1) { /* mix down to mono */ } else { std::copy(p_sample_data, p_sample_data + total_frames, full_audio.begin()); }
        drwav_free(p_sample_data, nullptr);
        std::cout << "Audio file loaded: " << wav_path << " (" << full_audio.size() / (float)SAMPLE_RATE << "s)" << std::endl;

        // --- 3. Main Processing Loop ---
        std::cout << "\n--- Starting command detection loop ---" << std::endl;
        size_t vad_scan_samples = static_cast<size_t>(0.1 * SAMPLE_RATE);
        size_t last_detection_end_sample = 0;
        int detection_count = 0;

        for (size_t current_pos = 0; current_pos + vad_scan_samples < full_audio.size(); ) {
            if (current_pos < last_detection_end_sample) { current_pos = last_detection_end_sample; continue; }
            std::vector<float> vad_chunk(&full_audio[current_pos], &full_audio[current_pos + vad_scan_samples]);

            if (is_speech_present(vad_chunk, VAD_THRESHOLD)) {
                long timestamp_ms = static_cast<long>(current_pos * 1000.0 / SAMPLE_RATE);
                detection_count++;

                size_t search_start = (current_pos > 0.5 * SAMPLE_RATE) ? current_pos - 0.5 * SAMPLE_RATE : 0;
                size_t search_end = std::min(full_audio.size(), current_pos + (size_t)(1.5 * SAMPLE_RATE));
                std::vector<float> search_window(&full_audio[search_start], &full_audio[search_end]);
                size_t energy_center = search_start + find_energy_center(search_window);
                
                size_t clip_start = (energy_center > OUTPUT_SEQ_LEN / 2) ? energy_center - OUTPUT_SEQ_LEN / 2 : 0;
                std::vector<float> command_chunk(OUTPUT_SEQ_LEN, 0.0f);
                size_t copy_len = std::min((size_t)OUTPUT_SEQ_LEN, full_audio.size() - clip_start);
                if (copy_len > 0) std::copy(&full_audio[clip_start], &full_audio[clip_start + copy_len], command_chunk.begin());

                std::string chunk_filename = "grok_detected_chunk_" + std::to_string(detection_count) + ".wav";
                save_chunk_to_wav(chunk_filename, command_chunk);

                // --- 4. PRE-PROCESS the audio chunk into a spectrogram ---
                std::vector<float> input_data = preprocess_audio_chunk(command_chunk);

                // --- 5. RUN INFERENCE on the spectrogram ---
                rknn_input inputs[1]; memset(inputs, 0, sizeof(inputs));
                inputs[0].index = 0; inputs[0].type = RKNN_TENSOR_FLOAT32;
                inputs[0].size = input_data.size() * sizeof(float);
                inputs[0].fmt = RKNN_TENSOR_NHWC; inputs[0].buf = input_data.data();
                ret = rknn_inputs_set(ctx, 1, inputs);
                if (ret < 0) throw std::runtime_error("rknn_inputs_set failed");
                ret = rknn_run(ctx, nullptr);
                if (ret < 0) throw std::runtime_error("rknn_run failed");

                rknn_output outputs[1]; memset(outputs, 0, sizeof(outputs));
                outputs[0].want_float = 1;
                ret = rknn_outputs_get(ctx, 1, outputs, nullptr);
                if (ret < 0) throw std::runtime_error("rknn_outputs_get failed");

                // --- 6. Post-process and Display Result ---
                float* logits = (float*)outputs[0].buf;
                int num_classes = labels.size();
                apply_softmax(logits, num_classes);
                int predicted_id = std::distance(logits, std::max_element(logits, logits + num_classes));
                float confidence = logits[predicted_id];

                std::cout << "\n[" << timestamp_ms << " ms] Detection #" << detection_count << ": '" << labels[predicted_id] << "' (Confidence: "
                          << std::fixed << std::setprecision(2) << confidence * 100.0f << "%)" << std::endl;

                rknn_outputs_release(ctx, 1, outputs);
                
                last_detection_end_sample = energy_center + static_cast<size_t>(1.5 * SAMPLE_RATE);
                current_pos = last_detection_end_sample;
            } else {
                current_pos += vad_scan_samples;
            }
        }
        std::cout << "\n--- Processing complete. Total commands detected: " << detection_count << " ---" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
    }
    if (ctx) rknn_destroy(ctx);
    return 0;
}


// --- Full definitions for pre-processing functions ---
void bilinear_resize(const std::vector<float>& input, int in_h, int in_w, std::vector<float>& output, int out_h, int out_w) {
    if (in_h <= 1 || in_w <= 1) { for(size_t i = 0; i < output.size(); ++i) output[i] = input[0]; return; }
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
    std::vector<float> audio = audio_chunk; // Already padded to OUTPUT_SEQ_LEN
    int n_frames = 1 + (OUTPUT_SEQ_LEN - N_FFT) / HOP_LENGTH; int n_freqs = N_FFT / 2 + 1;
    std::vector<float> spectrogram_flat(n_freqs * n_frames);
    std::vector<float> window(N_FFT); for(int i=0; i<N_FFT; ++i) window[i]=0.5f*(1.0f - cosf(2.0f*M_PI*i/(N_FFT-1)));
    std::vector<float> frame(N_FFT);
    std::vector<std::complex<float>> fft_out(n_freqs);
    fftwf_plan plan = fftwf_plan_dft_r2c_1d(N_FFT, frame.data(), reinterpret_cast<fftwf_complex*>(fft_out.data()), FFTW_ESTIMATE);
    for (int i = 0; i < n_frames; ++i) {
        int start = i * HOP_LENGTH;
        for (int j = 0; j < N_FFT; ++j) frame[j] = audio[start + j] * window[j];
        fftwf_execute(plan);
        for (int j = 0; j < n_freqs; ++j) spectrogram_flat[j * n_frames + i] = std::abs(fft_out[j]);
    }
    fftwf_destroy_plan(plan);
    std::vector<float> final_input(FINAL_H * FINAL_W);
    bilinear_resize(spectrogram_flat, n_freqs, n_frames, final_input, FINAL_H, FINAL_W);
    return final_input;
}