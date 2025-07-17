#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <sys/time.h>
#include <cstdint>
#include <cstring>
#include <memory>
#include <iomanip> // <-- THE FIX: Add this header for std::setprecision

// RKNN API
#include "libs/rknn_api/include/rknn_api.h"

// WAV Loading
#define DR_WAV_IMPLEMENTATION
#include "libs/single_header/dr_wav.h"

// Helper to read a whole file into a buffer
static unsigned char* load_file(const char* filename, int* model_size) {
    FILE* fp = fopen(filename, "rb");
    if (fp == nullptr) {
        printf("fopen %s fail!\n", filename);
        return nullptr;
    }
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    // Use a smart pointer to auto-manage memory, or manage it manually
    unsigned char* model_data = (unsigned char*)malloc(size);
    if (model_data == nullptr) {
        fclose(fp);
        return nullptr;
    }
    fread(model_data, 1, size, fp);
    fclose(fp);
    *model_size = size;
    return model_data;
}

// Helper to load labels from a text file
std::vector<std::string> load_labels(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) throw std::runtime_error("Failed to open label file: " + filepath);
    std::vector<std::string> labels;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            labels.push_back(line);
        }
    }
    return labels;
}

// Softmax function to convert logits to probabilities
void apply_softmax(float* data, int size) {
    if (size <= 0) return;
    float max_val = data[0];
    for (int i = 1; i < size; i++) {
        if (data[i] > max_val) max_val = data[i];
    }
    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        data[i] = exp(data[i] - max_val);
        sum += data[i];
    }
    for (int i = 0; i < size; i++) {
        data[i] /= sum;
    }
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_wav_file>" << std::endl;
        return 1;
    }

    const char* fe_model_path = "../models/feature_extractor_fp16.rknn";
    const char* cls_model_path = "../models/korean_command_classifier0715_fp16.rknn";
    const std::string label_path = "../models/labels.txt";
    const unsigned int fixed_len = 16000 * 4; // MUST match the fixed size used in conversion

    rknn_context fe_ctx = 0, cls_ctx = 0;
    int ret;

    try {
        // --- 1. Init and Load Both RKNN Models ---
        int fe_model_size = 0, cls_model_size = 0;
        unsigned char* fe_model_data = load_file(fe_model_path, &fe_model_size);
        unsigned char* cls_model_data = load_file(cls_model_path, &cls_model_size);
        if (!fe_model_data || !cls_model_data) throw std::runtime_error("Failed to load model data");

        // Initialize the feature extractor model
        ret = rknn_init(&fe_ctx, fe_model_data, fe_model_size, 0, nullptr);
        if(ret < 0) throw std::runtime_error("Feature extractor rknn_init failed");
        
        // Initialize the classifier model
        ret = rknn_init(&cls_ctx, cls_model_data, cls_model_size, 0, nullptr);
        if(ret < 0) throw std::runtime_error("Classifier rknn_init failed");

        // Free the memory buffers now that models are initialized
        free(fe_model_data);
        free(cls_model_data);

        auto labels = load_labels(label_path);

        // --- 2. Load and Preprocess Audio ---
        unsigned int sr, channels;
        drwav_uint64 total_frames;
        float* p_sample_data = drwav_open_file_and_read_pcm_frames_f32(argv[1], &channels, &sr, &total_frames, nullptr);
        if (!p_sample_data) throw std::runtime_error("Failed to load WAV file");
        
        // Create a buffer with the fixed size and copy the audio data, padding with zeros if necessary
        std::vector<float> audio_data(fixed_len, 0.0f); 
        memcpy(audio_data.data(), p_sample_data, std::min((drwav_uint64)fixed_len, total_frames) * sizeof(float));
        drwav_free(p_sample_data, nullptr);

        // --- 3. STAGE 1: Run Feature Extractor on the NPU ---
        rknn_input fe_inputs[1];
        memset(fe_inputs, 0, sizeof(fe_inputs));
        fe_inputs[0].index = 0;
        fe_inputs[0].type = RKNN_TENSOR_FLOAT32;
        fe_inputs[0].size = audio_data.size() * sizeof(float);
        fe_inputs[0].fmt = RKNN_TENSOR_NCHW; // For 1D data, NCHW is standard
        fe_inputs[0].buf = audio_data.data();

        ret = rknn_inputs_set(fe_ctx, 1, fe_inputs);
        if(ret < 0) throw std::runtime_error("Feature extractor rknn_inputs_set failed");
        ret = rknn_run(fe_ctx, nullptr);
        if(ret < 0) throw std::runtime_error("Feature extractor rknn_run failed");

        rknn_output fe_outputs[1];
        memset(fe_outputs, 0, sizeof(fe_outputs));
        fe_outputs[0].want_float = 1; // We want float output to feed into the next model
        ret = rknn_outputs_get(fe_ctx, 1, fe_outputs, nullptr);
        if(ret < 0) throw std::runtime_error("Feature extractor rknn_outputs_get failed");

        // --- 4. STAGE 2: Run Classifier on the NPU ---
        // The output of stage 1 is the input of stage 2
        rknn_input cls_inputs[1];
        memset(cls_inputs, 0, sizeof(cls_inputs));
        cls_inputs[0].index = 0;
        cls_inputs[0].type = RKNN_TENSOR_FLOAT32;
        cls_inputs[0].size = fe_outputs[0].size; // Use size from previous output
        cls_inputs[0].fmt = RKNN_TENSOR_NCHW;
        cls_inputs[0].buf = fe_outputs[0].buf; // Use buffer from previous output

        ret = rknn_inputs_set(cls_ctx, 1, cls_inputs);
        if(ret < 0) throw std::runtime_error("Classifier rknn_inputs_set failed");
        ret = rknn_run(cls_ctx, nullptr);
        if(ret < 0) throw std::runtime_error("Classifier rknn_run failed");

        rknn_output cls_outputs[1];
        memset(cls_outputs, 0, sizeof(cls_outputs));
        cls_outputs[0].want_float = 1; // Get final logits as float
        ret = rknn_outputs_get(cls_ctx, 1, cls_outputs, nullptr);
        if(ret < 0) throw std::runtime_error("Classifier rknn_outputs_get failed");

        // --- 5. Post-process and Display Result ---
        float* logits = (float*)cls_outputs[0].buf;
        int num_classes = cls_outputs[0].size / sizeof(float);
        apply_softmax(logits, num_classes);

        int predicted_id = std::distance(logits, std::max_element(logits, logits + num_classes));
        float confidence = logits[predicted_id];

        std::cout << "\n--- Prediction Result ---" << std::endl;
        std::cout << "Predicted Command: '" << labels[predicted_id] << "'" << std::endl;
        std::cout << "Confidence: " << std::fixed << std::setprecision(2) << confidence * 100.0f << "%" << std::endl;
        std::cout << "-------------------------" << std::endl;

        // Release the output buffers
        rknn_outputs_release(fe_ctx, 1, fe_outputs);
        rknn_outputs_release(cls_ctx, 1, cls_outputs);

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
    }

    // --- Cleanup: Destroy the RKNN contexts ---
    if(fe_ctx) rknn_destroy(fe_ctx);
    if(cls_ctx) rknn_destroy(cls_ctx);

    return 0;
}