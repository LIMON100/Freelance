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
// #include <sys/sysinfo.h> 
// #include <sys/stat.h> 
// #include <unistd.h>   
// #include <dirent.h>  

// // Include project headers
// #include "face_analyzer/face_analyzer.h"
// #include "yolo_detector/yolo11.h"
// #include "behavior_analysis/BlinkDetector.hpp"
// #include "behavior_analysis/YawnDetector.hpp"
// #include "behavior_analysis/HeadPoseTracker.hpp"
// #include "behavior_analysis/KSSCalculator.hpp"
// #include "image_utils.h"
// #include "file_utils.h"
// #include "image_drawing.h"

// // GStreamer Includes
// #include <gst/gst.h>
// #include <gst/video/video.h>
// #include <gst/app/gstappsink.h>
// #include <gst/allocators/gstdmabuf.h>

// // Resource Monitoring Includes
// #include <fstream>
// #include <numeric> 
// #include <deque>

// // Define colors (ensure these are defined consistently)
// #ifndef COLOR_MAGENTA
// #define COLOR_MAGENTA (0xFF00FF)
// #endif
// #ifndef COLOR_YELLOW
// #define COLOR_YELLOW (0x00FFFF)
// #endif
// #ifndef COLOR_WHITE
// #define COLOR_WHITE (0xFFFFFF)
// #endif
// #ifndef COLOR_GREEN
// #define COLOR_GREEN (0x00FF00)
// #endif
// #ifndef COLOR_RED
// #define COLOR_RED (0x0000FF)
// #endif
// #ifndef COLOR_BLUE
// #define COLOR_BLUE (0xFF0000)
// #endif
// #ifndef COLOR_ORANGE
// #define COLOR_ORANGE (0x00A5FF)
// #endif


// // --- Application Context ---
// typedef struct app_context_t {
//     face_analyzer_app_context_t face_ctx;
//     yolo11_app_context_t        yolo_ctx;
// } app_context_t;

// // --- Global GStreamer elements ---
// static GstElement* pipeline_ = nullptr; // Saving pipeline
// static GstElement* appsrc_ = nullptr;   // Source for saving pipeline
// static GstElement* appsink_ = nullptr;  // Sink for input pipeline

// // --- Helper Functions 
// std::vector<cv::Point> convert_landmarks_to_cvpoint(const point_t landmarks[], int count) { /* ... */
//     std::vector<cv::Point> cv_landmarks; cv_landmarks.reserve(count); for (int i = 0; i < count; ++i) cv_landmarks.emplace_back(cv::Point(landmarks[i].x, landmarks[i].y)); return cv_landmarks;
// }
// double parse_head_pose_value(const std::string& s) { /* ... */
//     try { std::string n=s; size_t d=n.find(" deg"); if(d!=std::string::npos) n=n.substr(0,d); size_t f=n.find_first_not_of(" \t"); if(f==std::string::npos) return 0.0; size_t l=n.find_last_not_of(" \t"); n=n.substr(f,l-f+1); if(n.empty()) return 0.0; return std::stod(n); } catch (const std::exception& e) { printf("WARN: Ex parse head pose '%s': %s\n", s.c_str(), e.what()); return 0.0; } catch (...) { printf("WARN: Unk ex parse head pose '%s'.\n", s.c_str()); return 0.0; }
// }
// template <typename T> T calculate_stddev(const std::deque<T>& data) { /* ... */
//     if (data.size() < 2) return T(0); T sum = std::accumulate(data.begin(), data.end(), T(0)); T mean = sum / data.size(); T sq_sum = std::inner_product(data.begin(), data.end(), data.begin(), T(0)); T variance = sq_sum / data.size() - mean * mean; return std::sqrt(std::max(T(0), variance)); // Ensure variance is non-negative
// }
// float calculate_ear_simple(const std::vector<cv::Point>& landmarks, const std::vector<int>& eye_points) { /* ... */
//     if (landmarks.empty()) return 1.0f; int max_idx = 0; for(int idx : eye_points) { if (idx > max_idx) max_idx = idx; } if (max_idx >= landmarks.size()) return 1.0f; try { cv::Point p1=landmarks[eye_points[0]], p2=landmarks[eye_points[1]], p3=landmarks[eye_points[2]], p4=landmarks[eye_points[3]], p5=landmarks[eye_points[4]], p6=landmarks[eye_points[5]]; double v1=cv::norm(p2-p6), v2=cv::norm(p3-p5), h=cv::norm(p1-p4); if(h<1e-6) return 1.0f; return static_cast<float>((v1+v2)/(2.0*h)); } catch(...) { return 1.0f; }
// }
// double calculate_mouth_dist_simple(const std::vector<cv::Point>& landmarks) { /* ... */
//     if (landmarks.empty() || landmarks.size() <= 14) return 0.0; try { return cv::norm(landmarks[13]-landmarks[14]); } catch (...) { return 0.0; }
// }
// // --- End Helper Functions ---


// // --- YOLO Worker Thread 
// struct YoloInputData { long frame_id; std::shared_ptr<image_buffer_t> image; };
// struct YoloOutputData { long frame_id; object_detect_result_list results; };
// std::queue<YoloInputData> yolo_input_queue;
// std::queue<YoloOutputData> yolo_output_queue;
// std::mutex yolo_input_mutex;
// std::mutex yolo_output_mutex;
// std::atomic<bool> stop_yolo_worker(false);
// const int MAX_QUEUE_SIZE = 5;
// void yolo_worker_thread_func(yolo11_app_context_t* yolo_ctx_ptr) { 
//      while (!stop_yolo_worker.load()) { YoloInputData input_data; bool got_data = false; { std::unique_lock<std::mutex> lock(yolo_input_mutex); if (!yolo_input_queue.empty()) { input_data = yolo_input_queue.front(); yolo_input_queue.pop(); got_data = true; } } if (got_data && input_data.image) { YoloOutputData output_data; output_data.frame_id = input_data.frame_id; memset(&output_data.results, 0, sizeof(output_data.results)); int ret = inference_yolo11(yolo_ctx_ptr, input_data.image.get(), &output_data.results); if (ret != 0) printf("WARN: YOLO Worker inference failed (frame %ld), ret=%d\n", input_data.frame_id, ret); { std::unique_lock<std::mutex> lock(yolo_output_mutex); if (yolo_output_queue.size() >= MAX_QUEUE_SIZE) yolo_output_queue.pop(); yolo_output_queue.push(output_data); } if (input_data.image->virt_addr) { free(input_data.image->virt_addr); } } else { std::this_thread::sleep_for(std::chrono::milliseconds(5)); } } printf("YOLO Worker Thread Exiting.\n");
// }
// // --- End YOLO Worker Thread ---

// void setupPipeline() { 
//     gst_init(nullptr, nullptr); std::string dir = "/userdata/test_cpp/dms_gst"; if (access(dir.c_str(), W_OK) != 0) { std::cerr << "Directory " << dir << " is not writable or does not exist" << std::endl; return; } DIR* directory = opendir(dir.c_str()); if (!directory) { std::cerr << "Failed to open directory " << dir << std::endl; return; } int mkv_count = 0; struct dirent* entry; while ((entry = readdir(directory)) != nullptr) { std::string filename = entry->d_name; if (filename.find(".mkv") != std::string::npos) mkv_count++; } closedir(directory); std::string filepath = dir + "/dms_multi_" + std::to_string(mkv_count + 1) + ".mkv"; std::string pipeline_str = "appsrc name=source ! queue ! videoconvert ! video/x-raw,format=NV12 ! mpph265enc rc-mode=cbr bps=4000000 gop=30 qp-min=10 qp-max=51 ! h265parse ! matroskamux ! filesink location=" + filepath; std::cout << "Saving Pipeline: " << pipeline_str << std::endl; GError* error = nullptr; pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error); if (!pipeline_ || error) { std::cerr << "Failed to create saving pipeline: " << (error ? error->message : "Unknown error") << std::endl; if (error) g_error_free(error); return; } appsrc_ = gst_bin_get_by_name(GST_BIN(pipeline_), "source"); if (!appsrc_) { std::cerr << "Failed to get appsrc" << std::endl; gst_object_unref(pipeline_); pipeline_ = nullptr; return; } GstCaps* caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", "width", G_TYPE_INT, 1920, "height", G_TYPE_INT, 1080, "framerate", GST_TYPE_FRACTION, 60, 1, nullptr); g_object_set(G_OBJECT(appsrc_), "caps", caps, "format", GST_FORMAT_TIME, nullptr); gst_caps_unref(caps); if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) { std::cerr << "Failed to set saving pipeline to playing" << std::endl; gst_object_unref(appsrc_); gst_object_unref(pipeline_); pipeline_ = nullptr; appsrc_ = nullptr; }
// }
// void pushFrameToPipeline(unsigned char* data, int size, int width, int height, GstClockTime duration) { 
//      if (!appsrc_) return; GstBuffer* buffer = gst_buffer_new_allocate(nullptr, size, nullptr); GstMapInfo map; if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) { std::cerr << "Failed map buffer" << std::endl; gst_buffer_unref(buffer); return; } if (map.size != (guint)size) { std::cerr << "Buffer size mismatch: " << map.size << " vs " << size << std::endl; gst_buffer_unmap(buffer, &map); gst_buffer_unref(buffer); return; } memcpy(map.data, data, size); gst_buffer_unmap(buffer, &map); static GstClockTime timestamp = 0; GST_BUFFER_PTS(buffer) = timestamp; GST_BUFFER_DURATION(buffer) = duration; timestamp += GST_BUFFER_DURATION(buffer); GstFlowReturn ret; g_signal_emit_by_name(appsrc_, "push-buffer", buffer, &ret); if (ret != GST_FLOW_OK) std::cerr << "Failed push buffer, ret=" << ret << std::endl; gst_buffer_unref(buffer);
// }
// // --- End GStreamer Saving Pipeline ---

// // --- Resource Monitoring Functions 
// void calculateOverallFPS(double frame_duration_ms, std::deque<double>& times_deque, double& fps_variable, int max_records) {
//      times_deque.push_back(frame_duration_ms); if (times_deque.size() > max_records) times_deque.pop_front(); if (!times_deque.empty()) { double sum = std::accumulate(times_deque.begin(), times_deque.end(), 0.0); double avg_time_ms = sum / times_deque.size(); fps_variable = (avg_time_ms > 0) ? (1000.0 / avg_time_ms) : 0.0; } else { fps_variable = 0.0; }
// }
// void getCPUUsage(double& cpu_usage_variable, long& prev_idle, long& prev_total) {
//     std::ifstream file("/proc/stat"); if (!file.is_open()) return; std::string line; std::getline(file, line); file.close(); long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice; user = nice = system = idle = iowait = irq = softirq = steal = guest = guest_nice = 0; std::istringstream iss(line); std::string cpu_label; iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice; long currentIdleTime = idle + iowait; long currentTotalTime = user + nice + system + idle + iowait + irq + softirq + steal; long diffIdle = currentIdleTime - prev_idle; long diffTotal = currentTotalTime - prev_total; if (diffTotal > 0) cpu_usage_variable = 100.0 * (double)(diffTotal - diffIdle) / diffTotal; prev_idle = currentIdleTime; prev_total = currentTotalTime;
// }
// void getTemperature(double& temp_variable) {
//     const char* temp_paths[] = {"/sys/class/thermal/thermal_zone0/temp", "/sys/class/thermal/thermal_zone1/temp"}; bool temp_read = false; for (const char* path : temp_paths) { std::ifstream file(path); double temp_milliC = 0; if (file >> temp_milliC) { temp_variable = temp_milliC / 1000.0; temp_read = true; file.close(); break; } file.close(); }
// }


// /*-------------------------------------------
//                   Main Function
// -------------------------------------------*/
// int main(int argc, char **argv) {

//     int ret = 0;
//     long current_frame_id = 0;

//     // --- Resource Monitoring Vars
//     std::deque<double> frame_times; const int max_time_records = 60; double overallFPS = 0.0;
//     double currentCpuUsage = 0.0; long prevIdleTime = 0, prevTotalTime = 0; double currentTemp = 0.0;

//     // Model paths 
//     // const char *detection_model_path = "../../model/faceD.rknn";
//     const char *detection_model_path = "../../model/rf.rknn";
//     const char *landmark_model_path  = "../../model/faceL.rknn";
//     const char *iris_model_path      = "../../model/faceI.rknn";
//     const char *yolo_model_path      = "../../model/od.rknn";

//     // GStreamer input pipeline string 
//     const char *video_source = "filesrc location=../../model/1.mkv ! decodebin ! queue ! videoconvert ! video/x-raw,format=BGR ! appsink name=sink sync=false";
//     // const char *video_source = "v4l2src device=/dev/video1 ! queue ! videoconvert ! video/x-raw,format=BGR,width=1920,height=1080,framerate=30/1 ! appsink name=sink sync=false";


//     // --- Initialization 
//     setupPipeline();
//     app_context_t app_ctx; memset(&app_ctx, 0, sizeof(app_context_t));
//     image_buffer_t src_image; memset(&src_image, 0, sizeof(image_buffer_t));
//     face_analyzer_result_t face_results; memset(&face_results, 0, sizeof(face_results));
//     object_detect_result_list yolo_results; memset(&yolo_results, 0, sizeof(yolo_results));

//     // Behavior analysis modules
//     my::BlinkDetector blinkDetector;
//     YawnDetector yawnDetector;
//     my::HeadPoseTracker headPoseTracker;
//     KSSCalculator kssCalculator;

//     // --- Calibration State Variables 
//     bool calibration_done = false;
//     std::chrono::steady_clock::time_point calibration_start_time;
//     bool calibration_timer_started = false;
//     int consecutive_valid_eyes_frames = 0;
//     const int REQUIRED_VALID_EYES_FRAMES = 60;
//     bool ear_calibrated = false;
//     bool mouth_calibrated = false;
//     std::deque<float> calib_left_ears;
//     std::deque<float> calib_right_ears;
//     std::deque<double> calib_mouth_dists;
//     int consecutive_stable_ear_frames = 0;
//     int consecutive_stable_mouth_frames = 0;
//     const int CALIB_WINDOW_SIZE = 30;
//     const float EAR_STDDEV_THRESHOLD = 0.04;
//     const double MOUTH_DIST_STDDEV_THRESHOLD = 15.0;
//     const int REQUIRED_STABLE_FRAMES = CALIB_WINDOW_SIZE + 5;

//     // State variables
//     std::string kssStatus = "Initializing";
//     YawnDetector::YawnMetrics yawnMetrics = {};
//     my::HeadPoseTracker::HeadPoseResults headPoseResults = {};
//     std::vector<std::string> detectedObjects;
//     int extractedTotalKSS = 1;
//     int perclosKSS = 1, blinkKSS = 1, headposeKSS = 1, yawnKSS = 1, objdectdetectionKSS = 1;

//     // Drawing helpers
//     std::stringstream text_stream; int text_y = 0; const int line_height = 22; const int text_size = 12; const int status_text_size = 16; unsigned int status_color_uint = COLOR_GREEN;

//     // YOLO thread handle
//     std::thread yolo_worker_thread;

//     // --- Init models 
//     ret = init_post_process(); if (ret != 0) { printf("Error init YOLO postprocess.\n"); return -1; }

//     // ret = init_face_analyzer(detection_model_path, landmark_model_path, iris_model_path, &app_ctx.face_ctx); 
//     ret = init_face_analyzer(detection_model_path, landmark_model_path, iris_model_path, &app_ctx.face_ctx);
    
//     if (ret != 0) { printf("init_face_analyzer fail! ret=%d\n", ret); deinit_post_process(); return -1; }
//     ret = init_yolo11(yolo_model_path, &app_ctx.yolo_ctx); if (ret != 0) { printf("init_yolo11 fail! ret=%d\n", ret); release_face_analyzer(&app_ctx.face_ctx); deinit_post_process(); return -1; }
//     yolo_worker_thread = std::thread(yolo_worker_thread_func, &app_ctx.yolo_ctx);

//     // --- Init GStreamer input 
//     gst_init(nullptr, nullptr); GError* error = nullptr;
//     GstElement* input_pipeline = gst_parse_launch(video_source, &error); if (!input_pipeline || error) { std::cerr << "Failed to create input pipeline: " << (error ? error->message : "Unknown error") << std::endl; if (error) g_error_free(error); /* ... more cleanup ... */ return -1; }
//     appsink_ = gst_bin_get_by_name(GST_BIN(input_pipeline), "sink"); if (!appsink_) { std::cerr << "Failed to get appsink" << std::endl; gst_object_unref(input_pipeline); /* ... more cleanup ... */ return -1; }
//     if (gst_element_set_state(input_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) { std::cerr << "Failed to set input pipeline to playing" << std::endl; gst_object_unref(appsink_); gst_object_unref(input_pipeline); /* ... more cleanup ... */ return -1; }

//     // --- Setup OpenCV window 
//     cv::namedWindow("DMS Output", cv::WINDOW_NORMAL); cv::resizeWindow("DMS Output", 1920, 1080);

//     GstSample* sample; GstVideoInfo video_info; GstBuffer* gst_buffer; GstMapInfo map_info;

//     // --- Main Processing Loop ---
//     while (true) {
//         // --- Get Frame 
//         sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink_)); if (!sample) { std::cerr << "End of stream or error pulling sample." << std::endl; break; }
//         gst_buffer = gst_sample_get_buffer(sample); GstCaps* caps = gst_sample_get_caps(sample);
//         if (!gst_buffer || !caps || !gst_video_info_from_caps(&video_info, caps) || !gst_buffer_map(gst_buffer, &map_info, GST_MAP_READ)) { std::cerr << "Failed to get valid buffer/caps/map" << std::endl; if (gst_buffer && map_info.memory) gst_buffer_unmap(gst_buffer, &map_info); if (sample) gst_sample_unref(sample); continue; }

//         auto frame_start_time = std::chrono::high_resolution_clock::now();
//         current_frame_id++;

//         // Wrap GStreamer Data 
//         src_image.width = GST_VIDEO_INFO_WIDTH(&video_info); src_image.height = GST_VIDEO_INFO_HEIGHT(&video_info); src_image.width_stride = GST_VIDEO_INFO_PLANE_STRIDE(&video_info, 0); src_image.height_stride = src_image.height;
// #ifdef IMAGE_FORMAT_BGR888
//         src_image.format = IMAGE_FORMAT_BGR888;
// #else
//         src_image.format = IMAGE_FORMAT_RGB888;
// #endif
//         src_image.virt_addr = map_info.data; src_image.size = map_info.size; src_image.fd = -1;

//         // --- Run Face Analysis (Always needed) ---
//         // ret = inference_face_analyzer(&app_ctx.face_ctx, &src_image, &face_results);
//         ret = inference_face_analyzer(&app_ctx.face_ctx, &src_image, &face_results);
//         // Handle face analysis error if necessary

//         // --- Calibration Phase Logic 
//         if (!calibration_done) {
//             bool head_pose_calibrated = false; bool eyes_consistently_valid = false; double elapsed_calib_seconds = 0.0; std::string calib_status_detail = "";
//             if (face_results.count > 0 && face_results.faces[0].face_landmarks_valid) {
//                 if (!calibration_timer_started) { calibration_start_time = std::chrono::steady_clock::now(); calibration_timer_started = true; printf("INFO: Calibration timer started.\n"); consecutive_valid_eyes_frames = 0; consecutive_stable_ear_frames = 0; consecutive_stable_mouth_frames = 0; calib_left_ears.clear(); calib_right_ears.clear(); calib_mouth_dists.clear(); ear_calibrated = false; mouth_calibrated = false; }
//                 std::vector<cv::Point> calib_faceLandmarks = convert_landmarks_to_cvpoint(face_results.faces[0].face_landmarks, NUM_FACE_LANDMARKS); headPoseTracker.run(calib_faceLandmarks); head_pose_calibrated = headPoseTracker.isCalibrated(); if (!head_pose_calibrated) calib_status_detail += " (Head)";
//                 bool eyes_valid_this_frame = face_results.faces[0].eye_landmarks_left_valid && face_results.faces[0].eye_landmarks_right_valid; if (eyes_valid_this_frame) consecutive_valid_eyes_frames++; else consecutive_valid_eyes_frames = 0; eyes_consistently_valid = (consecutive_valid_eyes_frames >= REQUIRED_VALID_EYES_FRAMES); if (!eyes_consistently_valid) calib_status_detail += " (Eyes Valid)";
//                 if (!ear_calibrated && eyes_valid_this_frame) { const std::vector<int> L={33,160,158,133,153,144}, R={362,385,387,263,380,373}; float lE=calculate_ear_simple(calib_faceLandmarks,L), rE=calculate_ear_simple(calib_faceLandmarks,R); calib_left_ears.push_back(lE); calib_right_ears.push_back(rE); if (calib_left_ears.size()>CALIB_WINDOW_SIZE) calib_left_ears.pop_front(); if (calib_right_ears.size()>CALIB_WINDOW_SIZE) calib_right_ears.pop_front(); if (calib_left_ears.size()>=CALIB_WINDOW_SIZE) { float lS=calculate_stddev(calib_left_ears), rS=calculate_stddev(calib_right_ears); if (lS<EAR_STDDEV_THRESHOLD && rS<EAR_STDDEV_THRESHOLD) consecutive_stable_ear_frames++; else consecutive_stable_ear_frames=0; } else consecutive_stable_ear_frames=0; ear_calibrated=(consecutive_stable_ear_frames>=REQUIRED_STABLE_FRAMES); } else if(!eyes_valid_this_frame){ consecutive_stable_ear_frames=0; ear_calibrated=false; calib_left_ears.clear(); calib_right_ears.clear(); } if(!ear_calibrated) calib_status_detail+=" (EAR Stable)";
//                 if (!mouth_calibrated) { double cM=calculate_mouth_dist_simple(calib_faceLandmarks); calib_mouth_dists.push_back(cM); if(calib_mouth_dists.size()>CALIB_WINDOW_SIZE) calib_mouth_dists.pop_front(); if(calib_mouth_dists.size()>=CALIB_WINDOW_SIZE){ double mS=calculate_stddev(calib_mouth_dists); if(mS<MOUTH_DIST_STDDEV_THRESHOLD) consecutive_stable_mouth_frames++; else consecutive_stable_mouth_frames=0; } else consecutive_stable_mouth_frames=0; mouth_calibrated=(consecutive_stable_mouth_frames>=REQUIRED_STABLE_FRAMES); } if(!mouth_calibrated) calib_status_detail+=" (Mouth Stable)";
//                 auto now = std::chrono::steady_clock::now(); elapsed_calib_seconds = std::chrono::duration<double>(now - calibration_start_time).count();
//                 if (elapsed_calib_seconds >= 10.0) { if (head_pose_calibrated && eyes_consistently_valid && ear_calibrated && mouth_calibrated) { calibration_done = true; printf("INFO: Calibration Complete!\n"); } else { calibration_timer_started = false; consecutive_valid_eyes_frames = 0; consecutive_stable_ear_frames = 0; consecutive_stable_mouth_frames = 0; ear_calibrated = false; mouth_calibrated = false; calib_left_ears.clear(); calib_right_ears.clear(); calib_mouth_dists.clear(); printf("WARN: Calibration time expired, criteria not met (H:%d, EV:%d, ES:%d, MS:%d). Retrying.\n", head_pose_calibrated, eyes_consistently_valid, ear_calibrated, mouth_calibrated); } }
//                 for (int i = 0; i < face_results.count; ++i) { draw_rectangle(&src_image, face_results.faces[i].box.left, face_results.faces[i].box.top, face_results.faces[i].box.right - face_results.faces[i].box.left, face_results.faces[i].box.bottom - face_results.faces[i].box.top, COLOR_YELLOW, 2); } std::string calib_text = "Calibrating... " + std::to_string(static_cast<int>(elapsed_calib_seconds)) + "s" + calib_status_detail; draw_text(&src_image, calib_text.c_str(), 10, 30, COLOR_YELLOW, status_text_size);
//             } else { calibration_timer_started = false; consecutive_valid_eyes_frames = 0; consecutive_stable_ear_frames = 0; consecutive_stable_mouth_frames = 0; ear_calibrated = false; mouth_calibrated = false; calib_left_ears.clear(); calib_right_ears.clear(); calib_mouth_dists.clear(); draw_text(&src_image, "Calibration: Waiting for face...", 10, 30, COLOR_YELLOW, status_text_size); }

//         } else { // Normal Processing Phase
//             // --- Queue Frame for YOLO ---
//             auto yolo_input_image = std::make_shared<image_buffer_t>();
//             yolo_input_image->size = src_image.size;
//             yolo_input_image->virt_addr = (unsigned char*)malloc(yolo_input_image->size);
//             if (yolo_input_image->virt_addr) {
//                 memcpy(yolo_input_image->virt_addr, src_image.virt_addr, src_image.size);
//                 yolo_input_image->width = src_image.width; yolo_input_image->height = src_image.height; yolo_input_image->format = src_image.format; yolo_input_image->width_stride = src_image.width_stride; yolo_input_image->height_stride = src_image.height_stride; yolo_input_image->fd = -1;
//                 { std::unique_lock<std::mutex> lock(yolo_input_mutex); if (yolo_input_queue.size() < MAX_QUEUE_SIZE) yolo_input_queue.push({current_frame_id, yolo_input_image}); else free(yolo_input_image->virt_addr); /* Drop frame */ }
//             } else { printf("ERROR: Failed alloc YOLO input copy.\n"); }

//             // --- Get YOLO Results ---
//             bool yolo_result_received = false;
//             { std::unique_lock<std::mutex> lock(yolo_output_mutex); if (!yolo_output_queue.empty()) { YoloOutputData data = yolo_output_queue.front(); yolo_output_queue.pop(); yolo_results = data.results; yolo_result_received = true; } }

//             // --- Run Behavior Analysis & KSS ---
//             detectedObjects.clear();
//             if (face_results.count > 0 && face_results.faces[0].face_landmarks_valid) {
//                 face_object_t *face = &face_results.faces[0];
//                 std::vector<cv::Point> faceLandmarksCv = convert_landmarks_to_cvpoint(face->face_landmarks, NUM_FACE_LANDMARKS);
//                 headPoseResults = headPoseTracker.run(faceLandmarksCv);
//                 blinkDetector.run(faceLandmarksCv, src_image.width, src_image.height);
//                 yawnMetrics = yawnDetector.run(faceLandmarksCv, src_image.width, src_image.height);

//                 // +++++++++++++++ START INSERTED CODE +++++++++++++++
//                 // Draw Landmarks for all detected faces (after calibration)
//                 for (int face_idx = 0; face_idx < face_results.count; ++face_idx) {
//                     face_object_t *current_display_face = &face_results.faces[face_idx];
//                     if (current_display_face->face_landmarks_valid) {
//                             // Draw bounding box (Green for consistency)
//                             draw_rectangle(&src_image, current_display_face->box.left, current_display_face->box.top,
//                                             current_display_face->box.right - current_display_face->box.left,
//                                             current_display_face->box.bottom - current_display_face->box.top,
//                                             COLOR_GREEN, 2);

//                         // Draw Face Landmarks (468 points) - Yellow
//                         for (int j = 0; j < NUM_FACE_LANDMARKS; ++j) {
//                             draw_circle(&src_image,
//                                         current_display_face->face_landmarks[j].x,
//                                         current_display_face->face_landmarks[j].y,
//                                         1,              // radius
//                                         COLOR_YELLOW,   // color
//                                         1);             // thickness
//                         }

//                         // Optionally draw Eye Contour and Iris Landmarks
//                         if (current_display_face->eye_landmarks_left_valid) {
//                             for (int j = 0; j < NUM_EYE_CONTOUR_LANDMARKS; ++j) {
//                                 draw_circle(&src_image, current_display_face->eye_landmarks_left[j].x, current_display_face->eye_landmarks_left[j].y, 1, COLOR_BLUE, 1);
//                             }
//                         }
//                         if (current_display_face->iris_landmarks_left_valid) {
//                             for (int j = 0; j < NUM_IRIS_LANDMARKS; ++j) {
//                                 draw_circle(&src_image, current_display_face->iris_landmarks_left[j].x, current_display_face->iris_landmarks_left[j].y, 1, COLOR_ORANGE, 1);
//                             }
//                         }
//                         if (current_display_face->eye_landmarks_right_valid) {
//                             for (int j = 0; j < NUM_EYE_CONTOUR_LANDMARKS; ++j) {
//                                 draw_circle(&src_image, current_display_face->eye_landmarks_right[j].x, current_display_face->eye_landmarks_right[j].y, 1, COLOR_BLUE, 1);
//                             }
//                         }
//                         if (current_display_face->iris_landmarks_right_valid) {
//                             for (int j = 0; j < NUM_IRIS_LANDMARKS; ++j) {
//                                 draw_circle(&src_image, current_display_face->iris_landmarks_right[j].x, current_display_face->iris_landmarks_right[j].y, 1, COLOR_ORANGE, 1);
//                             }
//                         }
//                     }
//                 }
//                 // +++++++++++++++ END INSERTED CODE +++++++++++++++


//                 // --- Set KSS inputs ---
//                 kssCalculator.setPerclos(blinkDetector.getPerclos());
//                 int headPoseKSSValue = 1;
//                 if (headPoseResults.rows.size() >= 4) { for (const auto& row : headPoseResults.rows) { if (row.size() >= 2 && row[0] == "Head KSS") { try { headPoseKSSValue = std::stoi(row[1]); } catch (...) { headPoseKSSValue = 1; } break; } } }
//                 kssCalculator.setHeadPose(headPoseKSSValue);
//                 // *** FIXED: Use correct variable name ***
//                 kssCalculator.setYawnMetrics(yawnMetrics.isYawning, yawnMetrics.yawnFrequency_5min, yawnMetrics.yawnDuration);
//                 kssCalculator.setBlinksLastMinute(blinkDetector.getBlinksInWindow());
//                 if (yolo_results.count > 0) { for (int j = 0; j < yolo_results.count; ++j) { if (yolo_results.results[j].prop > 0.4) detectedObjects.push_back(coco_cls_to_name(yolo_results.results[j].cls_id)); } }
//                 double now_seconds = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
//                 kssCalculator.setDetectedObjects(detectedObjects, now_seconds);

//                 // --- Calculate KSS ---
//                 auto kssBreakdownResults = kssCalculator.calculateCompositeKSS();
//                 if (kssBreakdownResults.size() > 5 && kssBreakdownResults[5].size() == 2) { try { perclosKSS = std::stoi(kssBreakdownResults[0][1]); blinkKSS = std::stoi(kssBreakdownResults[1][1]); headposeKSS = std::stoi(kssBreakdownResults[2][1]); yawnKSS = std::stoi(kssBreakdownResults[3][1]); objdectdetectionKSS = std::stoi(kssBreakdownResults[4][1]); extractedTotalKSS = std::stoi(kssBreakdownResults[5][1]); } catch (...) { extractedTotalKSS = 1; } } else { extractedTotalKSS = 1; }
//                 kssStatus = kssCalculator.getKSSAlertStatus(extractedTotalKSS);
                

//                 // EYE TRACKING 

//                 const int LEFT_EYE_TEXT_ANCHOR_IDX = 33;  // Left outer corner
//                 const int RIGHT_EYE_TEXT_ANCHOR_IDX = 263; // Right outer corner

//                 if (face->face_landmarks_valid) { // Check if landmarks are valid before accessing
//                     // Left Eye Status
//                     if (LEFT_EYE_TEXT_ANCHOR_IDX < NUM_FACE_LANDMARKS) {
//                         point_t left_anchor = face->face_landmarks[LEFT_EYE_TEXT_ANCHOR_IDX];
//                         std::string left_status = blinkDetector.isLeftEyeClosed() ? "CLOSED" : "OPEN";
//                         unsigned int left_color = blinkDetector.isLeftEyeClosed() ? COLOR_RED : COLOR_GREEN;
//                         // Position text above the anchor point
//                         draw_text(&src_image, left_status.c_str(), left_anchor.x - 30, left_anchor.y - 25, left_color, 14);
//                         // Optionally draw the EAR value too
//                         text_stream.str(""); text_stream << "L:" << std::fixed << std::setprecision(2) << blinkDetector.getLeftEARValue();
//                         draw_text(&src_image, text_stream.str().c_str(), left_anchor.x - 30, left_anchor.y - 10, COLOR_WHITE, 10);
//                     }

//                     // Right Eye Status
//                     if (RIGHT_EYE_TEXT_ANCHOR_IDX < NUM_FACE_LANDMARKS) {
//                         point_t right_anchor = face->face_landmarks[RIGHT_EYE_TEXT_ANCHOR_IDX];
//                         std::string right_status = blinkDetector.isRightEyeClosed() ? "CLOSED" : "OPEN";
//                         unsigned int right_color = blinkDetector.isRightEyeClosed() ? COLOR_RED : COLOR_GREEN;
//                         // Position text above the anchor point
//                         draw_text(&src_image, right_status.c_str(), right_anchor.x - 30, right_anchor.y - 25, right_color, 14);
//                          // Optionally draw the EAR value too
//                         text_stream.str(""); text_stream << "R:" << std::fixed << std::setprecision(2) << blinkDetector.getRightEARValue();
//                         draw_text(&src_image, text_stream.str().c_str(), right_anchor.x - 30, right_anchor.y - 10, COLOR_WHITE, 10);
//                     }
//                 }

//                 // END HERE
                

//             }
//              else { // No face detected after calibration
//                 kssStatus = "Face Not Detected"; extractedTotalKSS = 1; yawnMetrics = {}; headPoseResults = {};
//             }

//             // --- Drawing (Status Text Only) ---
//             if (kssStatus.empty()) status_color_uint = COLOR_GREEN; else if (extractedTotalKSS <= 3) status_color_uint = COLOR_GREEN; else if (extractedTotalKSS <= 7) status_color_uint = COLOR_BLUE; else status_color_uint = COLOR_RED;
//             text_y = 10; if (!kssStatus.empty()) { draw_text(&src_image, kssStatus.c_str(), 10, text_y, status_color_uint, status_text_size); text_y += (int)(line_height * 1.4); }
//             text_stream.str(""); text_stream << "PERCLOS: " << std::fixed << std::setprecision(2) << blinkDetector.getPerclos() << "%"; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;
//             text_stream.str(""); text_stream << "Blinks (Last Min): " << blinkDetector.getBlinksInWindow(); draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;
//             if (headPoseResults.rows.size() >= 3) { std::string headpose_text = "Yaw:" + headPoseResults.rows[0][1] + " Pitch:" + headPoseResults.rows[1][1] + " Roll:" + headPoseResults.rows[2][1]; draw_text(&src_image, headpose_text.c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height; } else { draw_text(&src_image, "Head Pose: N/A", 10, text_y, COLOR_WHITE, text_size); text_y += line_height; }
//             text_stream.str(""); text_stream << "Yawning: " << (yawnMetrics.isYawning ? "Yes" : "No"); draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;
//             // *** FIXED: Use correct variable name ***
//             text_stream.str(""); text_stream << "Yawn Freq: " << static_cast<int>(yawnMetrics.yawnFrequency_5min); draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;
//             std::string detected_objects_text = ""; if (!detectedObjects.empty()) { detected_objects_text = "Detected: "; for (size_t j = 0; j < detectedObjects.size(); ++j) { detected_objects_text += detectedObjects[j]; if (j < detectedObjects.size() - 1) detected_objects_text += ", "; } draw_text(&src_image, detected_objects_text.c_str(), 10, text_y, COLOR_ORANGE, text_size); text_y += line_height; }
//             // Draw KSS Breakdown
//             text_stream.str(""); text_stream << "Perclos KSS: " << perclosKSS; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_GREEN, text_size); text_y += line_height;
//             // text_stream.str(""); text_stream << "Blink Count KSS: " << blinkKSS; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_GREEN, text_size); text_y += line_height;
//             text_stream.str(""); text_stream << "Head-pose KSS: " << headposeKSS; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_GREEN, text_size); text_y += line_height;
//             text_stream.str(""); text_stream << "Yawn KSS: " << yawnKSS; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_GREEN, text_size); text_y += line_height;
//             text_stream.str(""); text_stream << "Object detection KSS: " << objdectdetectionKSS; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_GREEN, text_size); text_y += line_height;
//             text_stream.str(""); text_stream << "Composite KSS: " << extractedTotalKSS; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, status_color_uint, text_size); text_y += line_height;

//         } // End if (!calibration_done) / else

//         // --- Draw Resource Monitoring Info (Always Draw) ---
//         auto frame_processing_end_time = std::chrono::high_resolution_clock::now(); double frame_duration_ms = std::chrono::duration<double, std::milli>(frame_processing_end_time - frame_start_time).count(); calculateOverallFPS(frame_duration_ms, frame_times, overallFPS, max_time_records); getCPUUsage(currentCpuUsage, prevIdleTime, prevTotalTime); getTemperature(currentTemp); std::stringstream info_ss; info_ss << "FPS:" << std::fixed << std::setprecision(1) << overallFPS << "|CPU:" << currentCpuUsage << "%|T:" << currentTemp << "C"; draw_text(&src_image, info_ss.str().c_str(), 10, src_image.height - 20, COLOR_WHITE, 10);

//         // --- Display Frame (Always) ---
//         cv::Mat display_frame(src_image.height, src_image.width, CV_8UC3, map_info.data, src_image.width_stride); cv::imshow("DMS Output", display_frame);

//         // --- Push Frame to Saving Pipeline (Always) ---
//         if (pipeline_ && appsrc_) { GstClockTime duration = gst_util_uint64_scale_int(1, GST_SECOND, video_info.fps_n > 0 ? video_info.fps_d * video_info.fps_n : 30); pushFrameToPipeline(map_info.data, map_info.size, src_image.width, src_image.height, duration); }

//         // --- Cleanup Frame Resources (Always) ---
//         gst_buffer_unmap(gst_buffer, &map_info); gst_sample_unref(sample);

//         // --- Handle KeyPress (Always) ---
//         if (cv::waitKey(1) == 27) break; // Exit on ESC

//     } // End main loop

//     // --- Final Cleanup 
//     printf("INFO: Cleaning up resources...\n"); cv::destroyAllWindows(); stop_yolo_worker.store(true); if (yolo_worker_thread.joinable()) yolo_worker_thread.join(); printf("INFO: YOLO thread joined.\n"); if (input_pipeline) { gst_element_set_state(input_pipeline, GST_STATE_NULL); gst_object_unref(appsink_); gst_object_unref(input_pipeline); printf("INFO: Input pipeline released.\n"); } if (pipeline_) { gst_element_send_event(pipeline_, gst_event_new_eos()); GstBus* bus = gst_element_get_bus(pipeline_); gst_bus_poll(bus, GST_MESSAGE_EOS, GST_CLOCK_TIME_NONE); gst_object_unref(bus); gst_element_set_state(pipeline_, GST_STATE_NULL); gst_object_unref(appsrc_); gst_object_unref(pipeline_); printf("INFO: Saving pipeline released.\n"); } gst_deinit(); printf("INFO: GStreamer deinitialized.\n"); release_face_analyzer(&app_ctx.face_ctx); release_yolo11(&app_ctx.yolo_ctx); deinit_post_process(); printf("INFO: RKNN models released.\n");

//     printf("Exiting main (ret = %d)\n", ret);
//     return ret;
// }






//  sent full frame but track left side driver also

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
// #include <opencv2/core/types.hpp> // Needed for cv::Rect, cv::Point
// #include <opencv2/opencv.hpp>
// #include <thread>
// #include <queue>
// #include <mutex>
// #include <atomic>
// #include <memory>
// #include <sys/sysinfo.h>
// #include <sys/stat.h>
// #include <unistd.h>
// #include <dirent.h>

// // Include project headers
// #include "face_analyzer/face_analyzer.h"
// #include "yolo_detector/yolo11.h"
// #include "behavior_analysis/BlinkDetector.hpp"
// #include "behavior_analysis/YawnDetector.hpp"
// #include "behavior_analysis/HeadPoseTracker.hpp"
// #include "behavior_analysis/KSSCalculator.hpp"
// #include "image_utils.h"
// #include "file_utils.h"
// #include "image_drawing.h"

// // GStreamer Includes
// #include <gst/gst.h>
// #include <gst/video/video.h>
// #include <gst/app/gstappsink.h>
// #include <gst/allocators/gstdmabuf.h>

// // Resource Monitoring Includes
// #include <fstream>
// #include <numeric>
// #include <deque>

// // --- Debugging Flags ---
// #define DEBUG_DRAW_ROIS // Comment out to disable drawing ROIs

// // --- Define colors ---
// #ifndef COLOR_MAGENTA
// #define COLOR_MAGENTA (0xFF00FF) // Pinkish/Magenta
// #endif
// #ifndef COLOR_YELLOW
// #define COLOR_YELLOW (0x00FFFF) // Yellow
// #endif
// #ifndef COLOR_WHITE
// #define COLOR_WHITE (0xFFFFFF) // White
// #endif
// #ifndef COLOR_GREEN
// #define COLOR_GREEN (0x00FF00) // Green
// #endif
// #ifndef COLOR_RED
// #define COLOR_RED (0x0000FF) // Red
// #endif
// #ifndef COLOR_BLUE
// #define COLOR_BLUE (0xFF0000) // Blue
// #endif
// #ifndef COLOR_ORANGE
// #define COLOR_ORANGE (0x00A5FF) // Orange
// #endif
// #ifndef COLOR_CYAN
// #define COLOR_CYAN (0xFFFF00) // Cyan
// #endif


// // // --- Application Context ---
// typedef struct app_context_t {
//     face_analyzer_app_context_t face_ctx;
//     yolo11_app_context_t yolo_ctx;
// } app_context_t;

// // --- Global GStreamer Elements ---
// static GstElement* pipeline_ = nullptr; // Saving pipeline
// static GstElement* appsrc_ = nullptr;   // Source for saving pipeline
// static GstElement* appsink_ = nullptr;  // Sink for input pipeline

// // --- Helper Functions ---
// cv::Point calculate_centroid(const point_t landmarks[], int count) {
//     if (count == 0) {
//         return cv::Point(-1, -1);
//     }
//     long long sum_x = 0, sum_y = 0;
//     int valid_points = 0;
//     for (int i = 0; i < count; ++i) {
//         if (landmarks[i].x > -10000 && landmarks[i].x < 10000 &&
//             landmarks[i].y > -10000 && landmarks[i].y < 10000) {
//             sum_x += landmarks[i].x;
//             sum_y += landmarks[i].y;
//             valid_points++;
//         }
//     }
//     if (valid_points == 0) {
//         return cv::Point(-1, -1);
//     }
//     return cv::Point(static_cast<int>(sum_x / valid_points),
//                     static_cast<int>(sum_y / valid_points));
// }

// std::vector<cv::Point> convert_landmarks_to_cvpoint(const point_t landmarks[], int count) {
//     std::vector<cv::Point> cv_landmarks;
//     if (count <= 0) {
//         return cv_landmarks;
//     }
//     cv_landmarks.reserve(count);
//     for (int i = 0; i < count; ++i) {
//         if (landmarks[i].x > -10000 && landmarks[i].x < 10000 &&
//             landmarks[i].y > -10000 && landmarks[i].y < 10000) {
//             cv_landmarks.emplace_back(cv::Point(landmarks[i].x, landmarks[i].y));
//         }
//     }
//     return cv_landmarks;
// }

// float calculate_ear_simple(const std::vector<cv::Point>& landmarks,
//                           const std::vector<int>& eye_points) {
//     if (landmarks.empty()) {
//         return 1.0f;
//     }
//     int max_idx = 0;
//     for (int idx : eye_points) {
//         if (idx > max_idx) {
//             max_idx = idx;
//         }
//     }
//     if (max_idx >= landmarks.size()) {
//         return 1.0f;
//     }
//     try {
//         cv::Point p1 = landmarks.at(eye_points.at(0));
//         cv::Point p2 = landmarks.at(eye_points.at(1));
//         cv::Point p3 = landmarks.at(eye_points.at(2));
//         cv::Point p4 = landmarks.at(eye_points.at(3));
//         cv::Point p5 = landmarks.at(eye_points.at(4));
//         cv::Point p6 = landmarks.at(eye_points.at(5));
//         double v1 = cv::norm(p2 - p6);
//         double v2 = cv::norm(p3 - p5);
//         double h = cv::norm(p1 - p4);
//         if (h < 1e-6) {
//             return 1.0f;
//         }
//         return static_cast<float>((v1 + v2) / (2.0 * h));
//     } catch (const std::out_of_range& oor) {
//         std::cerr << "Out of Range error in calculate_ear_simple: " << oor.what() << std::endl;
//         return 1.0f;
//     } catch (...) {
//         std::cerr << "Unknown exception in calculate_ear_simple" << std::endl;
//         return 1.0f;
//     }
// }

// double calculate_mouth_dist_simple(const std::vector<cv::Point>& landmarks) {
//     if (landmarks.empty() || landmarks.size() <= 14) {
//         return 0.0;
//     }
//     try {
//         return cv::norm(landmarks.at(13) - landmarks.at(14));
//     } catch (const std::out_of_range& oor) {
//         std::cerr << "Out of Range error in calculate_mouth_dist_simple: " << oor.what() << std::endl;
//         return 0.0;
//     } catch (...) {
//         std::cerr << "Unknown exception in calculate_mouth_dist_simple" << std::endl;
//         return 0.0;
//     }
// }

// double parse_head_pose_value(const std::string& s) {
//     try {
//         std::string n = s;
//         size_t d = n.find(" deg");
//         if (d != std::string::npos) {
//             n = n.substr(0, d);
//         }
//         size_t f = n.find_first_not_of(" \t");
//         if (f == std::string::npos) {
//             return 0.0;
//         }
//         size_t l = n.find_last_not_of(" \t");
//         n = n.substr(f, l - f + 1);
//         if (n.empty()) {
//             return 0.0;
//         }
//         return std::stod(n);
//     } catch (const std::exception& e) {
//         printf("WARN: Ex parse head pose '%s': %s\n", s.c_str(), e.what());
//         return 0.0;
//     } catch (...) {
//         printf("WARN: Unk ex parse head pose '%s'.\n", s.c_str());
//         return 0.0;
//     }
// }

// template <typename T>
// T calculate_stddev(const std::deque<T>& data) {
//     if (data.size() < 2) {
//         return T(0);
//     }
//     T sum = std::accumulate(data.begin(), data.end(), T(0));
//     T mean = sum / data.size();
//     T sq_sum = std::inner_product(data.begin(), data.end(), data.begin(), T(0));
//     T variance = sq_sum / data.size() - mean * mean;
//     return std::sqrt(std::max(T(0), variance));
// }

// // // --- YOLO Worker Thread ---
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
//             int ret = inference_yolo11(yolo_ctx_ptr, input_data.image.get(), &output_data.results);
//             if (ret != 0) {
//                 printf("WARN: YOLO Worker inference failed (frame %ld), ret=%d\n",
//                        input_data.frame_id, ret);
//             }
//             {
//                 std::unique_lock<std::mutex> lock(yolo_output_mutex);
//                 if (yolo_output_queue.size() >= MAX_QUEUE_SIZE) {
//                     yolo_output_queue.pop();
//                 }
//                 yolo_output_queue.push(output_data);
//             }
//             if (input_data.image->virt_addr) {
//                 free(input_data.image->virt_addr);
//                 input_data.image->virt_addr = nullptr;
//             }
//         } else {
//             std::this_thread::sleep_for(std::chrono::milliseconds(5));
//         }
//     }
//     printf("YOLO Worker Thread Exiting.\n");
// }

// // --- GStreamer Saving Pipeline Setup ---
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
//         "appsrc name=source ! queue ! videoconvert ! video/x-raw,format=NV12 ! "
//         "mpph265enc rc-mode=cbr bps=4000000 gop=30 qp-min=10 qp-max=51 ! "
//         "h265parse ! matroskamux ! filesink location=" + filepath;
//     std::cout << "Saving Pipeline: " << pipeline_str << std::endl;
//     GError* error = nullptr;
//     pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
//     if (!pipeline_ || error) {
//         std::cerr << "Failed to create saving pipeline: "
//                   << (error ? error->message : "Unknown error") << std::endl;
//         if (error) {
//             g_error_free(error);
//         }
//         return;
//     }
//     appsrc_ = gst_bin_get_by_name(GST_BIN(pipeline_), "source");
//     if (!appsrc_) {
//         std::cerr << "Failed to get appsrc" << std::endl;
//         gst_object_unref(pipeline_);
//         pipeline_ = nullptr;
//         return;
//     }
//     GstCaps* caps = gst_caps_new_simple(
//         "video/x-raw",
//         "format", G_TYPE_STRING, "BGR",
//         "width", G_TYPE_INT, 1920,
//         "height", G_TYPE_INT, 1080,
//         "framerate", GST_TYPE_FRACTION, 60, 1,
//         nullptr);
//     g_object_set(G_OBJECT(appsrc_), "caps", caps, "format", GST_FORMAT_TIME, nullptr);
//     gst_caps_unref(caps);
//     if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
//         std::cerr << "Failed to set saving pipeline to playing" << std::endl;
//         gst_object_unref(appsrc_);
//         gst_object_unref(pipeline_);
//         pipeline_ = nullptr;
//         appsrc_ = nullptr;
//     }
// }

// void pushFrameToPipeline(unsigned char* data, int size, int width, int height,
//                         GstClockTime duration) {
//     if (!appsrc_) {
//         return;
//     }
//     GstBuffer* buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
//     GstMapInfo map;
//     if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
//         std::cerr << "Failed map buffer" << std::endl;
//         gst_buffer_unref(buffer);
//         return;
//     }
//     if (map.size != (guint)size) {
//         std::cerr << "Buffer size mismatch: " << map.size << " vs " << size << std::endl;
//         gst_buffer_unmap(buffer, &map);
//         gst_buffer_unref(buffer);
//         return;
//     }
//     memcpy(map.data, data, size);
//     gst_buffer_unmap(buffer, &map);
//     static GstClockTime timestamp = 0;
//     GST_BUFFER_PTS(buffer) = timestamp;
//     GST_BUFFER_DURATION(buffer) = duration;
//     timestamp += GST_BUFFER_DURATION(buffer);
//     GstFlowReturn ret;
//     g_signal_emit_by_name(appsrc_, "push-buffer", buffer, &ret);
//     if (ret != GST_FLOW_OK) {
//         std::cerr << "Failed push buffer, ret=" << ret << std::endl;
//     }
//     gst_buffer_unref(buffer);
// }

// // --- Resource Monitoring Functions ---
// void calculateOverallFPS(double frame_duration_ms, std::deque<double>& times_deque,
//                         double& fps_variable, int max_records) {
//     times_deque.push_back(frame_duration_ms);
//     if (times_deque.size() > max_records) {
//         times_deque.pop_front();
//     }
//     if (!times_deque.empty()) {
//         double sum = std::accumulate(times_deque.begin(), times_deque.end(), 0.0);
//         double avg_time_ms = sum / times_deque.size();
//         fps_variable = (avg_time_ms > 0) ? (1000.0 / avg_time_ms) : 0.0;
//     } else {
//         fps_variable = 0.0;
//     }
// }

// void getCPUUsage(double& cpu_usage_variable, long& prev_idle, long& prev_total) {
//     std::ifstream file("/proc/stat");
//     if (!file.is_open()) {
//         return;
//     }
//     std::string line;
//     std::getline(file, line);
//     file.close();
//     long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
//     user = nice = system = idle = iowait = irq = softirq = steal = guest = guest_nice = 0;
//     std::istringstream iss(line);
//     std::string cpu_label;
//     iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;
//     long currentIdleTime = idle + iowait;
//     long currentTotalTime = user + nice + system + idle + iowait + irq + softirq + steal;
//     long diffIdle = currentIdleTime - prev_idle;
//     long diffTotal = currentTotalTime - prev_total;
//     if (diffTotal > 0) {
//         cpu_usage_variable = 100.0 * (double)(diffTotal - diffIdle) / diffTotal;
//     }
//     prev_idle = currentIdleTime;
//     prev_total = currentTotalTime;
// }

// void getTemperature(double& temp_variable) {
//     const char* temp_paths[] = {
//         "/sys/class/thermal/thermal_zone0/temp",
//         "/sys/class/thermal/thermal_zone1/temp"
//     };
//     bool temp_read = false;
//     for (const char* path : temp_paths) {
//         std::ifstream file(path);
//         double temp_milliC = 0;
//         if (file >> temp_milliC) {
//             temp_variable = temp_milliC / 1000.0;
//             temp_read = true;
//             file.close();
//             break;
//         }
//         file.close();
//     }
// }


// /*-------------------------------------------
//                   Main Function
// -------------------------------------------*/
// int main(int argc, char **argv) {
//     int ret = 0;
//     long current_frame_id = 0;

//     // --- Resource Monitoring Vars ---
//     std::deque<double> frame_times;
//     const int max_time_records = 60;
//     double overallFPS = 0.0;
//     double currentCpuUsage = 0.0;
//     long prevIdleTime = 0, prevTotalTime = 0;
//     double currentTemp = 0.0;

//     // --- Model Paths ---
//     const char *detection_model_path = "../../model/rf.rknn";
//     const char *landmark_model_path = "../../model/faceL.rknn";
//     const char *iris_model_path = "../../model/faceI.rknn";
//     const char *yolo_model_path = "../../model/od.rknn";

//     // --- GStreamer Input Pipeline String ---
//     const char *video_source =
//         "filesrc location=../../model/1.mkv ! decodebin ! queue ! videoconvert ! "
//         "video/x-raw,format=BGR ! appsink name=sink sync=false";

//     // --- Initialization ---
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

//     // --- Calibration State Variables ---
//     bool calibration_done = false;
//     std::chrono::steady_clock::time_point calibration_start_time;
//     bool calibration_timer_started = false;
//     int consecutive_valid_eyes_frames = 0;
//     const int REQUIRED_VALID_EYES_FRAMES = 60;
//     bool ear_calibrated = false;
//     bool mouth_calibrated = false;
//     std::deque<float> calib_left_ears;
//     std::deque<float> calib_right_ears;
//     std::deque<double> calib_mouth_dists;
//     int consecutive_stable_ear_frames = 0;
//     int consecutive_stable_mouth_frames = 0;
//     const int CALIB_WINDOW_SIZE = 30;
//     const float EAR_STDDEV_THRESHOLD = 0.04;
//     const double MOUTH_DIST_STDDEV_THRESHOLD = 15.0;
//     const int REQUIRED_STABLE_FRAMES = CALIB_WINDOW_SIZE + 5;
//     const double CALIBRATION_TIMEOUT_SECONDS = 10.0;

//     // --- Driver ID & Tracking State ---
//     cv::Rect driver_identification_zone;
//     bool driver_identified_ever = false;
//     bool driver_tracked_this_frame = false;
//     int current_tracked_driver_idx = -1;
//     cv::Point prev_driver_centroid = cv::Point(-1, -1);
//     const double MAX_CENTROID_DISTANCE = 150.0;
//     int driver_search_timeout_frames = 0;
//     const int DRIVER_SEARCH_MAX_FRAMES = 60;

//     // --- Object Detection ROI Variables ---
//     cv::Rect driver_object_roi;
//     bool valid_object_roi = false;

//     // --- State Variables for KSS and Display ---
//     std::string kssStatus = "Initializing";
//     YawnDetector::YawnMetrics yawnMetrics = {};
//     my::HeadPoseTracker::HeadPoseResults headPoseResults = {};
//     std::vector<std::string> detectedObjects;
//     int extractedTotalKSS = 1;
//     int perclosKSS = 1, blinkKSS = 1, headposeKSS = 1, yawnKSS = 1, objdectdetectionKSS = 1;

//     // --- Drawing Helpers ---
//     std::stringstream text_stream;
//     int text_y = 0;
//     const int line_height = 22;
//     const int text_size = 12;
//     const int status_text_size = 16;
//     unsigned int status_color_uint = COLOR_GREEN;

//     // --- YOLO Thread Handle ---
//     std::thread yolo_worker_thread;

//     // --- Init Models ---
//     ret = init_post_process();
//     if (ret != 0) {
//         printf("Error init YOLO postprocess.\n");
//         return -1;
//     }
//     ret = init_face_analyzer(detection_model_path, landmark_model_path,
//                             iris_model_path, &app_ctx.face_ctx);
//     if (ret != 0) {
//         printf("init_face_analyzer fail! ret=%d\n", ret);
//         deinit_post_process();
//         return -1;
//     }
//     ret = init_yolo11(yolo_model_path, &app_ctx.yolo_ctx);
//     if (ret != 0) {
//         printf("init_yolo11 fail! ret=%d\n", ret);
//         release_face_analyzer(&app_ctx.face_ctx);
//         deinit_post_process();
//         return -1;
//     }
//     yolo_worker_thread = std::thread(yolo_worker_thread_func, &app_ctx.yolo_ctx);

//     // --- Init GStreamer Input ---
//     gst_init(nullptr, nullptr);
//     GError* error = nullptr;
//     GstElement* input_pipeline = gst_parse_launch(video_source, &error);
//     if (!input_pipeline || error) {
//         std::cerr << "Failed to create input pipeline: "
//                   << (error ? error->message : "Unknown error") << std::endl;
//         if (error) {
//             g_error_free(error);
//         }
//         return -1;
//     }
//     appsink_ = gst_bin_get_by_name(GST_BIN(input_pipeline), "sink");
//     if (!appsink_) {
//         std::cerr << "Failed to get appsink" << std::endl;
//         gst_object_unref(input_pipeline);
//         return -1;
//     }
//     if (gst_element_set_state(input_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
//         std::cerr << "Failed to set input pipeline to playing" << std::endl;
//         gst_object_unref(appsink_);
//         gst_object_unref(input_pipeline);
//         return -1;
//     }

//     // --- Setup OpenCV Window ---
//     cv::namedWindow("DMS Output", cv::WINDOW_NORMAL);
//     cv::resizeWindow("DMS Output", 1920, 1080);

//     GstSample* sample;
//     GstVideoInfo video_info;
//     GstBuffer* gst_buffer;
//     GstMapInfo map_info;
//     bool first_frame = true;

//     // --- Main Processing Loop ---
//     while (true) {
//         // --- Get Frame ---
//         sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink_));
//         if (!sample) {
//             std::cerr << "End of stream or error pulling sample." << std::endl;
//             break;
//         }
//         gst_buffer = gst_sample_get_buffer(sample);
//         GstCaps* caps = gst_sample_get_caps(sample);
//         if (!gst_buffer || !caps || !gst_video_info_from_caps(&video_info, caps) ||
//             !gst_buffer_map(gst_buffer, &map_info, GST_MAP_READ)) {
//             std::cerr << "Failed to get valid buffer/caps/map" << std::endl;
//             if (gst_buffer && map_info.memory) {
//                 gst_buffer_unmap(gst_buffer, &map_info);
//             }
//             if (sample) {
//                 gst_sample_unref(sample);
//             }
//             continue;
//         }

//         auto frame_start_time = std::chrono::high_resolution_clock::now();
//         current_frame_id++;

//         // --- Wrap GStreamer Data ---
//         src_image.width = GST_VIDEO_INFO_WIDTH(&video_info);
//         src_image.height = GST_VIDEO_INFO_HEIGHT(&video_info);
//         src_image.width_stride = GST_VIDEO_INFO_PLANE_STRIDE(&video_info, 0);
//         src_image.height_stride = src_image.height;
// #ifdef IMAGE_FORMAT_BGR888
//         src_image.format = IMAGE_FORMAT_BGR888;
// #else
//         src_image.format = IMAGE_FORMAT_RGB888;
// #endif
//         src_image.virt_addr = map_info.data;
//         src_image.size = map_info.size;
//         src_image.fd = -1;

//         // --- Define Driver Identification Zone (on first frame - REVISED FOR RHD & Size) ---
//         if (first_frame) {
//             int frame_width = src_image.width;
//             int frame_height = src_image.height;
//             // *** RHD Correction & Size Adjustment ***
//             int roi_start_x = (int)(frame_width * 0.40);
//             int roi_width = (int)(frame_width * 0.55);
//             driver_identification_zone = cv::Rect(
//                 roi_start_x,
//                 (int)(frame_height * 0.1),
//                 roi_width,
//                 (int)(frame_height * 0.8)
//             );
//             driver_identification_zone &= cv::Rect(0, 0, frame_width, frame_height);
//             printf("INFO: Driver Identification Zone (RHD Revised) set to [%d, %d, %d x %d]\n",
//                    driver_identification_zone.x, driver_identification_zone.y,
//                    driver_identification_zone.width, driver_identification_zone.height);
//             first_frame = false;
//         }

//         // --- Run Face Analysis ---
//         ret = inference_face_analyzer(&app_ctx.face_ctx, &src_image, &face_results);
//         if (ret != 0) {
//             printf("WARN: Face Analyzer Inference failed frame %ld, ret=%d\n",
//                    current_frame_id, ret);
//             face_results.count = 0;
//         }

//         // --- Reset Frame-Specific Flags ---
//         driver_tracked_this_frame = false;
//         current_tracked_driver_idx = -1;
//         valid_object_roi = false;

//         // --- Driver Identification and Tracking Logic ---
//         if (!driver_identified_ever) {
//             // Phase 1: Identification
//             int best_candidate_idx = -1;
//             float max_area = 0.0f;
//             for (int i = 0; i < face_results.count; ++i) {
//                 cv::Point face_center(
//                     (face_results.faces[i].box.left + face_results.faces[i].box.right) / 2,
//                     (face_results.faces[i].box.top + face_results.faces[i].box.bottom) / 2
//                 );
//                 if (driver_identification_zone.contains(face_center) &&
//                     face_results.faces[i].face_landmarks_valid) {
//                     float area = (float)(face_results.faces[i].box.right - face_results.faces[i].box.left) *
//                                  (face_results.faces[i].box.bottom - face_results.faces[i].box.top);
//                     if (area > max_area) {
//                         max_area = area;
//                         best_candidate_idx = i;
//                     }
//                 }
//             }
//             if (best_candidate_idx != -1) {
//                 driver_identified_ever = true;
//                 driver_tracked_this_frame = true;
//                 current_tracked_driver_idx = best_candidate_idx;
//                 prev_driver_centroid = calculate_centroid(
//                     face_results.faces[best_candidate_idx].face_landmarks, NUM_FACE_LANDMARKS);
//                 driver_search_timeout_frames = 0;
//                 printf("INFO: Driver Identified (Index %d) at frame %ld\n",
//                        current_tracked_driver_idx, current_frame_id);
//             } else {
//                 kssStatus = "Searching Driver...";
//             }
//         } else {
//             // Phase 2: Tracking
//             int best_match_idx = -1;
//             double min_dist = MAX_CENTROID_DISTANCE;
//             for (int i = 0; i < face_results.count; ++i) {
//                 if (face_results.faces[i].face_landmarks_valid) {
//                     cv::Point current_centroid = calculate_centroid(
//                         face_results.faces[i].face_landmarks, NUM_FACE_LANDMARKS);
//                     if (prev_driver_centroid.x >= 0 && current_centroid.x >= 0) {
//                         double dist = cv::norm(current_centroid - prev_driver_centroid);
//                         cv::Point face_center(
//                             (face_results.faces[i].box.left + face_results.faces[i].box.right) / 2,
//                             (face_results.faces[i].box.top + face_results.faces[i].box.bottom) / 2
//                         );
//                         if (dist < min_dist && driver_identification_zone.contains(face_center)) {
//                             min_dist = dist;
//                             best_match_idx = i;
//                         }
//                     }
//                 }
//             }
//             if (best_match_idx != -1) {
//                 driver_tracked_this_frame = true;
//                 current_tracked_driver_idx = best_match_idx;
//                 prev_driver_centroid = calculate_centroid(
//                     face_results.faces[best_match_idx].face_landmarks, NUM_FACE_LANDMARKS);
//                 driver_search_timeout_frames = 0;
//             } else {
//                 driver_tracked_this_frame = false;
//                 current_tracked_driver_idx = -1;
//                 prev_driver_centroid = cv::Point(-1, -1);
//                 driver_search_timeout_frames++;
//                 kssStatus = "Driver Lost...";
//                 if (driver_search_timeout_frames > DRIVER_SEARCH_MAX_FRAMES) {
//                     driver_identified_ever = false;
//                     driver_search_timeout_frames = 0;
//                     printf("INFO: Driver track lost for %d frames. Reverting to search.\n",
//                            DRIVER_SEARCH_MAX_FRAMES);
//                     kssStatus = "Searching Driver...";
//                 }
//             }
//         }

//         // --- Calibration Phase Logic ---
//         if (!calibration_done) {
//             bool head_pose_calibrated = false;
//             bool eyes_consistently_valid = false;
//             double elapsed_calib_seconds = 0.0;
//             std::string calib_status_detail = "";
//             if (driver_tracked_this_frame && current_tracked_driver_idx != -1) {
//                 face_object_t *calib_face = &face_results.faces[current_tracked_driver_idx];
//                 if (calib_face->face_landmarks_valid) {
//                     if (!calibration_timer_started) {
//                         calibration_start_time = std::chrono::steady_clock::now();
//                         calibration_timer_started = true;
//                         printf("INFO: Calibration timer started (Driver Index %d).\n",
//                                current_tracked_driver_idx);
//                         consecutive_valid_eyes_frames = 0;
//                         consecutive_stable_ear_frames = 0;
//                         consecutive_stable_mouth_frames = 0;
//                         calib_left_ears.clear();
//                         calib_right_ears.clear();
//                         calib_mouth_dists.clear();
//                         ear_calibrated = false;
//                         mouth_calibrated = false;
//                     }
//                     std::vector<cv::Point> calib_faceLandmarks = convert_landmarks_to_cvpoint(
//                         calib_face->face_landmarks, NUM_FACE_LANDMARKS);
//                     headPoseTracker.run(calib_faceLandmarks);
//                     head_pose_calibrated = headPoseTracker.isCalibrated();
//                     if (!head_pose_calibrated) {
//                         calib_status_detail += " (Head)";
//                     }
//                     bool eyes_valid_this_frame = calib_face->eye_landmarks_left_valid &&
//                                                calib_face->eye_landmarks_right_valid;
//                     if (eyes_valid_this_frame) {
//                         consecutive_valid_eyes_frames++;
//                     } else {
//                         consecutive_valid_eyes_frames = 0;
//                     }
//                     eyes_consistently_valid = (consecutive_valid_eyes_frames >= REQUIRED_VALID_EYES_FRAMES);
//                     if (!eyes_consistently_valid) {
//                         calib_status_detail += " (Eyes Valid)";
//                     }
//                     if (!ear_calibrated && eyes_valid_this_frame) {
//                         const std::vector<int> L = {33, 160, 158, 133, 153, 144};
//                         const std::vector<int> R = {362, 385, 387, 263, 380, 373};
//                         float lE = calculate_ear_simple(calib_faceLandmarks, L);
//                         float rE = calculate_ear_simple(calib_faceLandmarks, R);
//                         calib_left_ears.push_back(lE);
//                         calib_right_ears.push_back(rE);
//                         if (calib_left_ears.size() > CALIB_WINDOW_SIZE) {
//                             calib_left_ears.pop_front();
//                         }
//                         if (calib_right_ears.size() > CALIB_WINDOW_SIZE) {
//                             calib_right_ears.pop_front();
//                         }
//                         if (calib_left_ears.size() >= CALIB_WINDOW_SIZE) {
//                             float lS = calculate_stddev(calib_left_ears);
//                             float rS = calculate_stddev(calib_right_ears);
//                             if (lS < EAR_STDDEV_THRESHOLD && rS < EAR_STDDEV_THRESHOLD) {
//                                 consecutive_stable_ear_frames++;
//                             } else {
//                                 consecutive_stable_ear_frames = 0;
//                             }
//                         } else {
//                             consecutive_stable_ear_frames = 0;
//                         }
//                         ear_calibrated = (consecutive_stable_ear_frames >= REQUIRED_STABLE_FRAMES);
//                     } else if (!eyes_valid_this_frame) {
//                         consecutive_stable_ear_frames = 0;
//                         ear_calibrated = false;
//                         calib_left_ears.clear();
//                         calib_right_ears.clear();
//                     }
//                     if (!ear_calibrated) {
//                         calib_status_detail += " (EAR Stable)";
//                     }
//                     if (!mouth_calibrated) {
//                         double cM = calculate_mouth_dist_simple(calib_faceLandmarks);
//                         calib_mouth_dists.push_back(cM);
//                         if (calib_mouth_dists.size() > CALIB_WINDOW_SIZE) {
//                             calib_mouth_dists.pop_front();
//                         }
//                         if (calib_mouth_dists.size() >= CALIB_WINDOW_SIZE) {
//                             double mS = calculate_stddev(calib_mouth_dists);
//                             if (mS < MOUTH_DIST_STDDEV_THRESHOLD) {
//                                 consecutive_stable_mouth_frames++;
//                             } else {
//                                 consecutive_stable_mouth_frames = 0;
//                             }
//                         } else {
//                             consecutive_stable_mouth_frames = 0;
//                         }
//                         mouth_calibrated = (consecutive_stable_mouth_frames >= REQUIRED_STABLE_FRAMES);
//                     }
//                     if (!mouth_calibrated) {
//                         calib_status_detail += " (Mouth Stable)";
//                     }
//                     auto now = std::chrono::steady_clock::now();
//                     elapsed_calib_seconds = std::chrono::duration<double>(now - calibration_start_time).count();
//                     if (elapsed_calib_seconds >= CALIBRATION_TIMEOUT_SECONDS) {
//                         if (head_pose_calibrated && eyes_consistently_valid &&
//                             ear_calibrated && mouth_calibrated) {
//                             calibration_done = true;
//                             printf("INFO: Calibration Complete!\n");
//                         } else {
//                             calibration_timer_started = false;
//                             driver_identified_ever = false;
//                             driver_tracked_this_frame = false;
//                             current_tracked_driver_idx = -1;
//                             prev_driver_centroid = cv::Point(-1, -1);
//                             consecutive_valid_eyes_frames = 0;
//                             consecutive_stable_ear_frames = 0;
//                             consecutive_stable_mouth_frames = 0;
//                             ear_calibrated = false;
//                             mouth_calibrated = false;
//                             calib_left_ears.clear();
//                             calib_right_ears.clear();
//                             calib_mouth_dists.clear();
//                             printf("WARN: Calibration time expired, criteria not met "
//                                    "(H:%d, EV:%d, ES:%d, MS:%d). Retrying identification.\n",
//                                    head_pose_calibrated, eyes_consistently_valid,
//                                    ear_calibrated, mouth_calibrated);
//                         }
//                     }
//                     std::string calib_text = "Calibrating... " +
//                                            std::to_string(static_cast<int>(elapsed_calib_seconds)) +
//                                            "s" + calib_status_detail;
//                     draw_text(&src_image, calib_text.c_str(), 10, 30, COLOR_YELLOW, status_text_size);
//                     draw_rectangle(&src_image, calib_face->box.left, calib_face->box.top,
//                                   calib_face->box.right - calib_face->box.left,
//                                   calib_face->box.bottom - calib_face->box.top,
//                                   COLOR_YELLOW, 2);
//                 } else {
//                     calibration_timer_started = false;
//                     printf("WARN: Tracked driver landmarks not valid during calibration.\n");
//                 }
//                 std::string calib_text = "Calibrating... " +
//                                        std::to_string(static_cast<int>(elapsed_calib_seconds)) +
//                                        "s" + calib_status_detail;
//                 draw_text(&src_image, calib_text.c_str(), 10, 30, COLOR_YELLOW, status_text_size);
//             } else {
//                 calibration_timer_started = false;
//                 consecutive_valid_eyes_frames = 0;
//                 consecutive_stable_ear_frames = 0;
//                 consecutive_stable_mouth_frames = 0;
//                 ear_calibrated = false;
//                 mouth_calibrated = false;
//                 calib_left_ears.clear();
//                 calib_right_ears.clear();
//                 calib_mouth_dists.clear();
//                 if (driver_identified_ever) {
//                     draw_text(&src_image, "Calibration: Waiting for Driver Track...",
//                               10, 30, COLOR_YELLOW, status_text_size);
//                 } else {
//                     draw_text(&src_image, "Calibration: Searching Driver...",
//                               10, 30, COLOR_YELLOW, status_text_size);
//                 }
//                 for (int i = 0; i < face_results.count; ++i) {
//                     draw_rectangle(&src_image, face_results.faces[i].box.left,
//                                   face_results.faces[i].box.top,
//                                   face_results.faces[i].box.right - face_results.faces[i].box.left,
//                                   face_results.faces[i].box.bottom - face_results.faces[i].box.top,
//                                   COLOR_YELLOW, 1);
//                 }
//             }
//         } else {
//             // Normal Processing Phase (Calibration is Done)
//             // --- Queue Frame for YOLO ---
//             auto yolo_input_image = std::make_shared<image_buffer_t>();
//             yolo_input_image->size = src_image.size;
//             yolo_input_image->virt_addr = (unsigned char*)malloc(yolo_input_image->size);
//             if (yolo_input_image->virt_addr) {
//                 memcpy(yolo_input_image->virt_addr, src_image.virt_addr, src_image.size);
//                 yolo_input_image->width = src_image.width;
//                 yolo_input_image->height = src_image.height;
//                 yolo_input_image->format = src_image.format;
//                 yolo_input_image->width_stride = src_image.width_stride;
//                 yolo_input_image->height_stride = src_image.height_stride;
//                 yolo_input_image->fd = -1;
//                 {
//                     std::unique_lock<std::mutex> lock(yolo_input_mutex);
//                     if (yolo_input_queue.size() < MAX_QUEUE_SIZE) {
//                         yolo_input_queue.push({current_frame_id, yolo_input_image});
//                     } else {
//                         free(yolo_input_image->virt_addr);
//                         yolo_input_image->virt_addr = nullptr;
//                     }
//                 }
//             } else {
//                 printf("ERROR: Failed alloc YOLO input copy.\n");
//             }

//             // --- Get YOLO Results ---
//             bool yolo_result_received = false;
//             {
//                 std::unique_lock<std::mutex> lock(yolo_output_mutex);
//                 if (!yolo_output_queue.empty()) {
//                     YoloOutputData data = yolo_output_queue.front();
//                     yolo_output_queue.pop();
//                     yolo_results = data.results;
//                     yolo_result_received = true;
//                 }
//             }

//             // --- Run Behavior Analysis & KSS (Only if Driver Tracked) ---
//             detectedObjects.clear();
//             std::vector<std::string> driver_detectedObjects;
//             if (driver_tracked_this_frame && current_tracked_driver_idx != -1) {
//                 face_object_t *driver_face = &face_results.faces[current_tracked_driver_idx];
//                 // --- Calculate and Validate Object Detection ROI (REVISED TUNING) ---
//                 box_rect_t driver_box = driver_face->box;
//                 int box_w = driver_box.right - driver_box.left;
//                 int box_h = driver_box.bottom - driver_box.top;
//                 if (box_w > 0 && box_h > 0) {
//                     int box_center_x = (driver_box.left + driver_box.right) / 2;
//                     int box_center_y = (driver_box.top + driver_box.bottom) / 2;
//                     const float width_expansion = 1.65f;
//                     const float height_expansion_up = 1.0f;
//                     const float height_expansion_down = 2.3f;
//                     int roi_width = static_cast<int>(box_w * width_expansion);
//                     int roi_height = static_cast<int>(box_h * (height_expansion_up + height_expansion_down));
//                     int roi_x = box_center_x - roi_width / 2;
//                     int roi_y = box_center_y - static_cast<int>(box_h * height_expansion_up);
//                     driver_object_roi.x = std::max(0, roi_x);
//                     driver_object_roi.y = std::max(0, roi_y);
//                     driver_object_roi.width = std::min(roi_width, src_image.width - driver_object_roi.x);
//                     driver_object_roi.height = std::min(roi_height, src_image.height - driver_object_roi.y);
//                     valid_object_roi = (driver_object_roi.width > 0 && driver_object_roi.height > 0);
//                 } else {
//                     valid_object_roi = false;
//                 }

//                 // --- Filter YOLO Results for Driver ---
//                 if (valid_object_roi && yolo_results.count > 0) {
//                     for (int j = 0; j < yolo_results.count; ++j) {
//                         object_detect_result* det = &yolo_results.results[j];
//                         cv::Point obj_center(
//                             (det->box.left + det->box.right) / 2,
//                             (det->box.top + det->box.bottom) / 2
//                         );
//                         if (driver_object_roi.contains(obj_center) && det->prop > 0.4) {
//                             driver_detectedObjects.push_back(coco_cls_to_name(det->cls_id));
//                         }
//                     }
//                 }
//                 detectedObjects = driver_detectedObjects;

//                 // --- Run Behavior Modules on Driver ---
//                 if (driver_face->face_landmarks_valid) {
//                     std::vector<cv::Point> faceLandmarksCv = convert_landmarks_to_cvpoint(
//                         driver_face->face_landmarks, NUM_FACE_LANDMARKS);
//                     if (!faceLandmarksCv.empty()) {
//                         headPoseResults = headPoseTracker.run(faceLandmarksCv);
//                         blinkDetector.run(faceLandmarksCv, src_image.width, src_image.height);
//                         yawnMetrics = yawnDetector.run(faceLandmarksCv, src_image.width, src_image.height);
//                         kssCalculator.setPerclos(blinkDetector.getPerclos());
//                         int headPoseKSSValue = 1;
//                         if (headPoseResults.rows.size() >= 4) {
//                             for (const auto& row : headPoseResults.rows) {
//                                 if (row.size() >= 2 && row[0] == "Head KSS") {
//                                     try {
//                                         headPoseKSSValue = std::stoi(row[1]);
//                                     } catch (...) {
//                                         headPoseKSSValue = 1;
//                                     }
//                                     break;
//                                 }
//                             }
//                         }
//                         kssCalculator.setHeadPose(headPoseKSSValue);
//                         kssCalculator.setYawnMetrics(
//                             yawnMetrics.isYawning,
//                             yawnMetrics.yawnFrequency_5min,
//                             yawnMetrics.yawnDuration
//                         );
//                         kssCalculator.setBlinksLastMinute(blinkDetector.getBlinksInWindow());
//                     } else {
//                         printf("WARN: Landmark conversion failed for tracked driver.\n");
//                         headPoseResults = {};
//                         yawnMetrics = {};
//                         kssCalculator.setPerclos(0);
//                         kssCalculator.setHeadPose(1);
//                         kssCalculator.setYawnMetrics(false, 0, 0);
//                         kssCalculator.setBlinksLastMinute(0);
//                     }
//                 } else {
//                     printf("WARN: Tracked driver landmarks invalid for analysis.\n");
//                     headPoseResults = {};
//                     yawnMetrics = {};
//                     kssCalculator.setPerclos(0);
//                     kssCalculator.setHeadPose(1);
//                     kssCalculator.setYawnMetrics(false, 0, 0);
//                     kssCalculator.setBlinksLastMinute(0);
//                 }

//                 // --- Calculate KSS ---
//                 double now_seconds = std::chrono::duration<double>(
//                     std::chrono::system_clock::now().time_since_epoch()).count();
//                 kssCalculator.setDetectedObjects(driver_detectedObjects, now_seconds);
//                 auto kssBreakdownResults = kssCalculator.calculateCompositeKSS();
//                 if (kssBreakdownResults.size() > 5 && kssBreakdownResults[5].size() == 2) {
//                     try {
//                         perclosKSS = std::stoi(kssBreakdownResults[0][1]);
//                         blinkKSS = std::stoi(kssBreakdownResults[1][1]);
//                         headposeKSS = std::stoi(kssBreakdownResults[2][1]);
//                         yawnKSS = std::stoi(kssBreakdownResults[3][1]);
//                         objdectdetectionKSS = std::stoi(kssBreakdownResults[4][1]);
//                         extractedTotalKSS = std::stoi(kssBreakdownResults[5][1]);
//                     } catch (...) {
//                         extractedTotalKSS = 1;
//                     }
//                 } else {
//                     extractedTotalKSS = 1;
//                 }
//                 kssStatus = kssCalculator.getKSSAlertStatus(extractedTotalKSS);
//                 if (kssStatus.empty()) {
//                     kssStatus = "Normal";
//                 }
//             } else {
//                 // Driver NOT tracked in this frame (after calibration phase)
//                 kssStatus = "Driver Not Tracked";
//                 extractedTotalKSS = 1;
//                 yawnMetrics = {};
//                 headPoseResults = {};
//                 kssCalculator.setPerclos(0.0);
//                 kssCalculator.setHeadPose(1);
//                 kssCalculator.setYawnMetrics(false, 0, 0);
//                 kssCalculator.setBlinksLastMinute(0);
//                 double now_seconds = std::chrono::duration<double>(
//                     std::chrono::system_clock::now().time_since_epoch()).count();
//                 kssCalculator.setDetectedObjects({}, now_seconds);
//                 auto kssBreakdownResults = kssCalculator.calculateCompositeKSS();
//                 if (kssBreakdownResults.size() > 5 && kssBreakdownResults[5].size() == 2) {
//                     extractedTotalKSS = std::stoi(kssBreakdownResults[5][1]);
//                 } else {
//                     extractedTotalKSS = 1;
//                 }
//             }

//             // --- Drawing (Normal Processing Phase) ---
//             if (driver_tracked_this_frame && current_tracked_driver_idx != -1) {
//                 face_object_t *driver_face = &face_results.faces[current_tracked_driver_idx];
//                 draw_rectangle(&src_image,
//                               driver_face->box.left,
//                               driver_face->box.top,
//                               driver_face->box.right - driver_face->box.left,
//                               driver_face->box.bottom - driver_face->box.top,
//                               COLOR_GREEN, 2);
//                 if (driver_face->face_landmarks_valid) {
//                     for (int j = 0; j < NUM_FACE_LANDMARKS; ++j) {
//                         draw_circle(&src_image,
//                                    driver_face->face_landmarks[j].x,
//                                    driver_face->face_landmarks[j].y,
//                                    1, COLOR_CYAN, 1);
//                     }
//                     if (driver_face->eye_landmarks_left_valid) {
//                         for (int j = 0; j < NUM_EYE_CONTOUR_LANDMARKS; ++j) {
//                             draw_circle(&src_image,
//                                        driver_face->eye_landmarks_left[j].x,
//                                        driver_face->eye_landmarks_left[j].y,
//                                        1, COLOR_BLUE, 1);
//                         }
//                     }
//                     if (driver_face->iris_landmarks_left_valid) {
//                         for (int j = 0; j < NUM_IRIS_LANDMARKS; ++j) {
//                             draw_circle(&src_image,
//                                        driver_face->iris_landmarks_left[j].x,
//                                        driver_face->iris_landmarks_left[j].y,
//                                        1, COLOR_ORANGE, 1);
//                         }
//                     }
//                     if (driver_face->eye_landmarks_right_valid) {
//                         for (int j = 0; j < NUM_EYE_CONTOUR_LANDMARKS; ++j) {
//                             draw_circle(&src_image,
//                                        driver_face->eye_landmarks_right[j].x,
//                                        driver_face->eye_landmarks_right[j].y,
//                                        1, COLOR_BLUE, 1);
//                         }
//                     }
//                     if (driver_face->iris_landmarks_right_valid) {
//                         for (int j = 0; j < NUM_IRIS_LANDMARKS; ++j) {
//                             draw_circle(&src_image,
//                                        driver_face->iris_landmarks_right[j].x,
//                                        driver_face->iris_landmarks_right[j].y,
//                                        1, COLOR_ORANGE, 1);
//                         }
//                     }
//                     const int LEFT_EYE_TEXT_ANCHOR_IDX = 33;
//                     const int RIGHT_EYE_TEXT_ANCHOR_IDX = 263;
//                     if (LEFT_EYE_TEXT_ANCHOR_IDX < NUM_FACE_LANDMARKS) {
//                         point_t la = driver_face->face_landmarks[LEFT_EYE_TEXT_ANCHOR_IDX];
//                         std::string ls = blinkDetector.isLeftEyeClosed() ? "CLOSED" : "OPEN";
//                         unsigned int lc = blinkDetector.isLeftEyeClosed() ? COLOR_RED : COLOR_GREEN;
//                         draw_text(&src_image, ls.c_str(), la.x - 30, la.y - 25, lc, 14);
//                         text_stream.str("");
//                         text_stream << "L:" << std::fixed << std::setprecision(2)
//                                     << blinkDetector.getLeftEARValue();
//                         draw_text(&src_image, text_stream.str().c_str(),
//                                  la.x - 30, la.y - 10, COLOR_WHITE, 10);
//                     }
//                     if (RIGHT_EYE_TEXT_ANCHOR_IDX < NUM_FACE_LANDMARKS) {
//                         point_t ra = driver_face->face_landmarks[RIGHT_EYE_TEXT_ANCHOR_IDX];
//                         std::string rs = blinkDetector.isRightEyeClosed() ? "CLOSED" : "OPEN";
//                         unsigned int rc = blinkDetector.isRightEyeClosed() ? COLOR_RED : COLOR_GREEN;
//                         draw_text(&src_image, rs.c_str(), ra.x - 30, ra.y - 25, rc, 14);
//                         text_stream.str("");
//                         text_stream << "R:" << std::fixed << std::setprecision(2)
//                                     << blinkDetector.getRightEARValue();
//                         draw_text(&src_image, text_stream.str().c_str(),
//                                  ra.x - 30, ra.y - 10, COLOR_WHITE, 10);
//                     }
//                 }
//             }
// #ifdef DEBUG_DRAW_ROIS
//             for (int i = 0; i < face_results.count; ++i) {
//                 if (i != current_tracked_driver_idx) {
//                     draw_rectangle(&src_image,
//                                   face_results.faces[i].box.left,
//                                   face_results.faces[i].box.top,
//                                   face_results.faces[i].box.right - face_results.faces[i].box.left,
//                                   face_results.faces[i].box.bottom - face_results.faces[i].box.top,
//                                   COLOR_YELLOW, 1);
//                 }
//             }
// #endif
//             if (kssStatus.empty() && calibration_done) {
//                 status_color_uint = COLOR_GREEN;
//             } else if (extractedTotalKSS <= 3) {
//                 status_color_uint = COLOR_GREEN;
//             } else if (extractedTotalKSS <= 7) {
//                 status_color_uint = COLOR_BLUE;
//             } else {
//                 status_color_uint = COLOR_RED;
//             }
//             text_y = 10;
//             if (!kssStatus.empty()) {
//                 draw_text(&src_image, kssStatus.c_str(), 10, text_y, status_color_uint, status_text_size);
//                 text_y += (int)(line_height * 1.4);
//             }
//             text_stream.str("");
//             text_stream << "PERCLOS: " << std::fixed << std::setprecision(2) << blinkDetector.getPerclos() << "%";
//             draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
//             text_y += line_height;
//             text_stream.str("");
//             text_stream << "Blinks (Last Min): " << blinkDetector.getBlinksInWindow();
//             draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
//             text_y += line_height;
//             if (headPoseResults.rows.size() >= 3) {
//                 std::string headpose_text = "Yaw:" + headPoseResults.rows[0][1] +
//                                           " Pitch:" + headPoseResults.rows[1][1] +
//                                           " Roll:" + headPoseResults.rows[2][1];
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
//             text_stream.str("");
//             text_stream << "Yawn Freq(5m): " << static_cast<int>(yawnMetrics.yawnFrequency_5min);
//             draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size);
//             text_y += line_height;
//             std::string detected_objects_text = "";
//             if (!detectedObjects.empty()) {
//                 detected_objects_text = "Detected: ";
//                 for (size_t j = 0; j < detectedObjects.size(); ++j) {
//                     detected_objects_text += detectedObjects[j];
//                     if (j < detectedObjects.size() - 1) {
//                         detected_objects_text += ", ";
//                     }
//                 }
//                 draw_text(&src_image, detected_objects_text.c_str(), 10, text_y, COLOR_ORANGE, text_size);
//                 text_y += line_height;
//             }
//             text_stream.str("");
//             text_stream << "KSS Breakdown: P" << perclosKSS << " B" << blinkKSS
//                         << " H" << headposeKSS << " Y" << yawnKSS << " O" << objdectdetectionKSS;
//             draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_GREEN, text_size);
//             text_y += line_height;
//             text_stream.str("");
//             text_stream << "Composite KSS: " << extractedTotalKSS;
//             draw_text(&src_image, text_stream.str().c_str(), 10, text_y, status_color_uint, text_size);
//             text_y += line_height;
//         }

//         // --- Draw ROIs (Optional Debugging) ---
// #ifdef DEBUG_DRAW_ROIS
//         if (!first_frame) {
//             draw_rectangle(&src_image,
//                           driver_identification_zone.x,
//                           driver_identification_zone.y,
//                           driver_identification_zone.width,
//                           driver_identification_zone.height,
//                           COLOR_YELLOW, 1);
//             draw_text(&src_image, "Driver ID Zone",
//                       driver_identification_zone.x + 5,
//                       driver_identification_zone.y + 15,
//                       COLOR_YELLOW, 10);
//         }
//         if (valid_object_roi) {
//             draw_rectangle(&src_image,
//                           driver_object_roi.x,
//                           driver_object_roi.y,
//                           driver_object_roi.width,
//                           driver_object_roi.height,
//                           COLOR_MAGENTA, 1);
//             draw_text(&src_image, "Driver Obj ROI",
//                       driver_object_roi.x + 5,
//                       driver_object_roi.y + 15,
//                       COLOR_MAGENTA, 10);
//         }
// #endif

//         // --- Draw Resource Monitoring Info (Always Draw) ---
//         auto frame_processing_end_time = std::chrono::high_resolution_clock::now();
//         double frame_duration_ms = std::chrono::duration<double, std::milli>(
//             frame_processing_end_time - frame_start_time).count();
//         calculateOverallFPS(frame_duration_ms, frame_times, overallFPS, max_time_records);
//         getCPUUsage(currentCpuUsage, prevIdleTime, prevTotalTime);
//         getTemperature(currentTemp);
//         std::stringstream info_ss;
//         info_ss << "FPS:" << std::fixed << std::setprecision(1) << overallFPS
//                 << "|CPU:" << currentCpuUsage << "%|T:" << currentTemp << "C";
//         draw_text(&src_image, info_ss.str().c_str(), 10, src_image.height - 20, COLOR_WHITE, 10);

//         // --- Display Frame (Always) ---
//         cv::Mat display_frame(src_image.height, src_image.width, CV_8UC3,
//                              map_info.data, src_image.width_stride);
//         cv::imshow("DMS Output", display_frame);

//         // --- Push Frame to Saving Pipeline (Always) ---
//         if (pipeline_ && appsrc_) {
//             GstClockTime duration = gst_util_uint64_scale_int(
//                 1, GST_SECOND, video_info.fps_n > 0 ? video_info.fps_d * video_info.fps_n : 30);
//             pushFrameToPipeline(map_info.data, map_info.size,
//                                src_image.width, src_image.height, duration);
//         }

//         // --- Cleanup Frame Resources (Always) ---
//         gst_buffer_unmap(gst_buffer, &map_info);
//         gst_sample_unref(sample);

//         // --- Handle KeyPress (Always) ---
//         if (cv::waitKey(1) == 27) {
//             break; // Exit on ESC
//         }
//     }

//     // --- Final Cleanup ---
//     printf("INFO: Cleaning up resources...\n");
//     cv::destroyAllWindows();
//     stop_yolo_worker.store(true);
//     if (yolo_worker_thread.joinable()) {
//         yolo_worker_thread.join();
//     }
//     printf("INFO: YOLO thread joined.\n");
//     if (input_pipeline) {
//         gst_element_set_state(input_pipeline, GST_STATE_NULL);
//         gst_object_unref(appsink_);
//         gst_object_unref(input_pipeline);
//         printf("INFO: Input pipeline released.\n");
//     }
//     if (pipeline_) {
//         gst_element_send_event(pipeline_, gst_event_new_eos());
//         GstBus* bus = gst_element_get_bus(pipeline_);
//         gst_bus_poll(bus, GST_MESSAGE_EOS, GST_CLOCK_TIME_NONE);
//         gst_object_unref(bus);
//         gst_element_set_state(pipeline_, GST_STATE_NULL);
//         gst_object_unref(appsrc_);
//         gst_object_unref(pipeline_);
//         printf("INFO: Saving pipeline released.\n");
//     }
//     gst_deinit();
//     printf("INFO: GStreamer deinitialized.\n");
//     release_face_analyzer(&app_ctx.face_ctx);
//     release_yolo11(&app_ctx.yolo_ctx);
//     deinit_post_process();
//     printf("INFO: RKNN models released.\n");

//     printf("Exiting main (ret = %d)\n", ret);
//     return ret;
// }





//  SENT only the DRIVER-ROI to the all models

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

// --- Simple CPU Cropping Function (if not available in image_utils.h) ---
// Make sure a similar function exists or implement it here if needed.
// This is a placeholder, use RGA-accelerated crop if possible.
static int crop_image_simple(image_buffer_t *src_img, image_buffer_t *dst_img, box_rect_t crop_box) {
    if (!src_img || !src_img->virt_addr || !dst_img) return -1;
    int channels = 0;
    if (src_img->format == IMAGE_FORMAT_RGB888) channels = 3;
    #ifdef IMAGE_FORMAT_BGR888
    else if (src_img->format == IMAGE_FORMAT_BGR888) channels = 3;
    #endif
    else { printf("ERROR: crop_image_simple unsupported format %d\n", src_img->format); return -1; }

    int src_w = src_img->width; int src_h = src_img->height;
    int crop_x = crop_box.left; int crop_y = crop_box.top;
    int crop_w = crop_box.right - crop_box.left; int crop_h = crop_box.bottom - crop_box.top;

    if (crop_w <= 0 || crop_h <= 0) { printf("ERROR: crop ROI invalid size (%dx%d)\n", crop_w, crop_h); return -1; }
    int x_start = std::max(0, crop_x); int y_start = std::max(0, crop_y);
    int x_end = std::min(src_w, crop_x + crop_w); int y_end = std::min(src_h, crop_y + crop_h);
    int valid_crop_w = x_end - x_start; int valid_crop_h = y_end - y_start;

    if (valid_crop_w <= 0 || valid_crop_h <= 0) { printf("ERROR: Clamped crop ROI zero size.\n"); return -1; }

    dst_img->width = crop_w; dst_img->height = crop_h; dst_img->format = src_img->format;
    dst_img->size = crop_w * crop_h * channels;

    if (dst_img->virt_addr == NULL) {
        dst_img->virt_addr = (unsigned char*)malloc(dst_img->size);
        if (!dst_img->virt_addr) { printf("ERROR: Failed alloc memory for crop (%d bytes)\n", dst_img->size); return -1; }
    } else if (dst_img->size < (size_t)(crop_w * crop_h * channels)) {
        printf("ERROR: Dest buffer too small for crop.\n"); return -1;
    }

    memset(dst_img->virt_addr, 0, dst_img->size); // Clear destination

    unsigned char* src_data = src_img->virt_addr; unsigned char* dst_data = dst_img->virt_addr;
    size_t src_stride = src_img->width_stride ? src_img->width_stride : (size_t)src_w * channels;
    size_t dst_stride = (size_t)crop_w * channels;
    int dst_x_offset = x_start - crop_x; int dst_y_offset = y_start - crop_y;

    for (int y = 0; y < valid_crop_h; ++y) {
        unsigned char* src_row_ptr = src_data + (size_t)(y_start + y) * src_stride + (size_t)x_start * channels;
        unsigned char* dst_row_ptr = dst_data + (size_t)(dst_y_offset + y) * dst_stride + (size_t)dst_x_offset * channels;
        memcpy(dst_row_ptr, src_row_ptr, (size_t)valid_crop_w * channels);
    }
    return 0;
}
// --- End Helper Functions ---


// --- YOLO Worker Thread ---
struct YoloInputData { long frame_id; std::shared_ptr<image_buffer_t> image; };
struct YoloOutputData { long frame_id; object_detect_result_list results; };
std::queue<YoloInputData> yolo_input_queue;
std::queue<YoloOutputData> yolo_output_queue;
std::mutex yolo_input_mutex;
std::mutex yolo_output_mutex;
std::atomic<bool> stop_yolo_worker(false);
const int MAX_QUEUE_SIZE = 5;
void yolo_worker_thread_func(yolo11_app_context_t* yolo_ctx_ptr) { while (!stop_yolo_worker.load()) { YoloInputData input_data; bool got_data = false; { std::unique_lock<std::mutex> lock(yolo_input_mutex); if (!yolo_input_queue.empty()) { input_data = yolo_input_queue.front(); yolo_input_queue.pop(); got_data = true; } } if (got_data && input_data.image) { YoloOutputData output_data; output_data.frame_id = input_data.frame_id; memset(&output_data.results, 0, sizeof(output_data.results)); int ret = inference_yolo11(yolo_ctx_ptr, input_data.image.get(), &output_data.results); if (ret != 0) printf("WARN: YOLO Worker inference failed (frame %ld), ret=%d\n", input_data.frame_id, ret); { std::unique_lock<std::mutex> lock(yolo_output_mutex); if (yolo_output_queue.size() >= MAX_QUEUE_SIZE) yolo_output_queue.pop(); yolo_output_queue.push(output_data); } if (input_data.image->virt_addr) { free(input_data.image->virt_addr); input_data.image->virt_addr = nullptr; } } else { std::this_thread::sleep_for(std::chrono::milliseconds(5)); } } printf("YOLO Worker Thread Exiting.\n");}


// --- GStreamer Saving Pipeline Setup ---
void setupPipeline() { gst_init(nullptr, nullptr); std::string dir = "/userdata/test_cpp/dms_gst"; if (access(dir.c_str(), W_OK) != 0) { std::cerr << "Directory " << dir << " is not writable or does not exist" << std::endl; return; } DIR* directory = opendir(dir.c_str()); if (!directory) { std::cerr << "Failed to open directory " << dir << std::endl; return; } int mkv_count = 0; struct dirent* entry; while ((entry = readdir(directory)) != nullptr) { std::string filename = entry->d_name; if (filename.find(".mkv") != std::string::npos) mkv_count++; } closedir(directory); std::string filepath = dir + "/dms_multi_" + std::to_string(mkv_count + 1) + ".mkv"; std::string pipeline_str = "appsrc name=source ! queue ! videoconvert ! video/x-raw,format=NV12 ! mpph265enc rc-mode=cbr bps=4000000 gop=30 qp-min=10 qp-max=51 ! h265parse ! matroskamux ! filesink location=" + filepath; std::cout << "Saving Pipeline: " << pipeline_str << std::endl; GError* error = nullptr; pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error); if (!pipeline_ || error) { std::cerr << "Failed to create saving pipeline: " << (error ? error->message : "Unknown error") << std::endl; if (error) g_error_free(error); return; } appsrc_ = gst_bin_get_by_name(GST_BIN(pipeline_), "source"); if (!appsrc_) { std::cerr << "Failed to get appsrc" << std::endl; gst_object_unref(pipeline_); pipeline_ = nullptr; return; } GstCaps* caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", "width", G_TYPE_INT, 1920, "height", G_TYPE_INT, 1080, "framerate", GST_TYPE_FRACTION, 60, 1, nullptr); g_object_set(G_OBJECT(appsrc_), "caps", caps, "format", GST_FORMAT_TIME, nullptr); gst_caps_unref(caps); if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) { std::cerr << "Failed to set saving pipeline to playing" << std::endl; gst_object_unref(appsrc_); gst_object_unref(pipeline_); pipeline_ = nullptr; appsrc_ = nullptr; } }
void pushFrameToPipeline(unsigned char* data, int size, int width, int height, GstClockTime duration) { if (!appsrc_) return; GstBuffer* buffer = gst_buffer_new_allocate(nullptr, size, nullptr); GstMapInfo map; if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) { std::cerr << "Failed map buffer" << std::endl; gst_buffer_unref(buffer); return; } if (map.size != (guint)size) { std::cerr << "Buffer size mismatch: " << map.size << " vs " << size << std::endl; gst_buffer_unmap(buffer, &map); gst_buffer_unref(buffer); return; } memcpy(map.data, data, size); gst_buffer_unmap(buffer, &map); static GstClockTime timestamp = 0; GST_BUFFER_PTS(buffer) = timestamp; GST_BUFFER_DURATION(buffer) = duration; timestamp += GST_BUFFER_DURATION(buffer); GstFlowReturn ret; g_signal_emit_by_name(appsrc_, "push-buffer", buffer, &ret); if (ret != GST_FLOW_OK) std::cerr << "Failed push buffer, ret=" << ret << std::endl; gst_buffer_unref(buffer); }


// --- Resource Monitoring Functions ---
// ... (Keep resource monitoring functions as is) ...
void calculateOverallFPS(double frame_duration_ms, std::deque<double>& times_deque, double& fps_variable, int max_records) { times_deque.push_back(frame_duration_ms); if (times_deque.size() > max_records) times_deque.pop_front(); if (!times_deque.empty()) { double sum = std::accumulate(times_deque.begin(), times_deque.end(), 0.0); double avg_time_ms = sum / times_deque.size(); fps_variable = (avg_time_ms > 0) ? (1000.0 / avg_time_ms) : 0.0; } else { fps_variable = 0.0; } }
void getCPUUsage(double& cpu_usage_variable, long& prev_idle, long& prev_total) { std::ifstream file("/proc/stat"); if (!file.is_open()) return; std::string line; std::getline(file, line); file.close(); long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice; user = nice = system = idle = iowait = irq = softirq = steal = guest = guest_nice = 0; std::istringstream iss(line); std::string cpu_label; iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice; long currentIdleTime = idle + iowait; long currentTotalTime = user + nice + system + idle + iowait + irq + softirq + steal; long diffIdle = currentIdleTime - prev_idle; long diffTotal = currentTotalTime - prev_total; if (diffTotal > 0) cpu_usage_variable = 100.0 * (double)(diffTotal - diffIdle) / diffTotal; prev_idle = currentIdleTime; prev_total = currentTotalTime; }
void getTemperature(double& temp_variable) { const char* temp_paths[] = {"/sys/class/thermal/thermal_zone0/temp", "/sys/class/thermal/thermal_zone1/temp"}; bool temp_read = false; for (const char* path : temp_paths) { std::ifstream file(path); double temp_milliC = 0; if (file >> temp_milliC) { temp_variable = temp_milliC / 1000.0; temp_read = true; file.close(); break; } file.close(); } }


/*-------------------------------------------
                  Main Function
-------------------------------------------*/
int main(int argc, char **argv) {

    int ret = 0;
    long current_frame_id = 0;

    // --- Resource Monitoring Vars ---
    std::deque<double> frame_times; const int max_time_records = 60; double overallFPS = 0.0;
    double currentCpuUsage = 0.0; long prevIdleTime = 0, prevTotalTime = 0; double currentTemp = 0.0;

    // --- Model paths ---
    const char *detection_model_path = "../../model/rf.rknn";
    const char *landmark_model_path  = "../../model/faceL.rknn";
    const char *iris_model_path      = "../../model/faceI.rknn";
    const char *yolo_model_path      = "../../model/od.rknn";

    // --- GStreamer input pipeline string ---
    const char *video_source = "filesrc location=../../model/interacting_with_passenger.mkv ! decodebin ! queue ! videoconvert ! video/x-raw,format=BGR ! appsink name=sink sync=false";
    // const char *video_source = "v4l2src device=/dev/video0 ! queue ! videoconvert ! video/x-raw,format=BGR,width=1920,height=1080,framerate=30/1 ! appsink name=sink sync=false";

    // --- Initialization ---
    setupPipeline();
    app_context_t app_ctx; memset(&app_ctx, 0, sizeof(app_context_t));
    image_buffer_t src_image; memset(&src_image, 0, sizeof(image_buffer_t));
    face_analyzer_result_t face_results; memset(&face_results, 0, sizeof(face_results));
    object_detect_result_list yolo_results; memset(&yolo_results, 0, sizeof(yolo_results));
    my::BlinkDetector blinkDetector;
    YawnDetector yawnDetector;
    my::HeadPoseTracker headPoseTracker;
    KSSCalculator kssCalculator;

    // --- Calibration State Variables ---
     bool calibration_done = false; std::chrono::steady_clock::time_point calibration_start_time; bool calibration_timer_started = false; int consecutive_valid_eyes_frames = 0; const int REQUIRED_VALID_EYES_FRAMES = 60; bool ear_calibrated = false; bool mouth_calibrated = false; std::deque<float> calib_left_ears; std::deque<float> calib_right_ears; std::deque<double> calib_mouth_dists; int consecutive_stable_ear_frames = 0; int consecutive_stable_mouth_frames = 0; const int CALIB_WINDOW_SIZE = 30; const float EAR_STDDEV_THRESHOLD = 0.04; const double MOUTH_DIST_STDDEV_THRESHOLD = 15.0; const int REQUIRED_STABLE_FRAMES = CALIB_WINDOW_SIZE + 5; const double CALIBRATION_TIMEOUT_SECONDS = 10.0;


    // --- Driver ID & Tracking State ---
    cv::Rect driver_identification_zone; // Defined on first frame
    bool driver_identified_ever = false;
    bool driver_tracked_this_frame = false;
    int current_tracked_driver_idx = -1;
    cv::Point prev_driver_centroid = cv::Point(-1, -1);
    const double MAX_CENTROID_DISTANCE = 150.0; // TUNABLE
    int driver_search_timeout_frames = 0;
    const int DRIVER_SEARCH_MAX_FRAMES = 60;   // TUNABLE

    // --- Object Detection ROI Variables ---
    cv::Rect driver_object_roi;
    bool valid_object_roi = false;

    // --- State variables for KSS and display ---
    std::string kssStatus = "Initializing";
    YawnDetector::YawnMetrics yawnMetrics = {};
    my::HeadPoseTracker::HeadPoseResults headPoseResults = {};
    std::vector<std::string> detectedObjects; // Holds the list of objects to display
    int extractedTotalKSS = 1;
    int perclosKSS = 1, blinkKSS = 1, headposeKSS = 1, yawnKSS = 1, objdectdetectionKSS = 1;

    // --- Drawing helpers ---
    std::stringstream text_stream; int text_y = 0; const int line_height = 22; const int text_size = 12; const int status_text_size = 16; unsigned int status_color_uint = COLOR_GREEN;

    // --- YOLO thread handle ---
    std::thread yolo_worker_thread;

    // --- Init models ---
    ret = init_post_process(); if (ret != 0) { printf("Error init YOLO postprocess.\n"); return -1; }
    ret = init_face_analyzer(detection_model_path, landmark_model_path, iris_model_path, &app_ctx.face_ctx);
    if (ret != 0) { printf("init_face_analyzer fail! ret=%d\n", ret); deinit_post_process(); return -1; }
    ret = init_yolo11(yolo_model_path, &app_ctx.yolo_ctx); if (ret != 0) { printf("init_yolo11 fail! ret=%d\n", ret); release_face_analyzer(&app_ctx.face_ctx); deinit_post_process(); return -1; }
    yolo_worker_thread = std::thread(yolo_worker_thread_func, &app_ctx.yolo_ctx);


    // --- Init GStreamer input ---
    gst_init(nullptr, nullptr); GError* error = nullptr; GstElement* input_pipeline = gst_parse_launch(video_source, &error); if (!input_pipeline || error) { std::cerr << "Failed to create input pipeline: " << (error ? error->message : "Unknown error") << std::endl; if (error) g_error_free(error); return -1; } appsink_ = gst_bin_get_by_name(GST_BIN(input_pipeline), "sink"); if (!appsink_) { std::cerr << "Failed to get appsink" << std::endl; gst_object_unref(input_pipeline); return -1; } if (gst_element_set_state(input_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) { std::cerr << "Failed to set input pipeline to playing" << std::endl; gst_object_unref(appsink_); gst_object_unref(input_pipeline); return -1; }


    // --- Setup OpenCV window ---
    cv::namedWindow("DMS Output", cv::WINDOW_NORMAL); cv::resizeWindow("DMS Output", 1920, 1080);

    GstSample* sample; GstVideoInfo video_info; GstBuffer* gst_buffer; GstMapInfo map_info;
    bool first_frame = true;

    // +++++++++++++++ Buffer for the cropped Driver ID Zone +++++++++++++++
    image_buffer_t driver_zone_crop_img;
    memset(&driver_zone_crop_img, 0, sizeof(image_buffer_t));
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    // --- Main Processing Loop ---
    while (true) {
        // --- Get Frame ---
        sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink_)); if (!sample) { std::cerr << "End of stream or error pulling sample." << std::endl; break; }
        gst_buffer = gst_sample_get_buffer(sample); GstCaps* caps = gst_sample_get_caps(sample);
        if (!gst_buffer || !caps || !gst_video_info_from_caps(&video_info, caps) || !gst_buffer_map(gst_buffer, &map_info, GST_MAP_READ)) { /* ... error handling ... */ if (gst_buffer && map_info.memory) gst_buffer_unmap(gst_buffer, &map_info); if (sample) gst_sample_unref(sample); continue; }

        auto frame_start_time = std::chrono::high_resolution_clock::now();
        current_frame_id++;

        // --- Wrap GStreamer Data for the *full source image* ---
        src_image.width = GST_VIDEO_INFO_WIDTH(&video_info); src_image.height = GST_VIDEO_INFO_HEIGHT(&video_info); src_image.width_stride = GST_VIDEO_INFO_PLANE_STRIDE(&video_info, 0); src_image.height_stride = src_image.height;
        #ifdef IMAGE_FORMAT_BGR888
            src_image.format = IMAGE_FORMAT_BGR888;
        #else
            src_image.format = IMAGE_FORMAT_RGB888;
        #endif
        src_image.virt_addr = map_info.data; src_image.size = map_info.size; src_image.fd = -1;

        // --- Define Driver Identification Zone (on first frame - RHD Revised) ---
        if (first_frame) {
             // ... (Define driver_identification_zone as before) ...
             int frame_width = src_image.width; int frame_height = src_image.height; int roi_start_x = (int)(frame_width * 0.40); int roi_width = (int)(frame_width * 0.55); driver_identification_zone = cv::Rect(roi_start_x, (int)(frame_height * 0.1), roi_width, (int)(frame_height * 0.8)); driver_identification_zone &= cv::Rect(0, 0, frame_width, frame_height); printf("INFO: Driver Identification Zone (RHD Revised) set to [%d, %d, %d x %d]\n", driver_identification_zone.x, driver_identification_zone.y, driver_identification_zone.width, driver_identification_zone.height);
             first_frame = false;
        }

        // +++++++++++++++ NEW: Crop to Driver ID Zone +++++++++++++++
        // Free previous buffer if it exists
        if (driver_zone_crop_img.virt_addr) {
            free(driver_zone_crop_img.virt_addr);
            driver_zone_crop_img.virt_addr = nullptr;
        }
        memset(&driver_zone_crop_img, 0, sizeof(image_buffer_t)); // Clear struct

        box_rect_t id_zone_as_box = {
            driver_identification_zone.x, driver_identification_zone.y,
            driver_identification_zone.x + driver_identification_zone.width,
            driver_identification_zone.y + driver_identification_zone.height
        };

        // Perform the crop (using simple CPU version for now)
        ret = crop_image_simple(&src_image, &driver_zone_crop_img, id_zone_as_box);
        if (ret != 0 || !driver_zone_crop_img.virt_addr) {
            printf("ERROR: Failed to crop to Driver ID Zone for frame %ld.\n", current_frame_id);
            gst_buffer_unmap(gst_buffer, &map_info); gst_sample_unref(sample);
            continue; // Skip rest of processing for this frame
        }
        // Now driver_zone_crop_img holds the relevant part of the frame
        // Its dimensions are driver_identification_zone.width/height
        // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


        // --- Run Face Analysis on the CROPPED image ---
        ret = inference_face_analyzer(&app_ctx.face_ctx, &driver_zone_crop_img, &face_results);
        if (ret != 0) {
             printf("WARN: Face Analyzer Inference failed on crop frame %ld, ret=%d\n", current_frame_id, ret);
             face_results.count = 0; // Ensure no faces are processed if inference failed
        }
        // IMPORTANT: face_results coordinates are now RELATIVE to driver_zone_crop_img

        // --- Reset frame-specific flags ---
        driver_tracked_this_frame = false;
        current_tracked_driver_idx = -1;
        valid_object_roi = false;
        int crop_offset_x = driver_identification_zone.x; // Store offset for drawing
        int crop_offset_y = driver_identification_zone.y;

        // --- Driver Identification and Tracking Logic (Operates on coordinates relative to the crop) ---
        if (!driver_identified_ever) {
            // Phase 1: Identification (No need to check zone anymore, all faces are within it)
            int best_candidate_idx = -1;
            float max_area = 0.0f;
            for (int i = 0; i < face_results.count; ++i) {
                if (face_results.faces[i].face_landmarks_valid) { // Still need valid landmarks
                    float area = (float)(face_results.faces[i].box.right - face_results.faces[i].box.left) *
                                 (face_results.faces[i].box.bottom - face_results.faces[i].box.top);
                    // Additional logic: Check if face is reasonably large or central within the *crop*? Optional.
                    // Example: Check centrality within the cropped zone
                    // cv::Point face_center_rel_crop((face_results.faces[i].box.left + face_results.faces[i].box.right) / 2, ...);
                    // if (face_center_rel_crop.x > driver_zone_crop_img.width * 0.1 && face_center_rel_crop.x < driver_zone_crop_img.width * 0.9) { ... }

                    if (area > max_area) {
                        max_area = area;
                        best_candidate_idx = i;
                    }
                }
            }
            if (best_candidate_idx != -1) {
                driver_identified_ever = true;
                driver_tracked_this_frame = true;
                current_tracked_driver_idx = best_candidate_idx;
                // Centroid coordinates are relative to the crop
                prev_driver_centroid = calculate_centroid(
                    face_results.faces[best_candidate_idx].face_landmarks, NUM_FACE_LANDMARKS);
                driver_search_timeout_frames = 0;
                printf("INFO: Driver Identified (Index %d relative to crop) at frame %ld\n", current_tracked_driver_idx, current_frame_id);
            } else { kssStatus = "Searching Driver (in Zone)..."; }
        } else {
            // Phase 2: Tracking (using centroids relative to crop)
            int best_match_idx = -1;
            double min_dist = MAX_CENTROID_DISTANCE;
            for (int i = 0; i < face_results.count; ++i) {
                if (face_results.faces[i].face_landmarks_valid) {
                    cv::Point current_centroid = calculate_centroid(
                        face_results.faces[i].face_landmarks, NUM_FACE_LANDMARKS);
                    if (prev_driver_centroid.x >= 0 && current_centroid.x >= 0) { // Check validity
                        double dist = cv::norm(current_centroid - prev_driver_centroid);
                        // No need for zone check here, as all faces are already from the zone
                        if (dist < min_dist) {
                            min_dist = dist;
                            best_match_idx = i;
                        }
                    }
                }
            }
            if (best_match_idx != -1) { // Found match
                driver_tracked_this_frame = true;
                current_tracked_driver_idx = best_match_idx;
                prev_driver_centroid = calculate_centroid(
                    face_results.faces[best_match_idx].face_landmarks, NUM_FACE_LANDMARKS);
                driver_search_timeout_frames = 0;
            } else { // No match
                driver_tracked_this_frame = false;
                current_tracked_driver_idx = -1;
                prev_driver_centroid = cv::Point(-1, -1);
                driver_search_timeout_frames++;
                kssStatus = "Driver Lost...";
                if (driver_search_timeout_frames > DRIVER_SEARCH_MAX_FRAMES) {
                    driver_identified_ever = false; // Revert to search
                    driver_search_timeout_frames = 0;
                    printf("INFO: Driver track lost for %d frames. Reverting to search.\n", DRIVER_SEARCH_MAX_FRAMES);
                    kssStatus = "Searching Driver (in Zone)...";
                }
            }
        }

        // --- Calibration Phase Logic (Operates on tracked driver within the crop) ---
        if (!calibration_done) {
             bool head_pose_calibrated = false; bool eyes_consistently_valid = false; double elapsed_calib_seconds = 0.0; std::string calib_status_detail = ""; if (driver_tracked_this_frame && current_tracked_driver_idx != -1) { face_object_t *calib_face = &face_results.faces[current_tracked_driver_idx]; if (calib_face->face_landmarks_valid) { if (!calibration_timer_started) { calibration_start_time = std::chrono::steady_clock::now(); calibration_timer_started = true; printf("INFO: Calibration timer started (Driver Index %d).\n", current_tracked_driver_idx); consecutive_valid_eyes_frames = 0; consecutive_stable_ear_frames = 0; consecutive_stable_mouth_frames = 0; calib_left_ears.clear(); calib_right_ears.clear(); calib_mouth_dists.clear(); ear_calibrated = false; mouth_calibrated = false; } std::vector<cv::Point> calib_faceLandmarks = convert_landmarks_to_cvpoint(calib_face->face_landmarks, NUM_FACE_LANDMARKS); headPoseTracker.run(calib_faceLandmarks); head_pose_calibrated = headPoseTracker.isCalibrated(); if (!head_pose_calibrated) calib_status_detail += " (Head)"; bool eyes_valid_this_frame = calib_face->eye_landmarks_left_valid && calib_face->eye_landmarks_right_valid; if (eyes_valid_this_frame) consecutive_valid_eyes_frames++; else consecutive_valid_eyes_frames = 0; eyes_consistently_valid = (consecutive_valid_eyes_frames >= REQUIRED_VALID_EYES_FRAMES); if (!eyes_consistently_valid) calib_status_detail += " (Eyes Valid)"; if (!ear_calibrated && eyes_valid_this_frame) { const std::vector<int> L={33,160,158,133,153,144}, R={362,385,387,263,380,373}; float lE=calculate_ear_simple(calib_faceLandmarks,L), rE=calculate_ear_simple(calib_faceLandmarks,R); calib_left_ears.push_back(lE); calib_right_ears.push_back(rE); if (calib_left_ears.size()>CALIB_WINDOW_SIZE) calib_left_ears.pop_front(); if (calib_right_ears.size()>CALIB_WINDOW_SIZE) calib_right_ears.pop_front(); if (calib_left_ears.size()>=CALIB_WINDOW_SIZE) { float lS=calculate_stddev(calib_left_ears), rS=calculate_stddev(calib_right_ears); if (lS<EAR_STDDEV_THRESHOLD && rS<EAR_STDDEV_THRESHOLD) consecutive_stable_ear_frames++; else consecutive_stable_ear_frames=0; } else consecutive_stable_ear_frames=0; ear_calibrated=(consecutive_stable_ear_frames>=REQUIRED_STABLE_FRAMES); } else if(!eyes_valid_this_frame){ consecutive_stable_ear_frames=0; ear_calibrated=false; calib_left_ears.clear(); calib_right_ears.clear(); } if(!ear_calibrated) calib_status_detail+=" (EAR Stable)"; if (!mouth_calibrated) { double cM=calculate_mouth_dist_simple(calib_faceLandmarks); calib_mouth_dists.push_back(cM); if(calib_mouth_dists.size()>CALIB_WINDOW_SIZE) calib_mouth_dists.pop_front(); if(calib_mouth_dists.size()>=CALIB_WINDOW_SIZE){ double mS=calculate_stddev(calib_mouth_dists); if(mS<MOUTH_DIST_STDDEV_THRESHOLD) consecutive_stable_mouth_frames++; else consecutive_stable_mouth_frames=0; } else consecutive_stable_mouth_frames=0; mouth_calibrated=(consecutive_stable_mouth_frames>=REQUIRED_STABLE_FRAMES); } if(!mouth_calibrated) calib_status_detail+=" (Mouth Stable)"; auto now = std::chrono::steady_clock::now(); elapsed_calib_seconds = std::chrono::duration<double>(now - calibration_start_time).count(); if (elapsed_calib_seconds >= CALIBRATION_TIMEOUT_SECONDS) { if (head_pose_calibrated && eyes_consistently_valid && ear_calibrated && mouth_calibrated) { calibration_done = true; printf("INFO: Calibration Complete!\n"); } else { calibration_timer_started = false; driver_identified_ever = false; driver_tracked_this_frame = false; current_tracked_driver_idx = -1; prev_driver_centroid = cv::Point(-1, -1); consecutive_valid_eyes_frames = 0; consecutive_stable_ear_frames = 0; consecutive_stable_mouth_frames = 0; ear_calibrated = false; mouth_calibrated = false; calib_left_ears.clear(); calib_right_ears.clear(); calib_mouth_dists.clear(); printf("WARN: Calibration time expired, criteria not met (H:%d, EV:%d, ES:%d, MS:%d). Retrying identification.\n", head_pose_calibrated, eyes_consistently_valid, ear_calibrated, mouth_calibrated); } } } else { calibration_timer_started = false; printf("WARN: Tracked driver landmarks not valid during calibration.\n"); } std::string calib_text = "Calibrating... " + std::to_string(static_cast<int>(elapsed_calib_seconds)) + "s" + calib_status_detail; draw_text(&src_image, calib_text.c_str(), crop_offset_x + 10, crop_offset_y + 30, COLOR_YELLOW, status_text_size); /* Offset drawing */ draw_rectangle(&src_image, crop_offset_x + calib_face->box.left, crop_offset_y + calib_face->box.top, calib_face->box.right - calib_face->box.left, calib_face->box.bottom - calib_face->box.top, COLOR_YELLOW, 2); /* Offset drawing */ } else { /* No driver tracked during calibration */ calibration_timer_started = false; consecutive_valid_eyes_frames = 0; consecutive_stable_ear_frames = 0; consecutive_stable_mouth_frames = 0; ear_calibrated = false; mouth_calibrated = false; calib_left_ears.clear(); calib_right_ears.clear(); calib_mouth_dists.clear(); if (driver_identified_ever) { draw_text(&src_image, "Calibration: Waiting for Driver Track...", crop_offset_x + 10, crop_offset_y + 30, COLOR_YELLOW, status_text_size); } else { draw_text(&src_image, "Calibration: Searching Driver...", crop_offset_x + 10, crop_offset_y + 30, COLOR_YELLOW, status_text_size); } for (int i = 0; i < face_results.count; ++i) { draw_rectangle(&src_image, crop_offset_x + face_results.faces[i].box.left, crop_offset_y + face_results.faces[i].box.top, face_results.faces[i].box.right - face_results.faces[i].box.left, face_results.faces[i].box.bottom - face_results.faces[i].box.top, COLOR_YELLOW, 1); } }


        } else { // Normal Processing Phase (Calibration is Done)
            // --- Queue **CROPPED** Frame for YOLO ---
            auto yolo_input_image = std::make_shared<image_buffer_t>();
            yolo_input_image->width = driver_zone_crop_img.width;  // Use crop dimensions
            yolo_input_image->height = driver_zone_crop_img.height;
            yolo_input_image->format = driver_zone_crop_img.format;
            yolo_input_image->size = driver_zone_crop_img.size;
            yolo_input_image->virt_addr = (unsigned char*)malloc(yolo_input_image->size);
            if (yolo_input_image->virt_addr) {
                memcpy(yolo_input_image->virt_addr, driver_zone_crop_img.virt_addr, yolo_input_image->size);
                yolo_input_image->width_stride = 0; // Assuming packed crop
                yolo_input_image->height_stride = 0;
                yolo_input_image->fd = -1;
                { std::unique_lock<std::mutex> lock(yolo_input_mutex); if (yolo_input_queue.size() < MAX_QUEUE_SIZE) yolo_input_queue.push({current_frame_id, yolo_input_image}); else { free(yolo_input_image->virt_addr); yolo_input_image->virt_addr = nullptr; } }
            } else { printf("ERROR: Failed alloc YOLO input copy (cropped).\n"); }


            // --- Get YOLO Results ---
            bool yolo_result_received = false; { std::unique_lock<std::mutex> lock(yolo_output_mutex); if (!yolo_output_queue.empty()) { YoloOutputData data = yolo_output_queue.front(); yolo_output_queue.pop(); yolo_results = data.results; yolo_result_received = true; } }


            // --- Run Behavior Analysis & KSS (Only if Driver Tracked) ---
            detectedObjects.clear(); // Clear display list
            std::vector<std::string> driver_detectedObjects; // Filtered list for KSS
            // driver_object_roi and valid_object_roi are declared outside loop, valid_object_roi reset earlier

            if (driver_tracked_this_frame && current_tracked_driver_idx != -1) {
                face_object_t *driver_face = &face_results.faces[current_tracked_driver_idx];

                // --- Calculate and Validate Object Detection ROI (relative to CROP) ---
                box_rect_t driver_box_rel_crop = driver_face->box; // Already relative to crop
                int box_w = driver_box_rel_crop.right - driver_box_rel_crop.left;
                int box_h = driver_box_rel_crop.bottom - driver_box_rel_crop.top;
                if (box_w > 0 && box_h > 0) {
                    int box_center_x = (driver_box_rel_crop.left + driver_box_rel_crop.right) / 2;
                    int box_center_y = (driver_box_rel_crop.top + driver_box_rel_crop.bottom) / 2;
                    // --- TUNABLE Expansion Factors ---
                    const float width_expansion = 1.65f;
                    const float height_expansion_up = 1.0f;
                    const float height_expansion_down = 2.3f;
                    // ---------------------------------
                    int roi_width = static_cast<int>(box_w * width_expansion);
                    int roi_height = static_cast<int>(box_h * (height_expansion_up + height_expansion_down));
                    int roi_x = box_center_x - roi_width / 2;
                    int roi_y = box_center_y - static_cast<int>(box_h * height_expansion_up);

                    // ROI coordinates are relative to the crop image
                    driver_object_roi.x = std::max(0, roi_x);
                    driver_object_roi.y = std::max(0, roi_y);
                    driver_object_roi.width = std::min(roi_width, driver_zone_crop_img.width - driver_object_roi.x);
                    driver_object_roi.height = std::min(roi_height, driver_zone_crop_img.height - driver_object_roi.y);
                    valid_object_roi = (driver_object_roi.width > 0 && driver_object_roi.height > 0);
                } else { valid_object_roi = false; }

                // --- Filter YOLO Results for Driver (using coordinates relative to crop) ---
                if (valid_object_roi && yolo_results.count > 0) {
                    for (int j = 0; j < yolo_results.count; ++j) {
                        object_detect_result* det = &yolo_results.results[j];
                        // YOLO results 'det->box' are relative to the *YOLO input*,
                        // which was the *cropped* driver zone image.
                        cv::Point obj_center_rel_crop(
                            (det->box.left + det->box.right) / 2,
                            (det->box.top + det->box.bottom) / 2
                        );
                        // Check if this point (relative to crop) is within the object ROI (also relative to crop)
                        if (driver_object_roi.contains(obj_center_rel_crop) && det->prop > 0.4) {
                            driver_detectedObjects.push_back(coco_cls_to_name(det->cls_id));
                        }
                    }
                }
                detectedObjects = driver_detectedObjects; // Update display list


                // --- Run Behavior Modules on Driver ---
                 if (driver_face->face_landmarks_valid) { std::vector<cv::Point> faceLandmarksCv = convert_landmarks_to_cvpoint(driver_face->face_landmarks, NUM_FACE_LANDMARKS); if (!faceLandmarksCv.empty()) { headPoseResults = headPoseTracker.run(faceLandmarksCv); blinkDetector.run(faceLandmarksCv, driver_zone_crop_img.width, driver_zone_crop_img.height); yawnMetrics = yawnDetector.run(faceLandmarksCv, driver_zone_crop_img.width, driver_zone_crop_img.height); kssCalculator.setPerclos(blinkDetector.getPerclos()); int headPoseKSSValue = 1; if (headPoseResults.rows.size() >= 4) { for (const auto& row : headPoseResults.rows) { if (row.size() >= 2 && row[0] == "Head KSS") { try { headPoseKSSValue = std::stoi(row[1]); } catch (...) { headPoseKSSValue = 1; } break; } } } kssCalculator.setHeadPose(headPoseKSSValue); kssCalculator.setYawnMetrics(yawnMetrics.isYawning, yawnMetrics.yawnFrequency_5min, yawnMetrics.yawnDuration); kssCalculator.setBlinksLastMinute(blinkDetector.getBlinksInWindow()); } else { printf("WARN: Landmark conversion failed for tracked driver.\n"); headPoseResults = {}; yawnMetrics = {}; kssCalculator.setPerclos(0); kssCalculator.setHeadPose(1); kssCalculator.setYawnMetrics(false, 0, 0); kssCalculator.setBlinksLastMinute(0); } } else { printf("WARN: Tracked driver landmarks invalid for analysis.\n"); headPoseResults = {}; yawnMetrics = {}; kssCalculator.setPerclos(0); kssCalculator.setHeadPose(1); kssCalculator.setYawnMetrics(false, 0, 0); kssCalculator.setBlinksLastMinute(0); }


                // --- Calculate KSS (Common part) ---
       
                double now_seconds = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count(); kssCalculator.setDetectedObjects(driver_detectedObjects, now_seconds); auto kssBreakdownResults = kssCalculator.calculateCompositeKSS(); if (kssBreakdownResults.size() > 5 && kssBreakdownResults[5].size() == 2) { try { perclosKSS = std::stoi(kssBreakdownResults[0][1]); blinkKSS = std::stoi(kssBreakdownResults[1][1]); headposeKSS = std::stoi(kssBreakdownResults[2][1]); yawnKSS = std::stoi(kssBreakdownResults[3][1]); objdectdetectionKSS = std::stoi(kssBreakdownResults[4][1]); extractedTotalKSS = std::stoi(kssBreakdownResults[5][1]); } catch (...) { extractedTotalKSS = 1; } } else { extractedTotalKSS = 1; } kssStatus = kssCalculator.getKSSAlertStatus(extractedTotalKSS); if(kssStatus.empty()) kssStatus = "Normal";


            } else { // Driver NOT tracked in this frame (after calibration phase)

                 kssStatus = "Driver Not Tracked"; extractedTotalKSS = 1; yawnMetrics = {}; headPoseResults = {}; kssCalculator.setPerclos(0.0); kssCalculator.setHeadPose(1); kssCalculator.setYawnMetrics(false, 0, 0); kssCalculator.setBlinksLastMinute(0); double now_seconds = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count(); kssCalculator.setDetectedObjects({}, now_seconds); auto kssBreakdownResults = kssCalculator.calculateCompositeKSS(); if (kssBreakdownResults.size() > 5 && kssBreakdownResults[5].size() == 2) { extractedTotalKSS = std::stoi(kssBreakdownResults[5][1]); } else { extractedTotalKSS = 1;}
            }

            // --- Drawing (Normal Processing Phase - WITH OFFSETS) ---
            // Draw Landmarks, Boxes, Eye Status ONLY for the TRACKED driver
            if (driver_tracked_this_frame && current_tracked_driver_idx != -1) {
                face_object_t *driver_face = &face_results.faces[current_tracked_driver_idx];
                // Draw Green bounding box for tracked driver (Offset coordinates)
                draw_rectangle(&src_image,
                              crop_offset_x + driver_face->box.left, // Add offset
                              crop_offset_y + driver_face->box.top,  // Add offset
                              driver_face->box.right - driver_face->box.left,
                              driver_face->box.bottom - driver_face->box.top,
                              COLOR_GREEN, 2);

                if (driver_face->face_landmarks_valid) {
                    // Draw Cyan Face Landmarks for tracked driver (Offset coordinates)
                    for (int j = 0; j < NUM_FACE_LANDMARKS; ++j) {
                        draw_circle(&src_image,
                                   crop_offset_x + driver_face->face_landmarks[j].x, // Add offset
                                   crop_offset_y + driver_face->face_landmarks[j].y, // Add offset
                                   1, COLOR_CYAN, 1);
                    }
                    // Draw Eye/Iris Landmarks (Offset coordinates)
                    if (driver_face->eye_landmarks_left_valid) { for (int j=0; j<NUM_EYE_CONTOUR_LANDMARKS; ++j) draw_circle(&src_image, crop_offset_x + driver_face->eye_landmarks_left[j].x, crop_offset_y + driver_face->eye_landmarks_left[j].y, 1, COLOR_BLUE, 1); }
                    if (driver_face->iris_landmarks_left_valid) { for (int j=0; j<NUM_IRIS_LANDMARKS; ++j) draw_circle(&src_image, crop_offset_x + driver_face->iris_landmarks_left[j].x, crop_offset_y + driver_face->iris_landmarks_left[j].y, 1, COLOR_ORANGE, 1); }
                    if (driver_face->eye_landmarks_right_valid) { for (int j=0; j<NUM_EYE_CONTOUR_LANDMARKS; ++j) draw_circle(&src_image, crop_offset_x + driver_face->eye_landmarks_right[j].x, crop_offset_y + driver_face->eye_landmarks_right[j].y, 1, COLOR_BLUE, 1); }
                    if (driver_face->iris_landmarks_right_valid) { for (int j=0; j<NUM_IRIS_LANDMARKS; ++j) draw_circle(&src_image, crop_offset_x + driver_face->iris_landmarks_right[j].x, crop_offset_y + driver_face->iris_landmarks_right[j].y, 1, COLOR_ORANGE, 1); }

                    // Draw Eye Status Text near the tracked driver's eyes (Offset coordinates)
                    const int LEFT_EYE_TEXT_ANCHOR_IDX = 33; const int RIGHT_EYE_TEXT_ANCHOR_IDX = 263;
                    if (LEFT_EYE_TEXT_ANCHOR_IDX < NUM_FACE_LANDMARKS) { point_t la_rel=driver_face->face_landmarks[LEFT_EYE_TEXT_ANCHOR_IDX]; std::string ls=blinkDetector.isLeftEyeClosed()?"CLOSED":"OPEN"; unsigned int lc=blinkDetector.isLeftEyeClosed()?COLOR_RED:COLOR_GREEN; draw_text(&src_image, ls.c_str(), crop_offset_x + la_rel.x - 30, crop_offset_y + la_rel.y - 25, lc, 14); text_stream.str(""); text_stream << "L:" << std::fixed << std::setprecision(2) << blinkDetector.getLeftEARValue(); draw_text(&src_image, text_stream.str().c_str(), crop_offset_x + la_rel.x - 30, crop_offset_y + la_rel.y - 10, COLOR_WHITE, 10); }
                    if (RIGHT_EYE_TEXT_ANCHOR_IDX < NUM_FACE_LANDMARKS) { point_t ra_rel=driver_face->face_landmarks[RIGHT_EYE_TEXT_ANCHOR_IDX]; std::string rs=blinkDetector.isRightEyeClosed()?"CLOSED":"OPEN"; unsigned int rc=blinkDetector.isRightEyeClosed()?COLOR_RED:COLOR_GREEN; draw_text(&src_image, rs.c_str(), crop_offset_x + ra_rel.x - 30, crop_offset_y + ra_rel.y - 25, rc, 14); text_stream.str(""); text_stream << "R:" << std::fixed << std::setprecision(2) << blinkDetector.getRightEARValue(); draw_text(&src_image, text_stream.str().c_str(), crop_offset_x + ra_rel.x - 30, crop_offset_y + ra_rel.y - 10, COLOR_WHITE, 10); }
                }
            }
            // Draw Status Text (No offset needed for text in top-left corner of full frame)
            if (kssStatus.empty() && calibration_done) status_color_uint = COLOR_GREEN; else if (extractedTotalKSS <= 3) status_color_uint = COLOR_GREEN; else if (extractedTotalKSS <= 7) status_color_uint = COLOR_BLUE; else status_color_uint = COLOR_RED; text_y = 10; if (!kssStatus.empty()) { draw_text(&src_image, kssStatus.c_str(), 10, text_y, status_color_uint, status_text_size); text_y += (int)(line_height * 1.4); } text_stream.str(""); text_stream << "PERCLOS: " << std::fixed << std::setprecision(2) << blinkDetector.getPerclos() << "%"; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height; text_stream.str(""); text_stream << "Blinks (Last Min): " << blinkDetector.getBlinksInWindow(); draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height; if (headPoseResults.rows.size() >= 3) { std::string headpose_text = "Yaw:" + headPoseResults.rows[0][1] + " Pitch:" + headPoseResults.rows[1][1] + " Roll:" + headPoseResults.rows[2][1]; draw_text(&src_image, headpose_text.c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height; } else { draw_text(&src_image, "Head Pose: N/A", 10, text_y, COLOR_WHITE, text_size); text_y += line_height; } text_stream.str(""); text_stream << "Yawning: " << (yawnMetrics.isYawning ? "Yes" : "No"); draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height; text_stream.str(""); text_stream << "Yawn Freq(5m): " << static_cast<int>(yawnMetrics.yawnFrequency_5min); draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height; std::string detected_objects_text = ""; if (!detectedObjects.empty()) { detected_objects_text = "Detected: "; for (size_t j = 0; j < detectedObjects.size(); ++j) { detected_objects_text += detectedObjects[j]; if (j < detectedObjects.size() - 1) detected_objects_text += ", "; } draw_text(&src_image, detected_objects_text.c_str(), 10, text_y, COLOR_ORANGE, text_size); text_y += line_height; } text_stream.str(""); text_stream << "KSS Breakdown: P" << perclosKSS << " B" << blinkKSS << " H" << headposeKSS << " Y" << yawnKSS << " O" << objdectdetectionKSS; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_GREEN, text_size); text_y += line_height; text_stream.str(""); text_stream << "Composite KSS: " << extractedTotalKSS; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, status_color_uint, text_size); text_y += line_height;


        } // End if (!calibration_done) / else

        // --- Draw ROIs (Optional Debugging - DRAW WITH OFFSET) ---
        #ifdef DEBUG_DRAW_ROIS
        if (!first_frame) {
             draw_rectangle(&src_image,
                            driver_identification_zone.x, // Already in full-frame coords
                            driver_identification_zone.y,
                            driver_identification_zone.width,
                            driver_identification_zone.height,
                            COLOR_YELLOW, 1);
             draw_text(&src_image, "Driver ID Zone",
                       driver_identification_zone.x + 5, // Already in full-frame coords
                       driver_identification_zone.y + 15,
                       COLOR_YELLOW, 10);
        }
        if (valid_object_roi) { // This ROI was calculated relative to crop
            draw_rectangle(&src_image,
                           crop_offset_x + driver_object_roi.x, // Add offset
                           crop_offset_y + driver_object_roi.y, // Add offset
                           driver_object_roi.width,
                           driver_object_roi.height,
                           COLOR_MAGENTA, 1);
            draw_text(&src_image, "Driver Obj ROI",
                      crop_offset_x + driver_object_roi.x + 5, // Add offset
                      crop_offset_y + driver_object_roi.y + 15,
                      COLOR_MAGENTA, 10);
        }
        #endif

        // --- Draw Resource Monitoring Info (Always Draw) ---
         auto frame_processing_end_time = std::chrono::high_resolution_clock::now(); double frame_duration_ms = std::chrono::duration<double, std::milli>(frame_processing_end_time - frame_start_time).count(); calculateOverallFPS(frame_duration_ms, frame_times, overallFPS, max_time_records); getCPUUsage(currentCpuUsage, prevIdleTime, prevTotalTime); getTemperature(currentTemp); std::stringstream info_ss; info_ss << "FPS:" << std::fixed << std::setprecision(1) << overallFPS << "|CPU:" << currentCpuUsage << "%|T:" << currentTemp << "C"; draw_text(&src_image, info_ss.str().c_str(), 10, src_image.height - 20, COLOR_WHITE, 10);


        // --- Display Frame (Always) ---
        cv::Mat display_frame(src_image.height, src_image.width, CV_8UC3, map_info.data, src_image.width_stride); cv::imshow("DMS Output", display_frame);


        // --- Push Frame to Saving Pipeline (Always push original full frame) ---
         if (pipeline_ && appsrc_) { GstClockTime duration = gst_util_uint64_scale_int(1, GST_SECOND, video_info.fps_n > 0 ? video_info.fps_d * video_info.fps_n : 30); pushFrameToPipeline(map_info.data, map_info.size, src_image.width, src_image.height, duration); }


        // --- Cleanup Frame Resources (Always) ---
        // +++++++++++++++ FREE THE CROPPED IMAGE BUFFER +++++++++++++++
        if (driver_zone_crop_img.virt_addr) {
            free(driver_zone_crop_img.virt_addr);
            driver_zone_crop_img.virt_addr = nullptr; // Avoid double free
        }
        // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        gst_buffer_unmap(gst_buffer, &map_info);
        gst_sample_unref(sample);


        // --- Handle KeyPress (Always) ---
        if (cv::waitKey(1) == 27) break; // Exit on ESC

    } // End main loop

    // Free crop buffer if loop exited abruptly
    if (driver_zone_crop_img.virt_addr) {
        free(driver_zone_crop_img.virt_addr);
    }

    printf("INFO: Cleaning up resources...\n"); cv::destroyAllWindows(); stop_yolo_worker.store(true); if (yolo_worker_thread.joinable()) yolo_worker_thread.join(); printf("INFO: YOLO thread joined.\n"); if (input_pipeline) { gst_element_set_state(input_pipeline, GST_STATE_NULL); gst_object_unref(appsink_); gst_object_unref(input_pipeline); printf("INFO: Input pipeline released.\n"); } if (pipeline_) { gst_element_send_event(pipeline_, gst_event_new_eos()); GstBus* bus = gst_element_get_bus(pipeline_); gst_bus_poll(bus, GST_MESSAGE_EOS, GST_CLOCK_TIME_NONE); gst_object_unref(bus); gst_element_set_state(pipeline_, GST_STATE_NULL); gst_object_unref(appsrc_); gst_object_unref(pipeline_); printf("INFO: Saving pipeline released.\n"); } gst_deinit(); printf("INFO: GStreamer deinitialized.\n"); release_face_analyzer(&app_ctx.face_ctx); release_yolo11(&app_ctx.yolo_ctx); deinit_post_process(); printf("INFO: RKNN models released.\n");


    printf("Exiting main (ret = %d)\n", ret);
    return ret;
}