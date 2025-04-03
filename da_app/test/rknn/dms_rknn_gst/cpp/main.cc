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

#include <fstream>     // For reading /proc/stat and /sys/class/thermal/...
#include <numeric>     // For std::accumulate
#include <deque>       // For storing frame/inference times for FPS calculation
#include <sys/sysinfo.h> // Might be needed on some systems, though /proc/stat is usually sufficient

#ifndef COLOR_MAGENTA
#define COLOR_MAGENTA (0xFF00FF)
#endif
#ifndef COLOR_CYAN
#define COLOR_CYAN (0x00FFFF)
#endif
#ifndef COLOR_YELLOW
#define COLOR_YELLOW (0xFFFF00)
#endif
#ifndef COLOR_BLUE
#define COLOR_BLUE (0x0000FF)
#endif
#ifndef COLOR_ORANGE
#define COLOR_ORANGE (0xFFA500)
#endif
#ifndef COLOR_WHITE
#define COLOR_WHITE (0xFFFFFF)
#endif
#ifndef COLOR_GREEN
#define COLOR_GREEN (0x00FF00)
#endif
#ifndef COLOR_RED
#define COLOR_RED (0xFF0000)
#endif

typedef struct app_context_t {
    face_analyzer_app_context_t face_ctx;
    yolo11_app_context_t        yolo_ctx;
} app_context_t;

// 전역 변수 선언
static GstElement* pipeline_ = nullptr;  // 저장용 파이프라인
static GstElement* appsrc_ = nullptr;    // 저장용 파이프라인의 appsrc
static GstElement* appsink_ = nullptr;   // 입력용 파이프라인의 appsink

std::vector<cv::Point> convert_landmarks_to_cvpoint(const point_t landmarks[], int count) {
    std::vector<cv::Point> cv_landmarks;
    cv_landmarks.reserve(count);
    for (int i = 0; i < count; ++i) {
        cv_landmarks.emplace_back(cv::Point(landmarks[i].x, landmarks[i].y));
    }
    return cv_landmarks;
}

cv::Mat image_buffer_to_mat(const image_buffer_t* image_buffer) {
    return cv::Mat(image_buffer->height, image_buffer->width, CV_8UC3, image_buffer->virt_addr);
}

// double parse_head_pose_value(const std::string& s) {
//     try {
//         size_t first_space = s.find_first_of(" \t");
//         if (first_space == std::string::npos) {
//             return std::stod(s);
//         }
//         size_t first_digit = s.find_first_not_of(" \t", first_space);
//         if (first_digit == std::string::npos) {
//             return 0.0;
//         }
//         std::string value_part = s.substr(first_digit);
//         size_t value_end = value_part.find_first_not_of("0123456789.-+eE");
//         return std::stod(value_part.substr(0, value_end));
//     } catch (const std::exception& e) {
    
//         printf("WARN: Could not parse head pose value from string: '%s'. Error: %s\n", s.c_str(), e.what());
//         return 0.0;
//     }
// }


double parse_head_pose_value(const std::string& s) {
    try {
        std::string numeric_part = s;
        size_t deg_pos = numeric_part.find(" deg");
        if (deg_pos != std::string::npos) {
            numeric_part = numeric_part.substr(0, deg_pos);
        }
        // Optional: Trim whitespace just in case
        size_t first_digit = numeric_part.find_first_not_of(" \t");
        if (first_digit == std::string::npos) return 0.0; // Empty or whitespace
        size_t last_digit = numeric_part.find_last_not_of(" \t");
        numeric_part = numeric_part.substr(first_digit, last_digit - first_digit + 1);

        if (numeric_part.empty()) return 0.0; // Handle case after trimming

        return std::stod(numeric_part);
    } catch (const std::invalid_argument& e) {
        // More specific catch
        printf("WARN: Could not parse head pose value from numeric part: '%s'. Error: %s\n", s.c_str(), e.what());
        return 0.0;
    } catch (const std::out_of_range& e) {
         // More specific catch
        printf("WARN: Head pose value out of range from string: '%s'. Error: %s\n", s.c_str(), e.what());
        return 0.0;
    } catch (const std::exception& e) {
        printf("WARN: Generic exception parsing head pose value from string: '%s'. Error: %s\n", s.c_str(), e.what());
        return 0.0;
    } catch (...) { // Keep generic catch just in case
         printf("WARN: Unknown exception parsing head pose value from string: '%s'.\n", s.c_str());
         return 0.0;
    }
}

struct YoloInputData {
    long frame_id;
    std::shared_ptr<image_buffer_t> image;
};
struct YoloOutputData {
    long frame_id;
    object_detect_result_list results;
};
std::queue<YoloInputData> yolo_input_queue;
std::queue<YoloOutputData> yolo_output_queue;
std::mutex yolo_input_mutex;
std::mutex yolo_output_mutex;
std::atomic<bool> stop_yolo_worker(false);
const int MAX_QUEUE_SIZE = 5;

void yolo_worker_thread_func(yolo11_app_context_t* yolo_ctx_ptr) {
    // printf("YOLO Worker Thread Started.\n");
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
            // printf("YOLO Worker: Starting inference for frame %ld...\n", input_data.frame_id);
            auto yolo_start = std::chrono::high_resolution_clock::now();
            int ret = inference_yolo11(yolo_ctx_ptr, input_data.image.get(), &output_data.results);
            auto yolo_end = std::chrono::high_resolution_clock::now();
            auto yolo_duration = std::chrono::duration_cast<std::chrono::milliseconds>(yolo_end - yolo_start);
            // printf("YOLO Worker: Finished inference for frame %ld (ret=%d, time=%ld ms).\n", input_data.frame_id, ret, yolo_duration.count());
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

    std::string dir = "/userdata/test_cpp/dms_gst";
    if (access(dir.c_str(), W_OK) != 0) {
        std::cerr << "Directory " << dir << " is not writable or does not exist" << std::endl;
        return;
    }

    DIR* directory = opendir(dir.c_str());
    if (!directory) {
        std::cerr << "Failed to open directory " << dir << std::endl;
        return;
    }

    int mkv_count = 0;
    struct dirent* entry;
    while ((entry = readdir(directory)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.find(".mkv") != std::string::npos) {
            mkv_count++;
        }
    }
    closedir(directory);
    std::string filepath = dir + "/dms_multi_" + std::to_string(mkv_count + 1) + ".mkv";

    std::string pipeline_str = 
        "appsrc name=source ! "
        "queue ! videoconvert ! video/x-raw,format=NV12 ! "
        "mpph265enc rc-mode=cbr bps=4000000 gop=30 qp-min=10 qp-max=51 ! "
        "h265parse ! matroskamux ! "
        "filesink location=" + filepath;

    std::cout << "Pipeline: " << pipeline_str << std::endl;

    GError* error = nullptr;
    pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
    if (!pipeline_ || error) {
        std::cerr << "Failed to create pipeline: " << (error ? error->message : "Unknown error") << std::endl;
        if (error) g_error_free(error);
        return;
    }

    appsrc_ = gst_bin_get_by_name(GST_BIN(pipeline_), "source");
    if (!appsrc_) {
        std::cerr << "Failed to get appsrc element" << std::endl;
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
        return;
    }

    GstCaps* caps = gst_caps_new_simple("video/x-raw",
                                        // "format", G_TYPE_STRING, "RGB",
                                        "format", G_TYPE_STRING, "BGR",
                                        "width", G_TYPE_INT, 1920,
                                        "height", G_TYPE_INT, 1080,
                                        "framerate", GST_TYPE_FRACTION, 30, 1,
                                        nullptr);
    g_object_set(G_OBJECT(appsrc_), "caps", caps, "format", GST_FORMAT_TIME, nullptr);
    gst_caps_unref(caps);

    if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set pipeline to playing state" << std::endl;
        gst_object_unref(appsrc_);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
        appsrc_ = nullptr;
    }
}

void pushFrameToPipeline(GstVideoFrame* frame) {
    if (!appsrc_) {
        std::cerr << "appsrc is null" << std::endl;
        return;
    }

    int width = frame->info.width;
    int height = frame->info.height;
    int stride = frame->info.stride[0];
    unsigned char* data = (unsigned char*)frame->data[0];

    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, frame->info.size, nullptr);
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        std::cerr << "Failed to map buffer for writing" << std::endl;
        gst_buffer_unref(buffer);
        return;
    }

    if (map.size != frame->info.size) {
        std::cerr << "Buffer size mismatch: map.size=" << map.size << ", frame->info.size=" << frame->info.size << std::endl;
    }

    memcpy(map.data, frame->data[0], frame->info.size);
    gst_buffer_unmap(buffer, &map);

    static GstClockTime timestamp = 0;
    GST_BUFFER_PTS(buffer) = timestamp;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30);  // 30fps
    timestamp += GST_BUFFER_DURATION(buffer);

    GstFlowReturn ret;
    g_signal_emit_by_name(appsrc_, "push-buffer", buffer, &ret);
    if (ret != GST_FLOW_OK) {
        std::cerr << "Failed to push buffer to appsrc, ret=" << ret << std::endl;
    }

    gst_buffer_unref(buffer);
}


// ========= Resource Monitoring Functions ==========

// Stores frame duration (ms) and calculates overall FPS based on rolling average
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
    } // else: prev_total was 0 (first call), keep cpu_usage_variable at its initial value (0.0)

    // Update previous values for the next call
    prev_idle = currentIdleTime;
    prev_total = currentTotalTime;
}

// Reads temperature from sysfs
void getTemperature(double& temp_variable) {
    // Common paths, might need adjustment for specific hardware
    const char* temp_paths[] = {
        "/sys/class/thermal/thermal_zone0/temp",
        "/sys/class/thermal/thermal_zone1/temp", // Sometimes zone 1 is CPU
        // Add other potential paths if needed
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
        // printf("WARN: Could not read temperature from common sysfs paths.\n");
        // temp_variable = -1.0; // Indicate error? Or keep last value?
    }
}

// ========= END Resource Monitoring Functions ==========
    
    /*-------------------------------------------
                      Main Function
    -------------------------------------------*/
int main(int argc, char **argv) {

    #if(0)
    if (argc != 6) {
        printf("Usage: %s <face_detect_model> <face_lmk_model> <iris_lmk_model> <yolo_model> <image_path>\n", argv[0]);
        return -1;
    }
    #endif
    // --- Variable Declarations ---
    int ret = 0;

    std::deque<double> frame_times;         // Stores total processing time for recent frames (ms)
    std::deque<double> inference_times;     // Stores only inference time (ms) - NOTE: Need to add timing for this if desired
    const int max_time_records = 60;        // Number of frames to average for FPS (e.g., ~2 seconds at 30fps)
    double overallFPS = 0.0;
    double inferenceFPS = 0.0; // Will remain 0 unless you time inference separately

    // CPU Usage
    double currentCpuUsage = 0.0;
    long prevIdleTime = 0, prevTotalTime = 0; // Static-like storage for CPU calculation

    // Temperature
    double currentTemp = 0.0;



    const char *detection_model_path = "../../model/faceD.rknn";
    const char *landmark_model_path  = "../../model/faceL.rknn";
    const char *iris_model_path      = "../../model/faceI.rknn";
    const char *yolo_model_path      = "../../model/od.rknn";

    // const char *video_source         = "v4l2src device=/dev/video1 ! "
    //                                    "queue ! videoconvert ! video/x-raw,format=BGR ! "
    //                                    "appsink name=sink sync=false";

    const char *video_source = "filesrc location=../../model/cut_drinking.mkv ! "
                               "decodebin ! "
                               "queue ! videoconvert ! video/x-raw,format=BGR ! "
                               "appsink name=sink sync=false";

    setupPipeline();

    app_context_t app_ctx;
    memset(&app_ctx, 0, sizeof(app_context_t));
    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t));

    face_analyzer_result_t face_results;
    memset(&face_results, 0, sizeof(face_results));
    object_detect_result_list yolo_results;
    memset(&yolo_results, 0, sizeof(yolo_results));

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

    bool show_ = true; // Added declaration


    ret = init_post_process();
        if (ret != 0) {
            printf("Error init YOLO postprocess.\n");
            deinit_post_process();
            return -1;
        }
    ret = init_face_analyzer(detection_model_path, landmark_model_path, iris_model_path, &app_ctx.face_ctx);
    if (ret != 0) {
        printf("init_face_analyzer fail! ret=%d\n", ret);
        return -1;
    }
    ret = init_yolo11(yolo_model_path, &app_ctx.yolo_ctx);
    if (ret != 0) {
        printf("init_yolo11 fail! ret=%d\n", ret);
        release_face_analyzer(&app_ctx.face_ctx);
        return -1;
    }
    yolo_worker_thread = std::thread(yolo_worker_thread_func, &app_ctx.yolo_ctx);  // YOLO 스레드 시작
    
    
    
    gst_init(nullptr, nullptr);
    GError* error = nullptr;
    GstElement* input_pipeline = gst_parse_launch(video_source, &error);
    if (!input_pipeline || error) {
        std::cerr << "Failed to create pipeline: " << (error ? error->message : "Unknown error") << std::endl;
        if (error) g_error_free(error);
        return -1;
    }

    appsink_ = gst_bin_get_by_name(GST_BIN(input_pipeline), "sink");
    if (!appsink_) {
        std::cerr << "Failed to get appsink element" << std::endl;
        gst_object_unref(input_pipeline);
        return -1;
    }

    if (gst_element_set_state(input_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set pipeline to playing state" << std::endl;
        gst_object_unref(appsink_);
        gst_object_unref(input_pipeline);
        return -1;
    }

    cv::namedWindow("output", cv::WINDOW_NORMAL);
    cv::resizeWindow("output", 1920, 1080);
    // cv::resizeWindow("output", 640, 640);
    GstSample* sample;
    GstVideoFrame* in_frame = new GstVideoFrame;
    GstVideoFrame* out_frame = new GstVideoFrame;
    bool frame_mapped = false;

    int frame_counter = 0;
    int skip_frames = 1;
    const double target_frame_time = 33.3;

    while (true) {
        sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink_));
        if (!sample) {
            std::cerr << "Failed to pull sample or end of stream" << std::endl;
            break;
        }

        GstBuffer* buffer = gst_sample_get_buffer(sample);
        GstCaps* caps = gst_sample_get_caps(sample);
        if (!buffer || !caps) {
            std::cerr << "Invalid sample" << std::endl;
            gst_sample_unref(sample);
            continue;
        }

        GstVideoInfo video_info;
        gst_video_info_init(&video_info);
        if (!gst_video_info_from_caps(&video_info, caps)) {
            std::cerr << "Failed to parse video info" << std::endl;
            gst_sample_unref(sample);
            continue;
        }

        if (!gst_video_frame_map(in_frame, &video_info, buffer, GST_MAP_READ)) {
            std::cerr << "Failed to map input video frame" << std::endl;
            gst_sample_unref(sample);
            continue;
        }

        GstBuffer* out_buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&video_info), nullptr);
        if (!gst_video_frame_map(out_frame, &video_info, out_buffer, GST_MAP_WRITE)) {
            std::cerr << "Failed to map output video frame" << std::endl;
            gst_video_frame_unmap(in_frame);
            gst_buffer_unref(out_buffer);
            gst_sample_unref(sample);
            continue;
        }
        frame_mapped = true;

        auto start_time = std::chrono::high_resolution_clock::now();

        if (!calibration_done) {
            // printf("Attempting calibration...\n");
            if (face_results.count > 0 && face_results.faces[0].face_landmarks_valid) {
                std::vector<cv::Point> faceLandmarksCv = convert_landmarks_to_cvpoint(face_results.faces[0].face_landmarks, NUM_FACE_LANDMARKS);
                headPoseResults = headPoseTracker.run(faceLandmarksCv);
                if (headPoseTracker.isCalibrated()) {
                    calibration_done = true;
                    printf("Calibration Complete!\n");
                } else {
                    printf("Calibration Failed (Head pose not suitable?).\n");
                    calibration_done = true;
                    kssStatus = "Low Risk (Cal. Failed)";
                }
            } else {
                printf("Calibration Failed (No face/landmarks).\n");
                calibration_done = true;
                kssStatus = "Low Risk (No Face)";
            }
        }

        if (frame_counter % (skip_frames + 1) == 0) {
            src_image.width = in_frame->info.width;
            src_image.height = in_frame->info.height;
            src_image.width_stride = in_frame->info.stride[0];
            src_image.height_stride = in_frame->info.height;
            src_image.format = IMAGE_FORMAT_RGB888;
            src_image.virt_addr = static_cast<unsigned char*>(in_frame->data[0]);
            src_image.size = in_frame->info.size;
            src_image.fd = -1;

            auto src_image_ptr = std::make_shared<image_buffer_t>(src_image);

            {
                std::unique_lock<std::mutex> lock(yolo_input_mutex);
                if (yolo_input_queue.size() < MAX_QUEUE_SIZE) {
                    yolo_input_queue.push({frame_counter, src_image_ptr});
                }
            }

            inference_face_analyzer(&app_ctx.face_ctx, src_image_ptr.get(), &face_results);
            


            bool yolo_result_received = false;
            while (!yolo_result_received && !stop_yolo_worker.load()) {
                std::unique_lock<std::mutex> lock(yolo_output_mutex);
                if (!yolo_output_queue.empty()) {
                    YoloOutputData data = yolo_output_queue.front();
                    yolo_output_queue.pop();
                    yolo_results = data.results;
                    yolo_result_received = true;
                } else {
                    lock.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }

            if (face_results.count > 0 && face_results.faces[0].face_landmarks_valid) {
                face_object_t *face = &face_results.faces[0];
                std::vector<cv::Point> faceLandmarksCv = convert_landmarks_to_cvpoint(face->face_landmarks, NUM_FACE_LANDMARKS);


                // ---> ADD THIS BLOCK <---
                bool is_calibrated_before_run = headPoseTracker.isCalibrated();
                headPoseResults = headPoseTracker.run(faceLandmarksCv);
                bool is_calibrated_after_run = headPoseTracker.isCalibrated(); // Check again after run
                printf("DEBUG Calibration Status: Before Run = %s, After Run = %s, Reference Set = %s\n",
                    is_calibrated_before_run ? "TRUE" : "FALSE",
                    is_calibrated_after_run ? "TRUE" : "FALSE",
                    headPoseResults.reference_set ? "TRUE" : "FALSE");
                // ---> END ADDED BLOCK <---

                blinkDetector.run(faceLandmarksCv, src_image.width, src_image.height);
                yawnMetrics = yawnDetector.run(faceLandmarksCv, src_image.width, src_image.height);
                headPoseResults = headPoseTracker.run(faceLandmarksCv);

                detectedObjects.clear();
                for (int j = 0; j < yolo_results.count; ++j) {
                    detectedObjects.push_back(coco_cls_to_name(yolo_results.results[j].cls_id));
                }

                kssCalculator.setPerclos(blinkDetector.getPerclos());
                if (calibration_done && headPoseResults.rows.size() >= 3) {

                    try {
                        kssCalculator.setHeadPose(parse_head_pose_value(headPoseResults.rows[0][1]),
                                                  parse_head_pose_value(headPoseResults.rows[1][1]),
                                                  parse_head_pose_value(headPoseResults.rows[2][1]));
                    } catch (...) {
                        kssCalculator.setHeadPose(0.0, 0.0, 0.0);
                    }
                } else {
                    kssCalculator.setHeadPose(0.0, 0.0, 0.0);
                }
                kssCalculator.setYawnMetrics(yawnMetrics.isYawning, yawnMetrics.yawnFrequency, yawnMetrics.yawnDuration);
                kssCalculator.setDetectedObjects(detectedObjects);
                compositeKSS = kssCalculator.calculateCompositeKSS();
                if (kssStatus == "Initializing" || kssStatus == "Low Risk (Not Calibrated)" ||
                    kssStatus == "Low Risk (Cal. Failed)" || kssStatus == "Low Risk (No Face)") {
                    kssStatus = kssCalculator.getKSSAlertStatus(compositeKSS);
                }
            }

            for (int i = 0; i < face_results.count; ++i) {
                face_object_t *face = &face_results.faces[i];
                int rx = face->box.left;
                int ry = face->box.top;
                int rw = face->box.right - face->box.left;
                int rh = face->box.bottom - face->box.top;

                draw_rectangle(&src_image, rx, ry, rw, rh, COLOR_GREEN, 2); // Thinner box
                char score_text[20]; snprintf(score_text, 20, "%.2f", face->score);
                draw_text(&src_image, score_text, rx, ry - 15 > 0 ? ry - 15 : ry, COLOR_RED, 16); // Smaller score text
                
                if (face->face_landmarks_valid) {
                    for (int j = 0; j < NUM_FACE_LANDMARKS; j++) {
                        draw_circle(&src_image, face->face_landmarks[j].x, face->face_landmarks[j].y, 1, COLOR_ORANGE, 1);
                    }
                }
                if (face->eye_landmarks_left_valid) {
                    for (int j = 0; j < NUM_EYE_CONTOUR_LANDMARKS; j++) {
                        draw_circle(&src_image, face->eye_landmarks_left[j].x, face->eye_landmarks_left[j].y, 1, COLOR_BLUE, 1);
                    }
                }
                if (face->iris_landmarks_left_valid) {
                    for (int j = 0; j < NUM_IRIS_LANDMARKS; j++) {
                        draw_circle(&src_image, face->iris_landmarks_left[j].x, face->iris_landmarks_left[j].y, 2, COLOR_YELLOW, 2);
                    }
                }
                if (face->eye_landmarks_right_valid) {
                    for (int j = 0; j < NUM_EYE_CONTOUR_LANDMARKS; j++) {
                        draw_circle(&src_image, face->eye_landmarks_right[j].x, face->eye_landmarks_right[j].y, 1, COLOR_BLUE, 1);
                    }
                }
                if (face->iris_landmarks_right_valid) {
                    for (int j = 0; j < NUM_IRIS_LANDMARKS; j++) {
                        draw_circle(&src_image, face->iris_landmarks_right[j].x, face->iris_landmarks_right[j].y, 2, COLOR_YELLOW, 2);
                    }
                }
            }

            for (int i = 0; i < yolo_results.count; i++) {
                object_detect_result *det_result = &(yolo_results.results[i]);
                int x1 = det_result->box.left;
                int y1 = det_result->box.top;
                int x2 = det_result->box.right;
                int y2 = det_result->box.bottom;
                draw_rectangle(&src_image, x1, y1, x2 - x1, y2 - y1, COLOR_MAGENTA, 2);
                char text[256];
                sprintf(text, "%s %.0f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
                draw_text(&src_image, text, x1, y1 - 15 > 0 ? y1 - 15 : y1, COLOR_ORANGE, 14);
            }

            // status_color_uint = (compositeKSS >= 4) ? COLOR_RED : COLOR_GREEN;
            // text_y = 10;
            // draw_text(&src_image, kssStatus.c_str(), 10, text_y, status_color_uint, status_text_size);
            // text_y += (int)(line_height * 1.4);
            // text_stream.str("");
            // text_stream << "PERCLOS: " << std::fixed << std::setprecision(2) << blinkDetector.getPerclos() << "%";
            // draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
            // text_y += line_height;
            // text_stream.str("");
            // text_stream << "Blink Count: " << blinkDetector.getBlinkCount();
            // draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
            // text_y += line_height;
            // text_stream.str("");
            // text_stream << "Last Blink Dur: " << std::fixed << std::setprecision(2) << blinkDetector.getLastBlinkDuration() << " s";
            // draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
            // text_y += line_height;
            // if (calibration_done && headPoseResults.rows.size() >= 3) {
            //     std::string headpose_text = "Yaw:" + headPoseResults.rows[0][1] + " Pitch:" + headPoseResults.rows[1][1] + " Roll:" + headPoseResults.rows[2][1];
            //     draw_text(&src_image, headpose_text.c_str(), 10, text_y, COLOR_WHITE, text_size);
            //     text_y += line_height;
            // } else {
            //     draw_text(&src_image, "Head Pose: N/A", 10, text_y, COLOR_WHITE, text_size);
            //     text_y += line_height;
            // }
            // text_stream.str("");
            // text_stream << "Yawning: " << (yawnMetrics.isYawning ? "Yes" : "No");
            // draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
            // text_y += line_height;
            // text_stream.str("");
            // text_stream << "Yawn Freq (last min): " << static_cast<int>(yawnMetrics.yawnFrequency);
            // draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
            // text_y += line_height;
            // text_stream.str("");
            // text_stream << "Last Yawn Dur: " << std::fixed << std::setprecision(2) << yawnMetrics.yawnDuration << " s";
            // draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
            // text_y += line_height;
            // text_stream.str("");
            // text_stream << "KSS Score: " << compositeKSS;
            // draw_text(&src_image, text_stream.str().c_str(), 10, text_y, status_color_uint, text_size);
            // text_y += line_height;

            status_color_uint = (compositeKSS >= 4) ? COLOR_RED : COLOR_GREEN;
            text_y = 10;
            draw_text(&src_image, kssStatus.c_str(), 10, text_y, status_color_uint, status_text_size);
            text_y += (int)(line_height * 1.4);
            text_stream.str("");
            text_stream << "PERCLOS: " << std::fixed << std::setprecision(2) << blinkDetector.getPerclos() << "%";
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
            text_y += line_height;
            text_stream.str("");
            text_stream << "Blink Count: " << blinkDetector.getBlinkCount();
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
            text_y += line_height;
            text_stream.str("");
            text_stream << "Last Blink Dur: " << std::fixed << std::setprecision(2) << blinkDetector.getLastBlinkDuration() << " s";
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
            text_y += line_height;
            if (calibration_done && headPoseResults.rows.size() >= 3) {
                std::string headpose_text = "Yaw:" + headPoseResults.rows[0][1] + " Pitch:" + headPoseResults.rows[1][1] + " Roll:" + headPoseResults.rows[2][1];
                draw_text(&src_image, headpose_text.c_str(), 10, text_y, COLOR_WHITE, text_size);
                text_y += line_height;
            } else {
                draw_text(&src_image, "Head Pose: N/A", 10, text_y, COLOR_WHITE, text_size);
                text_y += line_height;
            }
            text_stream.str("");
            text_stream << "Yawning: " << (yawnMetrics.isYawning ? "Yes" : "No");
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
            text_y += line_height;
            text_stream.str("");
            text_stream << "Total Yawn Count: " << static_cast<int>(yawnMetrics.yawnCount);  // New: Display total yawn count
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
            text_y += line_height;
            text_stream.str("");
            text_stream << "Yawn Freq (last min): " << static_cast<int>(yawnMetrics.yawnFrequency);  // Optional: Keep frequency
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
            text_y += line_height;
            text_stream.str("");
            text_stream << "Last Yawn Dur: " << std::fixed << std::setprecision(2) << yawnMetrics.yawnDuration << " s";
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
            text_y += line_height;
            text_stream.str("");
            text_stream << "KSS Score: " << compositeKSS;
            draw_text(&src_image, text_stream.str().c_str(), 10, text_y, status_color_uint, text_size);
            text_y += line_height;

            cv::Mat display(src_image_ptr->height, src_image_ptr->width, CV_8UC3, src_image_ptr->virt_addr, src_image_ptr->width_stride);

            // EXTRA CPU UPSAGE
            auto frame_processing_end_time = std::chrono::high_resolution_clock::now(); // If start_time is at the beginning of the block
            double frame_duration_ms = std::chrono::duration<double, std::milli>(frame_processing_end_time - start_time).count();

            // Calculate FPS (Pass variables by reference)
            calculateOverallFPS(frame_duration_ms, frame_times, overallFPS, max_time_records);
            // Note: To calculate inferenceFPS, you need to time the inference calls
            // separately and pass those timings to calculateOverallFPS with inference_times deque.
            // calculateOverallFPS(inference_duration_ms, inference_times, inferenceFPS, max_time_records);

            // Update CPU and Temp (Pass variables by reference)
            getCPUUsage(currentCpuUsage, prevIdleTime, prevTotalTime);
            getTemperature(currentTemp);

            // Display Info on Frame
            std::stringstream info_ss;
            info_ss << "FPS: " << std::fixed << std::setprecision(1) << overallFPS
                    // << " | Inf FPS: " << std::fixed << std::setprecision(1) << inferenceFPS // Uncomment if you calculate inferenceFPS
                    << " | CPU: " << std::fixed << std::setprecision(1) << currentCpuUsage << "%"
                    << " | Temp: " << std::fixed << std::setprecision(1) << currentTemp << " C";
            std::string resource_info_text = info_ss.str();

            // Choose position (e.g., bottom-left)
            int base_line;
            cv::Size text_size_info = cv::getTextSize(resource_info_text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &base_line);
            cv::Point text_origin(10, display.rows - 10); // 10 pixels from bottom-left

            cv::putText(display, resource_info_text, text_origin, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1); // White text


            cv::imshow("output", display);

            auto current_timestamp = std::chrono::steady_clock::now();

            memcpy(out_frame->data[0], src_image.virt_addr, src_image.size);
            pushFrameToPipeline(out_frame);
            if (cv::waitKey(1) == 27) break;


        } else {
            memcpy(out_frame->data[0], in_frame->data[0], in_frame->info.height * in_frame->info.stride[0]);
            pushFrameToPipeline(out_frame);
        
        }
        frame_counter++;

        auto end_time = std::chrono::high_resolution_clock::now();
        double frame_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        if (frame_time > target_frame_time) {
            skip_frames = std::min(skip_frames + 1, 5);
        } else if (frame_time < target_frame_time / 2) {
            skip_frames = std::max(skip_frames - 1, 0);
        }

        if (frame_mapped) {
            gst_video_frame_unmap(out_frame);
            gst_video_frame_unmap(in_frame);
            frame_mapped = false;
        }
        gst_buffer_unref(out_buffer);
        gst_sample_unref(sample);
    }

    // 정리
    printf("Cleaning up...\n");
    delete in_frame;
    delete out_frame;

    stop_yolo_worker.store(true);
    if (yolo_worker_thread.joinable()) {
        yolo_worker_thread.join();
        // printf("YOLO Worker Thread Joined.\n");
    }
    if (input_pipeline) {
        gst_element_set_state(input_pipeline, GST_STATE_NULL);
        gst_object_unref(appsink_);
        gst_object_unref(input_pipeline);
    }
    if (pipeline_) {
        gst_element_send_event(pipeline_, gst_event_new_eos());
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(appsrc_);
        gst_object_unref(pipeline_);
    }
    gst_deinit();

    release_face_analyzer(&app_ctx.face_ctx);
    release_yolo11(&app_ctx.yolo_ctx);
    deinit_post_process();

    printf("Exiting (ret = %d)\n", ret);
    return ret;
}