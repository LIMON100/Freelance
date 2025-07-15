#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <map>

// --- Dependency Includes ---

// ONNX Runtime
#include <onnxruntime_cxx_api.h>

// JSON Library
#include "libs/single_header/json.hpp"
using json = nlohmann::json;

// WAV Loading Library
#define DR_WAV_IMPLEMENTATION
#include "libs/single_header/dr_wav.h"

// --- Helper Functions ---

// Function to load and preprocess a WAV file
std::vector<float> load_wav_file(const std::string& filepath, unsigned int expected_sample_rate) {
    unsigned int sample_rate;
    unsigned int channels;
    drwav_uint64 total_frame_count;

    // dr_wav decodes the audio into a 32-bit float array
    float* p_sample_data = drwav_open_file_and_read_pcm_frames_f32(
        filepath.c_str(), &channels, &sample_rate, &total_frame_count, nullptr
    );

    if (p_sample_data == nullptr) {
        throw std::runtime_error("Failed to load WAV file: " + filepath);
    }

    // --- IMPORTANT: Check format ---
    if (sample_rate != expected_sample_rate) {
        drwav_free(p_sample_data, nullptr);
        throw std::runtime_error("Invalid sample rate. Expected " + std::to_string(expected_sample_rate) + " Hz, but file has " + std::to_string(sample_rate) + " Hz.");
    }
    if (channels != 1) {
        drwav_free(p_sample_data, nullptr);
        throw std::runtime_error("Invalid channel count. Expected mono (1 channel), but file has " + std::to_string(channels) + " channels.");
    }

    std::vector<float> audio_data(p_sample_data, p_sample_data + total_frame_count);
    drwav_free(p_sample_data, nullptr);

    return audio_data;
}

// Function to load the id->label mapping from the JSON file
std::map<int, std::string> load_label_map(const std::string& filepath) {
    std::ifstream json_file(filepath);
    if (!json_file.is_open()) {
        throw std::runtime_error("Failed to open label map file: " + filepath);
    }

    json data = json::parse(json_file);
    std::map<int, std::string> id2label;
    for (auto const& [key, val] : data["id2label"].items()) {
        id2label[std::stoi(key)] = val;
    }
    return id2label;
}

// Numerically stable softmax function
void apply_softmax(std::vector<float>& data) {
    if (data.empty()) return;
    
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
    const std::string model_path = "/home/limonubuntu/Work/Limon/other_task/ble_project/training/models/06_22/test_korean_large/models/korean_command_model.onnx";
    const std::string label_map_path = "/home/limonubuntu/Work/Limon/other_task/ble_project/training/models/06_22/test_korean_large/models/label_mappings.json";
    const unsigned int target_sample_rate = 16000;

    try {
        // --- 1. Initialization ---
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "KoreanCommandPredictor");
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        Ort::Session session(env, model_path.c_str(), session_options);

        // --- 2. Load Label Map ---
        std::cout << "Loading label map..." << std::endl;
        auto id2label = load_label_map(label_map_path);
        std::cout << "Found " << id2label.size() << " labels." << std::endl;

        // --- 3. Load and Preprocess Audio ---
        std::cout << "Loading and processing audio file: " << wav_path << std::endl;
        std::vector<float> audio_buffer = load_wav_file(wav_path, target_sample_rate);
        
        // --- 4. Prepare Tensors for ONNX Runtime ---
        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        // Input Tensor (audio data)
        const int64_t input_shape[] = {1, static_cast<int64_t>(audio_buffer.size())};
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, audio_buffer.data(), audio_buffer.size(), input_shape, 2
        );

        // Attention Mask Tensor
        std::vector<float> attention_mask(audio_buffer.size(), 1.0f);
        const int64_t attention_shape[] = {1, static_cast<int64_t>(attention_mask.size())};
        Ort::Value attention_tensor = Ort::Value::CreateTensor<float>(
            memory_info, attention_mask.data(), attention_mask.size(), attention_shape, 2
        );

        // Define input/output names
        const char* input_names[] = {"input_values", "attention_mask"};
        const char* output_names[] = {"logits"};
        Ort::Value input_tensors[] = {std::move(input_tensor), std::move(attention_tensor)};

        // --- 5. Run Inference ---
        std::cout << "Running inference..." << std::endl;
        auto output_tensors = session.Run(
            Ort::RunOptions{nullptr}, input_names, input_tensors, 2, output_names, 1
        );

        // --- 6. Process Output ---
        float* logits = output_tensors[0].GetTensorMutableData<float>();
        size_t num_classes = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape()[1];

        std::vector<float> probabilities(logits, logits + num_classes);
        apply_softmax(probabilities);

        // Find the predicted class
        auto max_it = std::max_element(probabilities.begin(), probabilities.end());
        int predicted_id = std::distance(probabilities.begin(), max_it);
        float confidence = *max_it;

        // --- 7. Display Result ---
        std::cout << "\n--- Prediction Result ---" << std::endl;
        std::cout << "Predicted Command: '" << id2label[predicted_id] << "'" << std::endl;
        std::cout << "Confidence: " << std::fixed << std::setprecision(2) << confidence * 100.0f << "%" << std::endl;
        std::cout << "-------------------------" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
