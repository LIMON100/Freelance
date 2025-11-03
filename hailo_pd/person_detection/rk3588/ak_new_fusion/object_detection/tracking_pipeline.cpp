#include "tracking_pipeline.hpp"
#include "global_stabilizer.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

// Constructor: Initializes all members for dual-camera operation
TrackingPipeline::TrackingPipeline(const PipelineConfig& config)
    : m_config(config),
      m_stop_flag(false),
      m_total_frames(0)
{

    if (!loadHawkEyeCalibration("./calibration/", m_calibration)) {
        throw std::runtime_error("Failed to load HawkEye calibration data.");
    }
    
    m_preprocessed_queue_eo = std::make_shared<BoundedTSQueue<SingleStreamFrameData>>(2);
    m_preprocessed_queue_ir = std::make_shared<BoundedTSQueue<SingleStreamFrameData>>(2);
    m_results_queue_eo = std::make_shared<BoundedTSQueue<SingleStreamFrameData>>(2);
    m_results_queue_ir = std::make_shared<BoundedTSQueue<SingleStreamFrameData>>(2);
    m_fused_queue = std::make_shared<BoundedTSQueue<FusedFrameData>>(2);

    // Initialize two Hailo model instances
    m_model_eo = std::make_unique<AsyncModelInfer>(m_config.hef_path);
    
    // 2. Get the shared VDevice from the first model.
    std::shared_ptr<hailort::VDevice> shared_vdevice = m_model_eo->get_vdevice();
    m_model_ir = std::make_unique<AsyncModelInfer>(shared_vdevice, m_config.hef_path);
    // --- END OF FIX ---

    // ... (rest of the constructor is the same: open videos, load matrix, etc.)
    std::cout << "-I- Opening EO video file: " << m_config.eo_video_path << std::endl;

    // Initialize Video Captures for both streams
    std::cout << "-I- Opening EO video file: " << m_config.eo_video_path << std::endl;
    m_capture_eo.open(m_config.eo_video_path);
    std::cout << "-I- Opening IR video file: " << m_config.ir_video_path << std::endl;
    m_capture_ir.open(m_config.ir_video_path);

    if (!m_capture_eo.isOpened() || !m_capture_ir.isOpened()) {
        throw std::runtime_error("Error: Could not open one or both video files.");
    }

    // Load the boresight calibration matrix
    cv::FileStorage fs(m_config.boresight_calib_path, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        throw std::runtime_error("Error: Could not open boresight calibration file: " + m_config.boresight_calib_path);
    }
    fs["boresight_matrix"] >> m_boresight_matrix;
    fs.release();
    std::cout << "-I- Boresight calibration matrix loaded successfully." << std::endl;

    m_total_frames = m_capture_eo.get(cv::CAP_PROP_FRAME_COUNT);
    double fps = m_capture_eo.get(cv::CAP_PROP_FPS);

    m_tracker = std::make_unique<byte_track::BYTETracker>(fps, m_config.track_buffer_frames);
}

// Destructor
TrackingPipeline::~TrackingPipeline() {
    stop();
    release_resources();
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

void TrackingPipeline::preprocess_worker_eo() {
    GlobalStabilizer stabilizer;
    int frame_counter = 0;
    while (!m_stop_flag.load()) {
        SingleStreamFrameData item;
        item.frame_id = frame_counter++;
        item.t_capture_start = std::chrono::high_resolution_clock::now();

        cv::Mat raw_frame;
        m_capture_eo >> raw_frame;
        if (raw_frame.empty()) {
            m_preprocessed_queue_eo->stop();
            break;
        }
        
        cv::Mat frame_to_process;
        cv::Point crop_offset(0, 0);
        
        // FIX: Apply the same cropping logic to both streams for consistency
        if (m_config.enable_center_crop && m_config.crop_size > 0) {
            int crop_x = (raw_frame.cols - m_config.crop_size) / 2;
            int crop_y = (raw_frame.rows - m_config.crop_size) / 2;
            if (crop_x >= 0 && crop_y >= 0) {
                crop_offset = cv::Point(crop_x, crop_y);
                frame_to_process = raw_frame(cv::Rect(crop_x, crop_y, m_config.crop_size, m_config.crop_size)).clone();
            } else {
                frame_to_process = raw_frame.clone();
            }
        } else {
            frame_to_process = raw_frame.clone();
        }
        
        item.org_frame = raw_frame.clone();
        item.crop_offset = crop_offset;

        cv::Mat frame_for_inference;
        cv::Mat final_transform = cv::Mat::eye(2, 3, CV_64F);
        if (m_config.enable_global_stabilization) {
            stabilizer.stabilize(frame_to_process, frame_for_inference, final_transform);
        } else {
            frame_for_inference = frame_to_process.clone();
        }

        // Ensure both streams use the same resize method
        cv::resize(frame_for_inference, item.resized_for_infer, 
                   cv::Size(m_config.model_input_width, m_config.model_input_height), 
                   0, 0, cv::INTER_LINEAR);
        cv::invertAffineTransform(final_transform, item.affine_matrix);
        item.t_preprocess_end = std::chrono::high_resolution_clock::now();
        m_preprocessed_queue_eo->push(std::move(item));
    }
}

void TrackingPipeline::preprocess_worker_ir() {
    GlobalStabilizer stabilizer;
    int frame_counter = 0;
    while (!m_stop_flag.load()) {
        SingleStreamFrameData item;
        item.frame_id = frame_counter++;
        item.t_capture_start = std::chrono::high_resolution_clock::now();

        cv::Mat raw_frame;
        m_capture_ir >> raw_frame;
        if (raw_frame.empty()) {
            m_preprocessed_queue_ir->stop();
            break;
        }
        
        cv::Mat frame_to_process;
        cv::Point crop_offset(0, 0);
        
        // FIX: Apply the SAME cropping logic to IR stream
        if (m_config.enable_center_crop && m_config.crop_size > 0) {
            int crop_x = (raw_frame.cols - m_config.crop_size) / 2;
            int crop_y = (raw_frame.rows - m_config.crop_size) / 2;
            if (crop_x >= 0 && crop_y >= 0) {
                crop_offset = cv::Point(crop_x, crop_y);
                frame_to_process = raw_frame(cv::Rect(crop_x, crop_y, m_config.crop_size, m_config.crop_size)).clone();
            } else {
                frame_to_process = raw_frame.clone();
            }
        } else {
            frame_to_process = raw_frame.clone();
        }
        
        item.org_frame = raw_frame.clone();
        item.crop_offset = crop_offset; // Now both streams have crop_offset

        cv::Mat frame_for_inference;
        cv::Mat final_transform = cv::Mat::eye(2, 3, CV_64F);
        if (m_config.enable_global_stabilization) {
            stabilizer.stabilize(frame_to_process, frame_for_inference, final_transform);
        } else {
            frame_for_inference = frame_to_process.clone();
        }

        // Use the same resize method
        cv::resize(frame_for_inference, item.resized_for_infer, 
                   cv::Size(m_config.model_input_width, m_config.model_input_height), 
                   0, 0, cv::INTER_LINEAR);
        cv::invertAffineTransform(final_transform, item.affine_matrix);
        item.t_preprocess_end = std::chrono::high_resolution_clock::now();
        m_preprocessed_queue_ir->push(std::move(item));
    }
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

// void TrackingPipeline::fusion_worker() {
//     while (!m_stop_flag.load()) {
//         SingleStreamFrameData eo_item, ir_item;
//         if (!m_results_queue_eo->pop(eo_item) || !m_results_queue_ir->pop(ir_item)) {
//             break;
//         }

//         FusedFrameData fused_item;
//         fused_item.frame_id = eo_item.frame_id;
//         fused_item.display_frame = eo_item.org_frame;

//         // Copy timestamps
//         fused_item.t_eo_capture_start = eo_item.t_capture_start;
//         fused_item.t_eo_preprocess_end = eo_item.t_preprocess_end;
//         fused_item.t_eo_inference_end = eo_item.t_inference_end;
//         fused_item.t_ir_capture_start = ir_item.t_capture_start;
//         fused_item.t_ir_preprocess_end = ir_item.t_preprocess_end;
//         fused_item.t_ir_inference_end = ir_item.t_inference_end;

//         // --- CORRECTED FUSION LOGIC ---

//         // 1. Get EO detections in the full frame coordinate space
//         int eo_proc_width = m_config.enable_center_crop ? m_config.crop_size : eo_item.org_frame.cols;
//         int eo_proc_height = m_config.enable_center_crop ? m_config.crop_size : eo_item.org_frame.rows;
//         auto eo_bboxes = parse_nms_data(eo_item.output_data_and_infos[0].first, m_config.class_count);
//         std::vector<byte_track::Object> eo_objects = detections_to_bytetrack_objects(eo_bboxes, eo_proc_width, eo_proc_height);
//         for (auto& obj : eo_objects) {
//             obj.rect.x() += eo_item.crop_offset.x;
//             obj.rect.y() += eo_item.crop_offset.y;
//         }

//         // 2. Get IR detections and project them into the EO coordinate space
//         auto ir_bboxes = parse_nms_data(ir_item.output_data_and_infos[0].first, m_config.class_count);
//         std::vector<byte_track::Object> ir_objects_projected;
//         for (const auto& ir_bbox : ir_bboxes) {
//             // Create a temporary Object to hold the raw IR detection
//             byte_track::Object ir_obj = detections_to_bytetrack_objects({ir_bbox}, ir_item.org_frame.cols, ir_item.org_frame.rows)[0];
            
//             std::vector<cv::Point2f> ir_corners = {cv::Point2f(ir_obj.rect.tl_x(), ir_obj.rect.tl_y()), 
//                                                    cv::Point2f(ir_obj.rect.br_x(), ir_obj.rect.br_y())};
//             std::vector<cv::Point2f> eo_corners;
//             cv::perspectiveTransform(ir_corners, eo_corners, m_boresight_matrix);
            
//             ir_objects_projected.emplace_back(
//                 byte_track::Rect<float>(eo_corners[0].x, eo_corners[0].y, eo_corners[1].x - eo_corners[0].x, eo_corners[1].y - eo_corners[0].y),
//                 ir_obj.label,
//                 ir_obj.prob
//             );
//         }

//         // 3. Associate EO detections with projected IR detections
//         const float fusion_iou_thresh = 0.4f; // More forgiving threshold for fusion
//         std::vector<bool> eo_matched(eo_objects.size(), false);
//         std::vector<bool> ir_matched(ir_objects_projected.size(), false);

//         for (size_t i = 0; i < eo_objects.size(); ++i) {
//             for (size_t j = 0; j < ir_objects_projected.size(); ++j) {
//                 if (eo_objects[i].rect.calcIoU(ir_objects_projected[j].rect) > fusion_iou_thresh) {
//                     // This is a match! Create a single fused object.
//                     // We will use the EO box (more precise) and the highest confidence score.
//                     float fused_score = std::max(eo_objects[i].prob, ir_objects_projected[j].prob);
//                     fused_item.fused_detections.emplace_back(eo_objects[i].rect, eo_objects[i].label, fused_score);
                    
//                     eo_matched[i] = true;
//                     ir_matched[j] = true;
//                     break; // Move to the next EO object
//                 }
//             }
//         }

//         // 4. Add any unmatched EO detections to the final list
//         for (size_t i = 0; i < eo_objects.size(); ++i) {
//             if (!eo_matched[i]) {
//                 fused_item.fused_detections.push_back(eo_objects[i]);
//             }
//         }

//         // 5. Add any unmatched IR detections to the final list
//         for (size_t j = 0; j < ir_objects_projected.size(); ++j) {
//             if (!ir_matched[j]) {
//                 fused_item.fused_detections.push_back(ir_objects_projected[j]);
//             }
//         }
        
//         // --- END OF FUSION LOGIC ---
        
//         fused_item.t_fusion_end = std::chrono::high_resolution_clock::now();
//         m_fused_queue->push(std::move(fused_item));
//     }
//     m_fused_queue->stop();
// }


// In tracking_pipeline.cpp

void TrackingPipeline::fusion_worker() {
    while (!m_stop_flag.load()) {
        SingleStreamFrameData eo_item, ir_item;

        // Wait for a synchronized pair of frames from both inference workers.
        // If either stream ends, the pop will fail and we break the loop.
        if (!m_results_queue_eo->pop(eo_item) || !m_results_queue_ir->pop(ir_item)) {
            break;
        }

        // Prepare the output data structure for this frame pair
        FusedFrameData fused_item;
        fused_item.frame_id = eo_item.frame_id; // Use EO frame as the reference
        fused_item.display_frame = eo_item.org_frame.clone(); // Clone for safety before drawing

        // Copy all timestamps for detailed profiling
        fused_item.t_eo_capture_start = eo_item.t_capture_start;
        fused_item.t_eo_preprocess_end = eo_item.t_preprocess_end;
        fused_item.t_eo_inference_end = eo_item.t_inference_end;
        fused_item.t_ir_capture_start = ir_item.t_capture_start;
        fused_item.t_ir_preprocess_end = ir_item.t_preprocess_end;
        fused_item.t_ir_inference_end = ir_item.t_inference_end;

        // --- NEW 3D GEOMETRIC FUSION LOGIC ---

        // 1. Get current zoom and focus from EO camera.
        //    For this demonstration, we will use fixed placeholder values.
        //    In a real system, you would get these from the camera's SDK.
        double current_zoom = 300.0;  // Placeholder zoom value (e.g., from your LUT bins)
        double current_focus = 30.0;   // Placeholder focus value

        // 2. Interpolate the LUT to get the current Zoom Camera Intrinsics (K_zoom).
        double fx, fy, cx, cy;
        // The `bilinearLUT` function should be available from the provided `lut.cpp`.
        bilinearLUT(m_calibration.zoom_lut_grid, m_calibration.zoom_lut_params, current_zoom, current_focus, fx, fy, cx, cy);
        cv::Mat K_zoom = (cv::Mat_<double>(3, 3) << fx, 0, cx, 0, fy, cy, 0, 0, 1);
        // Note: For full accuracy, you would also interpolate and apply distortion coefficients. We'll omit that for simplicity here.

        // 3. Get native EO detections and adjust for any cropping.
        auto eo_bboxes = parse_nms_data(eo_item.output_data_and_infos[0].first, m_config.class_count);
        int eo_proc_width = m_config.enable_center_crop ? m_config.crop_size : eo_item.org_frame.cols;
        int eo_proc_height = m_config.enable_center_crop ? m_config.crop_size : eo_item.org_frame.rows;
        std::vector<byte_track::Object> eo_objects = detections_to_bytetrack_objects(eo_bboxes, eo_proc_width, eo_proc_height);
        for (auto& obj : eo_objects) {
            obj.rect.x() += eo_item.crop_offset.x;
            obj.rect.y() += eo_item.crop_offset.y;
        }

        // 4. Get IR detections and project them into the EO camera's image space.
        auto ir_bboxes = parse_nms_data(ir_item.output_data_and_infos[0].first, m_config.class_count);
        std::vector<byte_track::Object> ir_objects_projected;

        for (const auto& ir_bbox_named : ir_bboxes) {
            // Get the bounding box in the original, uncropped IR frame coordinates
            int ir_proc_width = m_config.enable_center_crop ? m_config.crop_size : ir_item.org_frame.cols;
            int ir_proc_height = m_config.enable_center_crop ? m_config.crop_size : ir_item.org_frame.rows;
            cv::Rect ir_rect_proc = get_bbox_coordinates(ir_bbox_named.bbox, ir_proc_width, ir_proc_height);
            
            // Adjust for crop offset to get coordinates in the full IR frame
            cv::Rect ir_rect_full = ir_rect_proc + ir_item.crop_offset;
            cv::Point2f ir_center(ir_rect_full.x + ir_rect_full.width / 2.0f, ir_rect_full.y + ir_rect_full.height / 2.0f);

            // A. Unproject the 2D IR point to a 3D ray in the thermal camera's coordinate system.
            //    This requires an assumption about the object's distance.
            double assumed_depth = 100.0; // Assume objects are 100 meters away. This is a critical parameter to tune.
            
            cv::Mat ir_point_2d_undistorted;
            // First, undistort the point using the thermal camera's intrinsics
            cv::undistortPoints(std::vector<cv::Point2f>{ir_center}, ir_point_2d_undistorted, m_calibration.thermal_intrinsics.K, m_calibration.thermal_intrinsics.dist);
            
            // The result of undistortPoints is normalized. We form a 3D ray from it.
            cv::Point3d thermal_ray(ir_point_2d_undistorted.at<double>(0,0), ir_point_2d_undistorted.at<double>(0,1), 1.0);
            
            // Scale the ray by the assumed depth to get a 3D point in the thermal camera's space.
            cv::Point3d pt_in_thermal_3d = thermal_ray * assumed_depth;

            // B. Transform the 3D point from Thermal space {T} to Zoom space {Z} using the extrinsics Z_T = [R|t].
            cv::Mat pt_thermal_mat = (cv::Mat_<double>(3,1) << pt_in_thermal_3d.x, pt_in_thermal_3d.y, pt_in_thermal_3d.z);
            cv::Mat pt_zoom_mat = m_calibration.thermal_to_zoom_extrinsics.R * pt_thermal_mat + m_calibration.thermal_to_zoom_extrinsics.t;
            cv::Point3d pt_in_zoom_3d(pt_zoom_mat);

            // C. Reproject the 3D point from Zoom space onto the Zoom camera's 2D image plane.
            std::vector<cv::Point2f> projected_points;
            // Using projectPoints is robust as it handles the full camera model.
            cv::projectPoints(std::vector<cv::Point3d>{pt_in_zoom_3d}, cv::Vec3d(0,0,0), cv::Vec3d(0,0,0), K_zoom, cv::Mat(), projected_points);
            
            if (projected_points.empty()) continue; // Skip if projection fails
            
            cv::Point2f eo_center = projected_points[0];

            // D. Create a bounding box in the EO image.
            //    For now, we'll simply copy the size from the IR detection.
            //    A more advanced method would scale the size based on the projected depth.
            float w = ir_rect_full.width;
            float h = ir_rect_full.height;
            
            ir_objects_projected.emplace_back(
                byte_track::Rect<float>(eo_center.x - w/2.0f, eo_center.y - h/2.0f, w, h),
                ir_bbox_named.class_id,
                ir_bbox_named.bbox.score
            );
        }

        // 5. Fuse the native EO detections with the projected IR detections.
        const float fusion_iou_thresh = 0.3f;
        const float single_source_confidence_thresh = 0.55f;
        std::vector<bool> eo_matched(eo_objects.size(), false);
        std::vector<bool> ir_matched(ir_objects_projected.size(), false);

        // First pass: Find confident matches seen by both sensors.
        for (size_t i = 0; i < eo_objects.size(); ++i) {
            for (size_t j = 0; j < ir_objects_projected.size(); ++j) {
                if (ir_matched[j]) continue; // Skip already matched IR detections.

                if (eo_objects[i].rect.calcIoU(ir_objects_projected[j].rect) > fusion_iou_thresh) {
                    // It's a match. Average the boxes for stability and take the highest score.
                    float avg_x = (eo_objects[i].rect.x() + ir_objects_projected[j].rect.x()) / 2.0f;
                    float avg_y = (eo_objects[i].rect.y() + ir_objects_projected[j].rect.y()) / 2.0f;
                    float avg_w = (eo_objects[i].rect.width() + ir_objects_projected[j].rect.width()) / 2.0f;
                    float avg_h = (eo_objects[i].rect.height() + ir_objects_projected[j].rect.height()) / 2.0f;
                    float fused_score = std::max(eo_objects[i].prob, ir_objects_projected[j].prob);
                    
                    fused_item.fused_detections.emplace_back(
                        byte_track::Rect<float>(avg_x, avg_y, avg_w, avg_h),
                        eo_objects[i].label,
                        fused_score
                    );
                    
                    eo_matched[i] = true;
                    ir_matched[j] = true;
                    break; // Match found for this EO object, move to the next.
                }
            }
        }

        // Second pass: Add high-confidence detections seen only by one sensor.
        for (size_t i = 0; i < eo_objects.size(); ++i) {
            if (!eo_matched[i] && eo_objects[i].prob > single_source_confidence_thresh) {
                fused_item.fused_detections.push_back(eo_objects[i]);
            }
        }
        for (size_t j = 0; j < ir_objects_projected.size(); ++j) {
            if (!ir_matched[j] && ir_objects_projected[j].prob > single_source_confidence_thresh) {
                fused_item.fused_detections.push_back(ir_objects_projected[j]);
            }
        }
        
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
            double fps = m_capture_eo.get(cv::CAP_PROP_FPS);
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