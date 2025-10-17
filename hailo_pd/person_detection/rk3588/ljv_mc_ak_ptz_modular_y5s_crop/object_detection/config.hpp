#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>

// This struct holds all the configuration for the tracking pipeline.
struct PipelineConfig {
    // --- SOURCE CONFIGURATION ---
    std::string input_video_path = "./30eo.mp4";
    std::string gstreamer_pipeline =
        "libcamerasrc ! "
        "videoconvert ! "
        "video/x-raw,format=BGR,width=640,height=480,framerate=30/1 ! "
        "queue max-size-buffers=1 leaky=downstream ! "
        "appsink drop=true max-buffers=1 async=false sync=false";

    // --- MODEL & PROCESSING CONFIGURATION ---
    std::string hef_path = "./y5s_person_drone.hef";
    int processing_width = 512;  // Set to -1 to use original video width
    int processing_height = 512; // Set to -1 to use original video height
    int model_input_width = 640;
    int model_input_height = 640;
    size_t class_count = 2;

    // --- FEATURE FLAGS ---
    bool use_live_stream = false;
    bool enable_profiling_log = true;
    bool enable_global_stabilization = false;
    bool enable_visualization = false;
    bool save_output_video = true;
    bool enable_center_crop = true; 
    int crop_size = 500;
    std::string output_video_path = "./processed_video.avi";

    // --- TRACKER CONFIGURATION ---
    float track_thresh = 0.5f;
    float high_thresh = 0.6f;
    float match_thresh = 0.8f;
    int track_buffer_frames = 30;
    float nms_iou_threshold = 0.6f;  // Stricter IoU for overlap suppression (0.5-0.7)
    float nms_score_threshold = 0.5f;

    // --- PTZ CONTROL CONFIGURATION ---
    float ptz_p_gain = 4.0f;
    float pixels_per_degree_at_current_zoom = 124.52f;
};

#endif // CONFIG_HPP
