#include "async_inference.hpp"
#include "utils.hpp"
#include <opencv2/opencv.hpp>
#include <chrono>
#include <opencv2/video.hpp> // Required for optical flow
#include <vector>
#include "ByteTrack/BYTETracker.h"
#include <iostream>
#include <atomic>
#include <algorithm>

/////////// Constants ///////////
constexpr size_t MAX_QUEUE_SIZE = 2; //60;
constexpr int MODEL_INPUT_WIDTH = 640;  
constexpr int MODEL_INPUT_HEIGHT = 640;
/////////////////////////////////
constexpr bool ENABLE_MOTION_COMPENSATION = true; 

void process_batch(AsyncModelInfer& model, std::vector<PreprocessedFrameItem>& batch);

struct Trajectory
{
    Trajectory() : x(0), y(0), a(0) {}
    Trajectory(double _x, double _y, double _a) : x(_x), y(_y), a(_a) {}

    double x, y, a; // x, y translation and angle

    // Helper operators for Kalman math
    Trajectory operator+(const Trajectory& other) { return Trajectory(x + other.x, y + other.y, a + other.a); }
    Trajectory operator-(const Trajectory& other) { return Trajectory(x - other.x, y - other.y, a - other.a); }
    Trajectory operator*(const Trajectory& other) { return Trajectory(x * other.x, y * other.y, a * other.a); }
    Trajectory operator/(const Trajectory& other) { return Trajectory(x / other.x, y / other.y, a / other.a); }
};

// Global thread-safe queues
std::shared_ptr<BoundedTSQueue<PreprocessedFrameItem>> preprocessed_queue =
    std::make_shared<BoundedTSQueue<PreprocessedFrameItem>>(MAX_QUEUE_SIZE);

std::shared_ptr<BoundedTSQueue<InferenceOutputItem>> results_queue =
    std::make_shared<BoundedTSQueue<InferenceOutputItem>>(MAX_QUEUE_SIZE);

// Function to release resources cleanly
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

// "Bridge" function to convert YOLO output to the tracker's expected format
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


// --- THIS IS THE FINAL, CORRECT VERSION OF THIS FUNCTION ---
// hailo_status run_post_process(
//     InputType &input_type,
//     bool save_video,
//     int org_height,
//     int org_width,
//     size_t frame_count,
//     cv::VideoCapture &capture,
//     size_t class_count = 80,
//     double fps = 30)
// {
//     cv::VideoWriter video;
//     if (input_type.is_video || (input_type.is_camera && save_video)) {
//         init_video_writer("./processed_video.mp4", video, fps, org_width, org_height);
//         if (!video.isOpened()) {
//             std::cerr << "Error: Could not open VideoWriter for processed_video.mp4" << std::endl;
//             return HAILO_INVALID_OPERATION;
//         }
//     }

//     bool enable_display = true;
//     if ((input_type.is_video || input_type.is_camera) && enable_display) {
//         cv::namedWindow("Object Detection", cv::WINDOW_AUTOSIZE);
//     }

//     // Initialize the tracker
//     byte_track::BYTETracker tracker(fps, 30);

//     int i = 0;
//     auto prev_time = std::chrono::high_resolution_clock::now();
    
//     while (true) {
//         show_progress(input_type, i, frame_count);
//         InferenceOutputItem output_item;
//         if (!results_queue->pop(output_item)) {
//             break;
//         }
//         auto& frame_to_draw = output_item.org_frame;
//         auto bboxes = parse_nms_data(output_item.output_data_and_infos[0].first, class_count);

//         // --- MOTION COMPENSATION LOGIC ---
        
//         // 1. Get the affine matrix that describes the camera motion.
//         cv::Mat affine_matrix = output_item.affine_matrix;
        
//         // 2. We need the INVERSE matrix to transform points from the stabilized frame back to the raw frame.
//         cv::Mat inverse_affine_matrix;
//         if (!affine_matrix.empty()) {
//             cv::invertAffineTransform(affine_matrix, inverse_affine_matrix);
//         }

//         // 3. Create a vector of byte_track::Object, transforming coordinates if necessary.
//         std::vector<byte_track::Object> objects;
//         objects.reserve(bboxes.size());
//         for (const auto& bbox : bboxes) {
//             // Get the bounding box in the stabilized coordinate system
//             float x_s = bbox.bbox.x_min * org_width;
//             float y_s = bbox.bbox.y_min * org_height;
//             float w_s = (bbox.bbox.x_max - bbox.bbox.x_min) * org_width;
//             float h_s = (bbox.bbox.y_max - bbox.bbox.y_min) * org_height;

//             byte_track::Rect<float> transformed_rect(x_s, y_s, w_s, h_s);

//             // If we have a valid transformation, apply it to the detection's corners
//             if (!inverse_affine_matrix.empty()) {
//                 std::vector<cv::Point2f> corners(2);
//                 corners[0] = cv::Point2f(x_s, y_s); // Top-left
//                 corners[1] = cv::Point2f(x_s + w_s, y_s + h_s); // Bottom-right
                
//                 std::vector<cv::Point2f> raw_corners;
//                 cv::transform(corners, raw_corners, inverse_affine_matrix);

//                 transformed_rect.x() = raw_corners[0].x;
//                 transformed_rect.y() = raw_corners[0].y;
//                 transformed_rect.width() = raw_corners[1].x - raw_corners[0].x;
//                 transformed_rect.height() = raw_corners[1].y - raw_corners[0].y;
//             }
            
//             objects.emplace_back(transformed_rect, bbox.class_id, bbox.bbox.score);
//         }

//         // --- TRACKER UPDATE ---
//         // The tracker now receives detections that are already in the correct (raw frame) coordinate space
//         std::vector<std::shared_ptr<byte_track::STrack>> tracked_objects = tracker.update(objects, output_item.affine_matrix);

//         // --- DRAWING LOGIC ---
//         for (const auto& track : tracked_objects) {
//             const byte_track::Rect<float>& rect = track->getRect();
//             cv::Rect2f tracked_bbox(rect.x(), rect.y(), rect.width(), rect.height());

//             cv::rectangle(frame_to_draw, tracked_bbox, cv::Scalar(0, 255, 0), 2);
            
//             std::string label = std::to_string(track->getTrackId());
//             cv::putText(frame_to_draw, label, cv::Point(tracked_bbox.x, tracked_bbox.y - 5),
//                         cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
//         }

//         // --- FPS and Display ---
//         auto current_time = std::chrono::high_resolution_clock::now();
//         auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(current_time - prev_time).count();
//         double fps_current = frame_time > 0 ? 1e6 / frame_time : 0;
//         prev_time = current_time;
//         std::string fps_text = "FPS: " + std::to_string(static_cast<int>(fps_current + 0.5));
//         cv::putText(frame_to_draw, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

//         if ((input_type.is_video || input_type.is_camera) && enable_display) {
//             cv::imshow("Object Detection", frame_to_draw);
//             if (cv::waitKey(1) == 'q') {
//                 break;
//             }
//         }

//         if (input_type.is_video || (input_type.is_camera && save_video)) {
//             video.write(frame_to_draw);
//         }

//         if (input_type.is_image || input_type.is_directory) {
//             cv::imwrite("processed_image_" + std::to_string(i) + ".jpg", frame_to_draw);
//             if (input_type.is_image) { break; }
//             else if (input_type.directory_entry_count - 1 == i) { break; }
//         }
//         i++;
//     }

//     // --- Cleanup ---
//     if ((input_type.is_video || input_type.is_camera) && enable_display) {
//         cv::destroyWindow("Object Detection");
//     }
//     release_resources(capture, video, input_type);
    
//     return HAILO_SUCCESS;
// }


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

    // Initialize the tracker
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

        // --- MOTION COMPENSATION LOGIC ---
        cv::Mat affine_matrix = output_item.affine_matrix;
        cv::Mat inverse_affine_matrix;
        if (!affine_matrix.empty()) {
            cv::invertAffineTransform(affine_matrix, inverse_affine_matrix);
        }

        std::vector<byte_track::Object> objects;
        objects.reserve(bboxes.size());
        for (const auto& bbox : bboxes) {
            float x_s = bbox.bbox.x_min * org_width;
            float y_s = bbox.bbox.y_min * org_height;
            float w_s = (bbox.bbox.x_max - bbox.bbox.x_min) * org_width;
            float h_s = (bbox.bbox.y_max - bbox.bbox.y_min) * org_height;

            byte_track::Rect<float> transformed_rect(x_s, y_s, w_s, h_s);

            if (!inverse_affine_matrix.empty()) {
                std::vector<cv::Point2f> corners = {cv::Point2f(x_s, y_s), cv::Point2f(x_s + w_s, y_s + h_s)};
                std::vector<cv::Point2f> raw_corners;
                cv::transform(corners, raw_corners, inverse_affine_matrix);

                transformed_rect.x() = raw_corners[0].x;
                transformed_rect.y() = raw_corners[0].y;
                transformed_rect.width() = raw_corners[1].x - raw_corners[0].x;
                transformed_rect.height() = raw_corners[1].y - raw_corners[0].y;
            }
            
            objects.emplace_back(transformed_rect, bbox.class_id, bbox.bbox.score);
        }

        // --- TRACKER UPDATE ---
        // std::vector<std::shared_ptr<byte_track::STrack>> tracked_objects = tracker.update(objects);
        std::vector<std::shared_ptr<byte_track::STrack>> tracked_objects = tracker.update(objects, output_item.affine_matrix);

        // --- PTZ GUIDANCE LOGIC ---
        if (!tracked_objects.empty()) {
            // Step 1: Select a target (e.g., the one with the lowest ID for consistency)
            auto target_track_it = std::min_element(tracked_objects.begin(), tracked_objects.end(), 
                [](const auto& a, const auto& b) {
                    return a->getTrackId() < b->getTrackId();
                });
            auto& target_track = *target_track_it;

            // Step 2: Calculate Pixel Error from the stabilized track position
            const byte_track::Rect<float>& rect = target_track->getRect();
            cv::Point2f stabilized_center(rect.x() + rect.width() / 2.0f, rect.y() + rect.height() / 2.0f);
            cv::Point2f screen_center(org_width / 2.0f, org_height / 2.0f);
            cv::Point2f pixel_error = stabilized_center - screen_center;

            // Step 3: Convert Pixel Error to Angular Error
            // NOTE: This value MUST come from your calibration data (Zoom-Focus-FOV Table).
            // This is a placeholder value. You will need to create a lookup function
            // to get the correct px/deg for the current camera zoom level.
            const float pixels_per_degree = 124.52f; // Example for EO @ 30mm
            float pan_error_degrees = pixel_error.x / pixels_per_degree;
            float tilt_error_degrees = -pixel_error.y / pixels_per_degree; // Vertical axis is often inverted

            // Step 4: Calculate Required Angular Speed (Proportional Controller)
            // This gain will need to be tuned based on the PTZ's responsiveness.
            const float P_GAIN = 4.0f;
            float pan_speed_dps = pan_error_degrees * P_GAIN;
            float tilt_speed_dps = tilt_error_degrees * P_GAIN;
            
            // Step 5: (Future enhancement) Add Feed-Forward from Kalman Filter velocity
            //std::cout << "Frame " << i << ": PTZ Command -> Pan Speed: " << std::fixed << std::setprecision(3) << pan_speed_dps << " deg/s, Tilt Speed: " << std::fixed << std::setprecision(3) << tilt_speed_dps << " deg/s" << std::endl;
            
            // For visualization, draw a red crosshair on the stabilized target
            cv::drawMarker(frame_to_draw, stabilized_center, cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 25, 2);
        }
        // --- END PTZ GUIDANCE LOGIC ---

        // --- DRAWING LOGIC ---
        for (const auto& track : tracked_objects) {
            const byte_track::Rect<float>& rect = track->getRect();
            cv::Rect2f tracked_bbox(rect.x(), rect.y(), rect.width(), rect.height());

            cv::rectangle(frame_to_draw, tracked_bbox, cv::Scalar(0, 255, 0), 2);
            
            std::string label = std::to_string(track->getTrackId());
            cv::putText(frame_to_draw, label, cv::Point(tracked_bbox.x, tracked_bbox.y - 5),
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        }

        // --- FPS and Display ---
        auto current_time = std::chrono::high_resolution_clock::now();
        auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(current_time - prev_time).count();
        double fps_current = frame_time > 0 ? 1e6 / frame_time : 0;
        prev_time = current_time;
        std::string fps_text = "FPS: " + std::to_string(static_cast<int>(fps_current + 0.5));
        cv::putText(frame_to_draw, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

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

    // --- Cleanup ---
    if ((input_type.is_video || input_type.is_camera) && enable_display) {
        cv::destroyWindow("Object Detection");
    }
    release_resources(capture, video, input_type);
    
    return HAILO_SUCCESS;
}

// void preprocess_video_frames(cv::VideoCapture &capture,
//                             uint32_t /*width*/, uint32_t /*height*/) {
//     cv::Mat org_frame, current_gray;
    
//     cv::Mat prev_gray;
//     std::vector<cv::Point2f> prev_features;
//     cv::Mat last_good_affine_matrix = cv::Mat::eye(2, 3, CV_64F);

//     while (true) {
//         capture >> org_frame;
//         if (org_frame.empty()) {
//             preprocessed_queue->stop();
//             break;
//         }
        
//         cv::Mat stabilized_frame_for_inference = org_frame.clone();
//         cv::Mat affine_matrix_for_tracker; // This will be passed forward

//         // --- USE THE ON/OFF SWITCH ---
//         if (ENABLE_MOTION_COMPENSATION) {
//             cv::cvtColor(org_frame, current_gray, cv::COLOR_BGR2GRAY);

//             if (prev_gray.empty()) {
//                 cv::Mat mask = cv::Mat::ones(current_gray.size(), CV_8U);
//                 cv::rectangle(mask, cv::Rect(0, current_gray.rows * 0.5, current_gray.cols, current_gray.rows * 0.5), cv::Scalar(0), cv::FILLED);
//                 cv::goodFeaturesToTrack(current_gray, prev_features, 200, 0.01, 10, mask);
//                 last_good_affine_matrix = cv::Mat::eye(2, 3, CV_64F);
//             } else {
//                 std::vector<cv::Point2f> current_features;
//                 std::vector<uchar> status;
//                 std::vector<float> err;
//                 if (!prev_features.empty()) {
//                      cv::calcOpticalFlowPyrLK(prev_gray, current_gray, prev_features, current_features, status, err);
//                 }

//                 std::vector<cv::Point2f> good_prev_features, good_current_features;
//                 for (size_t i = 0; i < status.size(); i++) {
//                     if (status[i]) {
//                         good_prev_features.push_back(prev_features[i]);
//                         good_current_features.push_back(current_features[i]);
//                     }
//                 }

//                 if (good_prev_features.size() > 20) {
//                     cv::Mat affine_matrix = cv::estimateAffine2D(good_prev_features, good_current_features, cv::noArray(), cv::RANSAC);
//                     if (!affine_matrix.empty()) {
//                         double dx = affine_matrix.at<double>(0, 2);
//                         double dy = affine_matrix.at<double>(1, 2);
//                         double max_translation = 0.25 * org_frame.cols;
//                         if (std::abs(dx) < max_translation && std::abs(dy) < max_translation) {
//                             last_good_affine_matrix = affine_matrix;
//                         }
//                     }
//                 }
                
//                 cv::warpAffine(org_frame, stabilized_frame_for_inference, last_good_affine_matrix, org_frame.size());

//                 cv::Mat mask = cv::Mat::ones(current_gray.size(), CV_8U);
//                 cv::rectangle(mask, cv::Rect(0, current_gray.rows * 0.5, current_gray.cols, current_gray.rows * 0.5), cv::Scalar(0), cv::FILLED);
//                 cv::goodFeaturesToTrack(current_gray, prev_features, 200, 0.01, 10, mask);
//             }
            
//             prev_gray = current_gray.clone();
//             affine_matrix_for_tracker = last_good_affine_matrix.clone();
//         }
//         // If ENABLE_MOTION_COMPENSATION is false, we skip all of the above.
//         // `stabilized_frame_for_inference` remains a clone of `org_frame`,
//         // and `affine_matrix_for_tracker` remains empty.

//         cv::Mat resized_frame;
//         cv::resize(stabilized_frame_for_inference, resized_frame, cv::Size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT), 0, 0, cv::INTER_LINEAR);
        
//         PreprocessedFrameItem item;
//         item.org_frame = org_frame.clone();
//         item.resized_for_infer = resized_frame;
//         item.affine_matrix = affine_matrix_for_tracker; // Pass the matrix (or an empty one)
        
//         preprocessed_queue->push(item);
//     }
// }


void preprocess_video_frames(cv::VideoCapture &capture,
                            int target_proc_width, int target_proc_height) {
    cv::Mat raw_frame, org_frame, cur_grey;
    cv::Mat prev_grey;

    // State for the camera motion Kalman Filter
    Trajectory X, P(1, 1, 1);
    double a = 0, x = 0, y = 0;
    double p_std = 4e-3, c_std = 0.25;
    Trajectory Q(p_std, p_std, p_std);
    Trajectory R(c_std, c_std, c_std);
    int k = 1;

    while (true) {
        try {
            capture >> raw_frame; // Read the original, raw frame
            if (raw_frame.empty()) {
                preprocessed_queue->stop();
                break;
            }
            
            // --- NEW: PRE-RESIZING STEP ---
            // If target dimensions are specified, resize the frame first.
            // All subsequent operations will use this resized 'org_frame'.
            if (target_proc_width > 0 && target_proc_height > 0) {
                cv::resize(raw_frame, org_frame, cv::Size(target_proc_width, target_proc_height));
            } else {
                org_frame = raw_frame.clone();
            }
            // --- END PRE-RESIZING ---

            cv::cvtColor(org_frame, cur_grey, cv::COLOR_BGR2GRAY);
            
            cv::Mat frame_for_inference = org_frame.clone();
            cv::Mat final_transform = cv::Mat::eye(2, 3, CV_64F);

            if (!prev_grey.empty()) {
                std::vector<cv::Point2f> prev_corner, cur_corner;
                cv::goodFeaturesToTrack(prev_grey, prev_corner, 200, 0.01, 30);
                
                if (prev_corner.size() > 10) { 
                    std::vector<uchar> status;
                    std::vector<float> err;
                    cv::calcOpticalFlowPyrLK(prev_grey, cur_grey, prev_corner, cur_corner, status, err);

                    std::vector<cv::Point2f> prev_corner2, cur_corner2;
                    for(size_t i=0; i < status.size(); i++) {
                        if(status[i]) {
                            prev_corner2.push_back(prev_corner[i]);
                            cur_corner2.push_back(cur_corner[i]);
                        }
                    }
                    
                    if (prev_corner2.size() > 10) {
                        cv::Mat T = cv::estimateAffine2D(prev_corner2, cur_corner2);

                        double dx = 0, dy = 0, da = 0;
                        if (!T.empty()) {
                            dx = T.at<double>(0, 2);
                            dy = T.at<double>(1, 2);
                            da = atan2(T.at<double>(1, 0), T.at<double>(0, 0));
                        }

                        x += dx; y += dy; a += da;
                        Trajectory z(x, y, a);

                        if (k == 1) {
                            X = Trajectory(0, 0, 0); P = Trajectory(1, 1, 1);
                        } else {
                            Trajectory X_ = X; Trajectory P_ = P + Q;
                            Trajectory K = P_ / (P_ + R);
                            X = X_ + K * (z - X_);
                            P = (Trajectory(1, 1, 1) - K) * P_;
                        }
                        k++;

                        double diff_x = X.x - x; double diff_y = X.y - y; double diff_a = X.a - a;
                        double new_dx = dx + diff_x; double new_dy = dy + diff_y; double new_da = da + diff_a;

                        final_transform.at<double>(0, 0) = cos(new_da);
                        final_transform.at<double>(0, 1) = -sin(new_da);
                        final_transform.at<double>(1, 0) = sin(new_da);
                        final_transform.at<double>(1, 1) = cos(new_da);
                        final_transform.at<double>(0, 2) = new_dx;
                        final_transform.at<double>(1, 2) = new_dy;
                    }
                }
                
                cv::warpAffine(org_frame, frame_for_inference, final_transform, org_frame.size());
            }
            
            cur_grey.copyTo(prev_grey);

            cv::Mat resized_frame;
            cv::resize(frame_for_inference, resized_frame, cv::Size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT), 0, 0, cv::INTER_LINEAR);
            
            PreprocessedFrameItem item;
            item.org_frame = org_frame.clone(); // Pass the (potentially resized) frame for display
            item.resized_for_infer = resized_frame;
            
            // We need the inverse of the final transform for the post-processing stage
            cv::Mat inverse_transform;
            cv::invertAffineTransform(final_transform, inverse_transform);
            item.affine_matrix = inverse_transform;
            
            preprocessed_queue->push(item);

        } catch (const cv::Exception& e) {
            std::cerr << "!!! OpenCV Exception in preprocess thread: " << e.what() << std::endl;
            prev_grey.release(); // Reset state
            continue;
        }
    }
}

void preprocess_image_frames(const std::string &input_path,
                            uint32_t width, uint32_t height) {
    cv::Mat org_frame = cv::imread(input_path);
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
                            cv::VideoCapture &capture,
                        int target_proc_width, int target_proc_height) {
    auto model_input_shape = model.get_infer_model()->hef().get_input_vstream_infos().release()[0].shape;
    uint32_t target_height = model_input_shape.height;
    uint32_t target_width = model_input_shape.width;

    if (input_type.is_image) {
        preprocess_image_frames("./full_mov_slow.mp4", target_width, target_height);
    }
    else if (input_type.is_directory) {
        preprocess_directory_of_images("./full_mov_slow.mp4", target_width, target_height);
    }
    else {
        // preprocess_video_frames(capture, target_width, target_height);
        preprocess_video_frames(capture, target_proc_width, target_proc_height);
    }
    return HAILO_SUCCESS;
}

// hailo_status run_inference_async(AsyncModelInfer& model,
//                                 std::chrono::duration<double>& inference_time) {
//     auto start_time = std::chrono::high_resolution_clock::now();
//     while (true) {
//         PreprocessedFrameItem item;
//         if (!preprocessed_queue->pop(item)) {
//             break;
//         }

//         model.infer(
//             std::make_shared<cv::Mat>(item.resized_for_infer),
//             [org_frame = item.org_frame, 
//              affine_matrix = item.affine_matrix,
//              queue = results_queue](
//                 const hailort::AsyncInferCompletionInfo &/*info*/,
//                 const std::vector<std::pair<uint8_t*, hailo_vstream_info_t>> &output_data_and_infos,
//                 const std::vector<std::shared_ptr<uint8_t>> &output_guards)
//             {
//                 InferenceOutputItem output_item;
//                 output_item.org_frame = org_frame;
//                 output_item.affine_matrix = affine_matrix;
//                 output_item.output_data_and_infos = output_data_and_infos;
//                 output_item.output_guards = output_guards;
//                 queue->push(output_item);
//             });
//     }
//     results_queue->stop();
//     auto end_time = std::chrono::high_resolution_clock::now();
//     inference_time = end_time - start_time;
//     return HAILO_SUCCESS;
// }


// File: object_detection.cpp
hailo_status run_inference_async(AsyncModelInfer& model,
                                std::chrono::duration<double>& inference_time) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Use a fixed batch size (you can change this number)
    size_t batch_size = 8;  // ← CHANGE THIS NUMBER AS NEEDED
    model.set_batch_size(batch_size);
    
    std::vector<PreprocessedFrameItem> current_batch;
    
    while (true) {
        PreprocessedFrameItem item;
        if (!preprocessed_queue->pop(item)) {
            // Process remaining items in batch before exiting
            if (!current_batch.empty()) {
                process_batch(model, current_batch);
                current_batch.clear();
            }
            break;
        }
        
        current_batch.push_back(item);
        
        // Process when batch is full
        if (current_batch.size() >= batch_size) {
            process_batch(model, current_batch);
            current_batch.clear();
        }
    }
    
    results_queue->stop();
    auto end_time = std::chrono::high_resolution_clock::now();
    inference_time = end_time - start_time;
    return HAILO_SUCCESS;
}

// Add the process_batch function implementation
void process_batch(AsyncModelInfer& model, std::vector<PreprocessedFrameItem>& batch) {
    if (batch.empty()) return;
    
    std::vector<std::shared_ptr<cv::Mat>> batch_frames;
    std::vector<PreprocessedFrameItem> batch_items_copy = batch;
    
    for (auto& item : batch) {
        batch_frames.push_back(std::make_shared<cv::Mat>(item.resized_for_infer));
    }
    
    model.infer_batch(batch_frames,
        [batch_items = std::move(batch_items_copy), queue = results_queue](
            const hailort::AsyncInferCompletionInfo &,  // ← Fixed unused parameter
            const std::vector<std::pair<uint8_t*, hailo_vstream_info_t>> &output_data_and_infos,
            const std::vector<std::shared_ptr<uint8_t>> &output_guards) {
            
            for (size_t i = 0; i < batch_items.size(); i++) {
                InferenceOutputItem output_item;
                output_item.org_frame = batch_items[i].org_frame.clone();
                output_item.affine_matrix = batch_items[i].affine_matrix;
                output_item.output_data_and_infos = output_data_and_infos;
                output_item.output_guards = output_guards;
                
                queue->push(output_item);
            }
        });
}

// int main(int, char**)
// {
//     // --- USER CONFIGURATION ---
//     // Set to -1 to use the video's original resolution.
//     const int TARGET_WIDTH = 640;
//     const int TARGET_HEIGHT = 512;

//     // --- Hardcoded paths ---
//     std::string hef_path = "./y11s_person.hef";
//     std::string input_path = "./75m.mp4";
//     bool save_video = true;
    
//     // --- Application Setup (largely unchanged) ---
//     size_t class_count = 1;
//     std::chrono::duration<double> inference_time;
//     auto t_start = std::chrono::high_resolution_clock::now();
    
//     double org_width, org_height;
//     cv::VideoCapture capture;
//     size_t frame_count;
//     InputType input_type;

//     AsyncModelInfer model(hef_path);

//     input_type.is_video = true;
//     capture.open(input_path);
//     if (!capture.isOpened()) {
//         std::cerr << "Error: Could not open video file " << input_path << std::endl;
//         return HAILO_INVALID_ARGUMENT;
//     }

//     // --- DYNAMIC RESOLUTION LOGIC ---
//     // If a target size is set, we override the original dimensions.
//     if (TARGET_WIDTH > 0 && TARGET_HEIGHT > 0) {
//         org_width = TARGET_WIDTH;
//         org_height = TARGET_HEIGHT;
//         std::cout << "-I- Forcing input resolution to: " << org_width << "x" << org_height << std::endl;
//     } else {
//         org_width = capture.get(cv::CAP_PROP_FRAME_WIDTH);
//         org_height = capture.get(cv::CAP_PROP_FRAME_HEIGHT);
//         std::cout << "-I- Using original video resolution: " << org_width << "x" << org_height << std::endl;
//     }
    
//     frame_count = capture.get(cv::CAP_PROP_FRAME_COUNT);
//     double fps = capture.get(cv::CAP_PROP_FPS);

//     // --- Start Threads ---
//     // Pass the final org_width and org_height to the preprocess thread
//     auto preprocess_thread = std::async(std::launch::async, run_preprocess,
//                                        std::ref(model),
//                                        std::ref(input_type),
//                                        std::ref(capture),
//                                        org_width, 
//                                        org_height);

//     auto inference_thread = std::async(std::launch::async, run_inference_async,
//                                       std::ref(model),
//                                       std::ref(inference_time));

//     // Post-processing also uses the final dimensions for scaling and display
//     auto output_parser_thread = std::async(std::launch::async, run_post_process,
//                                           std::ref(input_type),
//                                           save_video,
//                                           org_height,
//                                           org_width,
//                                           frame_count,
//                                           std::ref(capture),
//                                           class_count,
//                                           fps);

//     output_parser_thread.wait();

//     hailo_status status = wait_and_check_threads(
//         preprocess_thread,    "Preprocess",
//         inference_thread,     "Inference",
//         output_parser_thread, "Postprocess"
//     );
//     if (HAILO_SUCCESS != status) {
//         return status;
//     }

//     if (!input_type.is_camera) {
//         auto t_end = std::chrono::high_resolution_clock::now();
//         print_inference_statistics(inference_time, hef_path, frame_count, t_end - t_start);
//     }

//     return HAILO_SUCCESS;
// }


int main(int, char**)
{
    // --- USER CONFIGURATION ---
    const bool USE_LIVE_STREAM = false; // Set to true to use the camer

    std::string gstreamer_pipeline =
    "libcamerasrc ! "
    "videoconvert ! "
    "video/x-raw,format=BGR,width=640,height=480,framerate=30/1 ! "
    "queue max-size-buffers=1 leaky=downstream ! "
    "appsink drop=true max-buffers=1 async=false sync=false";

    // --- Video File Path (used if USE_LIVE_STREAM is false) ---
    std::string hef_path = "./y11s_person.hef";
    std::string input_path = "./return.mp4";
    
    // --- Processing Resolution ---
    // Since the camera is 640x480, we should process at that resolution.
    const int TARGET_WIDTH = 640;
    const int TARGET_HEIGHT = 512;

    // --- Other Settings ---
    bool save_video = true;
    size_t class_count = 1;
    
    // --- Application Setup ---
    std::chrono::duration<double> inference_time;
    auto t_start = std::chrono::high_resolution_clock::now();
    double org_width, org_height;
    cv::VideoCapture capture;
    size_t frame_count;
    InputType input_type;
    double fps = 30.0; // The camera is set to 30 FPS

    AsyncModelInfer model(hef_path);

    size_t batch_size = 8; // ← CHANGE THIS NUMBER ONLY
    model.set_batch_size(batch_size);

    std::cout << "Using batch size: " << batch_size << std::endl;

    // --- DYNAMIC INPUT SOURCE LOGIC ---
    if (USE_LIVE_STREAM) {
        std::cout << "-I- Opening GStreamer pipeline for live camera..." << std::endl;
        capture.open(gstreamer_pipeline, cv::CAP_GSTREAMER);
        input_type.is_camera = true;
        frame_count = -1; // Live streams run indefinitely
    } else {
        std::cout << "-I- Opening video file: " << input_path << std::endl;
        capture.open(input_path);
        input_type.is_video = true;
        frame_count = capture.get(cv::CAP_PROP_FRAME_COUNT);
        fps = capture.get(cv::CAP_PROP_FPS);
    }

    if (!capture.isOpened()) {
        std::cerr << "Error: Could not open video source." << std::endl;
        if (USE_LIVE_STREAM) {
            std::cerr << "      Please check your GStreamer pipeline string and camera connection/permissions." << std::endl;
        }
        return HAILO_INVALID_ARGUMENT;
    }

    // --- DYNAMIC RESOLUTION LOGIC ---
    if (TARGET_WIDTH > 0 && TARGET_HEIGHT > 0) {
        org_width = TARGET_WIDTH;
        org_height = TARGET_HEIGHT;
        std::cout << "-I- Forcing processing resolution to: " << org_width << "x" << org_height << std::endl;
    } else {
        org_width = capture.get(cv::CAP_PROP_FRAME_WIDTH);
        org_height = capture.get(cv::CAP_PROP_FRAME_HEIGHT);
        std::cout << "-I- Using original source resolution: " << org_width << "x" << org_height << std::endl;
    }
    
    // --- Start Threads ---
    auto preprocess_thread = std::async(std::launch::async, run_preprocess,
                                       std::ref(model),
                                       std::ref(input_type),
                                       std::ref(capture),
                                       org_width, 
                                       org_height);

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
        auto t_end = std::chrono::high_resolution_clock::now();
        print_inference_statistics(inference_time, hef_path, frame_count, t_end - t_start);
    }

    return HAILO_SUCCESS;
}
