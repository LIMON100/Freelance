#ifndef TRACKING_PIPELINE_HPP
#define TRACKING_PIPELINE_HPP

#include "config.hpp"
#include "utils.hpp" // This will now contain FrameData structs
#include "async_inference.hpp" // This now contains BoundedTSQueue
#include "ByteTrack/BYTETracker.h"

#include "utils/lut.hpp"
#include "utils/calibration.hpp"

#include <thread>
#include <atomic>
#include <memory>
#include <chrono>

class TrackingPipeline {
public:
    TrackingPipeline(const PipelineConfig& config);
    ~TrackingPipeline();
    void run();
    void stop();

private:
    void preprocess_worker_eo();
    void preprocess_worker_ir();
    void inference_worker_eo();
    void inference_worker_ir();
    void fusion_worker();
    void postprocess_worker();

    void release_resources();
    std::vector<byte_track::Object> detections_to_bytetrack_objects(
        const std::vector<NamedBbox>& bboxes, int frame_width, int frame_height);

    PipelineConfig m_config;
    std::atomic<bool> m_stop_flag;

    std::unique_ptr<AsyncModelInfer> m_model_eo;
    std::unique_ptr<AsyncModelInfer> m_model_ir;
    
    std::shared_ptr<BoundedTSQueue<SingleStreamFrameData>> m_preprocessed_queue_eo;
    std::shared_ptr<BoundedTSQueue<SingleStreamFrameData>> m_preprocessed_queue_ir;
    std::shared_ptr<BoundedTSQueue<SingleStreamFrameData>> m_results_queue_eo;
    std::shared_ptr<BoundedTSQueue<SingleStreamFrameData>> m_results_queue_ir;
    std::shared_ptr<BoundedTSQueue<FusedFrameData>> m_fused_queue;

    cv::VideoCapture m_capture_eo;
    cv::VideoCapture m_capture_ir;
    cv::VideoWriter m_video_writer;
    
    std::unique_ptr<byte_track::BYTETracker> m_tracker;
    cv::Mat m_boresight_matrix;

    HawkEyeCalibration m_calibration;

    std::chrono::duration<double> m_inference_time_eo;
    std::chrono::duration<double> m_inference_time_ir;
    size_t m_total_frames;
};

#endif // TRACKING_PIPELINE_HPP