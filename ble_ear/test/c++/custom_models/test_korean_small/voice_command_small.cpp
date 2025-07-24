#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <map>

// --- Dependencies ---
#include "libs/onnxruntime/include/onnxruntime_cxx_api.h"
#include "libs/single_header/json.hpp" // For label map (if you use json instead of txt)
#define DR_WAV_IMPLEMENTATION
#include "libs/single_header/dr_wav.h"

// --- Helper Functions ---

// Loads a WAV file (same as before)
std::vector<float> load_wav_file(const std::string& filepath, unsigned int expected_sample_rate) {
    unsigned int sample_rate, channels;
    drwav_uint64 total_frame_count;
    float* p_sample_data = drwav_open_file_and_read_pcm_frames_f32(filepath.c_str(), &channels, &sample_rate, &total_frame_count, nullptr);
    if (!p_sample_data) throw std::runtime_error("Failed to load WAV file: " + filepath);
    if (sample_rate != expected_sample_rate) {
        drwav_free(p_sample_data, nullptr);
        throw std::runtime_error("Invalid sample rate. Expected " + std::to_string(expected_sample_rate) + " Hz.");
    }
    std::vector<float> audio_data(p_sample_data, p_sample_data + total_frame_count);
    drwav_free(p_sample_data, nullptr);
    return audio_data;
}

// Loads labels from a simple text file
std::vector<std::string> load_labels(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open label file: " + filepath);
    }
    std::vector<std::string> labels;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            labels.push_back(line);
        }
    }
    return labels;
}

// Softmax function (same as before)
void apply_softmax(std::vector<float>& data) {
    float max_val = *std::max_element(data.begin(), data.end());
    float sum = 0.0f;
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = std::exp(data[i] - max_val);
        sum += data[i];
    }
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] /= sum;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_wav_file>" << std::endl;
        return 1;
    }

    const std::string wav_path = argv[1];
    const std::string feature_extractor_model_path = "../models/feature_extractor.onnx"; 
    const std::string classifier_model_path = "../models/korean_command_classifier0715.onnx";
    const std::string label_path = "../models/labels.txt";
    const unsigned int target_sample_rate = 16000;

    try {
        // --- 1. Initialization ---
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "KoreanCommandPredictor");
        Ort::SessionOptions session_options;

        // --- Load BOTH models ---
        std::cout << "Loading feature extractor model..." << std::endl;
        Ort::Session feature_extractor_session(env, feature_extractor_model_path.c_str(), session_options);

        std::cout << "Loading classifier model..." << std::endl;
        Ort::Session classifier_session(env, classifier_model_path.c_str(), session_options);

        // --- Load labels ---
        auto labels = load_labels(label_path);
        std::cout << "Loaded " << labels.size() << " labels." << std::endl;

        // --- Load Audio ---
        std::vector<float> audio_buffer = load_wav_file(wav_path, target_sample_rate);
        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        
        // =========================================================
        // STAGE 1: Run Feature Extractor
        // =========================================================
        std::cout << "Running feature extraction..." << std::endl;

        const int64_t input_shape[] = {1, static_cast<int64_t>(audio_buffer.size())};
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, audio_buffer.data(), audio_buffer.size(), input_shape, 2);
        
        const char* fe_input_names[] = {"input_values"};
        const char* fe_output_names[] = {"embedding"};

        auto fe_output_tensors = feature_extractor_session.Run(Ort::RunOptions{nullptr}, fe_input_names, &input_tensor, 1, fe_output_names, 1);

        // The output of this stage is the input to the next
        Ort::Value& embedding_tensor = fe_output_tensors[0];

        // =========================================================
        // STAGE 2: Run MLP Classifier
        // =========================================================
        std::cout << "Running classification..." << std::endl;
        const char* cls_input_names[] = {"input"};
        const char* cls_output_names[] = {"output"};

        auto cls_output_tensors = classifier_session.Run(Ort::RunOptions{nullptr}, cls_input_names, &embedding_tensor, 1, cls_output_names, 1);

        // --- Process final output ---
        float* logits = cls_output_tensors[0].GetTensorMutableData<float>();
        size_t num_classes = cls_output_tensors[0].GetTensorTypeAndShapeInfo().GetShape()[1];

        std::vector<float> probabilities(logits, logits + num_classes);
        apply_softmax(probabilities);

        int predicted_id = std::distance(probabilities.begin(), std::max_element(probabilities.begin(), probabilities.end()));
        float confidence = probabilities[predicted_id];

        // --- Display Result ---
        std::cout << "\n--- Prediction Result ---" << std::endl;
        std::cout << "Predicted Command: '" << labels[predicted_id] << "'" << std::endl;
        std::cout << "Confidence: " << std::fixed << std::setprecision(2) << confidence * 100.0f << "%" << std::endl;
        std::cout << "-------------------------" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}