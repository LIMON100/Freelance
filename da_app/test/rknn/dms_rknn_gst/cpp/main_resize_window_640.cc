#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <stdexcept>
#include <cmath>
#include <opencv2/core/types.hpp>
#include <opencv2/opencv.hpp>
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <unistd.h>
#include <dirent.h>

#include "face_analyzer/face_analyzer.h"
#include "yolo_detector/yolo11.h"
#include "behavior_analysis/BlinkDetector.hpp"
#include "behavior_analysis/YawnDetector.hpp"
#include "behavior_analysis/HeadPoseTracker.hpp"
#include "behavior_analysis/KSSCalculator.hpp"
#include "image_utils.h"
#include "file_utils.h"
#include "image_drawing.h"
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>

#include <iostream>

#include <fstream>    
#include <numeric>    
#include <deque>       
#include <sys/sysinfo.h> 

// --- Color Definitions ---
#ifndef COLOR_MAGENTA
#define COLOR_MAGENTA (0xFF00FF) // BGR: (255, 0, 255)
#endif
#ifndef COLOR_CYAN
#define COLOR_CYAN (0xFFFF00)    // BGR: (0, 255, 255)
#endif
#ifndef COLOR_YELLOW
#define COLOR_YELLOW (0x00FFFF)  // BGR: (255, 255, 0)
#endif
#ifndef COLOR_BLUE
#define COLOR_BLUE (0xFF0000)    // BGR: (255, 0, 0) -> Blue // Corrected BGR
#endif
#ifndef COLOR_ORANGE
#define COLOR_ORANGE (0x00A5FF)  // BGR: (0, 165, 255) // Corrected BGR
#endif
#ifndef COLOR_WHITE
#define COLOR_WHITE (0xFFFFFF)   // BGR: (255, 255, 255)
#endif
#ifndef COLOR_GREEN
#define COLOR_GREEN (0x00FF00)   // BGR: (0, 255, 0)
#endif
#ifndef COLOR_RED
#define COLOR_RED (0x0000FF)     // BGR: (0, 0, 255) -> Red // Corrected BGR
#endif
#ifndef COLOR_DARK_RED
#define COLOR_DARK_RED (0x00008B) // BGR: (0, 0, 139) -> Dark Red // Corrected BGR
#endif


// --- Structs and Globals ---
typedef struct app_context_t {
    face_analyzer_app_context_t face_ctx;
    yolo11_app_context_t        yolo_ctx;
} app_context_t;

static GstElement* pipeline_ = nullptr;
static GstElement* appsrc_ = nullptr;
static GstElement* appsink_ = nullptr;

struct YoloInputData {
    long frame_id;
    std::shared_ptr<image_buffer_t> image; // Image buffer for YOLO (640x640)
};
struct YoloOutputData {
    long frame_id;
    object_detect_result_list results; // Results are relative to YOLO input

};

std::queue<YoloInputData> yolo_input_queue;
std::queue<YoloOutputData> yolo_output_queue;
std::mutex yolo_input_mutex;
std::mutex yolo_output_mutex;
std::atomic<bool> stop_yolo_worker(false);
const int MAX_QUEUE_SIZE = 5;
const int YOLO_TARGET_SIZE = 640; // Define the target size for YOLO input


std::vector<cv::Point> convert_landmarks_to_cvpoint(const point_t landmarks[], int count) {
    std::vector<cv::Point> cv_landmarks;
    cv_landmarks.reserve(count);
    for (int i = 0; i < count; ++i) {
        cv_landmarks.emplace_back(cv::Point(landmarks[i].x, landmarks[i].y));
    }
    return cv_landmarks;
}
cv::Mat image_buffer_to_mat(const image_buffer_t* image_buffer) {
     if (!image_buffer || !image_buffer->virt_addr) {
        return cv::Mat(); // Return empty Mat if buffer is invalid
    }
    // Assuming BGR format as per pipeline setup
    return cv::Mat(image_buffer->height, image_buffer->width, CV_8UC3, image_buffer->virt_addr, image_buffer->width_stride);
}
double parse_head_pose_value(const std::string& s) {
    try {
        std::string numeric_part = s;
        size_t deg_pos = numeric_part.find(" deg");
        if (deg_pos != std::string::npos) {
            numeric_part = numeric_part.substr(0, deg_pos);
        }
        size_t first_digit = numeric_part.find_first_not_of(" \t");
        if (first_digit == std::string::npos) return 0.0;
        size_t last_digit = numeric_part.find_last_not_of(" \t");
        numeric_part = numeric_part.substr(first_digit, last_digit - first_digit + 1);
        if (numeric_part.empty()) return 0.0;
        return std::stod(numeric_part);
    } catch (const std::invalid_argument& e) {
        printf("WARN: Could not parse head pose value from numeric part: '%s'. Error: %s\n", s.c_str(), e.what()); return 0.0;
    } catch (const std::out_of_range& e) {
        printf("WARN: Head pose value out of range from string: '%s'. Error: %s\n", s.c_str(), e.what()); return 0.0;
    } catch (const std::exception& e) {
        printf("WARN: Generic exception parsing head pose value from string: '%s'. Error: %s\n", s.c_str(), e.what()); return 0.0;
    } catch (...) {
         printf("WARN: Unknown exception parsing head pose value from string: '%s'.\n", s.c_str()); return 0.0;
    }
}


// --- YOLO Worker Thread ---
void yolo_worker_thread_func(yolo11_app_context_t* yolo_ctx_ptr) {
    while (!stop_yolo_worker.load()) {
        YoloInputData input_data;
        bool got_data = false;
        {
            std::unique_lock<std::mutex> lock(yolo_input_mutex);
            if (!yolo_input_queue.empty()) {
                input_data = yolo_input_queue.front();
                yolo_input_queue.pop();
                got_data = true;
            }
        }

        if (got_data && input_data.image) {
            YoloOutputData output_data;
            output_data.frame_id = input_data.frame_id;
            memset(&output_data.results, 0, sizeof(output_data.results));
            // **Ideally copy scaling info from input_data to output_data here**

            int ret = inference_yolo11(yolo_ctx_ptr, input_data.image.get(), &output_data.results);

            if (ret != 0) {
                printf("WARN: YOLO Worker inference failed (frame %ld), ret=%d\n", input_data.frame_id, ret);
            }

            {
                std::unique_lock<std::mutex> lock(yolo_output_mutex);
                if (yolo_output_queue.size() >= MAX_QUEUE_SIZE) {
                    yolo_output_queue.pop();
                }
                yolo_output_queue.push(output_data);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    printf("YOLO Worker Thread Exiting.\n");
}



void setupPipeline() {
    gst_init(nullptr, nullptr);
    std::string dir = "/userdata/test_cpp/dms_gst"; // Make sure this path exists and is writable
    if (access(dir.c_str(), W_OK) != 0) { /* error handling */ return; }
    DIR* directory = opendir(dir.c_str()); if (!directory) { /* error */ return; }
    int mkv_count = 0; struct dirent* entry; while ((entry = readdir(directory)) != nullptr) { if (strstr(entry->d_name, ".mkv")) mkv_count++; } closedir(directory);
    std::string filepath = dir + "/dms_multi_" + std::to_string(mkv_count + 1) + ".mkv";
    std::string pipeline_str = "appsrc name=source ! queue ! videoconvert ! video/x-raw,format=NV12 ! mpph265enc rc-mode=cbr bps=4000000 gop=30 qp-min=10 qp-max=51 ! h265parse ! matroskamux ! filesink location=" + filepath;
    // std::cout << "Saving Pipeline: " << pipeline_str << std::endl;
    GError* error = nullptr; pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error); if (!pipeline_ || error) { /* error */ return; }
    appsrc_ = gst_bin_get_by_name(GST_BIN(pipeline_), "source"); if (!appsrc_) { /* error */ return; }
    GstCaps* caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", "width", G_TYPE_INT, 1920, "height", G_TYPE_INT, 1080, "framerate", GST_TYPE_FRACTION, 30, 1, nullptr);
    g_object_set(G_OBJECT(appsrc_), "caps", caps, "format", GST_FORMAT_TIME, nullptr); gst_caps_unref(caps);
    if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) { /* error */ }
}
void pushFrameToPipeline(GstVideoFrame* frame) {
     if (!appsrc_ || !frame || !frame->data[0]) { /* error */ return; }
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, frame->info.size, nullptr); GstMapInfo map; if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) { /* error */ gst_buffer_unref(buffer); return; }
    memcpy(map.data, frame->data[0], frame->info.size); gst_buffer_unmap(buffer, &map);
    static GstClockTime timestamp = 0; GST_BUFFER_PTS(buffer) = timestamp; GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30); timestamp += GST_BUFFER_DURATION(buffer);
    GstFlowReturn ret; g_signal_emit_by_name(appsrc_, "push-buffer", buffer, &ret); if (ret != GST_FLOW_OK) { /* error */ }
    gst_buffer_unref(buffer);
}


// ========= Resource Monitoring Functions ==========

void calculateOverallFPS(double frame_duration_ms, std::deque<double>& times_deque, double& fps_variable, int max_records) {
    times_deque.push_back(frame_duration_ms);
    if (times_deque.size() > max_records) {
        times_deque.pop_front(); // Remove the oldest time
    }

    if (!times_deque.empty()) {
        double sum = std::accumulate(times_deque.begin(), times_deque.end(), 0.0);
        double avg_time_ms = sum / times_deque.size();
        if (avg_time_ms > 0) { // Avoid division by zero
            fps_variable = 1000.0 / avg_time_ms;
        } else {
            fps_variable = 0.0; // Or some indicator of very fast/zero time
        }
    } else {
        fps_variable = 0.0;
    }
}

// Calculates current CPU usage based on /proc/stat differences
void getCPUUsage(double& cpu_usage_variable, long& prev_idle, long& prev_total) {
    std::ifstream file("/proc/stat");
    if (!file.is_open()) {
        // printf("WARN: Could not open /proc/stat for CPU usage.\n");
        // cpu_usage_variable = -1.0; // Indicate error? Or keep last value?
        return;
    }
    std::string line;
    std::getline(file, line);
    file.close(); // Close file quickly

    long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
    user = nice = system = idle = iowait = irq = softirq = steal = guest = guest_nice = 0; // Initialize

    std::istringstream iss(line);
    std::string cpu_label;
    iss >> cpu_label; // Read "cpu" label

    // Read values, handle cases where not all fields are present
    iss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;
    // Ignore guest/guest_nice for basic calculation if needed

    long currentIdleTime = idle + iowait;
    long currentTotalTime = user + nice + system + idle + iowait + irq + softirq + steal;

    long diffIdle = currentIdleTime - prev_idle;
    long diffTotal = currentTotalTime - prev_total;

    if (diffTotal > 0) { // Avoid division by zero and ensure time has passed
        double busyTime = (double)(diffTotal - diffIdle);
        cpu_usage_variable = 100.0 * (busyTime / diffTotal);
    } else if (diffTotal == 0 && prev_total != 0) {
        // No change in total time, usage is likely 0 or very low. Keep previous value or set to 0?
        // Setting to 0 might be misleading if called too frequently. Let's keep previous value.
        // cpu_usage_variable = 0.0;
    } 

    // Update previous values for the next call
    prev_idle = currentIdleTime;
    prev_total = currentTotalTime;
}

// Reads temperature from sysfs
void getTemperature(double& temp_variable) {
    // Common paths, might need adjustment for specific hardware
    const char* temp_paths[] = {
        "/sys/class/thermal/thermal_zone0/temp",
        "/sys/class/thermal/thermal_zone1/temp", 
    };
    bool temp_read = false;
    for (const char* path : temp_paths) {
        std::ifstream file(path);
        double temperature_milliC = 0;
        if (file >> temperature_milliC) {
            temp_variable = temperature_milliC / 1000.0; // Convert millidegree C to degree C
            temp_read = true;
            file.close();
            break; // Stop after reading the first successful one
        }
        file.close(); // Close even if read failed
    }

    if (!temp_read) {
        // printf("WARN: Could not read temperature from sysfs.\n");
    }
}

// --- Main Function ---
int main(int argc, char **argv) {
    // --- Variable Declarations ---
    int ret = 0;
    std::deque<double> frame_times; const int max_time_records = 60; double overallFPS = 0.0;
    double currentCpuUsage = 0.0; long prevIdleTime = 0, prevTotalTime = 0;
    double currentTemp = 0.0;

    const char *detection_model_path = "../../model/faceD.rknn";
    const char *landmark_model_path  = "../../model/faceL.rknn";
    const char *iris_model_path      = "../../model/faceI.rknn";
    const char *yolo_model_path      = "../../model/od.rknn";

    // const char *video_source         = "v4l2src device=/dev/video1 ! "
    //                                    "queue ! videoconvert ! video/x-raw,format=BGR ! "
    //                                    "appsink name=sink sync=false";


    const char *video_source = "filesrc location=../../model/interacting_with_passenger.mkv ! decodebin ! queue ! videoconvert ! video/x-raw,format=BGR ! appsink name=sink sync=false";

    app_context_t app_ctx; memset(&app_ctx, 0, sizeof(app_context_t));
    image_buffer_t src_image; memset(&src_image, 0, sizeof(image_buffer_t));
    face_analyzer_result_t face_results; memset(&face_results, 0, sizeof(face_results));
    YoloOutputData latest_yolo_output = {};

    my::BlinkDetector blinkDetector;
    YawnDetector yawnDetector;
    my::HeadPoseTracker headPoseTracker;
    KSSCalculator kssCalculator;

    bool calibration_done = false;
    int compositeKSS = 1;
    std::string kssStatus = "Initializing";
    YawnDetector::YawnMetrics yawnMetrics = {};
    my::HeadPoseTracker::HeadPoseResults headPoseResults = {};
    std::vector<std::string> detectedObjects;

    std::stringstream text_stream;
    int text_y = 0;
    const int line_height = 22;
    const int text_size = 12;
    const int status_text_size = 16;
    unsigned int status_color_uint = COLOR_GREEN;

    std::thread yolo_worker_thread;
    cv::Mat yolo_input_mat; // Reusable Mat for YOLO input


    // --- Initialization ---
    ret = init_post_process(); if (ret != 0) { printf("Error init YOLO postprocess.\n"); return -1; }
    ret = init_face_analyzer(detection_model_path, landmark_model_path, iris_model_path, &app_ctx.face_ctx); if (ret != 0) { printf("init_face_analyzer fail! ret=%d\n", ret); return -1; }
    ret = init_yolo11(yolo_model_path, &app_ctx.yolo_ctx); if (ret != 0) { printf("init_yolo11 fail! ret=%d\n", ret); release_face_analyzer(&app_ctx.face_ctx); return -1; }
    yolo_worker_thread = std::thread(yolo_worker_thread_func, &app_ctx.yolo_ctx);

    gst_init(nullptr, nullptr);
    GError* error = nullptr;
    GstElement* input_pipeline = gst_parse_launch(video_source, &error); if (!input_pipeline || error) { /* error */ return -1; }
    appsink_ = gst_bin_get_by_name(GST_BIN(input_pipeline), "sink"); if (!appsink_) { /* error */ gst_object_unref(input_pipeline); return -1; }
    if (gst_element_set_state(input_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) { /* error */ gst_object_unref(appsink_); gst_object_unref(input_pipeline); return -1; }
    setupPipeline(); // Setup the saving pipeline

    cv::namedWindow("output", cv::WINDOW_NORMAL); cv::resizeWindow("output", 1920, 1080);
    GstSample* sample;
    GstVideoFrame* in_frame = new GstVideoFrame;
    GstVideoFrame* out_frame = new GstVideoFrame;
    bool frame_mapped = false;

    int frame_counter = 0; int skip_frames = 1; const double target_frame_time = 33.3;

    // --- Main Loop ---
    while (true) {
        sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink_));
        if (!sample) { break; }

        GstBuffer* buffer = gst_sample_get_buffer(sample); GstCaps* caps = gst_sample_get_caps(sample);
        if (!buffer || !caps) { gst_sample_unref(sample); continue; }
        GstVideoInfo video_info; gst_video_info_init(&video_info);
        if (!gst_video_info_from_caps(&video_info, caps)) { gst_sample_unref(sample); continue; }
        if (!gst_video_frame_map(in_frame, &video_info, buffer, GST_MAP_READ)) { gst_sample_unref(sample); continue; }

    
        GstBuffer* out_buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&video_info), nullptr);
        if (!out_buffer) { // Check allocation success
             std::cerr << "Failed to allocate output buffer" << std::endl;
             gst_video_frame_unmap(in_frame);
             gst_sample_unref(sample);
             continue;
        }

        if (!gst_video_frame_map(out_frame, &video_info, out_buffer, GST_MAP_WRITE)) {
             gst_video_frame_unmap(in_frame);
             gst_buffer_unref(out_buffer); // Unref allocated buffer on error
             gst_sample_unref(sample);
             continue;
        }
        frame_mapped = true;

        auto start_time = std::chrono::high_resolution_clock::now();

        if (frame_counter % (skip_frames + 1) == 0) {
            // 1. Prepare Full Frame Buffer (`src_image`)
            src_image.width = in_frame->info.width;
            src_image.height = in_frame->info.height;
            src_image.width_stride = in_frame->info.stride[0];
            src_image.height_stride = in_frame->info.height;
      
            #ifdef IMAGE_FORMAT_BGR888
                src_image.format = IMAGE_FORMAT_BGR888;
            #else
                src_image.format = IMAGE_FORMAT_RGB888; 
        
            #endif
            src_image.virt_addr = static_cast<unsigned char*>(in_frame->data[0]);
            src_image.size = in_frame->info.size;
            src_image.fd = -1;

            cv::Mat original_mat = image_buffer_to_mat(&src_image);

            // 2. Run Face Analyzer ONCE on the full frame
            inference_face_analyzer(&app_ctx.face_ctx, &src_image, &face_results);

            // 3. Prepare 640x640 Image for YOLO based on face results
            image_buffer_t yolo_input_image_buffer;
            memset(&yolo_input_image_buffer, 0, sizeof(image_buffer_t));
            yolo_input_image_buffer.width = YOLO_TARGET_SIZE; yolo_input_image_buffer.height = YOLO_TARGET_SIZE;
            yolo_input_image_buffer.format = src_image.format; // Use same format as source (BGR/RGB)
            yolo_input_image_buffer.size = YOLO_TARGET_SIZE * YOLO_TARGET_SIZE * 3;
            // Ensure yolo_input_mat is allocated (if first time or size changed - though size is fixed here)
             if (yolo_input_mat.empty() || yolo_input_mat.size() != cv::Size(YOLO_TARGET_SIZE, YOLO_TARGET_SIZE)) {
                yolo_input_mat.create(YOLO_TARGET_SIZE, YOLO_TARGET_SIZE, CV_8UC3);
            }
            yolo_input_image_buffer.virt_addr = yolo_input_mat.data;
            yolo_input_image_buffer.width_stride = yolo_input_mat.step;
            yolo_input_image_buffer.height_stride = YOLO_TARGET_SIZE; yolo_input_image_buffer.fd = -1;

            float yolo_scale_x = 1.0f, yolo_scale_y = 1.0f; int yolo_crop_x = 0, yolo_crop_y = 0;
            bool face_detected_for_crop = (face_results.count > 0);

            if (face_detected_for_crop && !original_mat.empty()) {
                 face_object_t *face = &face_results.faces[0];
                 int rx = face->box.left; int ry = face->box.top; int rw = face->box.right - rx; int rh = face->box.bottom - ry;
                 int max_dim = std::max(rw, rh); int padding = max_dim * 0.5; int crop_size = max_dim + 2 * padding;
                 int face_center_x = rx + rw / 2; int face_center_y = ry + rh / 2;
                 yolo_crop_x = face_center_x - crop_size / 2; yolo_crop_y = face_center_y - crop_size / 2;
                 if (yolo_crop_x < 0) yolo_crop_x = 0; if (yolo_crop_y < 0) yolo_crop_y = 0;
                 int available_w = src_image.width - yolo_crop_x; int available_h = src_image.height - yolo_crop_y;
                 crop_size = std::min({crop_size, available_w, available_h});
                 if (crop_size <= 0) { face_detected_for_crop = false; }
                 else {
                     cv::Rect roi(yolo_crop_x, yolo_crop_y, crop_size, crop_size);
                     cv::resize(original_mat(roi), yolo_input_mat, cv::Size(YOLO_TARGET_SIZE, YOLO_TARGET_SIZE), 0, 0, cv::INTER_LINEAR);
                     yolo_scale_x = (float)crop_size / YOLO_TARGET_SIZE; yolo_scale_y = (float)crop_size / YOLO_TARGET_SIZE;
                 }
            }
            if (!face_detected_for_crop && !original_mat.empty()) {
                 cv::resize(original_mat, yolo_input_mat, cv::Size(YOLO_TARGET_SIZE, YOLO_TARGET_SIZE), 0, 0, cv::INTER_LINEAR);
                 yolo_crop_x = 0; yolo_crop_y = 0;
                 yolo_scale_x = (float)src_image.width / YOLO_TARGET_SIZE; yolo_scale_y = (float)src_image.height / YOLO_TARGET_SIZE;
            } else if (original_mat.empty()) {
                 // Handle case where original_mat is empty (e.g., first frame issue?)
                 // Maybe fill yolo_input_mat with black or skip YOLO
                 yolo_input_mat = cv::Scalar(0,0,0); // Fill with black
                 yolo_scale_x = 1.0f; yolo_scale_y = 1.0f; yolo_crop_x = 0; yolo_crop_y = 0;
                 printf("WARN: Original mat was empty, sending black frame to YOLO.\n");
            }


            auto yolo_input_ptr = std::make_shared<image_buffer_t>(yolo_input_image_buffer);
            // Ideally pass scaling info here
            {
                std::unique_lock<std::mutex> lock(yolo_input_mutex);
                if (yolo_input_queue.size() < MAX_QUEUE_SIZE) { yolo_input_queue.push({frame_counter, yolo_input_ptr}); }
                else { /* WARN queue full */ }
            }

            //Process Behavior Analysis using full-frame `face_results`
            if (face_results.count > 0 && face_results.faces[0].face_landmarks_valid) {
                 face_object_t *face = &face_results.faces[0];
                 std::vector<cv::Point> faceLandmarksCv = convert_landmarks_to_cvpoint(face->face_landmarks, NUM_FACE_LANDMARKS);
                 if (!calibration_done) {
                     headPoseResults = headPoseTracker.run(faceLandmarksCv);
                     if (headPoseTracker.isCalibrated()) { calibration_done = true; printf("Calibration Complete!\n"); }
                 }
                 blinkDetector.run(faceLandmarksCv, src_image.width, src_image.height);
                 yawnMetrics = yawnDetector.run(faceLandmarksCv, src_image.width, src_image.height);
                 if (calibration_done) { headPoseResults = headPoseTracker.run(faceLandmarksCv); }
                 else { headPoseResults = {}; headPoseResults.reference_set = false; }
            } else {
                 if (!calibration_done) { kssStatus = "Low Risk (No Face)"; }
                 yawnMetrics = {}; headPoseResults = {}; headPoseResults.reference_set = false;
  
                 // blinkDetector.reset(); // Remove this line
            }

            // Get latest YOLO results and scale them back
            bool yolo_result_processed = false;
            YoloOutputData current_yolo_output;
            {
                std::unique_lock<std::mutex> lock(yolo_output_mutex);
                if (!yolo_output_queue.empty()) {
                     while(yolo_output_queue.size() > 1) { yolo_output_queue.pop(); }
                     current_yolo_output = yolo_output_queue.front(); yolo_output_queue.pop();
                     latest_yolo_output = current_yolo_output;
                     yolo_result_processed = true;
                }
            }
            // if (yolo_result_processed) {
            //      // Use scaling/offset calculated in main thread for now
            //      float scale_x = yolo_scale_x; float scale_y = yolo_scale_y; int offset_x = yolo_crop_x; int offset_y = yolo_crop_y;
            //      for (int k = 0; k < latest_yolo_output.results.count; ++k) {
            //         latest_yolo_output.results.results[k].box.left = offset_x + latest_yolo_output.results.results[k].box.left * scale_x;
            //         latest_yolo_output.results.results[k].box.right = offset_x + latest_yolo_output.results.results[k].box.right * scale_x;
            //         latest_yolo_output.results.results[k].box.top = offset_y + latest_yolo_output.results.results[k].box.top * scale_y;
            //         latest_yolo_output.results.results[k].box.bottom = offset_y + latest_yolo_output.results.results[k].box.bottom * scale_y;
            //     }
            // }

            if (yolo_result_processed) {
                float scale_x = yolo_scale_x; float scale_y = yolo_scale_y; int offset_x = yolo_crop_x; int offset_y = yolo_crop_y;
                for (int k = 0; k < latest_yolo_output.results.count; ++k) {
                    latest_yolo_output.results.results[k].box.left = offset_x + latest_yolo_output.results.results[k].box.left * scale_x;
                    latest_yolo_output.results.results[k].box.right = offset_x + latest_yolo_output.results.results[k].box.right * scale_x;
                    latest_yolo_output.results.results[k].box.top = offset_y + latest_yolo_output.results.results[k].box.top * scale_y;
                    latest_yolo_output.results.results[k].box.bottom = offset_y + latest_yolo_output.results.results[k].box.bottom * scale_y;
                }
            
                // Draw YOLO bounding boxes
                for (int k = 0; k < latest_yolo_output.results.count; ++k) {
                    object_detect_result& result = latest_yolo_output.results.results[k]; // Fixed type
                    int left = result.box.left;
                    int top = result.box.top;
                    int right = result.box.right;
                    int bottom = result.box.bottom;
                    draw_rectangle(&src_image, left, top, right - left, bottom - top, COLOR_YELLOW, 2);
            
                    // Optionally, draw the object label
                    std::string label = coco_cls_to_name(result.cls_id);
                    draw_text(&src_image, label.c_str(), left, top - 5, COLOR_YELLOW, text_size);
                }
            }
            // 7. Calculate KSS Score
            detectedObjects.clear();
            for (int j = 0; j < latest_yolo_output.results.count; ++j) {
                detectedObjects.push_back(coco_cls_to_name(latest_yolo_output.results.results[j].cls_id));
            }

            kssCalculator.setPerclos(blinkDetector.getPerclos());
            int headPoseKSSValue = 1;
            if (calibration_done && headPoseResults.rows.size() >= 4) { /* get head KSS */ }
            kssCalculator.setHeadPose(headPoseKSSValue);
            kssCalculator.setYawnMetrics(yawnMetrics.isYawning, yawnMetrics.yawnFrequency, yawnMetrics.yawnDuration);

            // *** MODIFIED: Get current time and pass it to setDetectedObjects ***
            double now_seconds = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
            kssCalculator.setDetectedObjects(detectedObjects, now_seconds);
            // *** END MODIFICATION ***

            int blinks_last_minute = blinkDetector.getBlinksInWindow(); // Make sure getBlinksInWindow exists
            kssCalculator.setBlinksLastMinute(blinks_last_minute);


             compositeKSS = kssCalculator.calculateCompositeKSS();
             if (calibration_done) { kssStatus = kssCalculator.getKSSAlertStatus(compositeKSS); }
             else if (kssStatus != "Low Risk (Cal. Failed)" && kssStatus != "Low Risk (No Face)") { kssStatus = "Calibrating..."; }

            // Drawing (Skipped )
         

            // Draw status text, FPS, CPU, Temp etc.
            if (compositeKSS <= 3) status_color_uint = COLOR_GREEN;
            else if (compositeKSS <= 7) status_color_uint = COLOR_BLUE;
            else if (compositeKSS <= 9) status_color_uint = COLOR_YELLOW;
            else status_color_uint = COLOR_DARK_RED; // Use corrected Dark Red

            text_y = 10;
            draw_text(&src_image, kssStatus.c_str(), 10, text_y, status_color_uint, status_text_size);
            text_y += (int)(line_height * 1.4);
            // ... draw other behavior metrics ...
            text_stream.str(""); text_stream << "PERCLOS: " << std::fixed << std::setprecision(2) << blinkDetector.getPerclos() << "%"; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;
            if (calibration_done && headPoseResults.rows.size() >= 3) { /* draw head pose */ } else { /* draw N/A */}
            text_stream.str(""); text_stream << "Yawning: " << (yawnMetrics.isYawning ? "Yes" : "No"); draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;
            text_stream.str(""); text_stream << "Yawn Freq: " << static_cast<int>(yawnMetrics.yawnFrequency); draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;
            text_stream.str(""); text_stream << "Yawn Count (5min): " << static_cast<int>(yawnMetrics.yawnCount); draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height; // Display reset count


            // Resource Monitoring Drawing
            auto frame_processing_end_time = std::chrono::high_resolution_clock::now();
            double frame_duration_ms = std::chrono::duration<double, std::milli>(frame_processing_end_time - start_time).count();

            calculateOverallFPS(frame_duration_ms, frame_times, overallFPS, max_time_records);
            getCPUUsage(currentCpuUsage, prevIdleTime, prevTotalTime);
            getTemperature(currentTemp);

            // Display Info on Frame
            std::stringstream info_ss;
            info_ss << "FPS: " << std::fixed << std::setprecision(1) << overallFPS
                    << " | CPU: " << std::fixed << std::setprecision(1) << currentCpuUsage << "%"
                    << " | Temp: " << std::fixed << std::setprecision(1) << currentTemp << " C";
            std::string resource_info_text = info_ss.str();

            // Choose position (e.g., bottom-left)
            int base_line;
            cv::Size text_size_info = cv::getTextSize(resource_info_text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &base_line);
            draw_text(&src_image, resource_info_text.c_str(), 10, src_image.height - 20, COLOR_WHITE, 14);

            // --- Final Display and GStreamer Output ---
            cv::Mat display_mat_final = image_buffer_to_mat(&src_image); // Create Mat from potentially drawn-on buffer
            if (!display_mat_final.empty()) {
                cv::imshow("output", display_mat_final);
                memcpy(out_frame->data[0], src_image.virt_addr, src_image.size);
                pushFrameToPipeline(out_frame);
                if (cv::waitKey(1) == 27) break;
            } else {
                printf("WARN: Display Mat is empty!\n");
                 memcpy(out_frame->data[0], in_frame->data[0], in_frame->info.size); // Push original if display fails
                 pushFrameToPipeline(out_frame);
            }

        } else { // Frame skipped
             memcpy(out_frame->data[0], in_frame->data[0], in_frame->info.size);
             pushFrameToPipeline(out_frame);
        }

        frame_counter++;
        // Frame skipping logic...
        auto end_time = std::chrono::high_resolution_clock::now();
        // ... (rest of skipping logic) ...

        if (frame_mapped) {
            gst_video_frame_unmap(out_frame);
            gst_video_frame_unmap(in_frame);
            frame_mapped = false;
        }
        // *** FIX: Unref the correct buffer ***
        gst_buffer_unref(out_buffer);
        gst_sample_unref(sample);

    } // End while loop

    // --- Cleanup ---
    printf("Cleaning up...\n");
    delete in_frame; delete out_frame;
    stop_yolo_worker.store(true); if (yolo_worker_thread.joinable()) { yolo_worker_thread.join(); }
    // GStreamer cleanup...
     if (input_pipeline) { gst_element_set_state(input_pipeline, GST_STATE_NULL); gst_object_unref(appsink_); gst_object_unref(input_pipeline); }
     if (pipeline_) { gst_element_send_event(pipeline_, gst_event_new_eos()); gst_element_set_state(pipeline_, GST_STATE_NULL); gst_object_unref(appsrc_); gst_object_unref(pipeline_); }
     gst_deinit();
    // RKNN release...
     release_face_analyzer(&app_ctx.face_ctx); release_yolo11(&app_ctx.yolo_ctx); deinit_post_process();
    printf("Exiting (ret = %d)\n", ret);
    return ret;
}