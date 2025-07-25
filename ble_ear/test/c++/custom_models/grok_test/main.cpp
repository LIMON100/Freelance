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

// ===================================================================
//  GENERAL HELPER FUNCTIONS
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

static unsigned char* load_rknn_model(const char* filename, int* size) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        std::cerr << "Failed to open RKNN model file: " << filename << std::endl;
        return nullptr;
    }
    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    unsigned char* data = (unsigned char*)malloc(*size);
    if (!data) {
        fclose(fp);
        return nullptr;
    }
    fread(data, 1, *size, fp);
    fclose(fp);
    return data;
}

// ===================================================================
//  SPECTROGRAM PRE-PROCESSING (The working version from main_onnx.cpp)
// ===================================================================

const int SAMPLE_RATE = 16000;
const int OUTPUT_SEQ_LEN = 64000;
const int N_FFT = 512;
const int HOP_LENGTH = 128;
const int FINAL_H = 171;
const int FINAL_W = 560;

void bilinear_resize(const std::vector<float>& input, int in_h, int in_w, std::vector<float>& output, int out_h, int out_w); // Forward declaration
std::vector<float> preprocess_audio(const std::string& wav_path); // Forward declaration

// (The full definitions are below main for clarity)

// ===================================================================
//  MAIN INFERENCE LOGIC (Using RKNN API)
// ===================================================================

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_wav_file>" << std::endl;
        return 1;
    }

    // Use your properly quantized INT8 model for best performance
    const char* rknn_model_path = "../models/groknewmodel_fp.rknn"; 
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
        
        // --- 2. Pre-process Audio using the PROVEN-CORRECT function ---
        std::cout << "Processing audio file: " << wav_path << std::endl;
        std::vector<float> input_data = preprocess_audio(wav_path);
        std::cout << "Audio pre-processing complete. Running RKNN inference..." << std::endl;

        // --- 3. Run Inference on NPU ---
        rknn_input inputs[1];
        memset(inputs, 0, sizeof(inputs));
        inputs[0].index = 0;
        inputs[0].type = RKNN_TENSOR_FLOAT32; // We provide float data, the NPU handles internal conversion
        inputs[0].size = input_data.size() * sizeof(float);
        inputs[0].fmt = RKNN_TENSOR_NHWC; // Use the format required by the NPU
        inputs[0].buf = input_data.data();
        
        ret = rknn_inputs_set(ctx, 1, inputs);
        if (ret < 0) {
            fprintf(stderr, "rknn_inputs_set fail! ret=%d\n", ret);
            throw std::runtime_error("rknn_inputs_set failed");
        }
        
        ret = rknn_run(ctx, nullptr);
        if (ret < 0) throw std::runtime_error("rknn_run failed");

        rknn_output outputs[1];
        memset(outputs, 0, sizeof(outputs));
        outputs[0].want_float = 1; // Get final logits as float for easy processing
        
        ret = rknn_outputs_get(ctx, 1, outputs, nullptr);
        if (ret < 0) throw std::runtime_error("rknn_outputs_get failed");

        // --- 4. Post-process and Display Result ---
        float* logits = (float*)outputs[0].buf;
        std::vector<float> probabilities(logits, logits + labels.size());
        apply_softmax(probabilities);

        int predicted_id = std::distance(probabilities.begin(), std::max_element(probabilities.begin(), probabilities.end()));
        float confidence = probabilities[predicted_id];

        std::cout << "\n--- RKNN Prediction Result ---" << std::endl;
        std::cout << "Predicted Command: '" << labels[predicted_id] << "'" << std::endl;
        std::cout << "Confidence: " << std::fixed << std::setprecision(2) << confidence * 100.0f << "%" << std::endl;
        std::cout << "------------------------------" << std::endl;

        rknn_outputs_release(ctx, 1, outputs);

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
    }

    if (ctx) rknn_destroy(ctx);
    return 0;
}


// --- Full definitions for pre-processing functions ---

void bilinear_resize(const std::vector<float>& input, int in_h, int in_w,
                     std::vector<float>& output, int out_h, int out_w) {
    if (in_h <= 1 || in_w <= 1) {
        for(size_t i = 0; i < output.size(); ++i) output[i] = input[0];
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

std::vector<float> preprocess_audio(const std::string& wav_path) {
    unsigned int channels, sr;
    drwav_uint64 total_frames;
    float* p_sample_data = drwav_open_file_and_read_pcm_frames_f32(wav_path.c_str(), &channels, &sr, &total_frames, nullptr);
    if (!p_sample_data) throw std::runtime_error("Failed to load WAV file: " + wav_path);
    std::vector<float> audio(p_sample_data, p_sample_data + total_frames);
    drwav_free(p_sample_data, nullptr);
    
    if (sr != SAMPLE_RATE) {
         std::cerr << "Warning: Sample rate mismatch. Expected " << SAMPLE_RATE << " but got " << sr << ". Resampling is not implemented in this C++ version." << std::endl;
    }

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