// #ifndef CONFIG_HPP
// #define CONFIG_HPP

// #include <string>

// // This struct holds all the configuration for the tracking pipeline.
// struct PipelineConfig {
//     // --- SOURCE CONFIGURATION ---
//     std::string input_video_path = "./30eo.mp4";
//     std::string gstreamer_pipeline =
//         "libcamerasrc ! "
//         "videoconvert ! "
//         "video/x-raw,format=BGR,width=640,height=480,framerate=30/1 ! "
//         "queue max-size-buffers=1 leaky=downstream ! "
//         "appsink drop=true max-buffers=1 async=false sync=false";

//     // --- MODEL & PROCESSING CONFIGURATION ---
//     std::string hef_path = "./y5s_person_drone.hef";
//     int processing_width = 512;  // Set to -1 to use original video width
//     int processing_height = 512; // Set to -1 to use original video height
//     int model_input_width = 640;
//     int model_input_height = 640;
//     size_t class_count = 2;

//     // --- FEATURE FLAGS ---
//     bool use_live_stream = false;
//     bool enable_profiling_log = true;
//     bool enable_global_stabilization = false;
//     bool enable_visualization = false;
//     bool save_output_video = true;
//     bool enable_center_crop = true; 
//     int crop_size = 500;
//     std::string output_video_path = "./processed_video.avi";

//     // --- TRACKER CONFIGURATION ---
//     float track_thresh = 0.5f;
//     float high_thresh = 0.6f;
//     float match_thresh = 0.8f;
//     int track_buffer_frames = 30;
//     float nms_iou_threshold = 0.6f;  // Stricter IoU for overlap suppression (0.5-0.7)
//     float nms_score_threshold = 0.5f;

//     // --- PTZ CONTROL CONFIGURATION ---
//     float ptz_p_gain = 4.0f;
//     float pixels_per_degree_at_current_zoom = 124.52f;
// };

// #endif // CONFIG_HPP



// File: config.hpp
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>

// This struct holds all the configuration for the tracking pipeline.
struct PipelineConfig {
    
    bool use_live_stream = false; 

    // This is the video file that will be used when use_live_stream is false.
    // Make sure this file exists in the same directory where you run the executable.
    std::string input_video_path = "./30eo.mp4"; 

    // This GStreamer pipeline is for the Raspberry Pi. It will be ignored for now.
    std::string gstreamer_pipeline =
        "libcamerasrc ! "
        "video/x-raw, format=YUY2, width=640, height=480, framerate=30/1 ! "
        "videoconvert ! "
        "video/x-raw, format=BGR, width=512, height=512 ! " // Note: BGR is often more compatible than RGB
        "queue max-size-buffers=2 leaky=downstream ! "
        "appsink name=sink drop=true max-buffers=2 emit-signals=true sync=false";

    // --- MODEL & PROCESSING CONFIGURATION ---
    std::string hef_path = "./y5s_person_drone.hef";
    int processing_width = 512;
    int processing_height = 512;
    int model_input_width = 640;
    int model_input_height = 640;
    size_t class_count = 2;      // Example: If your model detects only 'person'

    // --- FEATURE FLAGS ---
    // bool use_live_stream = true; // Set to true to use GStreamer pipeline
    bool enable_profiling_log = true;
    bool enable_global_stabilization = false; // Stabilization might be tricky with GStreamer buffers, can disable for now.
    bool enable_visualization = false;
    bool save_output_video = true;
    bool enable_center_crop = false; // For simplicity, let's disable cropping initially. GStreamer handles sizing.
    int crop_size = 500; // Not used if enable_center_crop is false
    std::string output_video_path = "./processed_video.mp4";

    // --- NEW DEBUGGING FLAG ---
    bool save_processed_frames = false; 
    std::string processed_frames_dir = "./processed_frames";

    // --- TRACKER CONFIGURATION ---
    float track_thresh = 0.4f;     // Lowered for potential tracking of weaker detections
    float high_thresh = 0.5f;      // High confidence for initial activation
    float match_thresh = 0.8f;     // IoU for matching tracks
    int track_buffer_frames = 30;  // How long to keep lost tracks
    float nms_iou_threshold = 0.4f; // NMS for detections themselves
    float nms_score_threshold = 0.4f; // NMS for detections themselves

    // --- PTZ CONTROL CONFIGURATION ---
    float ptz_p_gain = 4.0f;
    float pixels_per_degree_at_current_zoom = 124.52f;
};

#endif // CONFIG_HPP