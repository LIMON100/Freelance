// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <vector>
// #include <string>
// #include <sstream>
// #include <iomanip>
// #include <chrono>
// #include <stdexcept>
// #include <cmath>
// #include <opencv2/core/types.hpp>
// #include <opencv2/opencv.hpp>
// #include <thread>
// #include <queue>
// #include <mutex>
// #include <atomic>
// #include <memory>
// #include "face_analyzer/face_analyzer.h"
// #include "yolo_detector/yolo11.h"
// #include "behavior_analysis/BlinkDetector.hpp"
// #include "behavior_analysis/YawnDetector.hpp"
// #include "behavior_analysis/HeadPoseTracker.hpp"
// #include "behavior_analysis/KSSCalculator.hpp"
// #include "image_utils.h"
// #include "file_utils.h"
// #include "image_drawing.h"
// #include <gst/gst.h>
// #include <gst/video/video.h>
// #include <gst/app/gstappsink.h>

// #include <iostream> 

// #include <fstream>     // For reading /proc/stat and /sys/class/thermal/...
// #include <numeric>     // For std::accumulate
// #include <deque>       // For storing frame/inference times for FPS calculation
// #include <sys/sysinfo.h> // Might be needed on some systems, though /proc/stat is usually sufficient

// #include <rga/im2d.h>
// #include <rga/rga.h>

// #ifndef COLOR_MAGENTA
// #define COLOR_MAGENTA (0xFF00FF) // BGR: (255, 0, 255)
// #endif
// #ifndef COLOR_CYAN
// #define COLOR_CYAN (0xFFFF00)    // BGR: (0, 255, 255)
// #endif
// #ifndef COLOR_YELLOW
// #define COLOR_YELLOW (0x00FFFF)  // BGR: (255, 255, 0)
// #endif
// #ifndef COLOR_BLUE
// #define COLOR_BLUE (0x0000FF)    // BGR: (0, 0, 255) -> Blue
// #endif
// #ifndef COLOR_ORANGE
// #define COLOR_ORANGE (0x00A5FF)  // BGR: (255, 165, 0)
// #endif
// #ifndef COLOR_WHITE
// #define COLOR_WHITE (0xFFFFFF)   // BGR: (255, 255, 255)
// #endif
// #ifndef COLOR_GREEN
// #define COLOR_GREEN (0x00FF00)   // BGR: (0, 255, 0)
// #endif
// #ifndef COLOR_RED
// #define COLOR_RED (0xFF0000)     // BGR: (255, 0, 0) -> Red
// #endif
// #ifndef COLOR_DARK_RED
// #define COLOR_DARK_RED (0x8B0000) // BGR: (139, 0, 0) -> Dark Red
// #endif

// typedef struct app_context_t {
//     face_analyzer_app_context_t face_ctx;
//     yolo11_app_context_t        yolo_ctx;
// } app_context_t;

// // 전역 변수 선언
// static GstElement* pipeline_ = nullptr;  // 저장용 파이프라인
// static GstElement* appsrc_ = nullptr;    // 저장용 파이프라인의 appsrc
// static GstElement* appsink_ = nullptr;   // 입력용 파이프라인의 appsink

// std::vector<cv::Point> convert_landmarks_to_cvpoint(const point_t landmarks[], int count) {
//     std::vector<cv::Point> cv_landmarks;
//     cv_landmarks.reserve(count);
//     for (int i = 0; i < count; ++i) {
//         cv_landmarks.emplace_back(cv::Point(landmarks[i].x, landmarks[i].y));
//     }
//     return cv_landmarks;
// }

// cv::Mat image_buffer_to_mat(const image_buffer_t* image_buffer) {
//     return cv::Mat(image_buffer->height, image_buffer->width, CV_8UC3, image_buffer->virt_addr);
// }


// double parse_head_pose_value(const std::string& s) {
//     try {
//         std::string numeric_part = s;
//         size_t deg_pos = numeric_part.find(" deg");
//         if (deg_pos != std::string::npos) {
//             numeric_part = numeric_part.substr(0, deg_pos);
//         }

//         size_t first_digit = numeric_part.find_first_not_of(" \t");
//         if (first_digit == std::string::npos) return 0.0; // Empty or whitespace
//         size_t last_digit = numeric_part.find_last_not_of(" \t");
//         numeric_part = numeric_part.substr(first_digit, last_digit - first_digit + 1);

//         if (numeric_part.empty()) return 0.0; // Handle case after trimming

//         return std::stod(numeric_part);
//     } catch (const std::invalid_argument& e) {
//         // More specific catch
//         printf("WARN: Could not parse head pose value from numeric part: '%s'. Error: %s\n", s.c_str(), e.what());
//         return 0.0;
//     } catch (const std::out_of_range& e) {
//          // More specific catch
//         printf("WARN: Head pose value out of range from string: '%s'. Error: %s\n", s.c_str(), e.what());
//         return 0.0;
//     } catch (const std::exception& e) {
//         printf("WARN: Generic exception parsing head pose value from string: '%s'. Error: %s\n", s.c_str(), e.what());
//         return 0.0;
//     } catch (...) { // Keep generic catch just in case
//          printf("WARN: Unknown exception parsing head pose value from string: '%s'.\n", s.c_str());
//          return 0.0;
//     }
// }

// struct YoloInputData {
//     long frame_id;
//     std::shared_ptr<image_buffer_t> image;
// };
// struct YoloOutputData {
//     long frame_id;
//     object_detect_result_list results;
// };
// std::queue<YoloInputData> yolo_input_queue;
// std::queue<YoloOutputData> yolo_output_queue;
// std::mutex yolo_input_mutex;
// std::mutex yolo_output_mutex;
// std::atomic<bool> stop_yolo_worker(false);
// const int MAX_QUEUE_SIZE = 5;

// void yolo_worker_thread_func(yolo11_app_context_t* yolo_ctx_ptr) {
//     // printf("YOLO Worker Thread Started.\n");
//     while (!stop_yolo_worker.load()) {
//         YoloInputData input_data;
//         bool got_data = false;
//         {
//             std::unique_lock<std::mutex> lock(yolo_input_mutex);
//             if (!yolo_input_queue.empty()) {
//                 input_data = yolo_input_queue.front();
//                 yolo_input_queue.pop();
//                 got_data = true;
//             }
//         }
//         if (got_data && input_data.image) {
//             YoloOutputData output_data;
//             output_data.frame_id = input_data.frame_id;
//             memset(&output_data.results, 0, sizeof(output_data.results));
//             // printf("YOLO Worker: Starting inference for frame %ld...\n", input_data.frame_id);
//             auto yolo_start = std::chrono::high_resolution_clock::now();
//             int ret = inference_yolo11(yolo_ctx_ptr, input_data.image.get(), &output_data.results);
//             auto yolo_end = std::chrono::high_resolution_clock::now();
//             auto yolo_duration = std::chrono::duration_cast<std::chrono::milliseconds>(yolo_end - yolo_start);
//             // printf("YOLO Worker: Finished inference for frame %ld (ret=%d, time=%ld ms).\n", input_data.frame_id, ret, yolo_duration.count());
//             if (ret != 0) {
//                 printf("WARN: YOLO Worker inference failed (frame %ld), ret=%d\n", input_data.frame_id, ret);
//             }
//             {
//                 std::unique_lock<std::mutex> lock(yolo_output_mutex);
//                 if (yolo_output_queue.size() >= MAX_QUEUE_SIZE) {
//                     yolo_output_queue.pop();
//                 }
//                 yolo_output_queue.push(output_data);
//             }
//         } else {
//             std::this_thread::sleep_for(std::chrono::milliseconds(5));
//         }
//     }
//     printf("YOLO Worker Thread Exiting.\n");
// }

// void setupPipeline() {
//     gst_init(nullptr, nullptr);

//     std::string dir = "/userdata/test_cpp/dms_gst";
//     if (access(dir.c_str(), W_OK) != 0) {
//         std::cerr << "Directory " << dir << " is not writable or does not exist" << std::endl;
//         return;
//     }

//     DIR* directory = opendir(dir.c_str());
//     if (!directory) {
//         std::cerr << "Failed to open directory " << dir << std::endl;
//         return;
//     }

//     int mkv_count = 0;
//     struct dirent* entry;
//     while ((entry = readdir(directory)) != nullptr) {
//         std::string filename = entry->d_name;
//         if (filename.find(".mkv") != std::string::npos) {
//             mkv_count++;
//         }
//     }
//     closedir(directory);
//     std::string filepath = dir + "/dms_multi_" + std::to_string(mkv_count + 1) + ".mkv";

//     std::string pipeline_str = 
//         "appsrc name=source ! "
//         "queue ! videoconvert ! video/x-raw,format=NV12 ! "
//         "mpph265enc rc-mode=cbr bps=4000000 gop=30 qp-min=10 qp-max=51 ! "
//         "h265parse ! matroskamux ! "
//         "filesink location=" + filepath;

//     // std::cout << "Pipeline: " << pipeline_str << std::endl;

//     GError* error = nullptr;
//     pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
//     if (!pipeline_ || error) {
//         std::cerr << "Failed to create pipeline: " << (error ? error->message : "Unknown error") << std::endl;
//         if (error) g_error_free(error);
//         return;
//     }

//     appsrc_ = gst_bin_get_by_name(GST_BIN(pipeline_), "source");
//     if (!appsrc_) {
//         std::cerr << "Failed to get appsrc element" << std::endl;
//         gst_object_unref(pipeline_);
//         pipeline_ = nullptr;
//         return;
//     }

//     GstCaps* caps = gst_caps_new_simple("video/x-raw",
//                                         // "format", G_TYPE_STRING, "RGB",
//                                         "format", G_TYPE_STRING, "BGR",
//                                         "width", G_TYPE_INT, 1920,
//                                         "height", G_TYPE_INT, 1080,
//                                         "framerate", GST_TYPE_FRACTION, 30, 1,
//                                         nullptr);
//     g_object_set(G_OBJECT(appsrc_), "caps", caps, "format", GST_FORMAT_TIME, nullptr);
//     gst_caps_unref(caps);

//     if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
//         std::cerr << "Failed to set pipeline to playing state" << std::endl;
//         gst_object_unref(appsrc_);
//         gst_object_unref(pipeline_);
//         pipeline_ = nullptr;
//         appsrc_ = nullptr;
//     }
// }

// void pushFrameToPipeline(GstVideoFrame* frame) {
//     if (!appsrc_) {
//         std::cerr << "appsrc is null" << std::endl;
//         return;
//     }

//     int width = frame->info.width;
//     int height = frame->info.height;
//     int stride = frame->info.stride[0];
//     unsigned char* data = (unsigned char*)frame->data[0];

//     GstBuffer* buffer = gst_buffer_new_allocate(nullptr, frame->info.size, nullptr);
//     GstMapInfo map;
//     if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
//         std::cerr << "Failed to map buffer for writing" << std::endl;
//         gst_buffer_unref(buffer);
//         return;
//     }

//     if (map.size != frame->info.size) {
//         std::cerr << "Buffer size mismatch: map.size=" << map.size << ", frame->info.size=" << frame->info.size << std::endl;
//     }

//     memcpy(map.data, frame->data[0], frame->info.size);
//     gst_buffer_unmap(buffer, &map);

//     static GstClockTime timestamp = 0;
//     GST_BUFFER_PTS(buffer) = timestamp;
//     GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30);  // 30fps
//     timestamp += GST_BUFFER_DURATION(buffer);

//     GstFlowReturn ret;
//     g_signal_emit_by_name(appsrc_, "push-buffer", buffer, &ret);
//     if (ret != GST_FLOW_OK) {
//         std::cerr << "Failed to push buffer to appsrc, ret=" << ret << std::endl;
//     }

//     gst_buffer_unref(buffer);
// }


// // ========= Resource Monitoring Functions ==========

// // Stores frame duration (ms) and calculates overall FPS based on rolling average
// void calculateOverallFPS(double frame_duration_ms, std::deque<double>& times_deque, double& fps_variable, int max_records) {
//     times_deque.push_back(frame_duration_ms);
//     if (times_deque.size() > max_records) {
//         times_deque.pop_front(); // Remove the oldest time
//     }

//     if (!times_deque.empty()) {
//         double sum = std::accumulate(times_deque.begin(), times_deque.end(), 0.0);
//         double avg_time_ms = sum / times_deque.size();
//         if (avg_time_ms > 0) { // Avoid division by zero
//             fps_variable = 1000.0 / avg_time_ms;
//         } else {
//             fps_variable = 0.0; // Or some indicator of very fast/zero time
//         }
//     } else {
//         fps_variable = 0.0;
//     }
// }

// // Calculates current CPU usage based on /proc/stat differences
// void getCPUUsage(double& cpu_usage_variable, long& prev_idle, long& prev_total) {
//     std::ifstream file("/proc/stat");
//     if (!file.is_open()) {
//         // printf("WARN: Could not open /proc/stat for CPU usage.\n");
//         // cpu_usage_variable = -1.0; // Indicate error? Or keep last value?
//         return;
//     }
//     std::string line;
//     std::getline(file, line);
//     file.close(); // Close file quickly

//     long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
//     user = nice = system = idle = iowait = irq = softirq = steal = guest = guest_nice = 0; // Initialize

//     std::istringstream iss(line);
//     std::string cpu_label;
//     iss >> cpu_label; // Read "cpu" label

//     // Read values, handle cases where not all fields are present
//     iss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;
//     // Ignore guest/guest_nice for basic calculation if needed

//     long currentIdleTime = idle + iowait;
//     long currentTotalTime = user + nice + system + idle + iowait + irq + softirq + steal;

//     long diffIdle = currentIdleTime - prev_idle;
//     long diffTotal = currentTotalTime - prev_total;

//     if (diffTotal > 0) { // Avoid division by zero and ensure time has passed
//         double busyTime = (double)(diffTotal - diffIdle);
//         cpu_usage_variable = 100.0 * (busyTime / diffTotal);
//     } else if (diffTotal == 0 && prev_total != 0) {

//         // cpu_usage_variable = 0.0;
//     } // else: prev_total was 0 (first call), keep cpu_usage_variable at its initial value (0.0)

//     // Update previous values for the next call
//     prev_idle = currentIdleTime;
//     prev_total = currentTotalTime;
// }

// // Reads temperature from sysfs
// void getTemperature(double& temp_variable) {
//     // Common paths, might need adjustment for specific hardware
//     const char* temp_paths[] = {
//         "/sys/class/thermal/thermal_zone0/temp",
//         "/sys/class/thermal/thermal_zone1/temp", // Sometimes zone 1 is CPU
//         // Add other potential paths if needed
//     };
//     bool temp_read = false;
//     for (const char* path : temp_paths) {
//         std::ifstream file(path);
//         double temperature_milliC = 0;
//         if (file >> temperature_milliC) {
//             temp_variable = temperature_milliC / 1000.0; // Convert millidegree C to degree C
//             temp_read = true;
//             file.close();
//             break; // Stop after reading the first successful one
//         }
//         file.close(); // Close even if read failed
//     }

//     if (!temp_read) {
//         // printf("WARN: Could not read temperature from common sysfs paths.\n");
//         // temp_variable = -1.0; // Indicate error? Or keep last value?
//     }
// }

// // ========= END Resource Monitoring Functions ==========
    
//     /*-------------------------------------------
//                       Main Function
//     -------------------------------------------*/
// int main(int argc, char **argv) {

//     #if(0)
//     if (argc != 6) {
//         printf("Usage: %s <face_detect_model> <face_lmk_model> <iris_lmk_model> <yolo_model> <image_path>\n", argv[0]);
//         return -1;
//     }
//     #endif
//     // --- Variable Declarations ---
//     int ret = 0;

//     std::deque<double> frame_times;         // Stores total processing time for recent frames (ms)
//     std::deque<double> inference_times;     // Stores only inference time (ms) - NOTE: Need to add timing for this if desired
//     const int max_time_records = 60;        // Number of frames to average for FPS (e.g., ~2 seconds at 30fps)
//     double overallFPS = 0.0;
//     double inferenceFPS = 0.0; // Will remain 0 unless you time inference separately

//     // CPU Usage
//     double currentCpuUsage = 0.0;
//     long prevIdleTime = 0, prevTotalTime = 0; // Static-like storage for CPU calculation

//     // Temperature
//     double currentTemp = 0.0;



//     const char *detection_model_path = "../../model/faceD.rknn";
//     const char *landmark_model_path  = "../../model/faceL.rknn";
//     const char *iris_model_path      = "../../model/faceI.rknn";
//     const char *yolo_model_path      = "../../model/od.rknn";

//     // const char *video_source         = "v4l2src device=/dev/video1 ! "
//     //                                    "queue ! videoconvert ! video/x-raw,format=BGR ! "
//     //                                    "appsink name=sink sync=false";

//     const char *video_source = "filesrc location=../../model/using_phone.mkv ! "
//                                "decodebin ! "
//                                "queue ! videoconvert ! video/x-raw,format=BGR ! "
//                                "appsink name=sink sync=false";

//     setupPipeline();

//     app_context_t app_ctx;
//     memset(&app_ctx, 0, sizeof(app_context_t));
//     image_buffer_t src_image;
//     memset(&src_image, 0, sizeof(image_buffer_t));

//     face_analyzer_result_t face_results;
//     memset(&face_results, 0, sizeof(face_results));
//     object_detect_result_list yolo_results;
//     memset(&yolo_results, 0, sizeof(yolo_results));

//     my::BlinkDetector blinkDetector;
//     YawnDetector yawnDetector;
//     my::HeadPoseTracker headPoseTracker;
//     KSSCalculator kssCalculator;

//     bool calibration_done = false;
//     int compositeKSS = 1;
//     std::string kssStatus = "Initializing";
//     YawnDetector::YawnMetrics yawnMetrics = {};
//     my::HeadPoseTracker::HeadPoseResults headPoseResults = {};
//     std::vector<std::string> detectedObjects;

//     std::stringstream text_stream;
//     int text_y = 0;
//     const int line_height = 22;
//     const int text_size = 12;
//     const int status_text_size = 16;
//     unsigned int status_color_uint = COLOR_GREEN;

//     std::thread yolo_worker_thread;

//     bool show_ = true; // Added declaration


//     ret = init_post_process();
//         if (ret != 0) {
//             printf("Error init YOLO postprocess.\n");
//             deinit_post_process();
//             return -1;
//         }
//     ret = init_face_analyzer(detection_model_path, landmark_model_path, iris_model_path, &app_ctx.face_ctx);
//     if (ret != 0) {
//         printf("init_face_analyzer fail! ret=%d\n", ret);
//         return -1;
//     }
//     ret = init_yolo11(yolo_model_path, &app_ctx.yolo_ctx);
//     if (ret != 0) {
//         printf("init_yolo11 fail! ret=%d\n", ret);
//         release_face_analyzer(&app_ctx.face_ctx);
//         return -1;
//     }
//     yolo_worker_thread = std::thread(yolo_worker_thread_func, &app_ctx.yolo_ctx);  // YOLO 스레드 시작
    
    
    
//     gst_init(nullptr, nullptr);
//     GError* error = nullptr;
//     GstElement* input_pipeline = gst_parse_launch(video_source, &error);
//     if (!input_pipeline || error) {
//         std::cerr << "Failed to create pipeline: " << (error ? error->message : "Unknown error") << std::endl;
//         if (error) g_error_free(error);
//         return -1;
//     }

//     appsink_ = gst_bin_get_by_name(GST_BIN(input_pipeline), "sink");
//     if (!appsink_) {
//         std::cerr << "Failed to get appsink element" << std::endl;
//         gst_object_unref(input_pipeline);
//         return -1;
//     }

//     if (gst_element_set_state(input_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
//         std::cerr << "Failed to set pipeline to playing state" << std::endl;
//         gst_object_unref(appsink_);
//         gst_object_unref(input_pipeline);
//         return -1;
//     }

//     cv::namedWindow("output", cv::WINDOW_NORMAL);
//     cv::resizeWindow("output", 1920, 1080);
//     GstSample* sample;
//     GstVideoFrame* in_frame = new GstVideoFrame;
//     GstVideoFrame* out_frame = new GstVideoFrame;
//     bool frame_mapped = false;

//     int frame_counter = 0;
//     int skip_frames = 1;
//     const double target_frame_time = 33.3;

//     while (true) {
//         sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink_));
//         if (!sample) {
//             std::cerr << "Failed to pull sample or end of stream" << std::endl;
//             break;
//         }

//         GstBuffer* buffer = gst_sample_get_buffer(sample);
//         GstCaps* caps = gst_sample_get_caps(sample);
//         if (!buffer || !caps) {
//             std::cerr << "Invalid sample" << std::endl;
//             gst_sample_unref(sample);
//             continue;
//         }

//         GstVideoInfo video_info;
//         gst_video_info_init(&video_info);
//         if (!gst_video_info_from_caps(&video_info, caps)) {
//             std::cerr << "Failed to parse video info" << std::endl;
//             gst_sample_unref(sample);
//             continue;
//         }

//         if (!gst_video_frame_map(in_frame, &video_info, buffer, GST_MAP_READ)) {
//             std::cerr << "Failed to map input video frame" << std::endl;
//             gst_sample_unref(sample);
//             continue;
//         }

//         GstBuffer* out_buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&video_info), nullptr);
//         if (!gst_video_frame_map(out_frame, &video_info, out_buffer, GST_MAP_WRITE)) {
//             std::cerr << "Failed to map output video frame" << std::endl;
//             gst_video_frame_unmap(in_frame);
//             gst_buffer_unref(out_buffer);
//             gst_sample_unref(sample);
//             continue;
//         }
//         frame_mapped = true;

//         auto start_time = std::chrono::high_resolution_clock::now();

//         if (!calibration_done) {
//             // printf("Attempting calibration...\n");
//             if (face_results.count > 0 && face_results.faces[0].face_landmarks_valid) {
//                 std::vector<cv::Point> faceLandmarksCv = convert_landmarks_to_cvpoint(face_results.faces[0].face_landmarks, NUM_FACE_LANDMARKS);
//                 headPoseResults = headPoseTracker.run(faceLandmarksCv);
//                 if (headPoseTracker.isCalibrated()) {
//                     calibration_done = true;
//                     printf("Calibration Complete!\n");
//                 } else {
//                     printf("Calibration Failed (Head pose not suitable?).\n");
//                     calibration_done = true;
//                     kssStatus = "Low Risk (Cal. Failed)";
//                 }
//             } else {
//                 printf("Calibration Failed (No face/landmarks).\n");
//                 calibration_done = true;
//                 kssStatus = "Low Risk (No Face)";
//             }
//         }

//         if (frame_counter % (skip_frames + 1) == 0) {
//             src_image.width = in_frame->info.width;
//             src_image.height = in_frame->info.height;
//             src_image.width_stride = in_frame->info.stride[0];
//             src_image.height_stride = in_frame->info.height;
//             src_image.format = IMAGE_FORMAT_RGB888;
//             src_image.virt_addr = static_cast<unsigned char*>(in_frame->data[0]);
//             src_image.size = in_frame->info.size;
//             src_image.fd = -1;

//             auto src_image_ptr = std::make_shared<image_buffer_t>(src_image);

//             {
//                 std::unique_lock<std::mutex> lock(yolo_input_mutex);
//                 if (yolo_input_queue.size() < MAX_QUEUE_SIZE) {
//                     yolo_input_queue.push({frame_counter, src_image_ptr});
//                 }
//             }

//             inference_face_analyzer(&app_ctx.face_ctx, src_image_ptr.get(), &face_results);
            


//             bool yolo_result_received = false;
//             while (!yolo_result_received && !stop_yolo_worker.load()) {
//                 std::unique_lock<std::mutex> lock(yolo_output_mutex);
//                 if (!yolo_output_queue.empty()) {
//                     YoloOutputData data = yolo_output_queue.front();
//                     yolo_output_queue.pop();
//                     yolo_results = data.results;
//                     yolo_result_received = true;
//                 } else {
//                     lock.unlock();
//                     std::this_thread::sleep_for(std::chrono::milliseconds(1));
//                 }
//             }

//             if (face_results.count > 0 && face_results.faces[0].face_landmarks_valid) {
//                 face_object_t *face = &face_results.faces[0];
//                 std::vector<cv::Point> faceLandmarksCv = convert_landmarks_to_cvpoint(face->face_landmarks, NUM_FACE_LANDMARKS);
            
//                 bool is_calibrated_before_run = headPoseTracker.isCalibrated();
//                 headPoseResults = headPoseTracker.run(faceLandmarksCv);
//                 bool is_calibrated_after_run = headPoseTracker.isCalibrated();
//                 printf("DEBUG Calibration Status: Before Run = %s, After Run = %s, Reference Set = %s\n",
//                     is_calibrated_before_run ? "TRUE" : "FALSE",
//                     is_calibrated_after_run ? "TRUE" : "FALSE",
//                     headPoseResults.reference_set ? "TRUE" : "FALSE");
            
//                 blinkDetector.run(faceLandmarksCv, src_image.width, src_image.height);
//                 yawnMetrics = yawnDetector.run(faceLandmarksCv, src_image.width, src_image.height);
//                 headPoseResults = headPoseTracker.run(faceLandmarksCv);
            
//                 detectedObjects.clear();
//                 for (int j = 0; j < yolo_results.count; ++j) {
//                     detectedObjects.push_back(coco_cls_to_name(yolo_results.results[j].cls_id));
//                 }
            
//                 kssCalculator.setPerclos(blinkDetector.getPerclos());
            
//                 // Updated head pose KSS setting
//                 int headPoseKSSValue = 1; // Default value if not found
//                 if (calibration_done && headPoseResults.rows.size() >= 4) {
//                     for (const auto& row : headPoseResults.rows) {
//                         if (row[0] == "Head KSS") {
//                             try {
//                                 headPoseKSSValue = std::stoi(row[1]);
//                             } catch (...) {
//                                 printf("WARN: Failed to parse Head KSS value: %s\n", row[1].c_str());
//                                 headPoseKSSValue = 1; // Fallback
//                             }
//                             break;
//                         }
//                     }
//                 }
//                 kssCalculator.setHeadPose(headPoseKSSValue);
            
//                 kssCalculator.setYawnMetrics(yawnMetrics.isYawning, yawnMetrics.yawnFrequency, yawnMetrics.yawnDuration);
//                 // kssCalculator.setDetectedObjects(detectedObjects);
//                 double now_seconds = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
//                 kssCalculator.setDetectedObjects(detectedObjects, now_seconds);
          

//                 int blinks_last_minute = blinkDetector.getBlinksInWindow(); 
//                 kssCalculator.setBlinksLastMinute(blinks_last_minute);   


//                 compositeKSS = kssCalculator.calculateCompositeKSS();
//                 kssStatus = kssCalculator.getKSSAlertStatus(compositeKSS);
//                 if (kssStatus == "Initializing" || kssStatus == "Low Risk (Not Calibrated)" ||
//                     kssStatus == "Low Risk (Cal. Failed)" || kssStatus == "Low Risk (No Face)") {
//                     kssStatus = kssCalculator.getKSSAlertStatus(compositeKSS);
//                 }
//             }

//             for (int i = 0; i < face_results.count; ++i) {
//                 face_object_t *face = &face_results.faces[i];
//                 int rx = face->box.left;
//                 int ry = face->box.top;
//                 int rw = face->box.right - face->box.left;
//                 int rh = face->box.bottom - face->box.top;

//                 char score_text[20]; snprintf(score_text, 20, "%.2f", face->score);

//                 const int head_pose_key_indices[] = {
//                     1,  // Nose Tip
//                     10, // Forehead Center (approx)
//                     13, // Mouth Center (approx)
//                     33, // Left Eye Outer Corner
//                     263 // Right Eye Outer Corner
//                 };
//                 unsigned int key_landmark_color = COLOR_CYAN; // Define Cyan color
//                 int key_landmark_radius = 4; // Make them bigger to see easily
//                 int key_landmark_thickness = 3;
//             }

//             for (int i = 0; i < yolo_results.count; i++) {
//                 object_detect_result *det_result = &(yolo_results.results[i]);
//                 int x1 = det_result->box.left;
//                 int y1 = det_result->box.top;
//                 int x2 = det_result->box.right;
//                 int y2 = det_result->box.bottom;
//                 draw_rectangle(&src_image, x1, y1, x2 - x1, y2 - y1, COLOR_MAGENTA, 2);
//                 char text[256];
//                 sprintf(text, "%s %.0f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
//                 draw_text(&src_image, text, x1, y1 - 15 > 0 ? y1 - 15 : y1, COLOR_ORANGE, 14);
//             }

//             // status_color_uint = (compositeKSS >= 4) ? COLOR_RED : COLOR_GREEN;

//             if (compositeKSS <= 3) {
//                 status_color_uint = COLOR_GREEN; // Green for KSS <= 3
//             } else if (compositeKSS >= 4 && compositeKSS <= 7) {
//                 status_color_uint = COLOR_BLUE;  // Blue for KSS 4-6
//             } else if (compositeKSS >= 8 && compositeKSS <= 9) {
//                 status_color_uint = COLOR_YELLOW;   // Red for KSS 8-9
//             } 
//             else if (compositeKSS >= 10) {
//                 status_color_uint = COLOR_YELLOW; // Dark red for KSS 10+
//             } 
//             else {
//                 // This block should no longer be reached with the updated ranges
//                 status_color_uint = COLOR_YELLOW; // Fallback
//             }

//             text_y = 10;
//             draw_text(&src_image, kssStatus.c_str(), 10, text_y, status_color_uint, status_text_size);
//             text_y += (int)(line_height * 1.4);
//             text_stream.str("");
//             text_stream << "PERCLOS: " << std::fixed << std::setprecision(2) << blinkDetector.getPerclos() << "%";
//             draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
//             text_y += line_height;
//             // text_stream.str("");
//             // text_stream << "Blink Count: " << blinkDetector.getBlinkCount();
//             // draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
//             // text_y += line_height;
//             // text_stream.str("");
//             // text_stream << "Last Blink Dur: " << std::fixed << std::setprecision(2) << blinkDetector.getLastBlinkDuration() << " s";
//             // draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
//             // text_y += line_height;
//             if (calibration_done && headPoseResults.rows.size() >= 3) {
//                 std::string headpose_text = "Yaw:" + headPoseResults.rows[0][1] + " Pitch:" + headPoseResults.rows[1][1] + " Roll:" + headPoseResults.rows[2][1]; //+ " KSS:" + headPoseResults.rows[3][1];
//                 draw_text(&src_image, headpose_text.c_str(), 10, text_y, COLOR_WHITE, text_size);
//                 text_y += line_height;
//             } else {
//                 draw_text(&src_image, "Head Pose: N/A", 10, text_y, COLOR_WHITE, text_size);
//                 text_y += line_height;
//             }

//             text_stream.str("");
//             text_stream << "Yawning: " << (yawnMetrics.isYawning ? "Yes" : "No");
//             draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
//             text_y += line_height;
//             // text_stream.str("");
//             // text_stream << "Yawn KSS: " << static_cast<int>(yawnMetrics.yawnKSS);  // New: Display total yawn count
//             // draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
//             // text_y += line_height;
//             // text_stream.str("");
//             // text_stream << "Yawn Freq (last min): " << static_cast<int>(yawnMetrics.yawnFrequency);  // Optional: Keep frequency
//             // draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
//             // text_y += line_height;
//             // text_stream.str("");
//             // text_stream << "Last Yawn Dur: " << std::fixed << std::setprecision(2) << yawnMetrics.yawnDuration << " s";
//             // draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
//             // text_y += line_height;
//             text_stream.str("");
//             text_stream << "KSS Score: " << compositeKSS;
//             draw_text(&src_image, text_stream.str().c_str(), 10, text_y, status_color_uint, text_size);
//             text_y += line_height;

//             cv::Mat display(src_image_ptr->height, src_image_ptr->width, CV_8UC3, src_image_ptr->virt_addr, src_image_ptr->width_stride);

//             // EXTRA CPU UPSAGE
//             auto frame_processing_end_time = std::chrono::high_resolution_clock::now(); // If start_time is at the beginning of the block
//             double frame_duration_ms = std::chrono::duration<double, std::milli>(frame_processing_end_time - start_time).count();

//             // Calculate FPS (Pass variables by reference)
//             calculateOverallFPS(frame_duration_ms, frame_times, overallFPS, max_time_records);
          
//             // Update CPU and Temp (Pass variables by reference)
//             getCPUUsage(currentCpuUsage, prevIdleTime, prevTotalTime);
//             getTemperature(currentTemp);

//             // Display Info on Frame
//             std::stringstream info_ss;
//             info_ss << "FPS: " << std::fixed << std::setprecision(1) << overallFPS
//                     // << " | Inf FPS: " << std::fixed << std::setprecision(1) << inferenceFPS // Uncomment if you calculate inferenceFPS
//                     << " | CPU: " << std::fixed << std::setprecision(1) << currentCpuUsage << "%"
//                     << " | Temp: " << std::fixed << std::setprecision(1) << currentTemp << " C";
//             std::string resource_info_text = info_ss.str();

//             // Choose position (e.g., bottom-left)
//             int base_line;
//             cv::Size text_size_info = cv::getTextSize(resource_info_text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &base_line);
//             cv::Point text_origin(10, display.rows - 10); // 10 pixels from bottom-left

//             cv::putText(display, resource_info_text, text_origin, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1); // White text


//             cv::imshow("output", display);

//             auto current_timestamp = std::chrono::steady_clock::now();

//             memcpy(out_frame->data[0], src_image.virt_addr, src_image.size);
//             pushFrameToPipeline(out_frame);
//             if (cv::waitKey(1) == 27) break;


//         } else {
//             memcpy(out_frame->data[0], in_frame->data[0], in_frame->info.height * in_frame->info.stride[0]);
//             pushFrameToPipeline(out_frame);
        
//         }
//         frame_counter++;

//         auto end_time = std::chrono::high_resolution_clock::now();
//         double frame_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
//         if (frame_time > target_frame_time) {
//             skip_frames = std::min(skip_frames + 1, 5);
//         } else if (frame_time < target_frame_time / 2) {
//             skip_frames = std::max(skip_frames - 1, 0);
//         }

//         if (frame_mapped) {
//             gst_video_frame_unmap(out_frame);
//             gst_video_frame_unmap(in_frame);
//             frame_mapped = false;
//         }
//         gst_buffer_unref(out_buffer);
//         gst_sample_unref(sample);
//     }

//     // 정리
//     printf("Cleaning up...\n");
//     delete in_frame;
//     delete out_frame;

//     stop_yolo_worker.store(true);
//     if (yolo_worker_thread.joinable()) {
//         yolo_worker_thread.join();
//     }
//     if (input_pipeline) {
//         gst_element_set_state(input_pipeline, GST_STATE_NULL);
//         gst_object_unref(appsink_);
//         gst_object_unref(input_pipeline);
//     }
//     if (pipeline_) {
//         gst_element_send_event(pipeline_, gst_event_new_eos());
//         gst_element_set_state(pipeline_, GST_STATE_NULL);
//         gst_object_unref(appsrc_);
//         gst_object_unref(pipeline_);
//     }
//     gst_deinit();

//     release_face_analyzer(&app_ctx.face_ctx);
//     release_yolo11(&app_ctx.yolo_ctx);
//     deinit_post_process();

//     printf("Exiting (ret = %d)\n", ret);
//     return ret;
// }



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
#include <algorithm> // <-- Include for std::clamp, std::min, std::max

// Include SDK headers
#include "face_analyzer/face_analyzer.h"
#include "yolo_detector/yolo11.h"
#include "behavior_analysis/BlinkDetector.hpp"
#include "behavior_analysis/YawnDetector.hpp"
#include "behavior_analysis/HeadPoseTracker.hpp"
#include "behavior_analysis/KSSCalculator.hpp"
#include "image_utils.h"
#include "file_utils.h"
#include "image_drawing.h"

// Include GStreamer headers
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

#include <iostream>

// Resource monitoring includes
#include <fstream>
#include <numeric>
#include <deque>
#include <sys/sysinfo.h> // Might be needed

// --- Color Definitions ---
// ... (Color definitions remain the same) ...
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


// --- Application Context ---
typedef struct app_context_t {
    face_analyzer_app_context_t face_ctx;
    yolo11_app_context_t        yolo_ctx;
} app_context_t;

// --- GStreamer Globals ---
static GstElement* pipeline_ = nullptr;  // Saving pipeline
static GstElement* appsrc_ = nullptr;    // Saving pipeline's source
static GstElement* appsink_ = nullptr;   // Input pipeline's sink

// --- Helper Functions ---
// ... (convert_landmarks_to_cvpoint, image_buffer_to_mat, parse_head_pose_value remain the same) ...
std::vector<cv::Point> convert_landmarks_to_cvpoint(const point_t landmarks[], int count) { /* ... */ std::vector<cv::Point> cv_landmarks; cv_landmarks.reserve(count); for (int i = 0; i < count; ++i) { cv_landmarks.emplace_back(cv::Point(landmarks[i].x, landmarks[i].y)); } return cv_landmarks; }
cv::Mat image_buffer_to_mat(const image_buffer_t* image_buffer) { /* ... */ if (!image_buffer || !image_buffer->virt_addr) { return cv::Mat(); } return cv::Mat(image_buffer->height, image_buffer->width, CV_8UC3, image_buffer->virt_addr, image_buffer->width_stride); }
double parse_head_pose_value(const std::string& s) { /* ... implementation ... */ try { std::string numeric_part = s; size_t deg_pos = numeric_part.find(" deg"); if (deg_pos != std::string::npos) { numeric_part = numeric_part.substr(0, deg_pos); } size_t first_digit = numeric_part.find_first_not_of(" \t"); if (first_digit == std::string::npos) return 0.0; size_t last_digit = numeric_part.find_last_not_of(" \t"); numeric_part = numeric_part.substr(first_digit, last_digit - first_digit + 1); if (numeric_part.empty()) return 0.0; return std::stod(numeric_part); } catch (const std::invalid_argument& e) { printf("WARN: Could not parse head pose value from numeric part: '%s'. Error: %s\n", s.c_str(), e.what()); return 0.0; } catch (const std::out_of_range& e) { printf("WARN: Head pose value out of range from string: '%s'. Error: %s\n", s.c_str(), e.what()); return 0.0; } catch (const std::exception& e) { printf("WARN: Generic exception parsing head pose value from string: '%s'. Error: %s\n", s.c_str(), e.what()); return 0.0; } catch (...) { printf("WARN: Unknown exception parsing head pose value from string: '%s'.\n", s.c_str()); return 0.0; } }


// --- UPDATED Queue Structs ---
// ... (YoloInputData, YoloOutputData definitions remain the same) ...
struct YoloInputData {
    long frame_id;
    int crop_offset_x;
    int crop_offset_y;
    float scale_to_original_x; // Scale factor from 640 back to original cropped width
    float scale_to_original_y; // Scale factor from 640 back to original cropped height
};
struct YoloOutputData {
    long frame_id;
    object_detect_result_list results; // Results relative to 640x640
    int crop_offset_x;
    int crop_offset_y;
    float scale_to_original_x;
    float scale_to_original_y;
};
std::queue<YoloInputData> yolo_input_queue;
std::queue<YoloOutputData> yolo_output_queue;
std::mutex yolo_input_mutex;
std::mutex yolo_output_mutex;
std::atomic<bool> stop_yolo_worker(false);
const int MAX_QUEUE_SIZE = 5;
const int YOLO_TARGET_SIZE = 640;

// --- MODIFIED YOLO Worker Thread Function ---
// ... (yolo_worker_thread_func remains the same) ...
void yolo_worker_thread_func(yolo11_app_context_t* yolo_ctx_ptr) { /* ... */ printf("YOLO Worker Thread Started.\n"); while (!stop_yolo_worker.load()) { YoloInputData input_info; bool got_data = false; { std::unique_lock<std::mutex> lock(yolo_input_mutex); if (!yolo_input_queue.empty()) { input_info = yolo_input_queue.front(); yolo_input_queue.pop(); got_data = true; } } if (got_data) { YoloOutputData output_data; output_data.frame_id = input_info.frame_id; output_data.crop_offset_x = input_info.crop_offset_x; output_data.crop_offset_y = input_info.crop_offset_y; output_data.scale_to_original_x = input_info.scale_to_original_x; output_data.scale_to_original_y = input_info.scale_to_original_y; memset(&output_data.results, 0, sizeof(output_data.results)); int ret = inference_yolo11(yolo_ctx_ptr, &output_data.results); if (ret != 0) { printf("WARN: YOLO Worker inference failed (frame %ld), ret=%d\n", input_info.frame_id, ret); } { std::unique_lock<std::mutex> lock(yolo_output_mutex); if (yolo_output_queue.size() >= MAX_QUEUE_SIZE) { yolo_output_queue.pop(); } yolo_output_queue.push(output_data); } } else { std::this_thread::sleep_for(std::chrono::milliseconds(5)); } } printf("YOLO Worker Thread Exiting.\n"); }


// --- GStreamer Pipeline Setup/Push (copied from previous) ---
// ... (setupPipeline, pushFrameToPipeline remain the same) ...
void setupPipeline() { /* ... */ gst_init(nullptr, nullptr); std::string dir = "/userdata/test_cpp/dms_gst"; if (access(dir.c_str(), W_OK) != 0) { std::cerr << "Directory " << dir << " is not writable or does not exist" << std::endl; return; } DIR* directory = opendir(dir.c_str()); if (!directory) { std::cerr << "Failed to open directory " << dir << std::endl; return; } int mkv_count = 0; struct dirent* entry; while ((entry = readdir(directory)) != nullptr) { std::string filename = entry->d_name; if (filename.find(".mkv") != std::string::npos) { mkv_count++; } } closedir(directory); std::string filepath = dir + "/dms_multi_" + std::to_string(mkv_count + 1) + ".mkv"; std::string pipeline_str = "appsrc name=source ! queue ! videoconvert ! video/x-raw,format=NV12 ! mpph265enc rc-mode=cbr bps=4000000 gop=30 qp-min=10 qp-max=51 ! h265parse ! matroskamux ! filesink location=" + filepath; GError* error = nullptr; pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error); if (!pipeline_ || error) { std::cerr << "Failed to create pipeline: " << (error ? error->message : "Unknown error") << std::endl; if (error) g_error_free(error); return; } appsrc_ = gst_bin_get_by_name(GST_BIN(pipeline_), "source"); if (!appsrc_) { std::cerr << "Failed to get appsrc element" << std::endl; gst_object_unref(pipeline_); pipeline_ = nullptr; return; } GstCaps* caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", "width", G_TYPE_INT, 1920, "height", G_TYPE_INT, 1080, "framerate", GST_TYPE_FRACTION, 30, 1, nullptr); g_object_set(G_OBJECT(appsrc_), "caps", caps, "format", GST_FORMAT_TIME, nullptr); gst_caps_unref(caps); if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) { std::cerr << "Failed to set pipeline to playing state" << std::endl; gst_object_unref(appsrc_); gst_object_unref(pipeline_); pipeline_ = nullptr; appsrc_ = nullptr; } }
void pushFrameToPipeline(GstVideoFrame* frame) { /* ... */ if (!appsrc_ || !frame || !frame->data[0]) { std::cerr << "appsrc is null or frame invalid in pushFrameToPipeline" << std::endl; return; } GstBuffer* buffer = gst_buffer_new_allocate(nullptr, frame->info.size, nullptr); GstMapInfo map; if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) { std::cerr << "Failed to map buffer for writing" << std::endl; gst_buffer_unref(buffer); return; } memcpy(map.data, frame->data[0], frame->info.size); gst_buffer_unmap(buffer, &map); static GstClockTime timestamp = 0; GST_BUFFER_PTS(buffer) = timestamp; GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30); timestamp += GST_BUFFER_DURATION(buffer); GstFlowReturn ret; g_signal_emit_by_name(appsrc_, "push-buffer", buffer, &ret); if (ret != GST_FLOW_OK) { std::cerr << "Failed to push buffer to appsrc, ret=" << ret << std::endl; } gst_buffer_unref(buffer); }


// ========= Resource Monitoring Functions (copied from previous) ==========
// ... (calculateOverallFPS, getCPUUsage, getTemperature remain the same) ...
void calculateOverallFPS(double frame_duration_ms, std::deque<double>& times_deque, double& fps_variable, int max_records) { /* ... */ times_deque.push_back(frame_duration_ms); if (times_deque.size() > max_records) { times_deque.pop_front(); } if (!times_deque.empty()) { double sum = std::accumulate(times_deque.begin(), times_deque.end(), 0.0); double avg_time_ms = sum / times_deque.size(); if (avg_time_ms > 0) { fps_variable = 1000.0 / avg_time_ms; } else { fps_variable = 0.0; } } else { fps_variable = 0.0; } }
void getCPUUsage(double& cpu_usage_variable, long& prev_idle, long& prev_total) { /* ... */ std::ifstream file("/proc/stat"); if (!file.is_open()) { return; } std::string line; std::getline(file, line); file.close(); long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice; user = nice = system = idle = iowait = irq = softirq = steal = guest = guest_nice = 0; std::istringstream iss(line); std::string cpu_label; iss >> cpu_label; iss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice; long currentIdleTime = idle + iowait; long currentTotalTime = user + nice + system + idle + iowait + irq + softirq + steal; long diffIdle = currentIdleTime - prev_idle; long diffTotal = currentTotalTime - prev_total; if (diffTotal > 0) { double busyTime = (double)(diffTotal - diffIdle); cpu_usage_variable = 100.0 * (busyTime / diffTotal); } prev_idle = currentIdleTime; prev_total = currentTotalTime; }
void getTemperature(double& temp_variable) { /* ... */ const char* temp_paths[] = {"/sys/class/thermal/thermal_zone0/temp", "/sys/class/thermal/thermal_zone1/temp"}; bool temp_read = false; for (const char* path : temp_paths) { std::ifstream file(path); double temperature_milliC = 0; if (file >> temperature_milliC) { temp_variable = temperature_milliC / 1000.0; temp_read = true; file.close(); break; } file.close(); } if (!temp_read) { /* WARN */ } }


/*-------------------------------------------
                Main Function
-------------------------------------------*/
int main(int argc, char **argv) {
    // --- Variable Declarations ---
    int ret = 0;
    std::deque<double> frame_times; const int max_time_records = 60; double overallFPS = 0.0;
    double currentCpuUsage = 0.0; long prevIdleTime = 0, prevTotalTime = 0;
    double currentTemp = 0.0;

    // Model Paths (Use relative paths or configure properly)
    const char *detection_model_path = "../../model/faceD.rknn";
    const char *landmark_model_path  = "../../model/faceL.rknn";
    const char *iris_model_path      = "../../model/faceI.rknn";
    const char *yolo_model_path      = "../../model/od.rknn";

    // GStreamer Source Pipeline
    // const char *video_source = "filesrc location=../../model/using_phone.mkv ! decodebin ! queue ! videoconvert ! video/x-raw,format=BGR ! appsink name=sink sync=false";
    // const char *video_source         = "v4l2src device=/dev/video1 ! queue ! videoconvert ! video/x-raw,format=BGR ! appsink name=sink sync=false";

    // App Contexts and Results
    app_context_t app_ctx; memset(&app_ctx, 0, sizeof(app_context_t));
    image_buffer_t src_image; memset(&src_image, 0, sizeof(image_buffer_t)); // Wraps GStreamer frame
    face_analyzer_result_t face_results; memset(&face_results, 0, sizeof(face_results));
    YoloOutputData latest_yolo_output = {}; // Store the latest processed YOLO output

    // Behavior Analysis Modules
    my::BlinkDetector blinkDetector;
    YawnDetector yawnDetector;
    my::HeadPoseTracker headPoseTracker;
    KSSCalculator kssCalculator;

    // State Variables
    bool calibration_done = false;
    int compositeKSS = 1;
    std::string kssStatus = "Initializing";
    YawnDetector::YawnMetrics yawnMetrics = {};
    my::HeadPoseTracker::HeadPoseResults headPoseResults = {};
    std::vector<std::string> detectedObjects; // Populated after YOLO post-processing

    // Drawing Variables
    std::stringstream text_stream; int text_y = 0; const int line_height = 22; const int text_size = 12; const int status_text_size = 16; unsigned int status_color_uint = COLOR_GREEN;

    // YOLO Worker Thread
    std::thread yolo_worker_thread;

    // Temporary buffer for YOLO RGA preprocessing (allocated once)
    image_buffer_t yolo_input_rga_buffer;
    memset(&yolo_input_rga_buffer, 0, sizeof(image_buffer_t));

    // GStreamer Variables
    GstElement* input_pipeline = nullptr; // Initialize to nullptr
    GError* error = nullptr;
    GstSample* sample = nullptr;
    GstVideoFrame* in_frame = new GstVideoFrame;
    GstVideoFrame* out_frame = new GstVideoFrame; // For saving pipeline
    bool frame_mapped = false; // Track if GStreamer frame is currently mapped

    // Main Loop Counters/Flags
    int frame_counter = 0;
    int skip_frames = 1; // Dynamic frame skipping
    const double target_frame_time = 33.3; // ms (for ~30fps target)
    long last_processed_yolo_frame = -1; // Track the last frame processed by YOLO

    // --- Initialization ---
    printf("Initializing...\n");
    ret = init_post_process();
    if (ret != 0) {
        printf("ERROR: init YOLO postprocess failed.\n");
        // Cleanup before returning
        delete in_frame; delete out_frame;
        return -1;
    }
    ret = init_face_analyzer(detection_model_path, landmark_model_path, iris_model_path, &app_ctx.face_ctx);
    if (ret != 0) {
        printf("ERROR: init_face_analyzer failed! ret=%d\n", ret);
        deinit_post_process();
        delete in_frame; delete out_frame;
        return -1;
    }
    ret = init_yolo11(yolo_model_path, &app_ctx.yolo_ctx);
    if (ret != 0) {
        printf("ERROR: init_yolo11 failed! ret=%d\n", ret);
        release_face_analyzer(&app_ctx.face_ctx);
        deinit_post_process();
        delete in_frame; delete out_frame;
        return -1;
    }
    printf("Model initialization complete.\n");

    // --- Allocate YOLO RGA Input Buffer ---
    yolo_input_rga_buffer.width = YOLO_TARGET_SIZE;
    yolo_input_rga_buffer.height = YOLO_TARGET_SIZE;
    yolo_input_rga_buffer.format = IMAGE_FORMAT_RGB888; // RGA will convert BGR->RGB if needed
    yolo_input_rga_buffer.size = get_image_size(&yolo_input_rga_buffer);
    if (app_ctx.yolo_ctx.io_num.n_input > 0 && app_ctx.yolo_ctx.input_mems && app_ctx.yolo_ctx.input_mems[0]) {
         yolo_input_rga_buffer.virt_addr = (unsigned char*)app_ctx.yolo_ctx.input_mems[0]->virt_addr;
         yolo_input_rga_buffer.fd = app_ctx.yolo_ctx.input_mems[0]->fd;
         yolo_input_rga_buffer.width_stride = app_ctx.yolo_ctx.input_attrs[0].w_stride;
         yolo_input_rga_buffer.height_stride = app_ctx.yolo_ctx.input_attrs[0].h_stride ? app_ctx.yolo_ctx.input_attrs[0].h_stride : YOLO_TARGET_SIZE;
         if (!yolo_input_rga_buffer.virt_addr) {
             printf("ERROR: Failed to get virt_addr from YOLO input_mems[0]\n");
             ret = -1; goto cleanup; // Use goto now that it's defined after declarations
         }
          printf("INFO: Using pre-allocated NPU buffer for YOLO input (virt: %p, fd: %d, size: %d)\n",
                 yolo_input_rga_buffer.virt_addr, yolo_input_rga_buffer.fd, yolo_input_rga_buffer.size);
    } else {
        printf("ERROR: YOLO input memory not properly initialized in context.\n");
        ret = -1; goto cleanup;
    }

    // --- Start YOLO Worker Thread ---
    yolo_worker_thread = std::thread(yolo_worker_thread_func, &app_ctx.yolo_ctx);
    printf("YOLO worker thread started.\n");

    // --- GStreamer Initialization ---
    printf("Initializing GStreamer...\n");
    gst_init(nullptr, nullptr); // Safe to call multiple times
    input_pipeline = gst_parse_launch(video_source, &error);
    if (!input_pipeline || error) {
        std::cerr << "Failed to create GStreamer input pipeline: " << (error ? error->message : "Unknown error") << std::endl;
        if (error) g_error_free(error);
        ret = -1; goto cleanup; // Use goto cleanup
    }
    appsink_ = gst_bin_get_by_name(GST_BIN(input_pipeline), "sink");
    if (!appsink_) {
        std::cerr << "Failed to get appsink element" << std::endl;
        gst_object_unref(input_pipeline); // Cleanup pipeline before jumping
        input_pipeline = nullptr;
        ret = -1; goto cleanup; // Use goto cleanup
    }
    if (gst_element_set_state(input_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set GStreamer input pipeline to playing state" << std::endl;
        gst_object_unref(appsink_); appsink_ = nullptr;
        gst_object_unref(input_pipeline); input_pipeline = nullptr;
        ret = -1; goto cleanup; // Use goto cleanup
    }
    setupPipeline(); // Setup the saving pipeline (assuming it handles its own errors)
    printf("GStreamer pipelines initialized and playing.\n");

    // --- UI Setup ---
    cv::namedWindow("output", cv::WINDOW_NORMAL);
    cv::resizeWindow("output", 1920, 1080); // Or desired display size

    printf("Starting main processing loop...\n");
    // --- Main Loop ---
    while (true) {
        sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink_));
        if (!sample) { std::cerr << "End of stream or error pulling sample." << std::endl; break; }

        GstBuffer* buffer = gst_sample_get_buffer(sample); GstCaps* caps = gst_sample_get_caps(sample);
        if (!buffer || !caps) { gst_sample_unref(sample); continue; }
        GstVideoInfo video_info; gst_video_info_init(&video_info);
        if (!gst_video_info_from_caps(&video_info, caps)) { gst_sample_unref(sample); continue; }
        if (!gst_video_frame_map(in_frame, &video_info, buffer, GST_MAP_READ)) { gst_sample_unref(sample); continue; }

        GstBuffer* out_buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&video_info), nullptr);
        if (!out_buffer || !gst_video_frame_map(out_frame, &video_info, out_buffer, GST_MAP_WRITE)) {
            gst_video_frame_unmap(in_frame); gst_sample_unref(sample); if(out_buffer) gst_buffer_unref(out_buffer); continue;
        }
        frame_mapped = true;

        auto frame_start_time = std::chrono::high_resolution_clock::now();

        src_image.width = in_frame->info.width; src_image.height = in_frame->info.height; src_image.width_stride = in_frame->info.stride[0]; src_image.height_stride = in_frame->info.height;
        src_image.format = IMAGE_FORMAT_RGB888; // IMPORTANT: Assumes GStreamer output is BGR
        src_image.virt_addr = static_cast<unsigned char*>(in_frame->data[0]); src_image.size = in_frame->info.size; src_image.fd = -1;

        // --- Run Face Analyzer ---
        ret = inference_face_analyzer(&app_ctx.face_ctx, &src_image, &face_results);
        if (ret != 0) { printf("WARN: inference_face_analyzer failed! ret=%d\n", ret); }

        // --- YOLO Preprocessing and Queueing ---
        image_rect_t yolo_crop_src_box; bool use_face_roi = (face_results.count > 0);
        if (use_face_roi) {
            face_object_t *face = &face_results.faces[0]; int rx = face->box.left; int ry = face->box.top; int rw = face->box.right - rx; int rh = face->box.bottom - ry;
            int max_dim = std::max(rw, rh); int padding = max_dim * 0.5; int crop_size = max_dim + 2 * padding; int face_center_x = rx + rw / 2; int face_center_y = ry + rh / 2;
            yolo_crop_src_box.left = face_center_x - crop_size / 2; yolo_crop_src_box.top = face_center_y - crop_size / 2; yolo_crop_src_box.right = yolo_crop_src_box.left + crop_size; yolo_crop_src_box.bottom = yolo_crop_src_box.top + crop_size;
            yolo_crop_src_box.left = std::clamp(yolo_crop_src_box.left, 0, src_image.width); // Use std::clamp
            yolo_crop_src_box.top = std::clamp(yolo_crop_src_box.top, 0, src_image.height);
            yolo_crop_src_box.right = std::clamp(yolo_crop_src_box.right, 0, src_image.width);
            yolo_crop_src_box.bottom = std::clamp(yolo_crop_src_box.bottom, 0, src_image.height);
             if (yolo_crop_src_box.right <= yolo_crop_src_box.left || yolo_crop_src_box.bottom <= yolo_crop_src_box.top) { use_face_roi = false; }
        }
        if (!use_face_roi) { yolo_crop_src_box = {0, 0, src_image.width, src_image.height}; }
        image_rect_t yolo_resize_dst_box = {0, 0, YOLO_TARGET_SIZE, YOLO_TARGET_SIZE};

        ret = convert_image(&src_image, &yolo_input_rga_buffer, &yolo_crop_src_box, &yolo_resize_dst_box, 0);
        if (ret != 0) {
            printf("ERROR: RGA convert_image for YOLO failed! ret=%d. Skipping YOLO for frame %d.\n", ret, frame_counter);
            // Skip queuing YOLO input if RGA fails (CPU fallback removed for simplicity here)
        } else {
            int crop_width_actual = yolo_crop_src_box.right - yolo_crop_src_box.left; int crop_height_actual = yolo_crop_src_box.bottom - yolo_crop_src_box.top;
            float rga_scale_x = (crop_width_actual > 0) ? (float)crop_width_actual / YOLO_TARGET_SIZE : 1.0f; float rga_scale_y = (crop_height_actual > 0) ? (float)crop_height_actual / YOLO_TARGET_SIZE : 1.0f;
            YoloInputData yolo_input_payload = {frame_counter, yolo_crop_src_box.left, yolo_crop_src_box.top, rga_scale_x, rga_scale_y};
            { std::unique_lock<std::mutex> lock(yolo_input_mutex); if (yolo_input_queue.size() < MAX_QUEUE_SIZE) { yolo_input_queue.push(yolo_input_payload); } }
        }
        // --- End YOLO Preprocessing and Queueing ---

        // --- Process Behavior Analysis ---
        // ... (Behavior analysis logic remains the same) ...
        if (face_results.count > 0 && face_results.faces[0].face_landmarks_valid) { std::vector<cv::Point> faceLandmarksCv = convert_landmarks_to_cvpoint(face_results.faces[0].face_landmarks, NUM_FACE_LANDMARKS); if (!calibration_done) { headPoseResults = headPoseTracker.run(faceLandmarksCv); if (headPoseTracker.isCalibrated()) { calibration_done = true; printf("Calibration Complete!\n"); } } blinkDetector.run(faceLandmarksCv, src_image.width, src_image.height); yawnMetrics = yawnDetector.run(faceLandmarksCv, src_image.width, src_image.height); if (calibration_done) { headPoseResults = headPoseTracker.run(faceLandmarksCv); } else { headPoseResults = {}; headPoseResults.reference_set = false; } } else { if (!calibration_done) { kssStatus = "Low Risk (No Face)"; } yawnMetrics = {}; headPoseResults = {}; headPoseResults.reference_set = false; }

        // --- Process YOLO Results ---
        bool yolo_result_updated_this_frame = false;
        { std::unique_lock<std::mutex> lock(yolo_output_mutex); while (!yolo_output_queue.empty()) { latest_yolo_output = yolo_output_queue.front(); yolo_output_queue.pop(); yolo_result_updated_this_frame = true; last_processed_yolo_frame = latest_yolo_output.frame_id; } }
        detectedObjects.clear();
        if (yolo_result_updated_this_frame || latest_yolo_output.frame_id >= 0) { // Process if new or we have old results
             float scale_x = latest_yolo_output.scale_to_original_x; float scale_y = latest_yolo_output.scale_to_original_y; int offset_x = latest_yolo_output.crop_offset_x; int offset_y = latest_yolo_output.crop_offset_y;
             for (int k = 0; k < latest_yolo_output.results.count; ++k) {
                 object_detect_result& res = latest_yolo_output.results.results[k];
                 int x1_640 = res.box.left; int y1_640 = res.box.top; int x2_640 = res.box.right; int y2_640 = res.box.bottom;
                 float x1_orig_crop = x1_640 * scale_x; float y1_orig_crop = y1_640 * scale_y; float x2_orig_crop = x2_640 * scale_x; float y2_orig_crop = y2_640 * scale_y;
                 res.box.left   = std::clamp((int)roundf(x1_orig_crop + offset_x), 0, src_image.width - 1); // Use std::clamp
                 res.box.top    = std::clamp((int)roundf(y1_orig_crop + offset_y), 0, src_image.height - 1);
                 res.box.right  = std::clamp((int)roundf(x2_orig_crop + offset_x), 0, src_image.width - 1);
                 res.box.bottom = std::clamp((int)roundf(y2_orig_crop + offset_y), 0, src_image.height - 1);
                  if (res.prop >= BOX_THRESH) { detectedObjects.push_back(coco_cls_to_name(res.cls_id)); }
             }
        }
        // --- Calculate KSS Score ---
        // ... (KSS calculation logic remains the same) ...
        kssCalculator.setPerclos(blinkDetector.getPerclos()); int headPoseKSSValue = 1; if (calibration_done && headPoseResults.rows.size() >= 4) { for (const auto& row : headPoseResults.rows) { if (row[0] == "Head KSS") { try { headPoseKSSValue = std::stoi(row[1]); } catch (...) { /*WARN*/ } break; } } } kssCalculator.setHeadPose(headPoseKSSValue); kssCalculator.setYawnMetrics(yawnMetrics.isYawning, yawnMetrics.yawnFrequency, yawnMetrics.yawnDuration); double now_seconds = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count(); kssCalculator.setDetectedObjects(detectedObjects, now_seconds); kssCalculator.setBlinksLastMinute(blinkDetector.getBlinksInWindow()); compositeKSS = kssCalculator.calculateCompositeKSS(); if (calibration_done) { kssStatus = kssCalculator.getKSSAlertStatus(compositeKSS); } else if (kssStatus != "Low Risk (Cal. Failed)" && kssStatus != "Low Risk (No Face)") { kssStatus = "Calibrating..."; }

        // --- Drawing ---
        // ... (Drawing logic remains the same, ensure it uses the scaled latest_yolo_output) ...
         for (int i = 0; i < face_results.count; ++i) { face_object_t *face = &face_results.faces[i]; draw_rectangle(&src_image, face->box.left, face->box.top, face->box.right - face->box.left, face->box.bottom - face->box.top, COLOR_GREEN, 2); if (face->face_landmarks_valid) { for (int j = 0; j < NUM_FACE_LANDMARKS; ++j) draw_circle(&src_image, face->face_landmarks[j].x, face->face_landmarks[j].y, 1, COLOR_CYAN, -1); } if (face->eye_landmarks_left_valid) { for (int j = 0; j < NUM_EYE_CONTOUR_LANDMARKS; ++j) draw_circle(&src_image, face->eye_landmarks_left[j].x, face->eye_landmarks_left[j].y, 1, COLOR_YELLOW, -1); } if (face->iris_landmarks_left_valid) { for (int j = 0; j < NUM_IRIS_LANDMARKS; ++j) draw_circle(&src_image, face->iris_landmarks_left[j].x, face->iris_landmarks_left[j].y, 2, COLOR_RED, -1); } if (face->eye_landmarks_right_valid) { for (int j = 0; j < NUM_EYE_CONTOUR_LANDMARKS; ++j) draw_circle(&src_image, face->eye_landmarks_right[j].x, face->eye_landmarks_right[j].y, 1, COLOR_YELLOW, -1); } if (face->iris_landmarks_right_valid) { for (int j = 0; j < NUM_IRIS_LANDMARKS; ++j) draw_circle(&src_image, face->iris_landmarks_right[j].x, face->iris_landmarks_right[j].y, 2, COLOR_RED, -1); } }
         for (int k = 0; k < latest_yolo_output.results.count; ++k) { object_detect_result& result = latest_yolo_output.results.results[k]; if (result.prop >= BOX_THRESH) { draw_rectangle(&src_image, result.box.left, result.box.top, result.box.right - result.box.left, result.box.bottom - result.box.top, COLOR_MAGENTA, 2); std::string label = coco_cls_to_name(result.cls_id); draw_text(&src_image, label.c_str(), result.box.left, result.box.top - 15 > 0 ? result.box.top - 15 : result.box.top, COLOR_ORANGE, 14); } }
         if (compositeKSS <= 3) status_color_uint = COLOR_GREEN; else if (compositeKSS <= 7) status_color_uint = COLOR_BLUE; else if (compositeKSS <= 9) status_color_uint = COLOR_YELLOW; else status_color_uint = COLOR_DARK_RED;
         text_y = 10; draw_text(&src_image, kssStatus.c_str(), 10, text_y, status_color_uint, status_text_size); text_y += (int)(line_height * 1.4); text_stream.str(""); text_stream << "PERCLOS: " << std::fixed << std::setprecision(2) << blinkDetector.getPerclos() << "%"; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height; if (calibration_done && headPoseResults.rows.size() >= 3) { std::string headpose_text = "Yaw:" + headPoseResults.rows[0][1] + " Pitch:" + headPoseResults.rows[1][1] + " Roll:" + headPoseResults.rows[2][1]; draw_text(&src_image, headpose_text.c_str(), 10, text_y, COLOR_WHITE, text_size); } else { draw_text(&src_image, "Head Pose: N/A", 10, text_y, COLOR_WHITE, text_size); } text_y += line_height; text_stream.str(""); text_stream << "Yawning: " << (yawnMetrics.isYawning ? "Yes" : "No"); draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height; text_stream.str(""); text_stream << "Yawn Freq (1min): " << static_cast<int>(yawnMetrics.yawnFrequency); draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height; text_stream.str(""); text_stream << "Yawn Count (5min): " << static_cast<int>(yawnMetrics.yawnCount); draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height; text_stream.str(""); text_stream << "KSS Score: " << compositeKSS; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, status_color_uint, text_size);

        // Draw Resource Monitoring
        auto frame_end_time = std::chrono::high_resolution_clock::now(); double frame_duration_ms = std::chrono::duration<double, std::milli>(frame_end_time - frame_start_time).count(); calculateOverallFPS(frame_duration_ms, frame_times, overallFPS, max_time_records); getCPUUsage(currentCpuUsage, prevIdleTime, prevTotalTime); getTemperature(currentTemp); std::stringstream info_ss; info_ss << "FPS: " << std::fixed << std::setprecision(1) << overallFPS << " | CPU: " << std::fixed << std::setprecision(1) << currentCpuUsage << "%" << " | Temp: " << std::fixed << std::setprecision(1) << currentTemp << " C"; draw_text(&src_image, info_ss.str().c_str(), 10, src_image.height - 20, COLOR_WHITE, 14);

        // --- Final Display and GStreamer Output ---
        cv::Mat display_mat_final = image_buffer_to_mat(&src_image);
        if (!display_mat_final.empty()) {
            cv::imshow("output", display_mat_final);
            memcpy(out_frame->data[0], src_image.virt_addr, src_image.size);
            pushFrameToPipeline(out_frame);
            if (cv::waitKey(1) == 27) break; // Exit on ESC
        } else {
             printf("WARN: Display Mat is empty! Pushing original frame to save pipeline.\n");
             memcpy(out_frame->data[0], in_frame->data[0], in_frame->info.size);
             pushFrameToPipeline(out_frame);
        }

        // --- Loop End Cleanup ---
        frame_counter++;
        // Frame skipping logic
        double process_time = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - frame_start_time).count();
        if (process_time > target_frame_time) skip_frames = std::min(skip_frames + 1, 5); else if (process_time < target_frame_time / 2) skip_frames = std::max(skip_frames - 1, 0);

        if (frame_mapped) {
            gst_video_frame_unmap(out_frame); gst_video_frame_unmap(in_frame); frame_mapped = false;
        }
        gst_buffer_unref(out_buffer); gst_sample_unref(sample); sample = nullptr; buffer = nullptr; // Reset pointers

    } // End while loop

// --- Application Cleanup ---
cleanup: // Label for cleanup jump target
    printf("Cleaning up application resources...\n");

    // Delete GStreamer frame objects if they were allocated
    if (in_frame) delete in_frame;
    if (out_frame) delete out_frame;

    // Signal worker thread to stop and wait for it
    stop_yolo_worker.store(true);
    if (yolo_worker_thread.joinable()) {
        yolo_worker_thread.join();
        printf("YOLO worker thread joined.\n");
    }

    // GStreamer Cleanup
    printf("Stopping GStreamer pipelines...\n");
    if (input_pipeline) {
        gst_element_set_state(input_pipeline, GST_STATE_NULL);
        // appsink_ is owned by input_pipeline, unref pipeline handles it
        // if (appsink_) gst_object_unref(appsink_); // No need to unref separately
        gst_object_unref(input_pipeline);
        printf("Input pipeline stopped.\n");
    }
    if (pipeline_) { // Check if saving pipeline was created
        gst_element_send_event(pipeline_, gst_event_new_eos());
        GstBus *bus = gst_element_get_bus(pipeline_);
        if(bus) {
             gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
             gst_object_unref(bus);
        }
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        // appsrc_ is owned by pipeline_, unref pipeline handles it
        // if (appsrc_) gst_object_unref(appsrc_); // No need to unref separately
        gst_object_unref(pipeline_);
        printf("Saving pipeline stopped.\n");
    }
    // gst_deinit(); // Can sometimes cause issues if not managed carefully

    // RKNN Release
    printf("Releasing RKNN models...\n");
    release_face_analyzer(&app_ctx.face_ctx); printf("Face analyzer released.\n");
    release_yolo11(&app_ctx.yolo_ctx); printf("YOLO model released.\n");
    deinit_post_process(); printf("Post-process deinitialized.\n");

    // --- FIX: Only destroy window if main loop was likely entered ---
    // We can check if frame_counter is > 0 as an indicator that the loop ran at least once
    if (frame_counter > 0) {
         printf("Destroying OpenCV window.\n");
         cv::destroyWindow("output");
    } else {
         printf("Skipping OpenCV window destruction (main loop likely not entered).\n");
    }
    // --- END FIX ---

    printf("Exiting application (ret = %d)\n", ret);
    return ret;
}