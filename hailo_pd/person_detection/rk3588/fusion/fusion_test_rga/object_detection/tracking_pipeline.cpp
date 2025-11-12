#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>

#include "tracking_pipeline.hpp"
#include "global_stabilizer.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "utils/calibration_data.hpp"


TrackingPipeline::TrackingPipeline(const PipelineConfig& config)
    : m_config(config),
      m_stop_flag(false),
      m_total_frames(0) // Will be set from EO video if applicable
{
    // Initialize all queues with larger buffers for better throughput
    m_raw_frames_queue_eo = std::make_shared<BoundedTSQueue<cv::Mat>>(5);
    m_raw_frames_queue_ir = std::make_shared<BoundedTSQueue<cv::Mat>>(5);
    m_preprocessed_queue_eo = std::make_shared<BoundedTSQueue<SingleStreamFrameData>>(10);
    m_preprocessed_queue_ir = std::make_shared<BoundedTSQueue<SingleStreamFrameData>>(10);
    m_results_queue_eo = std::make_shared<BoundedTSQueue<SingleStreamFrameData>>(10);
    m_results_queue_ir = std::make_shared<BoundedTSQueue<SingleStreamFrameData>>(10);
    m_fused_queue = std::make_shared<BoundedTSQueue<FusedFrameData>>(10);

    // Load calibration data
    if (!loadCalibrationData(m_config, m_calibration_data)) {
        throw std::runtime_error("FATAL: Failed to load calibration data.");
    }
    std::cout << "[INFO] Successfully loaded calibration data." << std::endl;

    // Initialize Hailo models
    m_model_eo = std::make_unique<AsyncModelInfer>(m_config.hef_path);
    std::shared_ptr<hailort::VDevice> shared_vdevice = m_model_eo->get_vdevice();
    m_model_ir = std::make_unique<AsyncModelInfer>(shared_vdevice, m_config.hef_path);
    
    // Get total frames from the EO video file for the progress bar
    if (!m_config.use_live_stream) {
        cv::VideoCapture temp_cap(m_config.eo_video_path);
        if (temp_cap.isOpened()) {
            m_total_frames = temp_cap.get(cv::CAP_PROP_FRAME_COUNT);
        }
        temp_cap.release();
    }
    
    m_tracker = std::make_unique<byte_track::BYTETracker>(30, m_config.track_buffer_frames);
}

std::string TrackingPipeline::create_gstreamer_pipeline(bool is_live, const std::string& source_path) {
    std::string source_element;
    if (is_live) {
        source_element = "v4l2src device=" + source_path;
    } else {
        source_element = "filesrc location=" + source_path + " ! qtdemux ! h264parse ! mppvideodec";
    }

    return source_element + " ! rgaconvert ! " +
           "video/x-raw,format=BGR,width=" + std::to_string(m_config.processing_width) +
           ",height=" + std::to_string(m_config.processing_height) + " ! " +
           "queue ! appsink name=sink drop=true max-buffers=1";
}

// Destructor
TrackingPipeline::~TrackingPipeline() {
    stop();
    release_resources();
}



cv::Point2f TrackingPipeline::transformIrToEo(const cv::Point2f& ir_point, double zoom, double focus) 
{

    if (!m_calibration_data.is_loaded) {
        throw std::runtime_error("FATAL: Attempting to use calibration data that wasn't loaded");
    }
    
    // Get current zoom-dependent intrinsics
    double fx, fy, cx, cy;
    bilinearInterpolate(m_calibration_data.zoom_lut, zoom, focus, fx, fy, cx, cy);
    
    // Create camera matrix for current zoom
    cv::Mat K_zoom = (cv::Mat_<double>(3, 3) << fx, 0, cx, 0, fy, cy, 0, 0, 1);
    
    // Undistort IR point
    std::vector<cv::Point2f> ir_points = {ir_point};
    std::vector<cv::Point2f> undistorted_ir;
    cv::undistortPoints(ir_points, undistorted_ir, 
                       m_calibration_data.thermal_intrinsics.K,
                       m_calibration_data.thermal_intrinsics.dist);
    
    // Back-project to 3D ray (assuming point at unit distance)
    cv::Mat ray = (cv::Mat_<double>(3, 1) << undistorted_ir[0].x, undistorted_ir[0].y, 1.0);
    
    // Transform ray to EO camera coordinates
    cv::Mat ray_eo = m_calibration_data.extrinsics.R * ray + m_calibration_data.extrinsics.t;
    
    // Project to EO image coordinates
    cv::Mat projected = K_zoom * ray_eo;
    double w = projected.at<double>(2, 0);
    
    return cv::Point2f(projected.at<double>(0, 0) / w, projected.at<double>(1, 0) / w);
}

std::vector<cv::Point2f> TrackingPipeline::transformIrToEo(const std::vector<cv::Point2f>& ir_points,
                                                         double zoom, double focus) {
    std::vector<cv::Point2f> eo_points;
    eo_points.reserve(ir_points.size());
    
    for (const auto& pt : ir_points) {
        eo_points.push_back(transformIrToEo(pt, zoom, focus));
    }
    
    return eo_points;
}


cv::Point2f TrackingPipeline::transformEoToIr(const cv::Point2f& eo_point, 
                                             double zoom, double focus) {
    if (!m_calibration_data.is_loaded) {
        throw std::runtime_error("FATAL: Attempting to use calibration data that wasn't loaded");
    }

    double fx, fy, cx, cy;
    bilinearInterpolate(m_calibration_data.zoom_lut, zoom, focus, fx, fy, cx, cy);
    cv::Mat K_zoom = (cv::Mat_<double>(3, 3) << fx, 0, cx, 0, fy, cy, 0, 0, 1);
    
    cv::Mat eo_point_2d = (cv::Mat_<double>(3,1) << eo_point.x, eo_point.y, 1.0);
    cv::Mat ray_in_zoom_space = K_zoom.inv() * eo_point_2d;


    cv::Mat R_inv = m_calibration_data.extrinsics.R.t(); // Transpose of rotation matrix is its inverse
    cv::Mat t_transformed = -R_inv * m_calibration_data.extrinsics.t;
    cv::Mat ray_in_thermal_space = R_inv * ray_in_zoom_space + t_transformed;

    // 4. Project ray onto Thermal image plane
    cv::Mat projected_homogeneous = m_calibration_data.thermal_intrinsics.K * ray_in_thermal_space;
    double w = projected_homogeneous.at<double>(2, 0);

    cv::Point2f distorted_point(projected_homogeneous.at<double>(0, 0) / w,
                                projected_homogeneous.at<double>(1, 0) / w);

                                
    return distorted_point;
}


void TrackingPipeline::stop() {
    m_stop_flag.store(true);
    m_preprocessed_queue_eo->stop();
    m_preprocessed_queue_ir->stop();
    m_results_queue_eo->stop();
    m_results_queue_ir->stop();
    m_fused_queue->stop();
}

void TrackingPipeline::release_resources() {
    if (m_config.save_output_video) m_video_writer.release();
    m_capture_eo.release();
    m_capture_ir.release();
    if (m_config.enable_visualization) cv::destroyAllWindows();
}

// Main run method launches all parallel threads
void TrackingPipeline::run() {

    // Create the pipeline strings based on the config
    std::string eo_pipeline_str = create_gstreamer_pipeline(m_config.use_live_stream, 
                                                            m_config.use_live_stream ? m_config.eo_device_path : m_config.eo_video_path);
    std::string ir_pipeline_str = create_gstreamer_pipeline(m_config.use_live_stream,
                                                            m_config.use_live_stream ? m_config.ir_device_path : m_config.ir_video_path);

    std::cout << "[GSTREAMER EO] " << eo_pipeline_str << std::endl;
    std::cout << "[GSTREAMER IR] " << ir_pipeline_str << std::endl;

    // Launch all threads
    auto capture_eo_thread = std::async(std::launch::async, &TrackingPipeline::capture_worker, this, eo_pipeline_str, m_raw_frames_queue_eo);
    auto capture_ir_thread = std::async(std::launch::async, &TrackingPipeline::capture_worker, this, ir_pipeline_str, m_raw_frames_queue_ir);

    // FIX: Removed unused t_start and t_end variables
    auto preprocess_eo_thread = std::async(std::launch::async, &TrackingPipeline::preprocess_worker_eo, this);
    auto preprocess_ir_thread = std::async(std::launch::async, &TrackingPipeline::preprocess_worker_ir, this);
    auto inference_eo_thread = std::async(std::launch::async, &TrackingPipeline::inference_worker_eo, this);
    auto inference_ir_thread = std::async(std::launch::async, &TrackingPipeline::inference_worker_ir, this);
    auto fusion_thread = std::async(std::launch::async, &TrackingPipeline::fusion_worker, this);
    auto postprocess_thread = std::async(std::launch::async, &TrackingPipeline::postprocess_worker, this);

    postprocess_thread.wait();
    stop(); 
    
    preprocess_eo_thread.wait();
    preprocess_ir_thread.wait();
    inference_eo_thread.wait();
    inference_ir_thread.wait();
    fusion_thread.wait();

    std::cout << "Pipeline finished." << std::endl;
}


// Generic GStreamer Capture Worker
void TrackingPipeline::capture_worker(const std::string& pipeline_str, std::shared_ptr<BoundedTSQueue<cv::Mat>> queue) {
    gst_init(nullptr, nullptr);
    GError *error = nullptr;
    GstElement *pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    
    if (!pipeline || error) { /* handle error */ stop(); return; }

    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    if (!appsink) { /* handle error */ stop(); return; }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    while (!m_stop_flag.load()) {
        GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
        if (!sample) {
            if (gst_app_sink_is_eos(GST_APP_SINK(appsink))) {
                std::cout << "GStreamer: End-Of-Stream reached." << std::endl;
            } else {
                std::cerr << "GStreamer: Error pulling sample." << std::endl;
            }
            break;
        }

        GstBuffer *buffer = gst_sample_get_buffer(sample);
        GstCaps *caps = gst_sample_get_caps(sample);
        GstVideoInfo video_info;
        if (!buffer || !caps || !gst_video_info_from_caps(&video_info, caps)) {
            gst_sample_unref(sample);
            continue;
        }

        GstMapInfo map_info;
        if (gst_buffer_map(buffer, &map_info, GST_MAP_READ)) {
            cv::Mat frame_wrapper(
                GST_VIDEO_INFO_HEIGHT(&video_info),
                GST_VIDEO_INFO_WIDTH(&video_info),
                CV_8UC3, // We requested BGR from the pipeline
                map_info.data
            );
            queue->push(frame_wrapper.clone());
            gst_buffer_unmap(buffer, &map_info);
        }
        gst_sample_unref(sample);
    }

    // Cleanup
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(appsink);
    gst_object_unref(pipeline);
    // Do not call gst_deinit() here if another pipeline is running
    
    queue->stop(); // Signal to the next stage that this stream has ended
}


// SIMPLIFIED Preprocess Worker for EO
void TrackingPipeline::preprocess_worker_eo() {
    int frame_counter = 0;
    while (!m_stop_flag.load()) {
        cv::Mat processed_frame;
        if (!m_raw_frames_queue_eo->pop(processed_frame)) break;

        SingleStreamFrameData item;
        item.frame_id = frame_counter++;
        item.t_capture_start = std::chrono::high_resolution_clock::now();
        
        // The frame from GStreamer is already the correct processing size (e.g., 512x512)
        item.org_frame = processed_frame.clone();

        // The only remaining task is resizing to the model's input size.
        // No stabilization or cropping logic is needed here anymore.
        cv::resize(processed_frame, item.resized_for_infer, 
                   cv::Size(m_config.model_input_width, m_config.model_input_height), 
                   0, 0, cv::INTER_LINEAR);
        
        // Since stabilization is disabled, the affine matrix is identity.
        item.affine_matrix = cv::Mat::eye(2, 3, CV_64F);
        item.crop_offset = cv::Point(0, 0);

        item.t_preprocess_end = std::chrono::high_resolution_clock::now();
        m_preprocessed_queue_eo->push(std::move(item));
    }
    m_preprocessed_queue_eo->stop();
}

// void TrackingPipeline::preprocess_worker_ir() {
//     int frame_counter = 0;
//     while (!m_stop_flag.load()) {
//         cv::Mat gstreamer_frame;
//         if (!m_raw_frames_queue_ir->pop(gstreamer_frame)) break;

//         SingleStreamFrameData item;
//         item.frame_id = frame_counter++;
//         item.t_capture_start = std::chrono::high_resolution_clock::now();
        
//         cv::Mat clean_frame;
//         cv::cvtColor(gstreamer_frame, clean_frame, cv::COLOR_BGR2BGR);

//         item.org_frame = clean_frame.clone(); // Use the clean frame

//         // The only remaining task is resizing to the model's input size.
//         cv::resize(clean_frame, item.resized_for_infer, 
//                    cv::Size(m_config.model_input_width, m_config.model_input_height), 
//                    0, 0, cv::INTER_LINEAR);
        
//         item.affine_matrix = cv::Mat::eye(2, 3, CV_64F);
//         item.crop_offset = cv::Point(0, 0);

//         item.t_preprocess_end = std::chrono::high_resolution_clock::now();
//         m_preprocessed_queue_ir->push(std::move(item));
//     }
//     m_preprocessed_queue_ir->stop();
// }

void TrackingPipeline::preprocess_worker_ir() {
    int frame_counter = 0;
    while (!m_stop_flag.load()) {
        cv::Mat processed_frame;
        if (!m_raw_frames_queue_ir->pop(processed_frame)) break;

        SingleStreamFrameData item;
        item.frame_id = frame_counter++;
        item.t_capture_start = std::chrono::high_resolution_clock::now();

        item.org_frame = processed_frame.clone();
        cv::resize(processed_frame, item.resized_for_infer, 
                   cv::Size(m_config.model_input_width, m_config.model_input_height), 
                   0, 0, cv::INTER_LINEAR);

        item.affine_matrix = cv::Mat::eye(2, 3, CV_64F);
        item.crop_offset = cv::Point(0, 0);

        item.t_preprocess_end = std::chrono::high_resolution_clock::now();
        m_preprocessed_queue_ir->push(std::move(item));
    }
    m_preprocessed_queue_ir->stop();
}

void TrackingPipeline::inference_worker_eo() {
    while (!m_stop_flag.load()) {
        SingleStreamFrameData item;
        if (!m_preprocessed_queue_eo->pop(item)) break;
        m_model_eo->infer(std::make_shared<cv::Mat>(item.resized_for_infer),
            [item = std::move(item), queue = m_results_queue_eo](const auto&, const auto& outs, const auto& guards) mutable {
                item.t_inference_end = std::chrono::high_resolution_clock::now();
                item.output_data_and_infos = outs;
                item.output_guards = guards;
                queue->push(std::move(item));
            });
    }
    m_results_queue_eo->stop();
}

void TrackingPipeline::inference_worker_ir() {
    while (!m_stop_flag.load()) {
        SingleStreamFrameData item;
        if (!m_preprocessed_queue_ir->pop(item)) break;
        m_model_ir->infer(std::make_shared<cv::Mat>(item.resized_for_infer),
            [item = std::move(item), queue = m_results_queue_ir](const auto&, const auto& outs, const auto& guards) mutable {
                item.t_inference_end = std::chrono::high_resolution_clock::now();
                item.output_data_and_infos = outs;
                item.output_guards = guards;
                queue->push(std::move(item));
            });
    }
    m_results_queue_ir->stop();
}

// In fusion_worker() method
// void TrackingPipeline::fusion_worker() {
//     while (!m_stop_flag.load()) {
//         SingleStreamFrameData eo_item, ir_item;
//         if (!m_results_queue_eo->pop(eo_item) || !m_results_queue_ir->pop(ir_item)) {
//             break;
//         }
        
//         FusedFrameData fused_item;
//         fused_item.frame_id = eo_item.frame_id;
//         fused_item.display_frame = eo_item.org_frame.clone();
        
//         // Copy timestamps
//         fused_item.t_eo_capture_start = eo_item.t_capture_start;
//         fused_item.t_eo_preprocess_end = eo_item.t_preprocess_end;
//         fused_item.t_eo_inference_end = eo_item.t_inference_end;
//         fused_item.t_ir_capture_start = ir_item.t_capture_start;
//         fused_item.t_ir_preprocess_end = ir_item.t_preprocess_end;
//         fused_item.t_ir_inference_end = ir_item.t_inference_end;
        
//         // Get current zoom/focus (use defaults if not available)
//         double current_zoom = m_config.default_zoom;
//         double current_focus = m_config.default_focus;
        
//         // Get EO detections
//         int eo_proc_width = m_config.enable_center_crop ? m_config.crop_size : eo_item.org_frame.cols;
//         int eo_proc_height = m_config.enable_center_crop ? m_config.crop_size : eo_item.org_frame.rows;
//         auto eo_bboxes = parse_nms_data(eo_item.output_data_and_infos[0].first, m_config.class_count);
//         std::vector<byte_track::Object> eo_objects = detections_to_bytetrack_objects(eo_bboxes, eo_proc_width, eo_proc_height);
//         for (auto& obj : eo_objects) {
//             obj.rect.x() += eo_item.crop_offset.x;
//             obj.rect.y() += eo_item.crop_offset.y;
//         }
        
//         // Get IR detections and transform them
//         int ir_proc_width = m_config.enable_center_crop ? m_config.crop_size : ir_item.org_frame.cols;
//         int ir_proc_height = m_config.enable_center_crop ? m_config.crop_size : ir_item.org_frame.rows;
//         auto ir_bboxes = parse_nms_data(ir_item.output_data_and_infos[0].first, m_config.class_count);
//         std::vector<byte_track::Object> ir_objects_raw = detections_to_bytetrack_objects(ir_bboxes, ir_proc_width, ir_proc_height);
        
//         std::vector<byte_track::Object> ir_objects_projected;
//         for (const auto& ir_obj_raw : ir_objects_raw) {
//             byte_track::Object ir_obj = ir_obj_raw;
//             ir_obj.rect.x() += ir_item.crop_offset.x;
//             ir_obj.rect.y() += ir_item.crop_offset.y;
            
//             // Get IR corners in full frame coordinates
//             std::vector<cv::Point2f> ir_corners = {
//                 cv::Point2f(ir_obj.rect.tl_x(), ir_obj.rect.tl_y()),
//                 cv::Point2f(ir_obj.rect.br_x(), ir_obj.rect.br_y())
//             };
            
//             // Transform to EO coordinates
//             std::vector<cv::Point2f> eo_corners = this->transformIrToEo(ir_corners, current_zoom, current_focus);
            
//             // Create projected object
//             if (eo_corners.size() >= 2) {
//                 ir_objects_projected.emplace_back(
//                     byte_track::Rect<float>(eo_corners[0].x, eo_corners[0].y, 
//                                           eo_corners[1].x - eo_corners[0].x, 
//                                           eo_corners[1].y - eo_corners[0].y),
//                     ir_obj.label,
//                     ir_obj.prob
//                 );
//             }
//         }
        
//         // Fusion logic (same as before)
//         const float fusion_iou_thresh = 0.3f;
//         std::vector<bool> eo_matched(eo_objects.size(), false);
//         std::vector<bool> ir_matched(ir_objects_projected.size(), false);
        
//         // Find high-confidence fused detections
//         for (size_t i = 0; i < eo_objects.size(); ++i) {
//             for (size_t j = 0; j < ir_objects_projected.size(); ++j) {
//                 if (ir_matched[j]) continue;
                
//                 if (eo_objects[i].rect.calcIoU(ir_objects_projected[j].rect) > fusion_iou_thresh) {
//                     // Average the boxes
//                     float avg_x = (eo_objects[i].rect.x() + ir_objects_projected[j].rect.x()) / 2.0f;
//                     float avg_y = (eo_objects[i].rect.y() + ir_objects_projected[j].rect.y()) / 2.0f;
//                     float avg_w = (eo_objects[i].rect.width() + ir_objects_projected[j].rect.width()) / 2.0f;
//                     float avg_h = (eo_objects[i].rect.height() + ir_objects_projected[j].rect.height()) / 2.0f;
                    
//                     float fused_score = std::max(eo_objects[i].prob, ir_objects_projected[j].prob);
                    
//                     fused_item.fused_detections.emplace_back(
//                         byte_track::Rect<float>(avg_x, avg_y, avg_w, avg_h),
//                         eo_objects[i].label,
//                         fused_score
//                     );
                    
//                     eo_matched[i] = true;
//                     ir_matched[j] = true;
//                     break;
//                 }
//             }
//         }
        
//         // Add unmatched high-confidence detections
//         const float single_source_confidence_thresh = 0.55f;
        
//         for (size_t i = 0; i < eo_objects.size(); ++i) {
//             if (!eo_matched[i] && eo_objects[i].prob > single_source_confidence_thresh) {
//                 fused_item.fused_detections.push_back(eo_objects[i]);
//             }
//         }
        
//         for (size_t j = 0; j < ir_objects_projected.size(); ++j) {
//             if (!ir_matched[j] && ir_objects_projected[j].prob > single_source_confidence_thresh) {
//                 fused_item.fused_detections.push_back(ir_objects_projected[j]);
//             }
//         }
        
//         fused_item.t_fusion_end = std::chrono::high_resolution_clock::now();
//         m_fused_queue->push(std::move(fused_item));
//     }
//     m_fused_queue->stop();
// }


void TrackingPipeline::fusion_worker() {
    while (!m_stop_flag.load()) {
        SingleStreamFrameData eo_item, ir_item;

        // Wait for a synchronized pair of frames from both inference workers.
        // If either stream ends, the pop will fail, and we'll break the loop.
        if (!m_results_queue_eo->pop(eo_item) || !m_results_queue_ir->pop(ir_item)) {
            break;
        }

        // Prepare the output data structure for this frame pair.
        FusedFrameData fused_item;
        fused_item.frame_id = eo_item.frame_id; // Use EO frame ID as the reference for the fused frame.
        fused_item.display_frame = eo_item.org_frame.clone(); // Use the original EO frame for display.

        // Copy all timestamps from both streams for detailed profiling.
        fused_item.t_eo_capture_start = eo_item.t_capture_start;
        fused_item.t_eo_preprocess_end = eo_item.t_preprocess_end;
        fused_item.t_eo_inference_end = eo_item.t_inference_end;
        fused_item.t_ir_capture_start = ir_item.t_capture_start;
        fused_item.t_ir_preprocess_end = ir_item.t_preprocess_end;
        fused_item.t_ir_inference_end = ir_item.t_inference_end;

        // --- EO->THERMAL HANDOVER & SCORE FUSION STRATEGY ---

        // Get current zoom/focus from the camera. For this demo, we use default values from the config.
        // In a real system, you would get these values from the camera's SDK each frame.
        double current_zoom = m_config.default_zoom;
        double current_focus = m_config.default_focus;

        // 1. Get native EO detections (Primary Sensor) and convert to full frame coordinates.
        int eo_proc_width = m_config.enable_center_crop ? m_config.crop_size : eo_item.org_frame.cols;
        int eo_proc_height = m_config.enable_center_crop ? m_config.crop_size : eo_item.org_frame.rows;
        auto eo_bboxes = parse_nms_data(eo_item.output_data_and_infos[0].first, m_config.class_count);
        std::vector<byte_track::Object> eo_objects = detections_to_bytetrack_objects(eo_bboxes, eo_proc_width, eo_proc_height);
        for (auto& obj : eo_objects) {
            obj.rect.x() += eo_item.crop_offset.x;
            obj.rect.y() += eo_item.crop_offset.y;
        }

        // 2. Get native IR detections (Secondary Sensor) and convert to full frame coordinates.
        int ir_proc_width = m_config.enable_center_crop ? m_config.crop_size : ir_item.org_frame.cols;
        int ir_proc_height = m_config.enable_center_crop ? m_config.crop_size : ir_item.org_frame.rows;
        auto ir_bboxes = parse_nms_data(ir_item.output_data_and_infos[0].first, m_config.class_count);
        std::vector<byte_track::Object> ir_objects = detections_to_bytetrack_objects(ir_bboxes, ir_proc_width, ir_proc_height);
        for (auto& obj : ir_objects) {
            obj.rect.x() += ir_item.crop_offset.x;
            obj.rect.y() += ir_item.crop_offset.y;
        }

        // 3. For each EO detection, search for a corresponding IR detection to validate and boost its score.
        const float search_roi_size = 80.0f; // Search radius in pixels (e.g., +/- 80px), as suggested in the doc.
        const float alpha = 0.7;             // Weight for EO score (e.g., for daytime). Can be scheduled.

        // This loop modifies the `eo_objects` vector in place.
        for (auto& eo_obj : eo_objects) {
            // Handover: Project the EO detection's center point into the IR image space.
            cv::Point2f eo_center(eo_obj.rect.x() + eo_obj.rect.width() / 2.0f,
                                  eo_obj.rect.y() + eo_obj.rect.height() / 2.0f);
            
            cv::Point2f projected_ir_center = transformEoToIr(eo_center, current_zoom, current_focus);

            // Seed: Search for a matching IR detection within the defined ROI around the projected center.
            byte_track::Object* best_ir_match = nullptr;
            
            // Define the square search Region of Interest (ROI) in the IR image.
            cv::Rect2f search_roi(projected_ir_center.x - search_roi_size / 2.0f,
                                  projected_ir_center.y - search_roi_size / 2.0f,
                                  search_roi_size, search_roi_size);

            for (auto& ir_obj : ir_objects) {
                // Check if the center of the current IR object falls within our search ROI.
                cv::Point2f ir_center(ir_obj.rect.x() + ir_obj.rect.width() / 2.0f,
                                      ir_obj.rect.y() + ir_obj.rect.height() / 2.0f);

                if (search_roi.contains(ir_center)) {
                     // We found a potential match in the secondary sensor.
                     // A simple strategy is to take the first one found.
                     // A more advanced strategy could find the one with the highest score or closest center.
                     best_ir_match = &ir_obj;
                     break; // Found a match, no need to search further for this EO object.
                }
            }

            // Score Fusion: If a corresponding IR detection was found, update the EO object's score.
            if (best_ir_match != nullptr) {
                // Apply the score fusion formula from the document.
                eo_obj.prob = alpha * eo_obj.prob + (1.0f - alpha) * best_ir_match->prob;
            }
        }

        // 4. The final list of detections to be sent to the tracker consists of the original EO detections.
        //    The key difference is that their confidence scores (`prob`) may have been boosted
        //    if they were confirmed by the thermal sensor.
        //    The geometry (bounding box) is still determined by the primary EO sensor.
        fused_item.fused_detections = eo_objects;
        
        // --- END OF FUSION LOGIC ---
        
        fused_item.t_fusion_end = std::chrono::high_resolution_clock::now();
        m_fused_queue->push(std::move(fused_item));
    }
    m_fused_queue->stop();
}

void TrackingPipeline::postprocess_worker() {
    // --- INITIAL SETUP ---
    bool video_writer_initialized = false;
    if (m_config.enable_visualization) {
        cv::namedWindow("Object Detection", cv::WINDOW_AUTOSIZE);
    }
    
    auto prev_time = std::chrono::high_resolution_clock::now();
    size_t frame_index = 0;


    // --- MAIN PROCESSING LOOP ---
    while (!m_stop_flag.load()) {
        if (!m_config.use_live_stream) {
            show_progress_helper(frame_index, m_total_frames);
        }
        
        FusedFrameData item;
        if (!m_fused_queue->pop(item)) {
            break; // Exit if the queue is stopped and empty
        }

        // Get a reference to the frame we will draw on. This is the full-resolution EO frame.
        auto& frame_to_draw = item.display_frame;
        
        // Initialize VideoWriter on the first valid frame
        if (m_config.save_output_video && !video_writer_initialized && !frame_to_draw.empty()) {
            // double fps = m_capture_eo.get(cv::CAP_PROP_FPS);
            double fps = 30.0;
            init_video_writer(m_config.output_video_path, m_video_writer, fps, frame_to_draw.cols, frame_to_draw.rows);
            video_writer_initialized = true;
        }

        // Run the tracker on the fused detections
        std::vector<std::shared_ptr<byte_track::STrack>> tracked_objects = m_tracker->update(item.fused_detections, cv::Mat());

        // --- TIMESTAMP 4: Postprocess End (after tracking, before drawing) ---
        auto t_postprocess_end = std::chrono::high_resolution_clock::now();

        // --- PROFILING LOG ---
        if (m_config.enable_profiling_log) {
            // ... (profiling calculation logic is the same)
            auto dur_eo_pre = std::chrono::duration<double, std::milli>(item.t_eo_preprocess_end - item.t_eo_capture_start).count();
            auto dur_eo_inf = std::chrono::duration<double, std::milli>(item.t_eo_inference_end - item.t_eo_preprocess_end).count();
            auto dur_ir_pre = std::chrono::duration<double, std::milli>(item.t_ir_preprocess_end - item.t_ir_capture_start).count();
            auto dur_ir_inf = std::chrono::duration<double, std::milli>(item.t_ir_inference_end - item.t_ir_preprocess_end).count();
            auto slowest_inference_end = std::max(item.t_eo_inference_end, item.t_ir_inference_end);
            auto dur_fusion = std::chrono::duration<double, std::milli>(item.t_fusion_end - slowest_inference_end).count();
            auto dur_post = std::chrono::duration<double, std::milli>(t_postprocess_end - item.t_fusion_end).count();
            auto earliest_capture_start = std::min(item.t_eo_capture_start, item.t_ir_capture_start);
            auto dur_total = std::chrono::duration<double, std::milli>(t_postprocess_end - earliest_capture_start).count();

            std::cout << "[PROFILE] Frame " << item.frame_id << ":"
                      << " EO(Pre:" << std::fixed << std::setprecision(1) << dur_eo_pre << " Inf:" << dur_eo_inf << ")"
                      << " IR(Pre:" << dur_ir_pre << " Inf:" << dur_ir_inf << ")"
                      << " | Fusion: " << dur_fusion << "ms"
                      << " | Postproc: " << dur_post << "ms"
                      << " | Total E2E: " << dur_total << "ms"
                      << std::endl;
        }
        
        // --- THIS IS THE FIX: Perform all drawing operations unconditionally ---
        // --- before checking if visualization is enabled. ---

        // PTZ Guidance and Target Crosshair Drawing
        if (!tracked_objects.empty()) {
            auto target_track_it = std::min_element(tracked_objects.begin(), tracked_objects.end(), 
                [](const auto& a, const auto& b) { return a->getTrackId() < b->getTrackId(); });
            
            if ((*target_track_it)->isActivated()) {
                const byte_track::Rect<float>& rect = (*target_track_it)->getRect();
                cv::Point2f stabilized_center(rect.x() + rect.width() / 2.0f, rect.y() + rect.height() / 2.0f);
                cv::drawMarker(frame_to_draw, stabilized_center, cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 25, 2);
            }
        }

        // Drawing all tracked bounding boxes and IDs
        for (const auto& track : tracked_objects) {
            const byte_track::Rect<float>& rect = track->getRect();
            cv::Rect2f tracked_bbox(rect.x(), rect.y(), rect.width(), rect.height());
            cv::rectangle(frame_to_draw, tracked_bbox, cv::Scalar(0, 255, 0), 2);
            std::string label = std::to_string(track->getTrackId());
            cv::putText(frame_to_draw, label, cv::Point(tracked_bbox.x, tracked_bbox.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        }

        // FPS Display Drawing
        auto current_time = std::chrono::high_resolution_clock::now();
        auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(current_time - prev_time).count();
        prev_time = current_time;
        double fps_current = frame_time > 0 ? 1e6 / frame_time : 0;
        std::string fps_text = "FPS: " + std::to_string(static_cast<int>(fps_current + 0.5));
        cv::putText(frame_to_draw, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
        
        // --- END OF UNCONDITIONAL DRAWING ---

        // Now, the 'frame_to_draw' has all the overlays.

        // Display the modified frame if visualization is enabled
        if (m_config.enable_visualization) {
            cv::imshow("Object Detection", frame_to_draw);
            if (cv::waitKey(1) == 'q') {
                stop();
            }
        }

        // Save the modified frame to video if saving is enabled
        if (m_config.save_output_video && video_writer_initialized) {
            m_video_writer.write(frame_to_draw);
        }

        frame_index++;
    }
}

// Bridge function (no changes needed)
std::vector<byte_track::Object> TrackingPipeline::detections_to_bytetrack_objects(
    const std::vector<NamedBbox>& bboxes, int frame_width, int frame_height) 
{
    std::vector<byte_track::Object> objects;
    objects.reserve(bboxes.size());
    for (const auto& bbox : bboxes) {
        float x1 = bbox.bbox.x_min * frame_width;
        float y1 = bbox.bbox.y_min * frame_height;
        float w = (bbox.bbox.x_max - bbox.bbox.x_min) * frame_width;
        float h = (bbox.bbox.y_max - bbox.bbox.y_min) * frame_height;
        objects.emplace_back(byte_track::Rect<float>(x1, y1, w, h), bbox.class_id, bbox.bbox.score);
    }
    return objects;
}