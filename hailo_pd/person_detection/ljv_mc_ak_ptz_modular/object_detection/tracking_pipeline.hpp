// #ifndef TRACKING_PIPELINE_HPP
// #define TRACKING_PIPELINE_HPP

// #include "config.hpp"
// #include "utils.hpp"
// #include "async_inference.hpp"
// #include "ByteTrack/BYTETracker.h"
// #include <thread>
// #include <atomic>
// #include <chrono>

// // The Trajectory struct is no longer needed in this header.

// class TrackingPipeline {
// public:
//     // Constructor takes the configuration struct
//     TrackingPipeline(const PipelineConfig& config);
//     ~TrackingPipeline();

//     // Main entry point to start the pipeline
//     void run();
//     void stop();

// private:
//     // --- Worker thread functions ---
//     void preprocess_worker();
//     void inference_worker();
//     void postprocess_worker();

//     // --- Helper methods ---
//     std::vector<byte_track::Object> detections_to_bytetrack_objects(
//         const std::vector<NamedBbox>& bboxes, int frame_width, int frame_height);
//     void release_resources();

//     // --- State Members ---
//     PipelineConfig m_config;
//     std::atomic<bool> m_stop_flag;

//     // Asynchronous components
//     std::unique_ptr<AsyncModelInfer> m_model;
//     std::shared_ptr<BoundedTSQueue<PreprocessedFrameItem>> m_preprocessed_queue;
//     std::shared_ptr<BoundedTSQueue<InferenceOutputItem>> m_results_queue;

//     // Video I/O
//     cv::VideoCapture m_capture;
//     cv::VideoWriter m_video_writer;
    
//     // Tracking
//     std::unique_ptr<byte_track::BYTETracker> m_tracker;

//     // Performance metrics
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
#include <chrono> // <-- Add for time points

// --- NEW: A single, unified data structure for the pipeline ---
struct FrameData {
    // Core Data
    int frame_id;
    cv::Mat org_frame;
    cv::Mat resized_for_infer;
    cv::Mat affine_matrix;

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

    // Helper is now part of the class
    std::vector<byte_track::Object> detections_to_bytetrack_objects(
        const std::vector<NamedBbox>& bboxes, int frame_width, int frame_height);

    PipelineConfig m_config;
    std::atomic<bool> m_stop_flag;

    std::unique_ptr<AsyncModelInfer> m_model;
    
    // Queues now use the new FrameData struct
    std::shared_ptr<BoundedTSQueue<FrameData>> m_preprocessed_queue;
    std::shared_ptr<BoundedTSQueue<FrameData>> m_results_queue;

    cv::VideoCapture m_capture;
    cv::VideoWriter m_video_writer;
    std::unique_ptr<byte_track::BYTETracker> m_tracker;
    std::chrono::duration<double> m_inference_time;
    size_t m_total_frames;
};

#endif // TRACKING_PIPELINE_HPP