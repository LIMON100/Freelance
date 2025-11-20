// tracking_pipeline.hpp
#ifndef TRACKING_PIPELINE_HPP
#define TRACKING_PIPELINE_HPP

#include "config.hpp"
#include "utils.hpp"
#include "async_inference.hpp"
#include "ByteTrack/BYTETracker.h"
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>
#include "utils/calibration_data.hpp"

class TrackingPipeline {
public:
    TrackingPipeline(const PipelineConfig& config);
    ~TrackingPipeline();
    void run();
    void stop();

private:

    void capture_worker(const std::string& pipeline_str, std::shared_ptr<BoundedTSQueue<cv::Mat>> queue);
    void mirror_capture_worker(const std::string& pipeline_str,
                               std::shared_ptr<BoundedTSQueue<cv::Mat>> queue_eo,
                               std::shared_ptr<BoundedTSQueue<cv::Mat>> queue_ir);

    

    void preprocess_worker_eo();
    void preprocess_worker_ir();
    void inference_worker_eo();
    void inference_worker_ir();
    void fusion_worker();
    void fusion_and_inference_worker();
    void inference_worker_ir_polly();
    void postprocess_worker();

    cv::Mat assemble_patches(const std::vector<cv::Mat>& patches, std::vector<cv::Rect>& patch_locations_on_sheet);
    std::vector<cv::Mat> extract_unique_patches(const cv::Mat& image, const std::vector<cv::Rect>& rois_to_subtract, std::vector<cv::Point>& patch_origins_in_image);

    float calculate_adaptive_alpha(const cv::Mat& eo_frame);

    std::string create_gstreamer_pipeline(bool is_live, const std::string& source_path);
    
    void release_resources();
    std::vector<byte_track::Object> detections_to_bytetrack_objects(
        const std::vector<NamedBbox>& bboxes, int frame_width, int frame_height);

    // Add these new function declarations
    cv::Point2f transformIrToEo(const cv::Point2f& ir_point, 
                               double zoom, double focus);
    std::vector<cv::Point2f> transformIrToEo(const std::vector<cv::Point2f>& ir_points,
                                           double zoom, double focus);

    cv::Point2f transformEoToIr(const cv::Point2f& eo_point, 
                               double zoom, double focus);


    PipelineConfig m_config;
    std::atomic<bool> m_stop_flag;

    std::unique_ptr<AsyncModelInfer> m_model;

    
    std::shared_ptr<BoundedTSQueue<SingleStreamFrameData>> m_preprocessed_queue_eo;
    std::shared_ptr<BoundedTSQueue<SingleStreamFrameData>> m_preprocessed_queue_ir;
    std::shared_ptr<BoundedTSQueue<SingleStreamFrameData>> m_results_queue_eo;
    std::shared_ptr<BoundedTSQueue<SingleStreamFrameData>> m_results_queue_ir;
    std::shared_ptr<BoundedTSQueue<FusedFrameData>> m_fused_queue;

    std::shared_ptr<BoundedTSQueue<cv::Mat>> m_raw_frames_queue_eo;
    std::shared_ptr<BoundedTSQueue<cv::Mat>> m_raw_frames_queue_ir;

    cv::VideoCapture m_capture_eo;
    cv::VideoCapture m_capture_ir;
    cv::VideoWriter m_video_writer;
    
    std::unique_ptr<byte_track::BYTETracker> m_tracker;
    // cv::Mat m_boresight_matrix;

    std::chrono::duration<double> m_inference_time_eo;
    std::chrono::duration<double> m_inference_time_ir;
    size_t m_total_frames;

    CalibrationData m_calibration_data;

    std::chrono::high_resolution_clock::time_point m_pipeline_start_time;
    std::atomic<size_t> m_processed_frames_count{0}; // Use std::atomic for thread-safety
};

#endif // TRACKING_PIPELINE_HPP