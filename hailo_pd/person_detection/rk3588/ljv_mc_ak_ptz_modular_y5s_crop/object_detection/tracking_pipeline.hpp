// #ifndef TRACKING_PIPELINE_HPP
// #define TRACKING_PIPELINE_HPP

// #include "config.hpp"
// #include "utils.hpp"
// #include "async_inference.hpp"
// #include "ByteTrack/BYTETracker.h"
// #include <thread>
// #include <atomic>
// #include <chrono> 


// struct FrameData {
//     // Core Data
//     int frame_id;
//     cv::Mat org_frame;
//     cv::Mat resized_for_infer;
//     cv::Mat affine_matrix;
//     cv::Point crop_offset;

//     // Inference Output
//     std::vector<std::pair<uint8_t*, hailo_vstream_info_t>> output_data_and_infos;
//     std::vector<std::shared_ptr<uint8_t>> output_guards;

//     // Profiling Timestamps
//     std::chrono::high_resolution_clock::time_point t_capture_start;
//     std::chrono::high_resolution_clock::time_point t_preprocess_end;
//     std::chrono::high_resolution_clock::time_point t_inference_end;
//     std::chrono::high_resolution_clock::time_point t_postprocess_end;
// };

// class TrackingPipeline {
// public:
//     TrackingPipeline(const PipelineConfig& config);
//     ~TrackingPipeline();
//     void run();
//     void stop();

// private:
//     void preprocess_worker();
//     void inference_worker();
//     void postprocess_worker();
//     void release_resources();

//     // Helper is now part of the class
//     std::vector<byte_track::Object> detections_to_bytetrack_objects(
//         const std::vector<NamedBbox>& bboxes, int frame_width, int frame_height);

//     PipelineConfig m_config;
//     std::atomic<bool> m_stop_flag;

//     std::unique_ptr<AsyncModelInfer> m_model;
    
//     // Queues now use the new FrameData struct
//     std::shared_ptr<BoundedTSQueue<FrameData>> m_preprocessed_queue;
//     std::shared_ptr<BoundedTSQueue<FrameData>> m_results_queue;

//     cv::VideoCapture m_capture;
//     cv::VideoWriter m_video_writer;
//     std::unique_ptr<byte_track::BYTETracker> m_tracker;
//     std::chrono::duration<double> m_inference_time;
//     size_t m_total_frames;
// };

// #endif // TRACKING_PIPELINE_HPP



#ifndef TRACKING_PIPELINE_HPP
#define TRACKING_PIPELINE_HPP

#include "config.hpp"
#include "utils.hpp"
#include "async_inference.hpp"
#include "ByteTrack/BYTETracker.h"
#include <thread>
#include <atomic>
#include <chrono> 

// NEW: GStreamer Includes
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

struct FrameData {
    // Core Data
    int frame_id;
    cv::Mat org_frame;
    cv::Mat resized_for_infer;
    cv::Mat affine_matrix;
    cv::Point crop_offset;

    // Inference Output
    std::vector<std::pair<uint8_t*, hailo_vstream_info_t>> output_data_and_infos;
    std::vector<std::shared_ptr<uint8_t>> output_guards;

    // Profiling Timestamps
    std::chrono::high_resolution_clock::time_point t_capture_start;
    std::chrono::high_resolution_clock::time_point t_preprocess_end;
    std::chrono::high_resolution_clock::time_point t_inference_end;
    std::chrono::high_resolution_clock::time_point t_postprocess_end;
};

class TrackingPipeline {
public:
    TrackingPipeline(const PipelineConfig& config);
    ~TrackingPipeline();
    void run();
    void stop();

private:
    void preprocess_worker();
    void inference_worker();
    void postprocess_worker();
    void release_resources();

    std::vector<byte_track::Object> detections_to_bytetrack_objects(
        const std::vector<NamedBbox>& bboxes, int frame_width, int frame_height);

    PipelineConfig m_config;
    std::atomic<bool> m_stop_flag;

    std::unique_ptr<AsyncModelInfer> m_model;
    std::shared_ptr<BoundedTSQueue<FrameData>> m_preprocessed_queue;
    std::shared_ptr<BoundedTSQueue<FrameData>> m_results_queue;

    // --- REPLACED VideoCapture with GStreamer elements ---
    GstElement* m_gst_pipeline;
    GstElement* m_appsink;
    GMainLoop* m_main_loop;
    std::thread m_main_loop_thread;
    // --- END REPLACEMENT ---
    
    cv::VideoWriter m_video_writer;
    std::unique_ptr<byte_track::BYTETracker> m_tracker;
    std::chrono::duration<double> m_inference_time;
    size_t m_total_frames;
};

#endif // TRACKING_PIPELINE_HPP