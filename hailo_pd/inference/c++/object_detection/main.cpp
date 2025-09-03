/**
 * Copyright (c) 2020-2025 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the MIT license (https://opensource.org/licenses/MIT)
 **/
/**
 * @file object_detection.cpp
 * This example demonstrates the Async Infer API usage with a specific model.
 **/

#include "async_inference.hpp"
#include "utils.hpp"
#include <opencv2/opencv.hpp>
#include <chrono>

/////////// Constants ///////////
constexpr size_t MAX_QUEUE_SIZE = 60;
constexpr int MODEL_INPUT_WIDTH = 640;  // YOLOv8n input size
constexpr int MODEL_INPUT_HEIGHT = 640; // YOLOv8n input size
/////////////////////////////////

std::shared_ptr<BoundedTSQueue<PreprocessedFrameItem>> preprocessed_queue =
    std::make_shared<BoundedTSQueue<PreprocessedFrameItem>>(MAX_QUEUE_SIZE);

std::shared_ptr<BoundedTSQueue<InferenceOutputItem>> results_queue =
    std::make_shared<BoundedTSQueue<InferenceOutputItem>>(MAX_QUEUE_SIZE);

void release_resources(cv::VideoCapture &capture, cv::VideoWriter &video, InputType &input_type) {
    if (input_type.is_video) {
        video.release(); // Explicitly release video writer
    }
    if (input_type.is_camera || input_type.is_video) {
        capture.release();
        cv::destroyAllWindows(); // Ensure all OpenCV windows are closed
    }
    preprocessed_queue->stop();
    results_queue->stop();
}

hailo_status run_post_process(
    InputType &input_type,
    bool save_video,
    int org_height,
    int org_width,
    size_t frame_count,
    cv::VideoCapture &capture,
    size_t class_count = 80,
    double fps = 30)
{
    cv::VideoWriter video;
    if (input_type.is_video || (input_type.is_camera && save_video)) {
        init_video_writer("./processed_video.mp4", video, fps, org_width, org_height);
        if (!video.isOpened()) {
            std::cerr << "Error: Could not open VideoWriter for processed_video.mp4" << std::endl;
            return HAILO_INVALID_OPERATION;
        }
    }

    // Initialize display window (optional to reduce FPS overhead)
    bool enable_display = true; // Set to false to disable real-time display for better FPS
    if ((input_type.is_video || input_type.is_camera) && enable_display) {
        cv::namedWindow("Object Detection", cv::WINDOW_AUTOSIZE);
    }

    int i = 0;
    auto prev_time = std::chrono::high_resolution_clock::now();
    
    while (true) {
        show_progress(input_type, i, frame_count);
        InferenceOutputItem output_item;
        if (!results_queue->pop(output_item)) {
            break;
        }
        auto& frame_to_draw = output_item.org_frame;
        auto bboxes = parse_nms_data(output_item.output_data_and_infos[0].first, class_count);
        draw_bounding_boxes(frame_to_draw, bboxes);

        // Calculate FPS
        auto current_time = std::chrono::high_resolution_clock::now();
        auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(current_time - prev_time).count();
        double fps_current = frame_time > 0 ? 1e6 / frame_time : 0; // Avoid division by zero
        prev_time = current_time;

        // Overlay FPS on the frame
        std::string fps_text = "FPS: " + std::to_string(static_cast<int>(fps_current + 0.5));
        cv::putText(frame_to_draw, fps_text, cv::Point(10, 30), 
                    cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

        // Display the frame in real-time (optional)
        if ((input_type.is_video || input_type.is_camera) && enable_display) {
            cv::imshow("Object Detection", frame_to_draw);
            // Check for 'q' key to exit
            if (cv::waitKey(1) == 'q') {
                break;
            }
        }

        // Save the frame to video
        if (input_type.is_video || (input_type.is_camera && save_video)) {
            video.write(frame_to_draw);
        }

        // Handle image or directory inputs
        if (input_type.is_image || input_type.is_directory) {
            cv::imwrite("processed_image_" + std::to_string(i) + ".jpg", frame_to_draw);
            if (input_type.is_image) { break; }
            else if (input_type.directory_entry_count - 1 == i) { break; }
        }
        i++;
    }

    // Clean up display window
    if ((input_type.is_video || input_type.is_camera) && enable_display) {
        cv::destroyWindow("Object Detection");
    }

    release_resources(capture, video, input_type);
    return HAILO_SUCCESS;
}

void preprocess_video_frames(cv::VideoCapture &capture,
                            uint32_t width, uint32_t height) {
    cv::Mat org_frame;
    while (true) {
        capture >> org_frame;
        if (org_frame.empty()) {
            preprocessed_queue->stop();
            break;
        }
        // Explicitly resize to 640x640 to match model input
        cv::Mat resized_frame;
        cv::resize(org_frame, resized_frame, cv::Size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT), 0, 0, cv::INTER_LINEAR);
        auto preprocessed_frame_item = create_preprocessed_frame_item(resized_frame, width, height);
        preprocessed_queue->push(preprocessed_frame_item);
    }
}

void preprocess_image_frames(const std::string &input_path,
                            uint32_t width, uint32_t height) {
    cv::Mat org_frame = cv::imread(input_path);
    // Explicitly resize to 640x640
    cv::Mat resized_frame;
    cv::resize(org_frame, resized_frame, cv::Size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT), 0, 0, cv::INTER_LINEAR);
    auto preprocessed_frame_item = create_preprocessed_frame_item(resized_frame, width, height);
    preprocessed_queue->push(preprocessed_frame_item);
}

void preprocess_directory_of_images(const std::string &input_path,
                                   uint32_t width, uint32_t height) {
    for (const auto &entry : fs::directory_iterator(input_path)) {
        preprocess_image_frames(entry.path().string(), width, height);
    }
}

hailo_status run_preprocess(AsyncModelInfer &model,
                            InputType &input_type,
                            cv::VideoCapture &capture) {
    auto model_input_shape = model.get_infer_model()->hef().get_input_vstream_infos().release()[0].shape;
    uint32_t target_height = model_input_shape.height; // Should be 640
    uint32_t target_width = model_input_shape.width;   // Should be 640
    print_net_banner("./yolov8n.hef", std::ref(model.get_inputs()), std::ref(model.get_outputs()));

    if (input_type.is_image) {
        preprocess_image_frames("./full_mov_slow.mp4", target_width, target_height);
    }
    else if (input_type.is_directory) {
        preprocess_directory_of_images("./full_mov_slow.mp4", target_width, target_height);
    }
    else {
        preprocess_video_frames(capture, target_width, target_height);
    }
    return HAILO_SUCCESS;
}

hailo_status run_inference_async(AsyncModelInfer& model,
                                std::chrono::duration<double>& inference_time) {
    auto start_time = std::chrono::high_resolution_clock::now();
    while (true) {
        PreprocessedFrameItem item;
        if (!preprocessed_queue->pop(item)) {
            break;
        }

        model.infer(
            std::make_shared<cv::Mat>(item.resized_for_infer),
            [org_frame = item.org_frame, queue = results_queue](
                const hailort::AsyncInferCompletionInfo &info,
                const std::vector<std::pair<uint8_t*, hailo_vstream_info_t>> &output_data_and_infos,
                const std::vector<std::shared_ptr<uint8_t>> &output_guards)
            {
                InferenceOutputItem output_item;
                output_item.org_frame = org_frame;
                output_item.output_data_and_infos = output_data_and_infos;
                output_item.output_guards = output_guards;
                queue->push(output_item);
            });
    }
    results_queue->stop();
    auto end_time = std::chrono::high_resolution_clock::now();
    inference_time = end_time - start_time;
    return HAILO_SUCCESS;
}

int main(int argc, char** argv)
{
    size_t class_count = 80; // 80 classes in COCO dataset
    double fps = 30;
    std::chrono::duration<double> inference_time;
    std::chrono::time_point<std::chrono::system_clock> t_start = std::chrono::high_resolution_clock::now();
    double org_height, org_width;
    cv::VideoCapture capture;
    size_t frame_count;
    InputType input_type;

    // Hardcode the HEF and input video paths
    std::string hef_path = "./yolov8n.hef";
    std::string input_path = "./full_mov_slow.mp4";
    bool save_video = true; // Equivalent to -s flag

    // Initialize AsyncModelInfer with hardcoded HEF path
    AsyncModelInfer model(hef_path);

    // Set input type to video and initialize capture
    input_type.is_video = true;
    input_type.is_camera = false;
    input_type.is_image = false;
    input_type.is_directory = false;
    capture.open(input_path);
    if (!capture.isOpened()) {
        std::cerr << "Error: Could not open video file " << input_path << std::endl;
        return HAILO_INVALID_ARGUMENT;
    }
    org_width = capture.get(cv::CAP_PROP_FRAME_WIDTH);
    org_height = capture.get(cv::CAP_PROP_FRAME_HEIGHT);
    frame_count = capture.get(cv::CAP_PROP_FRAME_COUNT);

    auto preprocess_thread = std::async(run_preprocess,
                                       std::ref(model),
                                       std::ref(input_type),
                                       std::ref(capture));

    auto inference_thread = std::async(run_inference_async,
                                      std::ref(model),
                                      std::ref(inference_time));

    auto output_parser_thread = std::async(run_post_process,
                                          std::ref(input_type),
                                          save_video,
                                          org_height,
                                          org_width,
                                          frame_count,
                                          std::ref(capture),
                                          class_count,
                                          fps);

    hailo_status status = wait_and_check_threads(
        preprocess_thread,    "Preprocess",
        inference_thread,     "Inference",
        output_parser_thread, "Postprocess"
    );
    if (HAILO_SUCCESS != status) {
        return status;
    }

    if (!input_type.is_camera) {
        std::chrono::time_point<std::chrono::system_clock> t_end = std::chrono::high_resolution_clock::now();
        print_inference_statistics(inference_time, hef_path, frame_count, t_end - t_start);
    }

    return HAILO_SUCCESS;
}