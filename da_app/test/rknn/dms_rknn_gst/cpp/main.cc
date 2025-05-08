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
#include <opencv2/core/types.hpp> // Needed for cv::Rect, cv::Point
#include <opencv2/opencv.hpp>
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

// Include project headers
#include "face_analyzer/face_analyzer.h"
#include "yolo_detector/yolo11.h"
#include "behavior_analysis/BlinkDetector.hpp"
#include "behavior_analysis/YawnDetector.hpp"
#include "behavior_analysis/HeadPoseTracker.hpp"
#include "behavior_analysis/KSSCalculator.hpp"
#include "image_utils.h"         // Make sure crop_image_simple is declared here or implement it
#include "file_utils.h"
#include "image_drawing.h"

// GStreamer Includes
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
#include <gst/allocators/gstdmabuf.h>
#include <gst/app/gstappsrc.h> 

// Resource Monitoring Includes
#include <fstream>
#include <numeric>
#include <deque>

// --- Debugging Flags ---
#define DEBUG_DRAW_ROIS // Comment out to disable drawing ROIs

// --- Define colors ---
#ifndef COLOR_MAGENTA
#define COLOR_MAGENTA (0xFF00FF) // Pinkish/Magenta
#endif
#ifndef COLOR_YELLOW
#define COLOR_YELLOW (0x00FFFF) // Yellow
#endif
#ifndef COLOR_WHITE
#define COLOR_WHITE (0xFFFFFF) // White
#endif
#ifndef COLOR_GREEN
#define COLOR_GREEN (0x00FF00) // Green
#endif
#ifndef COLOR_RED
#define COLOR_RED (0x0000FF) // Red
#endif
#ifndef COLOR_BLUE
#define COLOR_BLUE (0xFF0000) // Blue
#endif
#ifndef COLOR_ORANGE
#define COLOR_ORANGE (0x00A5FF) // Orange
#endif
#ifndef COLOR_CYAN
#define COLOR_CYAN (0xFFFF00) // Cyan
#endif


// --- Application Context ---
typedef struct app_context_t {
    face_analyzer_app_context_t face_ctx;
    yolo11_app_context_t        yolo_ctx;
} app_context_t;

// --- Global GStreamer elements ---
static GstElement* pipeline_ = nullptr; // Saving pipeline
static GstElement* appsrc_ = nullptr;   // Source for saving pipeline
static GstElement* appsink_ = nullptr;  // Sink for input pipeline

// --- Helper Functions ---
// ... (Keep all helper functions: calculate_centroid, convert_landmarks_to_cvpoint, calculate_ear_simple, calculate_mouth_dist_simple, parse_head_pose_value, calculate_stddev) ...
cv::Point calculate_centroid(const point_t landmarks[], int count) { if (count == 0) return cv::Point(-1, -1); long long sum_x = 0, sum_y = 0; int valid_points = 0; for (int i = 0; i < count; ++i) { if (landmarks[i].x > -10000 && landmarks[i].x < 10000 && landmarks[i].y > -10000 && landmarks[i].y < 10000) { sum_x += landmarks[i].x; sum_y += landmarks[i].y; valid_points++; } } if (valid_points == 0) return cv::Point(-1, -1); return cv::Point(static_cast<int>(sum_x / valid_points), static_cast<int>(sum_y / valid_points)); }
std::vector<cv::Point> convert_landmarks_to_cvpoint(const point_t landmarks[], int count) { std::vector<cv::Point> cv_landmarks; if (count <= 0) return cv_landmarks; cv_landmarks.reserve(count); for (int i = 0; i < count; ++i) { if (landmarks[i].x > -10000 && landmarks[i].x < 10000 && landmarks[i].y > -10000 && landmarks[i].y < 10000) { cv_landmarks.emplace_back(cv::Point(landmarks[i].x, landmarks[i].y)); } } return cv_landmarks; }
float calculate_ear_simple(const std::vector<cv::Point>& landmarks, const std::vector<int>& eye_points) { if (landmarks.empty()) return 1.0f; int max_idx = 0; for(int idx : eye_points) { if (idx > max_idx) max_idx = idx; } if (max_idx >= landmarks.size()) { return 1.0f; } try { cv::Point p1=landmarks.at(eye_points.at(0)); cv::Point p2=landmarks.at(eye_points.at(1)); cv::Point p3=landmarks.at(eye_points.at(2)); cv::Point p4=landmarks.at(eye_points.at(3)); cv::Point p5=landmarks.at(eye_points.at(4)); cv::Point p6=landmarks.at(eye_points.at(5)); double v1=cv::norm(p2-p6); double v2=cv::norm(p3-p5); double h=cv::norm(p1-p4); if(h<1e-6) return 1.0f; return static_cast<float>((v1+v2)/(2.0*h)); } catch(const std::out_of_range& oor) { std::cerr << "Out of Range error in calculate_ear_simple: " << oor.what() << std::endl; return 1.0f; } catch(...) { std::cerr << "Unknown exception in calculate_ear_simple" << std::endl; return 1.0f; } }
double calculate_mouth_dist_simple(const std::vector<cv::Point>& landmarks) { if (landmarks.empty() || landmarks.size() <= 14) return 0.0; try { return cv::norm(landmarks.at(13)-landmarks.at(14)); } catch(const std::out_of_range& oor) { std::cerr << "Out of Range error in calculate_mouth_dist_simple: " << oor.what() << std::endl; return 0.0; } catch (...) { std::cerr << "Unknown exception in calculate_mouth_dist_simple" << std::endl; return 0.0; } }
double parse_head_pose_value(const std::string& s) { try { std::string n=s; size_t d=n.find(" deg"); if(d!=std::string::npos) n=n.substr(0,d); size_t f=n.find_first_not_of(" \t"); if(f==std::string::npos) return 0.0; size_t l=n.find_last_not_of(" \t"); n=n.substr(f,l-f+1); if(n.empty()) return 0.0; return std::stod(n); } catch (const std::exception& e) { printf("WARN: Ex parse head pose '%s': %s\n", s.c_str(), e.what()); return 0.0; } catch (...) { printf("WARN: Unk ex parse head pose '%s'.\n", s.c_str()); return 0.0; } }
template <typename T> T calculate_stddev(const std::deque<T>& data) { if (data.size() < 2) return T(0); T sum = std::accumulate(data.begin(), data.end(), T(0)); T mean = sum / data.size(); T sq_sum = std::inner_product(data.begin(), data.end(), data.begin(), T(0)); T variance = sq_sum / data.size() - mean * mean; return std::sqrt(std::max(T(0), variance)); }

// --- Simple CPU Cropping Function ---
static int crop_image_simple(image_buffer_t *src_img, image_buffer_t *dst_img, box_rect_t crop_box) {
    if (!src_img || !src_img->virt_addr || !dst_img) {
        printf("ERROR: crop_image_simple null input/output pointer.\n");
        return -1;
    }

    int channels = 0;
    if (src_img->format == IMAGE_FORMAT_RGB888) {
        channels = 3;
    }
// Put preprocessor directives on separate lines
#ifdef IMAGE_FORMAT_BGR888
    else if (src_img->format == IMAGE_FORMAT_BGR888) {
        channels = 3;
    }
#endif
    // Add checks for other formats if needed, otherwise error out
    else if (channels == 0) { // Check if format wasn't handled
        printf("ERROR: crop_image_simple unsupported format %d\n", src_img->format);
        return -1;
    }

    int src_w = src_img->width;
    int src_h = src_img->height;
    int crop_x = crop_box.left;
    int crop_y = crop_box.top;
    int crop_w = crop_box.right - crop_box.left;
    int crop_h = crop_box.bottom - crop_box.top;

    if (crop_w <= 0 || crop_h <= 0) {
        printf("ERROR: crop ROI invalid size (%dx%d) from box [%d,%d,%d,%d]\n",
               crop_w, crop_h, crop_box.left, crop_box.top, crop_box.right, crop_box.bottom);
        return -1;
    }

    int x_start = std::max(0, crop_x);
    int y_start = std::max(0, crop_y);
    int x_end = std::min(src_w, crop_x + crop_w);
    int y_end = std::min(src_h, crop_y + crop_h);
    int valid_crop_w = x_end - x_start;
    int valid_crop_h = y_end - y_start;

    if (valid_crop_w <= 0 || valid_crop_h <= 0) {
        printf("ERROR: Clamped crop ROI zero size. Original ROI [%d,%d,%d,%d], Clamped to src %dx%d -> [%d,%d,%d,%d]\n",
               crop_box.left, crop_box.top, crop_box.right, crop_box.bottom,
               src_w, src_h,
               x_start, y_start, x_end, y_end);
        return -1;
    }

    dst_img->width = crop_w;
    dst_img->height = crop_h;
    dst_img->format = src_img->format; // Keep original format
    dst_img->size = crop_w * crop_h * channels; // Use calculated channels

    // Allocate or check destination buffer
    if (dst_img->virt_addr == NULL) {
        dst_img->virt_addr = (unsigned char*)malloc(dst_img->size);
        if (!dst_img->virt_addr) {
            printf("ERROR: Failed alloc memory for crop (%d bytes)\n", dst_img->size);
            return -1;
        }
    } else if (dst_img->size < (size_t)(crop_w * crop_h * channels)) {
        // Optional: Realloc if needed, or return error
        printf("ERROR: Dest buffer size %d too small for crop %d.\n", dst_img->size, crop_w * crop_h * channels);
        // Consider reallocating:
        // void* new_addr = realloc(dst_img->virt_addr, dst_img->size);
        // if (!new_addr) { /* handle realloc failure */ return -1; }
        // dst_img->virt_addr = (unsigned char*)new_addr;
        return -1; // Current behavior is error
    }

    memset(dst_img->virt_addr, 0, dst_img->size); // Clear destination

    unsigned char* src_data = src_img->virt_addr;
    unsigned char* dst_data = dst_img->virt_addr;
    // Use width_stride if provided, otherwise calculate based on width
    size_t src_stride = src_img->width_stride > 0 ? src_img->width_stride : (size_t)src_w * channels;
    size_t dst_stride = (size_t)crop_w * channels; // Destination is assumed packed
    int dst_x_offset = x_start - crop_x;
    int dst_y_offset = y_start - crop_y;

    // Copy valid region row by row
    for (int y = 0; y < valid_crop_h; ++y) {
        unsigned char* src_row_ptr = src_data + (size_t)(y_start + y) * src_stride + (size_t)x_start * channels;
        unsigned char* dst_row_ptr = dst_data + (size_t)(dst_y_offset + y) * dst_stride + (size_t)dst_x_offset * channels;
        memcpy(dst_row_ptr, src_row_ptr, (size_t)valid_crop_w * channels);
    }
    return 0;
}
// --- End Helper Functions ---


// --- YOLO Worker Thread ---
struct YoloInputData { long frame_id; std::shared_ptr<image_buffer_t> image; }; struct YoloOutputData { long frame_id; object_detect_result_list results; }; std::queue<YoloInputData> yolo_input_queue; std::queue<YoloOutputData> yolo_output_queue; std::mutex yolo_input_mutex; std::mutex yolo_output_mutex; std::atomic<bool> stop_yolo_worker(false); const int MAX_QUEUE_SIZE = 5;
void yolo_worker_thread_func(yolo11_app_context_t* yolo_ctx_ptr) { while (!stop_yolo_worker.load()) { YoloInputData input_data; bool got_data = false; { std::unique_lock<std::mutex> lock(yolo_input_mutex); if (!yolo_input_queue.empty()) { input_data = yolo_input_queue.front(); yolo_input_queue.pop(); got_data = true; } } if (got_data && input_data.image) { YoloOutputData output_data; output_data.frame_id = input_data.frame_id; memset(&output_data.results, 0, sizeof(output_data.results)); int ret = inference_yolo11(yolo_ctx_ptr, input_data.image.get(), &output_data.results); if (ret != 0) printf("WARN: YOLO Worker inference failed (frame %ld), ret=%d\n", input_data.frame_id, ret); { std::unique_lock<std::mutex> lock(yolo_output_mutex); if (yolo_output_queue.size() >= MAX_QUEUE_SIZE) yolo_output_queue.pop(); yolo_output_queue.push(output_data); } if (input_data.image->virt_addr) { free(input_data.image->virt_addr); input_data.image->virt_addr = nullptr; } } else { std::this_thread::sleep_for(std::chrono::milliseconds(5)); } } printf("YOLO Worker Thread Exiting.\n");}



// void setupPipeline() { /* ... (keep as is) ... */ gst_init(nullptr, nullptr); const std::string dir = "/userdata/test_cpp/dms_gst"; struct stat st = {0}; if (stat(dir.c_str(), &st) == -1) { if (mkdir(dir.c_str(), 0700) == -1) { std::cerr << "Error: Cannot create directory " << dir << ": " << strerror(errno) << std::endl; return; } std::cout << "INFO: Created directory " << dir << std::endl; } else if (access(dir.c_str(), W_OK) != 0) { std::cerr << "Error: Directory " << dir << " is not writable." << std::endl; return; } DIR* directory = opendir(dir.c_str()); if (!directory) { std::cerr << "Failed to open directory " << dir << "\n"; return; } int mkv_count = 0; struct dirent* entry; while ((entry = readdir(directory)) != nullptr) { if (std::string_view(entry->d_name).find(".mkv") != std::string_view::npos) { ++mkv_count; } } closedir(directory); const std::string filepath = dir + "/dms_multi_" + std::to_string(mkv_count + 1) + ".mkv"; const std::string pipeline_str = "appsrc name=source ! queue ! videoconvert ! video/x-raw,format=NV12 ! mpph265enc rc-mode=cbr bps=4000000 gop=30 qp-min=10 qp-max=51 ! h265parse ! matroskamux ! filesink location=" + filepath; std::cout << "Saving Pipeline: " << pipeline_str << "\n"; GError* error = nullptr; pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error); if (!pipeline_ || error) { std::cerr << "Failed to create saving pipeline: " << (error ? error->message : "Unknown error") << "\n"; if (error) { g_error_free(error); } return; } appsrc_ = gst_bin_get_by_name(GST_BIN(pipeline_), "source"); if (!appsrc_) { std::cerr << "Failed to get appsrc\n"; gst_object_unref(pipeline_); pipeline_ = nullptr; return; } g_object_set(G_OBJECT(appsrc_), "stream-type", GST_APP_STREAM_TYPE_STREAM, "format", GST_FORMAT_TIME, "is-live", FALSE, NULL); if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) { std::cerr << "Failed to set saving pipeline to playing state\n"; gst_object_unref(appsrc_); gst_object_unref(pipeline_); appsrc_ = nullptr; pipeline_ = nullptr; } else { std::cout << "INFO: Saving pipeline created and set to PLAYING (waiting for caps/data).\n"; } }

// void pushFrameToPipeline(unsigned char* data, int size, int width, int height, GstClockTime duration) { if (!appsrc_) return; GstBuffer* buffer = gst_buffer_new_allocate(nullptr, size, nullptr); GstMapInfo map; if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) { std::cerr << "Failed map buffer" << std::endl; gst_buffer_unref(buffer); return; } if (map.size != (guint)size) { std::cerr << "Buffer size mismatch: " << map.size << " vs " << size << std::endl; gst_buffer_unmap(buffer, &map); gst_buffer_unref(buffer); return; } memcpy(map.data, data, size); gst_buffer_unmap(buffer, &map); static GstClockTime timestamp = 0; GST_BUFFER_PTS(buffer) = timestamp; GST_BUFFER_DURATION(buffer) = duration; timestamp += GST_BUFFER_DURATION(buffer); GstFlowReturn ret; g_signal_emit_by_name(appsrc_, "push-buffer", buffer, &ret); if (ret != GST_FLOW_OK) std::cerr << "Failed push buffer, ret=" << ret << std::endl; gst_buffer_unref(buffer); }


void setupPipeline() {
    gst_init(nullptr, nullptr);
    const std::string dir = "/userdata/test_cpp/dms_gst";
    // ... (directory checking/creation logic remains the) ...
    struct stat st = {0}; if (stat(dir.c_str(), &st) == -1) { if (mkdir(dir.c_str(), 0700) == -1) { std::cerr << "Error: Cannot create directory " << dir << ": " << strerror(errno) << std::endl; return; } std::cout << "INFO: Created directory " << dir << std::endl; } else if (access(dir.c_str(), W_OK) != 0) { std::cerr << "Error: Directory " << dir << " is not writable." << std::endl; return; } DIR* directory = opendir(dir.c_str()); if (!directory) { std::cerr << "Failed to open directory " << dir << "\n"; return; } int mkv_count = 0; struct dirent* entry; while ((entry = readdir(directory)) != nullptr) { if (std::string_view(entry->d_name).find(".mkv") != std::string_view::npos) { ++mkv_count; } } closedir(directory); const std::string filepath = dir + "/dms_multi_" + std::to_string(mkv_count + 1) + ".mkv";

    const std::string pipeline_str = "appsrc name=source ! queue ! videoconvert ! video/x-raw,format=NV12 ! mpph265enc rc-mode=cbr bps=4000000 gop=30 qp-min=10 qp-max=51 ! h265parse ! matroskamux ! filesink location=" + filepath;
    std::cout << "Saving Pipeline: " << pipeline_str << "\n";
    GError* error = nullptr;
    pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
    if (!pipeline_ || error) {
        std::cerr << "Failed to create saving pipeline: " << (error ? error->message : "Unknown error") << "\n";
        if (error) { g_error_free(error); }
        return;
    }
    appsrc_ = gst_bin_get_by_name(GST_BIN(pipeline_), "source");
    if (!appsrc_) {
        std::cerr << "Failed to get appsrc\n";
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
        return;
    }

    // Configure appsrc properties
    g_object_set(G_OBJECT(appsrc_),
                 "stream-type", GST_APP_STREAM_TYPE_STREAM, // Use STREAM for continuous data
                 "format", GST_FORMAT_TIME,
                 "is-live", FALSE,         // Important for filesrc input, allows seeking/rate changes potentially
                 "do-timestamp", TRUE,     // <--- ADD THIS LINE: Let appsrc create PTS
                 NULL);

    // Setting caps will be done dynamically in the main loop now

    if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set saving pipeline to playing state\n";
        gst_object_unref(appsrc_);
        gst_object_unref(pipeline_);
        appsrc_ = nullptr;
        pipeline_ = nullptr;
    } else {
        std::cout << "INFO: Saving pipeline created and set to PLAYING (waiting for caps/data).\n";
    }
}

void pushFrameToPipeline(unsigned char* data, int size, int width, int height, GstClockTime duration) { // Keep duration parameter
    if (!appsrc_) return;
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        std::cerr << "Failed map buffer" << std::endl;
        gst_buffer_unref(buffer);
        return;
    }
    if (map.size != (guint)size) {
        std::cerr << "Buffer size mismatch: " << map.size << " vs " << size << std::endl;
        gst_buffer_unmap(buffer, &map);
        gst_buffer_unref(buffer);
        return;
    }
    memcpy(map.data, data, size);
    gst_buffer_unmap(buffer, &map);

    // KEEP Duration setting (useful metadata for encoder/muxer)
    GST_BUFFER_DURATION(buffer) = duration;

    GstFlowReturn ret;
    g_signal_emit_by_name(appsrc_, "push-buffer", buffer, &ret);
    if (ret != GST_FLOW_OK) {
        // Handle potential downstream errors (e.g., queue full, EOS)
        std::cerr << "Failed push buffer, ret=" << gst_flow_get_name(ret) << std::endl;
        if (ret == GST_FLOW_ERROR || ret == GST_FLOW_NOT_LINKED || ret == GST_FLOW_FLUSHING) {
           // Consider stopping the pipeline or handling the error appropriately
        }
    }
    gst_buffer_unref(buffer);
}

// --- Resource Monitoring Functions ---
void calculateOverallFPS(double frame_duration_ms, std::deque<double>& times_deque, double& fps_variable, int max_records) { times_deque.push_back(frame_duration_ms); if (times_deque.size() > max_records) times_deque.pop_front(); if (!times_deque.empty()) { double sum = std::accumulate(times_deque.begin(), times_deque.end(), 0.0); double avg_time_ms = sum / times_deque.size(); fps_variable = (avg_time_ms > 0) ? (1000.0 / avg_time_ms) : 0.0; } else { fps_variable = 0.0; } }
void getCPUUsage(double& cpu_usage_variable, long& prev_idle, long& prev_total) { std::ifstream file("/proc/stat"); if (!file.is_open()) return; std::string line; std::getline(file, line); file.close(); long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice; user = nice = system = idle = iowait = irq = softirq = steal = guest = guest_nice = 0; std::istringstream iss(line); std::string cpu_label; iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice; long currentIdleTime = idle + iowait; long currentTotalTime = user + nice + system + idle + iowait + irq + softirq + steal; long diffIdle = currentIdleTime - prev_idle; long diffTotal = currentTotalTime - prev_total; if (diffTotal > 0) cpu_usage_variable = 100.0 * (double)(diffTotal - diffIdle) / diffTotal; prev_idle = currentIdleTime; prev_total = currentTotalTime; }
void getTemperature(double& temp_variable) { const char* temp_paths[] = {"/sys/class/thermal/thermal_zone0/temp", "/sys/class/thermal/thermal_zone1/temp"}; bool temp_read = false; for (const char* path : temp_paths) { std::ifstream file(path); double temp_milliC = 0; if (file >> temp_milliC) { temp_variable = temp_milliC / 1000.0; temp_read = true; file.close(); break; } file.close(); } }


/*-------------------------------------------
                  Main Function
-------------------------------------------*/
int main(int argc, char **argv) {

    // ... (Variable declarations: ret, frame_id, monitoring vars, model paths, video source) ...
    int ret = 0; long current_frame_id = 0; std::deque<double> frame_times; const int max_time_records = 60; double overallFPS = 0.0; double currentCpuUsage = 0.0; long prevIdleTime = 0, prevTotalTime = 0; double currentTemp = 0.0;

    // const char *detection_model_path = "../../model/faceD.rknn";
    const char *detection_model_path = "../../model/rf.rknn";
    const char *landmark_model_path  = "../../model/faceL.rknn";
    const char *iris_model_path = "../../model/faceI.rknn";
    const char *yolo_model_path = "../../model/od.rknn";

    const char *video_source = "filesrc location=../../model/ADG_R_dms.mkv ! decodebin ! queue ! videoconvert ! video/x-raw,format=BGR ! appsink name=sink sync=false";
    
    // const char *video_source = "v4l2src device=/dev/video0 ! queue ! videoconvert ! video/x-raw,format=BGR,width=1920,height=1080,framerate=30/1 ! appsink name=sink sync=false";


    // --- Initialization ---
    setupPipeline(); // Setup saving pipeline (without caps)
    // ... (Init contexts, buffers, behavior modules) ...
    app_context_t app_ctx; memset(&app_ctx, 0, sizeof(app_context_t)); image_buffer_t src_image; memset(&src_image, 0, sizeof(image_buffer_t)); face_analyzer_result_t face_results; memset(&face_results, 0, sizeof(face_results)); object_detect_result_list yolo_results; memset(&yolo_results, 0, sizeof(yolo_results)); my::BlinkDetector blinkDetector; YawnDetector yawnDetector; my::HeadPoseTracker headPoseTracker; KSSCalculator kssCalculator;

    // ... (Calibration vars) ...
     bool calibration_done = false; std::chrono::steady_clock::time_point calibration_start_time; bool calibration_timer_started = false; int consecutive_valid_eyes_frames = 0; const int REQUIRED_VALID_EYES_FRAMES = 60; bool ear_calibrated = false; bool mouth_calibrated = false; std::deque<float> calib_left_ears; std::deque<float> calib_right_ears; std::deque<double> calib_mouth_dists; int consecutive_stable_ear_frames = 0; int consecutive_stable_mouth_frames = 0; const int CALIB_WINDOW_SIZE = 30; const float EAR_STDDEV_THRESHOLD = 0.04; const double MOUTH_DIST_STDDEV_THRESHOLD = 15.0; const int REQUIRED_STABLE_FRAMES = CALIB_WINDOW_SIZE + 5; const double CALIBRATION_TIMEOUT_SECONDS = 10.0;

    // ... (Driver ID vars) ...
     bool driver_identified_ever = false; bool driver_tracked_this_frame = false; int current_tracked_driver_idx = -1; cv::Point prev_driver_centroid = cv::Point(-1, -1); const double MAX_CENTROID_DISTANCE = 150.0; int driver_search_timeout_frames = 0; const int DRIVER_SEARCH_MAX_FRAMES = 60;

    // ... (Fixed ROI vars) ...
     cv::Rect driver_object_roi; bool driver_roi_defined = false; bool valid_object_roi = false;

    // ... (KSS/Display vars) ...
     std::string kssStatus = "Initializing"; YawnDetector::YawnMetrics yawnMetrics = {}; my::HeadPoseTracker::HeadPoseResults headPoseResults = {}; std::vector<std::string> detectedObjects; int extractedTotalKSS = 1; int perclosKSS = 1, blinkKSS = 1, headposeKSS = 1, yawnKSS = 1, objdectdetectionKSS = 1; std::stringstream text_stream; int text_y = 0; const int line_height = 22; const int text_size = 14; const int status_text_size = 16; unsigned int status_color_uint = COLOR_GREEN;

    // --- YOLO thread handle ---
    std::thread yolo_worker_thread;

    // --- Init models ---
    // ... (Model init) ...
     ret = init_post_process(); if (ret != 0) { /*...*/ return -1; } ret = init_face_analyzer(detection_model_path, landmark_model_path, iris_model_path, &app_ctx.face_ctx); if (ret != 0) { /*...*/ return -1; } ret = init_yolo11(yolo_model_path, &app_ctx.yolo_ctx); if (ret != 0) { /*...*/ return -1; } yolo_worker_thread = std::thread(yolo_worker_thread_func, &app_ctx.yolo_ctx);

    // --- Init GStreamer input ---
    // ... (GStreamer input init) ...
     gst_init(nullptr, nullptr); GError* error = nullptr; GstElement* input_pipeline = gst_parse_launch(video_source, &error); if (!input_pipeline || error) { /*...*/ return -1; } appsink_ = gst_bin_get_by_name(GST_BIN(input_pipeline), "sink"); if (!appsink_) { /*...*/ return -1; } if (gst_element_set_state(input_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) { /*...*/ return -1; }

    // --- Setup OpenCV window ---
    cv::namedWindow("DMS Output", cv::WINDOW_NORMAL); cv::resizeWindow("DMS Output", 1920, 1080);

    GstSample* sample; GstVideoInfo video_info; GstBuffer* gst_buffer; GstMapInfo map_info;
    // bool first_frame = true; // Can remove this if only used for ROI def
    bool saving_pipeline_caps_set = false; // <<< Flag to track if saving caps are set

    // --- Buffer for the FIXED ROI Crop ---
    image_buffer_t fixed_roi_crop_img;
    memset(&fixed_roi_crop_img, 0, sizeof(image_buffer_t));

    // --- Main Processing Loop ---
    while (true) {
        // --- Get Frame ---
         sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink_)); if (!sample) { std::cerr << "End of stream or error pulling sample." << std::endl; break; } gst_buffer = gst_sample_get_buffer(sample); GstCaps* caps = gst_sample_get_caps(sample); if (!gst_buffer || !caps || !gst_video_info_from_caps(&video_info, caps) || !gst_buffer_map(gst_buffer, &map_info, GST_MAP_READ)) { std::cerr << "Failed to get valid buffer/caps/map" << std::endl; if (gst_buffer && map_info.memory) gst_buffer_unmap(gst_buffer, &map_info); if (sample) gst_sample_unref(sample); continue; }

        auto frame_start_time = std::chrono::high_resolution_clock::now();
        current_frame_id++;

        // --- Wrap GStreamer Data for the *full source image* ---
        src_image.width = GST_VIDEO_INFO_WIDTH(&video_info);
        src_image.height = GST_VIDEO_INFO_HEIGHT(&video_info);
        src_image.width_stride = GST_VIDEO_INFO_PLANE_STRIDE(&video_info, 0);
        src_image.height_stride = src_image.height;
        // Determine format based on GStreamer info if possible, otherwise default
        GstVideoFormat gst_format = GST_VIDEO_INFO_FORMAT(&video_info);


        src_image.format = IMAGE_FORMAT_RGB888;
        src_image.virt_addr = map_info.data;
        src_image.size = map_info.size;
        src_image.fd = -1;


        // --- Set Saving Pipeline Caps (on first valid frame) ---
        // if (pipeline_ && appsrc_ && !saving_pipeline_caps_set) {
        //      GstCaps* save_caps = gst_caps_new_simple("video/x-raw",
        //                                      "format", G_TYPE_STRING, "BGR", // Input to saving pipeline is BGR
        //                                      "width", G_TYPE_INT, src_image.width,
        //                                      "height", G_TYPE_INT, src_image.height,
        //                                      "framerate", GST_TYPE_FRACTION, GST_VIDEO_INFO_FPS_N(&video_info), GST_VIDEO_INFO_FPS_D(&video_info),
        //                                      NULL);
        //      if (save_caps) {
        //           printf("INFO: Setting saving pipeline caps to: %s\n", gst_caps_to_string(save_caps));
        //           g_object_set(G_OBJECT(appsrc_), "caps", save_caps, NULL);
        //           gst_caps_unref(save_caps);
        //           saving_pipeline_caps_set = true;
        //      } else {
        //           std::cerr << "ERROR: Failed to create caps for saving pipeline!\n";
        //           // Consider stopping or disabling saving
        //      }
        // }

        if (pipeline_ && appsrc_ && !saving_pipeline_caps_set && video_info.fps_n > 0) { // Check if framerate is valid
            GstCaps* save_caps = gst_caps_new_simple("video/x-raw",
                                            "format", G_TYPE_STRING, "BGR", // Input to saving pipeline is BGR
                                            "width", G_TYPE_INT, src_image.width,
                                            "height", G_TYPE_INT, src_image.height,
                                            "framerate", GST_TYPE_FRACTION, video_info.fps_n, video_info.fps_d, // USE ACTUAL FRAMERATE FROM INPUT
                                            NULL);
            if (save_caps) {
                 printf("INFO: Setting saving pipeline caps to: %s\n", gst_caps_to_string(save_caps));
                 g_object_set(G_OBJECT(appsrc_), "caps", save_caps, NULL);
                 gst_caps_unref(save_caps);
                 saving_pipeline_caps_set = true; // Set caps only once
            } else {
                 std::cerr << "ERROR: Failed to create caps for saving pipeline!\n";
            }
        }
        // --- End Set Saving Caps ---


        // --- Run Face Analysis on the FULL FRAME ---
        ret = inference_face_analyzer(&app_ctx.face_ctx, &src_image, &face_results);
        if (ret != 0) { printf("WARN: Face Analyzer Inference failed frame %ld, ret=%d\n", current_frame_id, ret); face_results.count = 0; }

        // --- Reset frame-specific flags ---
        driver_tracked_this_frame = false;
        current_tracked_driver_idx = -1;
        valid_object_roi = false; // Reset each frame

        // --- Driver Identification and Tracking (Based on Full Frame Results) ---
        if (!driver_identified_ever) { /* Identification */ int best_candidate_idx = -1; float max_area = 0.0f; cv::Rect temp_id_zone((int)(src_image.width * 0.40),(int)(src_image.height * 0.1),(int)(src_image.width * 0.55),(int)(src_image.height * 0.8)); temp_id_zone &= cv::Rect(0, 0, src_image.width, src_image.height); for (int i = 0; i < face_results.count; ++i) { cv::Point face_center((face_results.faces[i].box.left + face_results.faces[i].box.right) / 2, (face_results.faces[i].box.top + face_results.faces[i].box.bottom) / 2); if (temp_id_zone.contains(face_center) && face_results.faces[i].face_landmarks_valid) { float area = (float)(face_results.faces[i].box.right - face_results.faces[i].box.left) * (face_results.faces[i].box.bottom - face_results.faces[i].box.top); if (area > max_area) { max_area = area; best_candidate_idx = i; } } } if (best_candidate_idx != -1) { driver_identified_ever = true; driver_tracked_this_frame = true; current_tracked_driver_idx = best_candidate_idx; prev_driver_centroid = calculate_centroid(face_results.faces[best_candidate_idx].face_landmarks, NUM_FACE_LANDMARKS); driver_search_timeout_frames = 0; printf("INFO: Driver Identified (Index %d) at frame %ld\n", current_tracked_driver_idx, current_frame_id); kssStatus = "Calibrating..."; } else { kssStatus = "Searching Driver..."; } } else { /* Tracking */ int best_match_idx = -1; double min_dist = MAX_CENTROID_DISTANCE; for (int i = 0; i < face_results.count; ++i) { if (face_results.faces[i].face_landmarks_valid) { cv::Point current_centroid = calculate_centroid(face_results.faces[i].face_landmarks, NUM_FACE_LANDMARKS); if (prev_driver_centroid.x >= 0 && current_centroid.x >= 0) { double dist = cv::norm(current_centroid - prev_driver_centroid); if (dist < min_dist) { min_dist = dist; best_match_idx = i; } } } } if (best_match_idx != -1) { driver_tracked_this_frame = true; current_tracked_driver_idx = best_match_idx; prev_driver_centroid = calculate_centroid(face_results.faces[best_match_idx].face_landmarks, NUM_FACE_LANDMARKS); driver_search_timeout_frames = 0; if (!calibration_done) kssStatus = "Calibrating..."; } else { driver_tracked_this_frame = false; current_tracked_driver_idx = -1; prev_driver_centroid = cv::Point(-1, -1); driver_search_timeout_frames++; kssStatus = "Driver Lost..."; if (driver_search_timeout_frames > DRIVER_SEARCH_MAX_FRAMES) { driver_identified_ever = false; driver_search_timeout_frames = 0; printf("INFO: Driver track lost for %d frames. Reverting to search.\n", DRIVER_SEARCH_MAX_FRAMES); kssStatus = "Searching Driver..."; } } }


        // --- Calibration or Normal Processing ---
        // if (!calibration_done) {
        //     // --- Calibration Logic ---
        //      bool head_pose_calibrated = false; bool eyes_consistently_valid = false; double elapsed_calib_seconds = 0.0; std::string calib_status_detail = ""; if (driver_tracked_this_frame && current_tracked_driver_idx != -1) { face_object_t *calib_face = &face_results.faces[current_tracked_driver_idx]; if (calib_face->face_landmarks_valid) { if (!calibration_timer_started) { /* Start timer, reset state */ calibration_start_time = std::chrono::steady_clock::now(); calibration_timer_started = true; printf("INFO: Calibration timer started (Driver Index %d).\n", current_tracked_driver_idx); consecutive_valid_eyes_frames = 0; consecutive_stable_ear_frames = 0; consecutive_stable_mouth_frames = 0; calib_left_ears.clear(); calib_right_ears.clear(); calib_mouth_dists.clear(); ear_calibrated = false; mouth_calibrated = false; } /* ... Run checks ... */ std::vector<cv::Point> calib_faceLandmarks = convert_landmarks_to_cvpoint(calib_face->face_landmarks, NUM_FACE_LANDMARKS); headPoseTracker.run(calib_faceLandmarks); head_pose_calibrated = headPoseTracker.isCalibrated(); if (!head_pose_calibrated) calib_status_detail += " (Head)"; bool eyes_valid_this_frame = calib_face->eye_landmarks_left_valid && calib_face->eye_landmarks_right_valid; if (eyes_valid_this_frame) consecutive_valid_eyes_frames++; else consecutive_valid_eyes_frames = 0; eyes_consistently_valid = (consecutive_valid_eyes_frames >= REQUIRED_VALID_EYES_FRAMES); if (!eyes_consistently_valid) calib_status_detail += " (Eyes Valid)"; if (!ear_calibrated && eyes_valid_this_frame) { const std::vector<int> L={33,160,158,133,153,144}, R={362,385,387,263,380,373}; float lE=calculate_ear_simple(calib_faceLandmarks,L), rE=calculate_ear_simple(calib_faceLandmarks,R); calib_left_ears.push_back(lE); calib_right_ears.push_back(rE); if (calib_left_ears.size()>CALIB_WINDOW_SIZE) calib_left_ears.pop_front(); if (calib_right_ears.size()>CALIB_WINDOW_SIZE) calib_right_ears.pop_front(); if (calib_left_ears.size()>=CALIB_WINDOW_SIZE) { float lS=calculate_stddev(calib_left_ears), rS=calculate_stddev(calib_right_ears); if (lS<EAR_STDDEV_THRESHOLD && rS<EAR_STDDEV_THRESHOLD) consecutive_stable_ear_frames++; else consecutive_stable_ear_frames=0; } else consecutive_stable_ear_frames=0; ear_calibrated=(consecutive_stable_ear_frames>=REQUIRED_STABLE_FRAMES); } else if(!eyes_valid_this_frame){ consecutive_stable_ear_frames=0; ear_calibrated=false; calib_left_ears.clear(); calib_right_ears.clear(); } if(!ear_calibrated) calib_status_detail+=" (EAR Stable)"; if (!mouth_calibrated) { double cM=calculate_mouth_dist_simple(calib_faceLandmarks); calib_mouth_dists.push_back(cM); if(calib_mouth_dists.size()>CALIB_WINDOW_SIZE) calib_mouth_dists.pop_front(); if(calib_mouth_dists.size()>=CALIB_WINDOW_SIZE){ double mS=calculate_stddev(calib_mouth_dists); if(mS<MOUTH_DIST_STDDEV_THRESHOLD) consecutive_stable_mouth_frames++; else consecutive_stable_mouth_frames=0; } else consecutive_stable_mouth_frames=0; mouth_calibrated=(consecutive_stable_mouth_frames>=REQUIRED_STABLE_FRAMES); } if(!mouth_calibrated) calib_status_detail+=" (Mouth Stable)"; auto now = std::chrono::steady_clock::now(); elapsed_calib_seconds = std::chrono::duration<double>(now - calibration_start_time).count(); if (elapsed_calib_seconds >= CALIBRATION_TIMEOUT_SECONDS) { if (head_pose_calibrated && eyes_consistently_valid && ear_calibrated && mouth_calibrated) { calibration_done = true; printf("INFO: Calibration Complete!\n"); /* Define Fixed ROI */ face_object_t *calib_driver_face = &face_results.faces[current_tracked_driver_idx]; box_rect_t calib_driver_box = calib_driver_face->box; int calib_box_w = calib_driver_box.right - calib_driver_box.left; int calib_box_h = calib_driver_box.bottom - calib_driver_box.top; if(calib_box_w > 0 && calib_box_h > 0) { int box_center_x = (calib_driver_box.left + calib_driver_box.right) / 2; int box_center_y = (calib_driver_box.top + calib_driver_box.bottom) / 2; const float width_expansion = 2.2f; const float height_expansion_up = 0.8f; const float height_expansion_down = 3.0f; int roi_width = static_cast<int>(calib_box_w * width_expansion); int roi_height = static_cast<int>(calib_box_h * (height_expansion_up + height_expansion_down)); int roi_size = std::max(roi_width, roi_height); roi_width = roi_size; roi_height = roi_size; if (roi_width < 640) roi_width = 640; int roi_x = box_center_x - roi_width / 2; int roi_y = box_center_y - static_cast<int>(calib_box_h * height_expansion_up); driver_object_roi.x = std::max(0, roi_x); driver_object_roi.y = std::max(0, roi_y); driver_object_roi.width = std::min(roi_width, src_image.width - driver_object_roi.x); driver_object_roi.height = std::min(roi_height, src_image.height - driver_object_roi.y); driver_roi_defined = (driver_object_roi.width > 0 && driver_object_roi.height > 0); printf("INFO: Fixed Driver Object ROI defined: [%d, %d, %d x %d]\n", driver_object_roi.x, driver_object_roi.y, driver_object_roi.width, driver_object_roi.height); } else { driver_roi_defined = false; printf("WARN: Could not define fixed ROI due to invalid calibration box.\n"); } } else { /* Timeout Failed */ calibration_timer_started = false; driver_identified_ever = false; driver_tracked_this_frame = false; current_tracked_driver_idx = -1; prev_driver_centroid = cv::Point(-1, -1); consecutive_valid_eyes_frames = 0; consecutive_stable_ear_frames = 0; consecutive_stable_mouth_frames = 0; ear_calibrated = false; mouth_calibrated = false; calib_left_ears.clear(); calib_right_ears.clear(); calib_mouth_dists.clear(); printf("WARN: Calibration time expired, criteria not met (H:%d, EV:%d, ES:%d, MS:%d). Retrying identification.\n", head_pose_calibrated, eyes_consistently_valid, ear_calibrated, mouth_calibrated); } } } else { calibration_timer_started = false; printf("WARN: Tracked driver landmarks not valid during calibration.\n"); } kssStatus = "Calibrating... " + std::to_string(static_cast<int>(elapsed_calib_seconds)) + "s" + calib_status_detail; if(driver_tracked_this_frame && current_tracked_driver_idx != -1) draw_rectangle(&src_image, face_results.faces[current_tracked_driver_idx].box.left, face_results.faces[current_tracked_driver_idx].box.top, face_results.faces[current_tracked_driver_idx].box.right - face_results.faces[current_tracked_driver_idx].box.left, face_results.faces[current_tracked_driver_idx].box.bottom - face_results.faces[current_tracked_driver_idx].box.top, COLOR_YELLOW, 2); } else { /* No driver tracked */ calibration_timer_started = false; consecutive_valid_eyes_frames = 0; consecutive_stable_ear_frames = 0; consecutive_stable_mouth_frames = 0; ear_calibrated = false; mouth_calibrated = false; calib_left_ears.clear(); calib_right_ears.clear(); calib_mouth_dists.clear(); if (driver_identified_ever) { kssStatus = "Calibration: Waiting for Driver Track..."; } else { kssStatus = "Calibration: Searching Driver..."; } for (int i = 0; i < face_results.count; ++i) { draw_rectangle(&src_image, face_results.faces[i].box.left, face_results.faces[i].box.top, face_results.faces[i].box.right - face_results.faces[i].box.left, face_results.faces[i].box.bottom - face_results.faces[i].box.top, COLOR_YELLOW, 1); } }
        //     // Draw Calibration Status Text
        //      draw_text(&src_image, kssStatus.c_str(), 10, 10, COLOR_YELLOW, status_text_size);

        // } 
        if (!calibration_done) {
            // --- Calibration Logic ---
             bool head_pose_calibrated = false; bool eyes_consistently_valid = false; double elapsed_calib_seconds = 0.0; std::string calib_status_detail = "";
             kssStatus = "Initializing"; // Default status during calibration attempts

             if (driver_tracked_this_frame && current_tracked_driver_idx != -1) { // Calibration requires a tracked driver
                 face_object_t *calib_face = &face_results.faces[current_tracked_driver_idx];
                 if (calib_face->face_landmarks_valid) {
                    // ... (Start timer logic, run calibration checks as before) ...
                     if (!calibration_timer_started) { /* Start timer, reset state */ calibration_start_time = std::chrono::steady_clock::now(); calibration_timer_started = true; printf("INFO: Calibration timer started (Driver Index %d).\n", current_tracked_driver_idx); consecutive_valid_eyes_frames = 0; consecutive_stable_ear_frames = 0; consecutive_stable_mouth_frames = 0; calib_left_ears.clear(); calib_right_ears.clear(); calib_mouth_dists.clear(); ear_calibrated = false; mouth_calibrated = false; }
                     std::vector<cv::Point> calib_faceLandmarks = convert_landmarks_to_cvpoint(calib_face->face_landmarks, NUM_FACE_LANDMARKS);
                     if (!calib_faceLandmarks.empty()) { // Check if conversion yielded points
                         headPoseTracker.run(calib_faceLandmarks); head_pose_calibrated = headPoseTracker.isCalibrated(); if (!head_pose_calibrated) calib_status_detail += " (Head)"; bool eyes_valid_this_frame = calib_face->eye_landmarks_left_valid && calib_face->eye_landmarks_right_valid; if (eyes_valid_this_frame) consecutive_valid_eyes_frames++; else consecutive_valid_eyes_frames = 0; eyes_consistently_valid = (consecutive_valid_eyes_frames >= REQUIRED_VALID_EYES_FRAMES); if (!eyes_consistently_valid) calib_status_detail += " (Eyes Valid)"; if (!ear_calibrated && eyes_valid_this_frame) { const std::vector<int> L={33,160,158,133,153,144}, R={362,385,387,263,380,373}; float lE=calculate_ear_simple(calib_faceLandmarks,L), rE=calculate_ear_simple(calib_faceLandmarks,R); calib_left_ears.push_back(lE); calib_right_ears.push_back(rE); if (calib_left_ears.size()>CALIB_WINDOW_SIZE) calib_left_ears.pop_front(); if (calib_right_ears.size()>CALIB_WINDOW_SIZE) calib_right_ears.pop_front(); if (calib_left_ears.size()>=CALIB_WINDOW_SIZE) { float lS=calculate_stddev(calib_left_ears), rS=calculate_stddev(calib_right_ears); if (lS<EAR_STDDEV_THRESHOLD && rS<EAR_STDDEV_THRESHOLD) consecutive_stable_ear_frames++; else consecutive_stable_ear_frames=0; } else consecutive_stable_ear_frames=0; ear_calibrated=(consecutive_stable_ear_frames>=REQUIRED_STABLE_FRAMES); } else if(!eyes_valid_this_frame){ consecutive_stable_ear_frames=0; ear_calibrated=false; calib_left_ears.clear(); calib_right_ears.clear(); } if(!ear_calibrated) calib_status_detail+=" (EAR Stable)"; if (!mouth_calibrated) { double cM=calculate_mouth_dist_simple(calib_faceLandmarks); calib_mouth_dists.push_back(cM); if(calib_mouth_dists.size()>CALIB_WINDOW_SIZE) calib_mouth_dists.pop_front(); if(calib_mouth_dists.size()>=CALIB_WINDOW_SIZE){ double mS=calculate_stddev(calib_mouth_dists); if(mS<MOUTH_DIST_STDDEV_THRESHOLD) consecutive_stable_mouth_frames++; else consecutive_stable_mouth_frames=0; } else consecutive_stable_mouth_frames=0; mouth_calibrated=(consecutive_stable_mouth_frames>=REQUIRED_STABLE_FRAMES); } if(!mouth_calibrated) calib_status_detail+=" (Mouth Stable)"; auto now = std::chrono::steady_clock::now(); elapsed_calib_seconds = std::chrono::duration<double>(now - calibration_start_time).count();
                    } else {
                        // Handle case where landmark conversion failed
                         head_pose_calibrated = false; // Cannot calibrate head pose
                         eyes_consistently_valid = false; // Cannot check eye validity
                         ear_calibrated = false; // Cannot check EAR
                         mouth_calibrated = false; // Cannot check mouth
                         calibration_timer_started = false; // Reset timer
                         printf("WARN: Landmark conversion failed during calibration.\n");
                         calib_status_detail += " (Lmk Err)";
                    }


                     // ... (Check for completion or timeout, define fixed ROI logic remains ) ...
                     if (elapsed_calib_seconds >= CALIBRATION_TIMEOUT_SECONDS) { /* ... timeout/completion logic ... */ if (head_pose_calibrated && eyes_consistently_valid && ear_calibrated && mouth_calibrated) { calibration_done = true; printf("INFO: Calibration Complete!\n"); /* Define Fixed ROI */ face_object_t *calib_driver_face = &face_results.faces[current_tracked_driver_idx]; box_rect_t calib_driver_box = calib_driver_face->box; int calib_box_w = calib_driver_box.right - calib_driver_box.left; int calib_box_h = calib_driver_box.bottom - calib_driver_box.top; if(calib_box_w > 0 && calib_box_h > 0) { int box_center_x = (calib_driver_box.left + calib_driver_box.right) / 2; int box_center_y = (calib_driver_box.top + calib_driver_box.bottom) / 2; const float width_expansion = 2.2f; const float height_expansion_up = 0.8f; const float height_expansion_down = 3.0f; int roi_width = static_cast<int>(calib_box_w * width_expansion); int roi_height = static_cast<int>(calib_box_h * (height_expansion_up + height_expansion_down)); int roi_size = std::max(roi_width, roi_height); roi_width = roi_size; roi_height = roi_size; if (roi_width < 640) {roi_width = 640; roi_height = 640;} int roi_x = box_center_x - roi_width / 2; int roi_y = box_center_y - static_cast<int>(calib_box_h * height_expansion_up); driver_object_roi.x = std::max(0, roi_x); driver_object_roi.y = std::max(0, roi_y); driver_object_roi.width = std::min(roi_width, src_image.width - driver_object_roi.x); driver_object_roi.height = std::min(roi_height, src_image.height - driver_object_roi.y); driver_roi_defined = (driver_object_roi.width > 0 && driver_object_roi.height > 0); printf("INFO: Fixed Driver Object ROI defined: [%d, %d, %d x %d]\n", driver_object_roi.x, driver_object_roi.y, driver_object_roi.width, driver_object_roi.height); } else { driver_roi_defined = false; printf("WARN: Could not define fixed ROI due to invalid calibration box.\n"); } } else { /* Timeout Failed */ calibration_timer_started = false; driver_identified_ever = false; driver_tracked_this_frame = false; current_tracked_driver_idx = -1; prev_driver_centroid = cv::Point(-1, -1); consecutive_valid_eyes_frames = 0; consecutive_stable_ear_frames = 0; consecutive_stable_mouth_frames = 0; ear_calibrated = false; mouth_calibrated = false; calib_left_ears.clear(); calib_right_ears.clear(); calib_mouth_dists.clear(); printf("WARN: Calibration time expired, criteria not met (H:%d, EV:%d, ES:%d, MS:%d). Retrying identification.\n", head_pose_calibrated, eyes_consistently_valid, ear_calibrated, mouth_calibrated); } }

                 } else { calibration_timer_started = false; printf("WARN: Tracked driver landmarks not valid during calibration.\n"); kssStatus = "Calibration: Landmark Invalid"; } // Update status

                 // Update overall calibration status text
                 if (calibration_timer_started && !calibration_done) {
                     kssStatus = "Calibrating... " + std::to_string(static_cast<int>(elapsed_calib_seconds)) + "s" + calib_status_detail;
                 }

                 // Draw yellow box around the face being calibrated
                 draw_rectangle(&src_image, calib_face->box.left, calib_face->box.top, calib_face->box.right - calib_face->box.left, calib_face->box.bottom - calib_face->box.top, COLOR_YELLOW, 2);

                 // +++++++++++++ ADDED: Draw Calibration Reference Points +++++++++++++
                 if (calib_face->face_landmarks_valid) {
                     const int calib_indices[] = {
                         my::HeadPoseTracker::LandmarkIndex::LEFT_EYE,
                         my::HeadPoseTracker::LandmarkIndex::RIGHT_EYE,
                         my::HeadPoseTracker::LandmarkIndex::FOREHEAD,
                         my::HeadPoseTracker::LandmarkIndex::MOUTH_CENTER,
                         my::HeadPoseTracker::LandmarkIndex::NOSE_TIP
                     };
                     for (int idx : calib_indices) {
                         if (idx < NUM_FACE_LANDMARKS) { // Basic bounds check
                             draw_circle(&src_image,
                                         calib_face->face_landmarks[idx].x,
                                         calib_face->face_landmarks[idx].y,
                                         3,         // Make calibration points slightly larger
                                         COLOR_RED, // Use Red for visibility
                                         -1);       // Filled circle
                         }
                     }
                 }
                 // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

             } else { // No driver tracked during calibration phase
                 // ... (Reset calibration state variables as before) ...
                 calibration_timer_started = false; consecutive_valid_eyes_frames = 0; consecutive_stable_ear_frames = 0; consecutive_stable_mouth_frames = 0; ear_calibrated = false; mouth_calibrated = false; calib_left_ears.clear(); calib_right_ears.clear(); calib_mouth_dists.clear();
                 if (driver_identified_ever) { kssStatus = "Calibration: Waiting for Driver Track..."; } else { kssStatus = "Calibration: Searching Driver..."; }
                 // Draw boxes for all detected faces during search
                 for (int i = 0; i < face_results.count; ++i) { draw_rectangle(&src_image, face_results.faces[i].box.left, face_results.faces[i].box.top, face_results.faces[i].box.right - face_results.faces[i].box.left, face_results.faces[i].box.bottom - face_results.faces[i].box.top, COLOR_YELLOW, 1); }
             }
             // Draw the overall calibration status text determined above
             draw_text(&src_image, kssStatus.c_str(), 10, 10, COLOR_YELLOW, status_text_size);

        }
        
        else { // --- Normal Processing Phase (Calibration is Done) ---

            // Default Status for this phase
            kssStatus = "Normal"; // Assume normal unless KSS says otherwise or driver lost

            if (!driver_roi_defined) {
                kssStatus = "ROI Error";
                // ... (Reset KSS values) ...
                printf("ERROR: Driver Object ROI was not defined. Skipping processing.\n"); extractedTotalKSS = 1; perclosKSS = 1; blinkKSS = 1; headposeKSS = 1; yawnKSS = 1; objdectdetectionKSS = 1; yawnMetrics = {}; headPoseResults = {}; detectedObjects.clear();
            } else {
                // --- Crop to the FIXED Driver Object ROI ---
                // ... (Crop logic) ...
                if (fixed_roi_crop_img.virt_addr) { free(fixed_roi_crop_img.virt_addr); fixed_roi_crop_img.virt_addr = nullptr; } memset(&fixed_roi_crop_img, 0, sizeof(image_buffer_t)); box_rect_t fixed_roi_as_box = { driver_object_roi.x, driver_object_roi.y, driver_object_roi.x + driver_object_roi.width, driver_object_roi.y + driver_object_roi.height}; ret = crop_image_simple(&src_image, &fixed_roi_crop_img, fixed_roi_as_box);

                if (ret == 0 && fixed_roi_crop_img.virt_addr) {
                    // --- Queue FIXED ROI CROP for YOLO ---
                    // ... (YOLO queuing logic) ...
                    auto yolo_input_image = std::make_shared<image_buffer_t>(); yolo_input_image->width = fixed_roi_crop_img.width; yolo_input_image->height = fixed_roi_crop_img.height; yolo_input_image->format = fixed_roi_crop_img.format; yolo_input_image->size = fixed_roi_crop_img.size; yolo_input_image->virt_addr = (unsigned char*)malloc(yolo_input_image->size); if (yolo_input_image->virt_addr) { memcpy(yolo_input_image->virt_addr, fixed_roi_crop_img.virt_addr, yolo_input_image->size); yolo_input_image->width_stride = 0; yolo_input_image->height_stride = 0; yolo_input_image->fd = -1; { std::unique_lock<std::mutex> lock(yolo_input_mutex); if (yolo_input_queue.size() < MAX_QUEUE_SIZE) yolo_input_queue.push({current_frame_id, yolo_input_image}); else { free(yolo_input_image->virt_addr); yolo_input_image->virt_addr = nullptr; } } } else { printf("ERROR: Failed alloc YOLO input copy (fixed roi crop).\n"); }
                } else { printf("ERROR: Failed to crop to FIXED ROI for YOLO frame %ld.\n", current_frame_id); }

                // --- Get YOLO Results ---
                 bool yolo_result_received = false; { std::unique_lock<std::mutex> lock(yolo_output_mutex); if (!yolo_output_queue.empty()) { YoloOutputData data = yolo_output_queue.front(); yolo_output_queue.pop(); yolo_results = data.results; yolo_result_received = true; } }

                // --- Run Behavior Analysis & KSS (Conditional on Driver Tracked & In ROI) ---
                detectedObjects.clear();
                std::vector<std::string> driver_detectedObjects_final;
                valid_object_roi = driver_roi_defined;

                if (driver_tracked_this_frame && current_tracked_driver_idx != -1) {
                    face_object_t *driver_face = &face_results.faces[current_tracked_driver_idx];
                    cv::Point driver_center_full_frame( (driver_face->box.left + driver_face->box.right) / 2, (driver_face->box.top + driver_face->box.bottom) / 2 );

                    if (driver_object_roi.contains(driver_center_full_frame)) { // Driver IN ROI
                        // Filter YOLO results
                        if (yolo_results.count > 0) { for (int j = 0; j < yolo_results.count; ++j) { object_detect_result* det = &yolo_results.results[j]; if (det->prop > 0.4) { driver_detectedObjects_final.push_back(coco_cls_to_name(det->cls_id)); } } }
                        detectedObjects = driver_detectedObjects_final;

                        // Run Behavior Modules
                        if (driver_face->face_landmarks_valid) {
                            std::vector<cv::Point> faceLandmarksCv = convert_landmarks_to_cvpoint(driver_face->face_landmarks, NUM_FACE_LANDMARKS);
                            if (!faceLandmarksCv.empty()) {
                                 headPoseResults = headPoseTracker.run(faceLandmarksCv); blinkDetector.run(faceLandmarksCv, src_image.width, src_image.height); yawnMetrics = yawnDetector.run(faceLandmarksCv, src_image.width, src_image.height); kssCalculator.setPerclos(blinkDetector.getPerclos()); int headPoseKSSValue = 1; if (headPoseResults.rows.size() >= 4) { for (const auto& row : headPoseResults.rows) { if (row.size() >= 2 && row[0] == "Head KSS") { try { headPoseKSSValue = std::stoi(row[1]); } catch (...) { headPoseKSSValue = 1; } break; } } } kssCalculator.setHeadPose(headPoseKSSValue); kssCalculator.setYawnMetrics(yawnMetrics.isYawning, yawnMetrics.yawnFrequency_5min, yawnMetrics.yawnDuration); kssCalculator.setBlinksLastMinute(blinkDetector.getBlinksInWindow());
                            } else { kssStatus = "Landmark Error"; /* Reset Inputs */ headPoseResults = {}; yawnMetrics = {}; kssCalculator.setPerclos(0); kssCalculator.setHeadPose(1); kssCalculator.setYawnMetrics(false, 0, 0); kssCalculator.setBlinksLastMinute(0); }
                        } else { kssStatus = "Driver Data Invalid"; /* Reset Inputs */ headPoseResults = {}; yawnMetrics = {}; kssCalculator.setPerclos(0); kssCalculator.setHeadPose(1); kssCalculator.setYawnMetrics(false, 0, 0); kssCalculator.setBlinksLastMinute(0); }

                        // Calculate KSS
                         double now_seconds = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count(); kssCalculator.setDetectedObjects(driver_detectedObjects_final, now_seconds); auto kssBreakdownResults = kssCalculator.calculateCompositeKSS(); if (kssBreakdownResults.size() > 5 && kssBreakdownResults[5].size() == 2) { try { perclosKSS = std::stoi(kssBreakdownResults[0][1]); blinkKSS = std::stoi(kssBreakdownResults[1][1]); headposeKSS = std::stoi(kssBreakdownResults[2][1]); yawnKSS = std::stoi(kssBreakdownResults[3][1]); objdectdetectionKSS = std::stoi(kssBreakdownResults[4][1]); extractedTotalKSS = std::stoi(kssBreakdownResults[5][1]); } catch (...) { extractedTotalKSS = 1; } } else { extractedTotalKSS = 1; }
                        std::string alertStatus = kssCalculator.getKSSAlertStatus(extractedTotalKSS);
                        // Update status only if not already an error, or if there's an alert
                        if (kssStatus == "Normal" || !alertStatus.empty()) { kssStatus = alertStatus.empty() ? "Normal" : alertStatus; }

                    } else { // Driver tracked BUT outside fixed ROI
                         kssStatus = "Driver Outside ROI";
                         driver_tracked_this_frame = false; current_tracked_driver_idx = -1; extractedTotalKSS = 1; yawnMetrics = {}; headPoseResults = {}; kssCalculator.setPerclos(0.0); kssCalculator.setHeadPose(1); kssCalculator.setYawnMetrics(false, 0, 0); kssCalculator.setBlinksLastMinute(0); double now_seconds = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count(); kssCalculator.setDetectedObjects({}, now_seconds); auto kssBreakdownResults = kssCalculator.calculateCompositeKSS(); if (kssBreakdownResults.size() > 5 && kssBreakdownResults[5].size() == 2) { extractedTotalKSS = std::stoi(kssBreakdownResults[5][1]); } else { extractedTotalKSS = 1;}
                    }
                } else { // Driver NOT tracked in this frame
                    kssStatus = "Driver Not Tracked";
                    extractedTotalKSS = 1; yawnMetrics = {}; headPoseResults = {}; kssCalculator.setPerclos(0.0); kssCalculator.setHeadPose(1); kssCalculator.setYawnMetrics(false, 0, 0); kssCalculator.setBlinksLastMinute(0); double now_seconds = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count(); kssCalculator.setDetectedObjects({}, now_seconds); auto kssBreakdownResults = kssCalculator.calculateCompositeKSS(); if (kssBreakdownResults.size() > 5 && kssBreakdownResults[5].size() == 2) { extractedTotalKSS = std::stoi(kssBreakdownResults[5][1]); } else { extractedTotalKSS = 1;}
                }
            } // End if(driver_roi_defined) / else


            // --- Drawing (Normal Processing Phase - Full Frame Coords) ---
            // Draw Green box ONLY if driver is tracked AND INSIDE ROI
            if (driver_tracked_this_frame && current_tracked_driver_idx != -1) {
                face_object_t *driver_face = &face_results.faces[current_tracked_driver_idx];
                cv::Point driver_center_full_frame( (driver_face->box.left + driver_face->box.right) / 2, (driver_face->box.top + driver_face->box.bottom) / 2 );
                if (driver_roi_defined && driver_object_roi.contains(driver_center_full_frame)) {
                     draw_rectangle(&src_image, driver_face->box.left, driver_face->box.top, driver_face->box.right - driver_face->box.left, driver_face->box.bottom - driver_face->box.top, COLOR_GREEN, 2);                
                 } 
                 else { // Driver tracked but outside ROI - draw yellow box
                      draw_rectangle(&src_image, driver_face->box.left, driver_face->box.top, driver_face->box.right - driver_face->box.left, driver_face->box.bottom - driver_face->box.top, COLOR_YELLOW, 1);
                 }
            }

            // +++++++++++++++ MODIFIED: Object Detection Event Count Display (Top-Right) +++++++++++++++
            const int obj_text_size_new = 16;
            const int obj_line_height_new = obj_text_size_new + 13;
            const int top_right_margin = 10;
            const unsigned int text_bg_color = 0x000000; // Black background for text area
                                                       // For semi-transparent: 0x80000000 (black with 50% alpha if RGBA)
                                                       // If your draw_rectangle supports alpha, otherwise use solid.

            std::vector<std::string> object_event_lines;
            // ... (Build object_event_lines vector as before) ...
            object_event_lines.push_back("Object Events ");
            text_stream.str(""); 
            
            text_stream << "Mobile (1s/1m): " << kssCalculator.getMobileEventsL1_1m(); 
            object_event_lines.push_back(text_stream.str()); 
            
            text_stream.str(""); 
            text_stream << "Mobile (2s/5m): " << kssCalculator.getMobileEventsL2_5m(); 
            object_event_lines.push_back(text_stream.str()); 
            
            text_stream.str(""); 
            text_stream << "Mobile (3s/10m): " << kssCalculator.getMobileEventsL3_10m(); 
            object_event_lines.push_back(text_stream.str()); 
            object_event_lines.push_back(""); text_stream.str(""); 
            text_stream << "E/D (1s/5m): " << kssCalculator.getEatDrinkEventsL1_5m(); 
            object_event_lines.push_back(text_stream.str()); 
            text_stream.str(""); 
            text_stream << "E/D (2s/10m): " << kssCalculator.getEatDrinkEventsL2_10m();
            object_event_lines.push_back(text_stream.str()); 
            text_stream.str(""); 
            text_stream << "E/D (3s/10m): " << kssCalculator.getEatDrinkEventsL3_10m(); 
            object_event_lines.push_back(text_stream.str()); 
            object_event_lines.push_back(""); text_stream.str(""); text_stream << "Smoking (1s/5m): " << kssCalculator.getSmokeEventsL1_5m(); object_event_lines.push_back(text_stream.str()); text_stream.str(""); text_stream << "Smoking (2s/10m): " << kssCalculator.getSmokeEventsL2_10m(); object_event_lines.push_back(text_stream.str()); text_stream.str(""); text_stream << "Smoking (3s/10m): " << kssCalculator.getSmokeEventsL3_10m(); object_event_lines.push_back(text_stream.str());


            int max_text_width = 0;
            // ... (Calculate max_text_width using cv::getTextSize or your util's equivalent) ...
            // (Using the simplified character count for brevity here, replace with your actual width calculation)
            int max_char_count = 0; for (const auto& line : object_event_lines) { if (line.length() > max_char_count) max_char_count = line.length(); } max_text_width = max_char_count * obj_text_size_new; // Approximate


            int obj_text_x_dynamic = src_image.width - max_text_width - top_right_margin;
            int current_obj_text_y = top_right_margin; // Use a temporary y for background calculation

            // *** ADDED: Calculate total height of the text block for background ***
            int total_text_block_height = 0;
            for (const auto& line : object_event_lines) {
                if (line.empty()) {
                    total_text_block_height += obj_line_height_new / 2; // Spacer
                } else {
                    total_text_block_height += obj_line_height_new;
                }
            }
            // Add a little padding to the background height
            total_text_block_height += 5;

            // *** ADDED: Draw the background rectangle ***
            // Ensure drawing coordinates are within frame boundaries
            int bg_rect_x = std::max(0, obj_text_x_dynamic - 5); // Small padding
            int bg_rect_y = std::max(0, current_obj_text_y - 5);
            int bg_rect_w = std::min(max_text_width + 10, src_image.width - bg_rect_x);
            int bg_rect_h = std::min(total_text_block_height, src_image.height - bg_rect_y);

            if (bg_rect_w > 0 && bg_rect_h > 0) { // Only draw if valid
                draw_rectangle(&src_image, bg_rect_x, bg_rect_y, bg_rect_w, bg_rect_h,
                               text_bg_color, -1); // -1 for filled rectangle
            }
            // *******************************************

            // Now draw the lines using the dynamically calculated starting X
            bool is_title = true;
            for (const auto& line : object_event_lines) {
                if (line.empty()) {
                    current_obj_text_y += obj_line_height_new / 2;
                    continue;
                }
                if (is_title) {
                    draw_text(&src_image, line.c_str(), obj_text_x_dynamic, current_obj_text_y, COLOR_ORANGE, obj_text_size_new + 2);
                    is_title = false;
                } else {
                    draw_text(&src_image, line.c_str(), obj_text_x_dynamic, current_obj_text_y, COLOR_WHITE, obj_text_size_new);
                }
                current_obj_text_y += obj_line_height_new;
            }
            // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



            // Draw Status Text (Common)
            if (kssStatus == "Normal") status_color_uint = COLOR_GREEN; else if (extractedTotalKSS <= 7) status_color_uint = COLOR_BLUE; else status_color_uint = COLOR_RED;

            text_y = 10; // Reset Y position for text

            // Draw Main Status
            if (!kssStatus.empty()) {
                 draw_text(&src_image, kssStatus.c_str(), 10, text_y, status_color_uint, status_text_size);
                 text_y += (int)(line_height * 1.4); // Extra space after main status
            }

            // --- Draw Behavior Metrics ---
            text_stream.str(""); text_stream << "PERCLOS: " << std::fixed << std::setprecision(2) << blinkDetector.getPerclos() << "%";
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;

            text_stream.str(""); text_stream << "Blinks (Last Min): " << blinkDetector.getBlinksInWindow();
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;

            if (headPoseResults.rows.size() >= 3) { std::string headpose_text = "Yaw:" + headPoseResults.rows[0][1] + " Pitch:" + headPoseResults.rows[1][1] + " Roll:" + headPoseResults.rows[2][1]; draw_text(&src_image, headpose_text.c_str(), 10, text_y, COLOR_WHITE, text_size); } else { draw_text(&src_image, "Head Pose: N/A", 10, text_y, COLOR_WHITE, text_size); } text_y += line_height;

            text_stream.str(""); text_stream << "Yawning: " << (yawnMetrics.isYawning ? "Yes" : "No");
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;

            text_stream.str(""); text_stream << "Yawn Freq(5m): " << static_cast<int>(yawnMetrics.yawnFrequency_5min);
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;

            std::string detected_objects_text = ""; if (!detectedObjects.empty()) { detected_objects_text = "Detected: "; for (size_t j = 0; j < detectedObjects.size(); ++j) { detected_objects_text += detectedObjects[j]; if (j < detectedObjects.size() - 1) detected_objects_text += ", "; } draw_text(&src_image, detected_objects_text.c_str(), 10, text_y, COLOR_ORANGE, text_size); text_y += line_height; }

            // --- Draw KSS Breakdown ---
            text_stream.str(""); text_stream << "KSS Breakdown: P" << perclosKSS << " H" << headposeKSS << " Y" << yawnKSS << " O" << objdectdetectionKSS; // Removed BlinkCount KSS for consistency
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_GREEN, text_size); text_y += line_height;

            text_stream.str(""); text_stream << "Composite KSS: " << extractedTotalKSS;
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, status_color_uint, text_size); text_y += line_height;

            // +++++++++++++++ ADDED: Timers and Counts +++++++++++++++
            text_y += 5; // Add a small gap

            // PERCLOS Timer
            double current_time_sec_perf = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
            double perclos_start = blinkDetector.getPerclosWindowStartTime();
            double perclos_elapsed = current_time_sec_perf - perclos_start;
            double perclos_remaining = std::max(0.0, 60.0 - perclos_elapsed);
            text_stream.str(""); text_stream << "PERCLOS Window: " << std::fixed << std::setprecision(0) << perclos_remaining << "s";
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_CYAN, text_size); text_y += line_height;

            // Head Pose Counts (within last 5 min)
            text_stream.str(""); text_stream << "Head Turns(5m): >=15: " << headPoseTracker.getHeadTurnCount15()
                                            << " | >=30: " << headPoseTracker.getHeadTurnCount30()
                                            << " | >=45: " << headPoseTracker.getHeadTurnCount45();
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_CYAN, text_size); text_y += line_height;

             // Yawn Counts (within last 5 min)
            text_stream.str(""); text_stream << "Yawns(5m): >=2s: " << yawnDetector.getYawnCount2s()
                                            << " | >=3s: " << yawnDetector.getYawnCount3s()
                                            << " | >=4s: " << yawnDetector.getYawnCount4s();
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_CYAN, text_size); text_y += line_height;

            // Object Detection Consecutive Frames & Event Count (Example: Mobile L3 10min)
            text_stream.str(""); text_stream << "Obj Cons Frames: M:" << kssCalculator.getConsecutiveMobileFrames()
                                            << " E:" << kssCalculator.getConsecutiveEatDrinkFrames()
                                            << " S:" << kssCalculator.getConsecutiveSmokeFrames();
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_CYAN, text_size); text_y += line_height;


            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            if (driver_tracked_this_frame && current_tracked_driver_idx != -1) { face_object_t *driver_face = &face_results.faces[current_tracked_driver_idx]; cv::Point driver_center_full_frame( (driver_face->box.left + driver_face->box.right) / 2, (driver_face->box.top + driver_face->box.bottom) / 2 ); if (driver_roi_defined && driver_object_roi.contains(driver_center_full_frame)) { draw_rectangle(&src_image, driver_face->box.left, driver_face->box.top, driver_face->box.right - driver_face->box.left, driver_face->box.bottom - driver_face->box.top, COLOR_GREEN, 2); if (driver_face->face_landmarks_valid) { /* Landmark drawing commented out */ const int LEFT_EYE_TEXT_ANCHOR_IDX = 33; const int RIGHT_EYE_TEXT_ANCHOR_IDX = 263; if (LEFT_EYE_TEXT_ANCHOR_IDX < NUM_FACE_LANDMARKS) { point_t la=driver_face->face_landmarks[LEFT_EYE_TEXT_ANCHOR_IDX]; std::string ls=blinkDetector.isLeftEyeClosed()?"CLOSED":"OPEN"; unsigned int lc=blinkDetector.isLeftEyeClosed()?COLOR_RED:COLOR_GREEN; draw_text(&src_image, ls.c_str(), la.x - 30, la.y - 25, lc, 14); text_stream.str(""); text_stream << "L:" << std::fixed << std::setprecision(2) << blinkDetector.getLeftEARValue(); draw_text(&src_image, text_stream.str().c_str(), la.x - 30, la.y - 10, COLOR_WHITE, 10); } if (RIGHT_EYE_TEXT_ANCHOR_IDX < NUM_FACE_LANDMARKS) { point_t ra=driver_face->face_landmarks[RIGHT_EYE_TEXT_ANCHOR_IDX]; std::string rs=blinkDetector.isRightEyeClosed()?"CLOSED":"OPEN"; unsigned int rc=blinkDetector.isRightEyeClosed()?COLOR_RED:COLOR_GREEN; draw_text(&src_image, rs.c_str(), ra.x - 30, ra.y - 25, rc, 14); text_stream.str(""); text_stream << "R:" << std::fixed << std::setprecision(2) << blinkDetector.getRightEARValue(); draw_text(&src_image, text_stream.str().c_str(), ra.x - 30, ra.y - 10, COLOR_WHITE, 10); } } } else { draw_rectangle(&src_image, driver_face->box.left, driver_face->box.top, driver_face->box.right - driver_face->box.left, driver_face->box.bottom - driver_face->box.top, COLOR_YELLOW, 1); } }



        } // End if (!calibration_done) / else

        // --- Draw ROIs (Optional Debugging - FIXED ROI) ---
        #ifdef DEBUG_DRAW_ROIS
        if (driver_roi_defined) { /* Draw Fixed ROI */ draw_rectangle(&src_image, driver_object_roi.x, driver_object_roi.y, driver_object_roi.width, driver_object_roi.height, COLOR_MAGENTA, 1); draw_text(&src_image, "Driver ROI", driver_object_roi.x + 5, driver_object_roi.y + 15, COLOR_MAGENTA, 10); }
        // Draw non-tracked faces (yellow box) - ALWAYS based on full frame results
         for (int i = 0; i < face_results.count; ++i) { if (i != current_tracked_driver_idx) { draw_rectangle(&src_image, face_results.faces[i].box.left, face_results.faces[i].box.top, face_results.faces[i].box.right - face_results.faces[i].box.left, face_results.faces[i].box.bottom - face_results.faces[i].box.top, COLOR_YELLOW, 1); } }
        #endif

        // --- Draw Resource Monitoring Info ---
        auto frame_processing_end_time = std::chrono::high_resolution_clock::now(); double frame_duration_ms = std::chrono::duration<double, std::milli>(frame_processing_end_time - frame_start_time).count(); calculateOverallFPS(frame_duration_ms, frame_times, overallFPS, max_time_records); getCPUUsage(currentCpuUsage, prevIdleTime, prevTotalTime); getTemperature(currentTemp); std::stringstream info_ss; info_ss << "FPS:" << std::fixed << std::setprecision(1) << overallFPS << "|CPU:" << currentCpuUsage << "%|T:" << currentTemp << "C"; draw_text(&src_image, info_ss.str().c_str(), 10, src_image.height - 20, COLOR_WHITE, 10);

        // --- Display Frame ---
        cv::Mat display_frame(src_image.height, src_image.width, CV_8UC3, map_info.data, src_image.width_stride);
        // cv::imshow("DMS Output", display_frame);

        // --- Push Frame to Saving Pipeline ---
        // if (pipeline_ && appsrc_ && saving_pipeline_caps_set) { GstClockTime duration = gst_util_uint64_scale_int(1, GST_SECOND, video_info.fps_n > 0 ? video_info.fps_d * video_info.fps_n : 30); pushFrameToPipeline(map_info.data, map_info.size, src_image.width, src_image.height, duration); }

        if (pipeline_ && appsrc_ && saving_pipeline_caps_set) { // Check flag
            // Calculate duration based on INPUT video info
            GstClockTime duration = gst_util_uint64_scale_int(GST_SECOND, video_info.fps_d, video_info.fps_n);

            pushFrameToPipeline(map_info.data, map_info.size, src_image.width, src_image.height, duration); // Pass duration
        }


        // --- Cleanup Frame Resources ---
        if (fixed_roi_crop_img.virt_addr) { free(fixed_roi_crop_img.virt_addr); fixed_roi_crop_img.virt_addr = nullptr; }
        gst_buffer_unmap(gst_buffer, &map_info);
        gst_sample_unref(sample);

        // --- Handle KeyPress ---
        if (cv::waitKey(1) == 27) break; // Exit on ESC

    } // End main loop

    // ... (Cleanup logic) ...
    if (fixed_roi_crop_img.virt_addr) { free(fixed_roi_crop_img.virt_addr); } printf("INFO: Cleaning up resources...\n"); cv::destroyAllWindows(); stop_yolo_worker.store(true); if (yolo_worker_thread.joinable()) yolo_worker_thread.join(); printf("INFO: YOLO thread joined.\n"); if (input_pipeline) { gst_element_set_state(input_pipeline, GST_STATE_NULL); gst_object_unref(appsink_); gst_object_unref(input_pipeline); printf("INFO: Input pipeline released.\n"); } if (pipeline_) { gst_element_send_event(pipeline_, gst_event_new_eos()); GstBus* bus = gst_element_get_bus(pipeline_); gst_bus_poll(bus, GST_MESSAGE_EOS, GST_CLOCK_TIME_NONE); gst_object_unref(bus); gst_element_set_state(pipeline_, GST_STATE_NULL); gst_object_unref(appsrc_); gst_object_unref(pipeline_); printf("INFO: Saving pipeline released.\n"); } gst_deinit(); printf("INFO: GStreamer deinitialized.\n"); release_face_analyzer(&app_ctx.face_ctx); release_yolo11(&app_ctx.yolo_ctx); deinit_post_process(); printf("INFO: RKNN models released.\n");

    printf("Exiting main (ret = %d)\n", ret);
    return ret;
}