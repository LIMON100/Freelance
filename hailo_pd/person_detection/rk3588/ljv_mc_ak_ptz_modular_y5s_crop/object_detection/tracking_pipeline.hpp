// // File: tracking_pipeline.hpp
// #ifndef TRACKING_PIPELINE_HPP
// #define TRACKING_PIPELINE_HPP

// #include "config.hpp"
// #include "utils.hpp" // For AppSinkData and GStreamer functions
// #include "async_inference.hpp"
// #include "ByteTrack/BYTETracker.h"
// #include <thread>
// #include <atomic>
// #include <chrono>
// #include <queue> // For BoundedTSQueue
// #include <memory> // For std::unique_ptr

// // --- GStreamer Include ---
// #include <gst/gst.h>
// // --- End GStreamer Include ---

// // --- Custom Frame Data Structure ---
// struct FrameData {
//     // Core Data
//     int frame_id; // Frame ID from GStreamer
//     cv::Mat org_frame; // The original frame captured by GStreamer for display
//     cv::Mat resized_for_infer; // Frame resized for model input
//     cv::Mat affine_matrix; // Transformation matrix (from stabilization/crop)
//     cv::Point crop_offset; // Offset if cropping was applied (GStreamer caps handle this now)

//     // GStreamer specific data
//     std::vector<char> gst_buffer; // Raw frame data from GStreamer
//     int gst_width;
//     int gst_height;
//     int gst_format; // e.g., GST_VIDEO_FORMAT_RGB

//     // Inference Output
//     std::vector<std::pair<uint8_t*, hailo_vstream_info_t>> output_data_and_infos;
//     std::vector<std::shared_ptr<uint8_t>> output_guards;

//     // Profiling Timestamps
//     std::chrono::high_resolution_clock::time_point t_capture_start;
//     std::chrono::high_resolution_clock::time_point t_preprocess_end;
//     std::chrono::high_resolution_clock::time_point t_inference_end;
//     std::chrono::high_resolution_clock::time_point t_postprocess_end;
// };
// // --- End Custom Frame Data Structure ---


// class TrackingPipeline {
// public:
//     TrackingPipeline(const PipelineConfig& config);
//     ~TrackingPipeline();
//     void run();
//     void stop();

// private:
//     // --- Worker thread functions ---
//     void capture_thread_worker(); // NEW: Replaces GStreamer logic in preprocess
//     void preprocess_worker();
//     void inference_worker();
//     void postprocess_worker();
    
//     void release_resources();
//     std::vector<byte_track::Object> detections_to_bytetrack_objects(
//         const std::vector<NamedBbox>& bboxes, int frame_width, int frame_height);

//     // --- ADD HELPER FUNCTIONS HERE ---
// private:
//     // Helper function to apply affine transformation to a point
//     cv::Point2f apply_affine_transform(const cv::Point2f& point, const cv::Mat& affine_matrix, 
//                                       const cv::Size& src_size, const cv::Size& dst_size);
    
//     // Helper function to transform entire bounding box
//     cv::Rect2f transform_bounding_box(const byte_track::Rect<float>& rect, const cv::Mat& affine_matrix,
//                                      const cv::Size& src_size, const cv::Size& dst_size);
//     // --- END HELPER FUNCTIONS ---

//     PipelineConfig m_config;
//     std::atomic<bool> m_stop_flag;

//     std::unique_ptr<AsyncModelInfer> m_model;
    
//     // --- NEW: A queue for raw frames from the capture thread ---
//     std::shared_ptr<BoundedTSQueue<cv::Mat>> m_raw_frames_queue;
    
//     std::shared_ptr<BoundedTSQueue<FrameData>> m_preprocessed_queue;
//     std::shared_ptr<BoundedTSQueue<FrameData>> m_results_queue;

//     // cv::VideoCapture is now only used for file input
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

// --- Custom Frame Data Structure ---
struct FrameData {
    int frame_id;
    cv::Mat org_frame;
    cv::Mat resized_for_infer;
    cv::Mat affine_matrix;
    cv::Point crop_offset;
    std::vector<std::pair<uint8_t*, hailo_vstream_info_t>> output_data_and_infos;
    std::vector<std::shared_ptr<uint8_t>> output_guards;
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
    // --- Worker thread functions ---
    void capture_thread_worker(); // <-- ADD THIS DECLARATION
    void preprocess_worker();
    void inference_worker();
    void postprocess_worker();
    void release_resources();

    std::vector<byte_track::Object> detections_to_bytetrack_objects(
        const std::vector<NamedBbox>& bboxes, int frame_width, int frame_height);

    // std::vector<byte_track::Object> detections_to_bytetrack_objects(
    //     const std::vector<NamedBbox>& bboxes, 
    //     int model_input_width, int model_input_height,
    //     int crop_width, int crop_height,
    //     const cv::Point& crop_offset);

    PipelineConfig m_config;
    std::atomic<bool> m_stop_flag;

    std::unique_ptr<AsyncModelInfer> m_model;
    
    // --- Queues ---
    std::shared_ptr<BoundedTSQueue<cv::Mat>> m_raw_frames_queue; // <-- ADD THIS DECLARATION
    std::shared_ptr<BoundedTSQueue<FrameData>> m_preprocessed_queue;
    std::shared_ptr<BoundedTSQueue<FrameData>> m_results_queue;

    cv::VideoCapture m_capture;
    cv::VideoWriter m_video_writer;
    std::unique_ptr<byte_track::BYTETracker> m_tracker;
    std::chrono::duration<double> m_inference_time;
    size_t m_total_frames;
};

#endif // TRACKING_PIPELINE_HPP