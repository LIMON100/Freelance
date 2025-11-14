// File: config.hpp
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>

// This struct holds all the configuration for the tracking pipeline.
struct PipelineConfig {
    
    bool use_live_stream = false; 

    // This is the video file that will be used when use_live_stream is false.
    std::string input_video_path = "./50ir.mp4"; 

    // Path for live camera input 
    std::string camera_device_path = "/dev/video1"; 

    // GStreamer pipeline string for live camera input
    // std::string gstreamer_pipeline =
    // "v4l2src device=/dev/video1 ! "
    // "video/x-raw,width=640,height=480 ! "
    // "deinterlace ! "
    // "videoconvert ! "
    // "video/x-raw,format=BGR ! " 
    // "queue ! "
    // "appsink name=sink drop=true max-buffers=1 emit-signals=true sync=false";
    
    // --- MODEL & PROCESSING CONFIGURATION ---
    std::string hef_path = "./y5s_person_drone.hef";
    int processing_width = 512;
    int processing_height = 512;
    int model_input_width = 640;
    int model_input_height = 640;
    size_t class_count = 2;     

    // --- FEATURE FLAGS ---
    bool enable_profiling_log = true;
    bool enable_global_stabilization = false; 
    bool enable_visualization = false;
    bool save_output_video = false;
    bool enable_center_crop = false; 
    int crop_size = 500; 
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