// config.hpp
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>

struct PipelineConfig {
    bool use_live_stream = true;
    bool mirror_single_camera = true; 

    // --- SOURCE CONFIGURATION ---
    std::string eo_video_path = "./30eo.mp4"; // Example path for EO video
    std::string ir_video_path = "./30ir.mp4"; // Example path for IR video
    
    // Camera device paths (used if use_live_stream is true)
    std::string eo_device_path = "/dev/video1";
    std::string ir_device_path = "/dev/video0";

    // --- MODEL & PROCESSING CONFIGURATION ---
    // Using one model for both streams as requested
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
    bool save_output_video = true; 
    bool enable_center_crop = false; 
    int crop_size = 500;
    std::string output_video_path = "./processed_video.avi";

    // --- TRACKER CONFIGURATION ---
    float track_thresh = 0.5f;
    float high_thresh = 0.6f;
    float match_thresh = 0.8f;
    int track_buffer_frames = 30;

    // --- PTZ CONTROL CONFIGURATION ---
    float ptz_p_gain = 4.0f;
    float pixels_per_degree_at_current_zoom = 124.52f;
    
    // --- NEW CALIBRATION CONFIGURATION ---
    std::string intrinsics_zoom_path = "./calibration/intrinsics_zoom.json";
    std::string intrinsics_thermal_path = "./calibration/intrinsics_thermal.json";
    std::string extrinsics_path = "./calibration/extrinsics.json";
    std::string rig_meta_path = "./calibration/rig_meta.json";
    
    // Zoom/focus parameters
    bool has_zoom_control = false;
    double default_zoom = 300.0; // Middle of zoom range
    double default_focus = 30.0; // Middle of focus range
};

#endif // CONFIG_HPP