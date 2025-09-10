#include "async_inference.hpp"
#include "utils.hpp"
#include <opencv2/opencv.hpp>
#include <chrono>
#include <opencv2/video.hpp> // Added for optical flow
#include <vector>

// NEW: Include the header for the new tracker
#include "ByteTrack/BYTETracker.h"

/////////// Constants ///////////
constexpr size_t MAX_QUEUE_SIZE = 60;
constexpr int MODEL_INPUT_WIDTH = 640;  
constexpr int MODEL_INPUT_HEIGHT = 640;
/////////////////////////////////

std::shared_ptr<BoundedTSQueue<PreprocessedFrameItem>> preprocessed_queue =
    std::make_shared<BoundedTSQueue<PreprocessedFrameItem>>(MAX_QUEUE_SIZE);

std::shared_ptr<BoundedTSQueue<InferenceOutputItem>> results_queue =
    std::make_shared<BoundedTSQueue<InferenceOutputItem>>(MAX_QUEUE_SIZE);

void release_resources(cv::VideoCapture &capture, cv::VideoWriter &video, InputType &input_type) {
    if (input_type.is_video) {
        video.release();
    }
    if (input_type.is_camera || input_type.is_video) {
        capture.release();
        cv::destroyAllWindows();
    }
    preprocessed_queue->stop();
    results_queue->stop();
}

// Helper function to convert YOLO output to the new tracker's format
std::vector<byte_track::Object> detections_to_bytetrack_objects(
    const std::vector<NamedBbox>& bboxes, int frame_width, int frame_height) 
{
    std::vector<byte_track::Object> objects;
    objects.reserve(bboxes.size());
    for (const auto& bbox : bboxes) {
        float x1 = bbox.bbox.x_min * frame_width;
        float y1 = bbox.bbox.y_min * frame_height;
        float w = (bbox.bbox.x_max - bbox.bbox.x_min) * frame_width;
        float h = (bbox.bbox.y_max - bbox.bbox.y_min) * frame_height;

        objects.emplace_back(byte_track::Rect<float>(x1, y1, w, h),
                             bbox.class_id,
                             bbox.bbox.score);
    }
    return objects;
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

    bool enable_display = true;
    if ((input_type.is_video || input_type.is_camera) && enable_display) {
        cv::namedWindow("Object Detection", cv::WINDOW_AUTOSIZE);
    }

    // --- TRACKER INTEGRATION ---
    byte_track::BYTETracker tracker(fps, 30);

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

        // --- TRACKER INTEGRATION: The Bridge ---
        std::vector<byte_track::Object> objects = detections_to_bytetrack_objects(bboxes, org_width, org_height);

        // --- TRACKER INTEGRATION: The Update ---
        // FIX #1: Use the explicit type instead of the alias 'STrackPtr'
        std::vector<std::shared_ptr<byte_track::STrack>> tracked_objects = tracker.update(objects);

        // --- TRACKER INTEGRATION: Drawing ---
        // FIX #2: This loop will now work because 'tracked_objects' is a known type
        for (const auto& track : tracked_objects) {
            const byte_track::Rect<float>& rect = track->getRect();
            cv::Rect2f tracked_bbox(rect.x(), rect.y(), rect.width(), rect.height());

            cv::rectangle(frame_to_draw, tracked_bbox, cv::Scalar(0, 255, 0), 2);
            
            std::string label = "ID: " + std::to_string(track->getTrackId());
            // FIX #3: Access member variables directly, not as functions
            cv::putText(frame_to_draw, label, cv::Point(tracked_bbox.x, tracked_bbox.y - 5),
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        }

        // Calculate and display FPS
        auto current_time = std::chrono::high_resolution_clock::now();
        auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(current_time - prev_time).count();
        double fps_current = frame_time > 0 ? 1e6 / frame_time : 0;
        prev_time = current_time;
        std::string fps_text = "FPS: " + std::to_string(static_cast<int>(fps_current + 0.5));
        cv::putText(frame_to_draw, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

        // Display the frame
        if ((input_type.is_video || input_type.is_camera) && enable_display) {
            cv::imshow("Object Detection", frame_to_draw);
            if (cv::waitKey(1) == 'q') {
                break;
            }
        }

        // Save frame to video
        if (input_type.is_video || (input_type.is_camera && save_video)) {
            video.write(frame_to_draw);
        }

        // Handle single image/directory
        if (input_type.is_image || input_type.is_directory) {
            cv::imwrite("processed_image_" + std::to_string(i) + ".jpg", frame_to_draw);
            if (input_type.is_image) { break; }
            else if (input_type.directory_entry_count - 1 == i) { break; }
        }
        i++;
    }

    // Cleanup
    if ((input_type.is_video || input_type.is_camera) && enable_display) {
        cv::destroyWindow("Object Detection");
    }
    release_resources(capture, video, input_type);
    
    return HAILO_SUCCESS;
}

void preprocess_video_frames(cv::VideoCapture &capture,
                            uint32_t /*width*/, uint32_t /*height*/) { // Mark unused params
    cv::Mat org_frame, current_gray;
    
    cv::Mat prev_gray;
    std::vector<cv::Point2f> prev_features;
    cv::Mat last_good_affine_matrix = cv::Mat::eye(2, 3, CV_32F);

    while (true) {
        capture >> org_frame;
        if (org_frame.empty()) {
            preprocessed_queue->stop();
            break;
        }
        
        cv::cvtColor(org_frame, current_gray, cv::COLOR_BGR2GRAY);
        cv::Mat stabilized_frame_for_inference;

        if (prev_gray.empty()) {
            cv::goodFeaturesToTrack(current_gray, prev_features, 200, 0.01, 10);
            stabilized_frame_for_inference = org_frame.clone();
        } else {
            std::vector<cv::Point2f> current_features;
            std::vector<uchar> status;
            std::vector<float> err;
            cv::calcOpticalFlowPyrLK(prev_gray, current_gray, prev_features, current_features, status, err);

            std::vector<cv::Point2f> good_prev_features, good_current_features;
            for (size_t i = 0; i < status.size(); i++) {
                if (status[i]) {
                    good_prev_features.push_back(prev_features[i]);
                    good_current_features.push_back(current_features[i]);
                }
            }

            if (good_prev_features.size() > 20) {
                cv::Mat affine_matrix = cv::estimateAffine2D(good_prev_features, good_current_features, cv::noArray(), cv::RANSAC);
                
                if (!affine_matrix.empty()) {
                    last_good_affine_matrix = affine_matrix;
                }
            }
            
            cv::warpAffine(org_frame, stabilized_frame_for_inference, last_good_affine_matrix, org_frame.size());

            cv::goodFeaturesToTrack(current_gray, prev_features, 200, 0.01, 10);
        }
        
        prev_gray = current_gray.clone();

        cv::Mat resized_frame;
        cv::resize(stabilized_frame_for_inference, resized_frame, cv::Size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT), 0, 0, cv::INTER_LINEAR);
        
        PreprocessedFrameItem item;
        item.org_frame = org_frame.clone();
        item.resized_for_infer = resized_frame;
        
        preprocessed_queue->push(item);
    }
}

void preprocess_image_frames(const std::string &input_path,
                            uint32_t width, uint32_t height) {
    cv::Mat org_frame = cv::imread(input_path);
    cv::Mat resized_frame;
    cv::resize(org_frame, resized_frame, cv::Size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT), 0, 0, cv::INTER_LINEAR);
    // Note: The function create_preprocessed_frame_item seems to be defined in utils.hpp/cpp.
    // Ensure it correctly creates the item.
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
    uint32_t target_height = model_input_shape.height;
    uint32_t target_width = model_input_shape.width;
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
                const hailort::AsyncInferCompletionInfo &/*info*/, // Mark unused
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

int main(int /*argc*/, char** /*argv*/) // Mark unused
{
    size_t class_count = 80;
    double fps = 30;
    std::chrono::duration<double> inference_time;
    std::chrono::time_point<std::chrono::system_clock> t_start = std::chrono::high_resolution_clock::now();
    double org_height, org_width;
    cv::VideoCapture capture;
    size_t frame_count;
    InputType input_type;

    std::string hef_path = "./y11s_person.hef";
    std::string input_path = "./2.mp4";
    bool save_video = true;

    AsyncModelInfer model(hef_path);

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

    auto preprocess_thread = std::async(std::launch::async, run_preprocess,
                                       std::ref(model),
                                       std::ref(input_type),
                                       std::ref(capture));

    auto inference_thread = std::async(std::launch::async, run_inference_async,
                                      std::ref(model),
                                      std::ref(inference_time));

    auto output_parser_thread = std::async(std::launch::async, run_post_process,
                                          std::ref(input_type),
                                          save_video,
                                          org_height,
                                          org_width,
                                          frame_count,
                                          std::ref(capture),
                                          class_count,
                                          fps);

    // Explicitly wait for the post-processing thread to finish before others.
    output_parser_thread.wait();

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
