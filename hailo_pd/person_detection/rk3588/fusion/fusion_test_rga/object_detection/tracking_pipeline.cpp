// For double inference and separate inference fusion worker
// #include <gst/gst.h>
// #include <gst/app/gstappsink.h>
// #include <gst/video/video.h>

// #include "tracking_pipeline.hpp"
// #include "global_stabilizer.hpp"
// #include <iostream>
// #include <iomanip>
// #include <algorithm>
// #include "utils/calibration_data.hpp"


// TrackingPipeline::TrackingPipeline(const PipelineConfig& config)
//     : m_config(config),
//       m_stop_flag(false),
//       m_total_frames(0) // Will be set from EO video if applicable
// {
//     // Initialize all queues with larger buffers for better throughput
//     m_raw_frames_queue_eo = std::make_shared<BoundedTSQueue<cv::Mat>>(5);
//     m_raw_frames_queue_ir = std::make_shared<BoundedTSQueue<cv::Mat>>(5);
//     m_preprocessed_queue_eo = std::make_shared<BoundedTSQueue<SingleStreamFrameData>>(10);
//     m_preprocessed_queue_ir = std::make_shared<BoundedTSQueue<SingleStreamFrameData>>(10);
//     m_results_queue_eo = std::make_shared<BoundedTSQueue<SingleStreamFrameData>>(10);
//     m_results_queue_ir = std::make_shared<BoundedTSQueue<SingleStreamFrameData>>(10);
//     m_fused_queue = std::make_shared<BoundedTSQueue<FusedFrameData>>(10);

//     // Load calibration data
//     if (!loadCalibrationData(m_config, m_calibration_data)) {
//         throw std::runtime_error("FATAL: Failed to load calibration data.");
//     }
//     std::cout << "[INFO] Successfully loaded calibration data." << std::endl;

//     // Initialize Hailo models
//     // m_model_eo = std::make_unique<AsyncModelInfer>(m_config.hef_path);
//     // std::shared_ptr<hailort::VDevice> shared_vdevice = m_model_eo->get_vdevice();
//     // m_model_ir = std::make_unique<AsyncModelInfer>(shared_vdevice, m_config.hef_path);
    
//     m_model = std::make_unique<AsyncModelInfer>(m_config.hef_path);
//     std::cout << "[INFO] Single Hailo model instance created for both streams." << std::endl;
    
//     // Get total frames from the EO video file for the progress bar
//     if (!m_config.use_live_stream) {
//         cv::VideoCapture temp_cap(m_config.eo_video_path);
//         if (temp_cap.isOpened()) {
//             m_total_frames = temp_cap.get(cv::CAP_PROP_FRAME_COUNT);
//         }
//         temp_cap.release();
//     }
    
//     m_tracker = std::make_unique<byte_track::BYTETracker>(30, m_config.track_buffer_frames);
// }


// // Destructor
// TrackingPipeline::~TrackingPipeline() {
//     stop();
//     release_resources();
// }

// // std::string TrackingPipeline::create_gstreamer_pipeline(bool is_live, const std::string& source_path) {
// //     std::string source_element;
// //     if (is_live) {
// //         source_element = "v4l2src device=" + source_path;
// //     } else {
// //         source_element = "filesrc location=" + source_path + " ! qtdemux ! h264parse ! mppvideodec";
// //     }

// //     return source_element + " ! rgaconvert ! " +
// //            "video/x-raw,format=BGR,width=" + std::to_string(m_config.processing_width) +
// //            ",height=" + std::to_string(m_config.processing_height) + " ! " +
// //            "queue ! appsink name=sink drop=true max-buffers=1";
// // }

// // In tracking_pipeline.cpp

// std::string TrackingPipeline::create_gstreamer_pipeline(bool is_live, const std::string& source_path) {
//     std::string source_element;

//     if (is_live) {
//         // Live camera pipeline - this one is already working well.
//         source_element = "v4l2src device=" + source_path;
        
//         return source_element + " ! video/x-raw,width=640,height=480 ! " +
//                "rgaconvert ! " +
//                "video/x-raw,format=BGR,width=" + std::to_string(m_config.processing_width) +
//                ",height=" + std::to_string(m_config.processing_height) + " ! " +
//                "queue ! appsink name=sink drop=true max-buffers=1";

//     } else {
//         // --- THIS IS THE FIX for VIDEO FILES ---
//         // Video file pipeline with an explicit copy-to-cpu step.
//         source_element = "filesrc location=" + source_path + " ! qtdemux ! h264parse ! mppvideodec";
        
//         return source_element + " ! " +
//                "rgaconvert ! " +
//                "video/x-raw,format=BGR,width=" + std::to_string(m_config.processing_width) +
//                ",height=" + std::to_string(m_config.processing_height) + " ! " +
//                // Add videoconvert here. It will see the BGR format from rgaconvert
//                // and ensure the output buffer is in standard system memory for the appsink.
//                "videoconvert ! " +
//                "queue ! appsink name=sink drop=true max-buffers=1";
//     }
// }

// cv::Point2f TrackingPipeline::transformIrToEo(const cv::Point2f& ir_point, double zoom, double focus) 
// {

//     if (!m_calibration_data.is_loaded) {
//         throw std::runtime_error("FATAL: Attempting to use calibration data that wasn't loaded");
//     }
    
//     // Get current zoom-dependent intrinsics
//     double fx, fy, cx, cy;
//     bilinearInterpolate(m_calibration_data.zoom_lut, zoom, focus, fx, fy, cx, cy);
    
//     // Create camera matrix for current zoom
//     cv::Mat K_zoom = (cv::Mat_<double>(3, 3) << fx, 0, cx, 0, fy, cy, 0, 0, 1);
    
//     // Undistort IR point
//     std::vector<cv::Point2f> ir_points = {ir_point};
//     std::vector<cv::Point2f> undistorted_ir;
//     cv::undistortPoints(ir_points, undistorted_ir, 
//                        m_calibration_data.thermal_intrinsics.K,
//                        m_calibration_data.thermal_intrinsics.dist);
    
//     // Back-project to 3D ray (assuming point at unit distance)
//     cv::Mat ray = (cv::Mat_<double>(3, 1) << undistorted_ir[0].x, undistorted_ir[0].y, 1.0);
    
//     // Transform ray to EO camera coordinates
//     cv::Mat ray_eo = m_calibration_data.extrinsics.R * ray + m_calibration_data.extrinsics.t;
    
//     // Project to EO image coordinates
//     cv::Mat projected = K_zoom * ray_eo;
//     double w = projected.at<double>(2, 0);
    
//     return cv::Point2f(projected.at<double>(0, 0) / w, projected.at<double>(1, 0) / w);
// }

// std::vector<cv::Point2f> TrackingPipeline::transformIrToEo(const std::vector<cv::Point2f>& ir_points,
//                                                          double zoom, double focus) {
//     std::vector<cv::Point2f> eo_points;
//     eo_points.reserve(ir_points.size());
    
//     for (const auto& pt : ir_points) {
//         eo_points.push_back(transformIrToEo(pt, zoom, focus));
//     }
    
//     return eo_points;
// }


// cv::Point2f TrackingPipeline::transformEoToIr(const cv::Point2f& eo_point, 
//                                              double zoom, double focus) {
//     if (!m_calibration_data.is_loaded) {
//         throw std::runtime_error("FATAL: Attempting to use calibration data that wasn't loaded");
//     }

//     double fx, fy, cx, cy;
//     bilinearInterpolate(m_calibration_data.zoom_lut, zoom, focus, fx, fy, cx, cy);
//     cv::Mat K_zoom = (cv::Mat_<double>(3, 3) << fx, 0, cx, 0, fy, cy, 0, 0, 1);
    
//     cv::Mat eo_point_2d = (cv::Mat_<double>(3,1) << eo_point.x, eo_point.y, 1.0);
//     cv::Mat ray_in_zoom_space = K_zoom.inv() * eo_point_2d;


//     cv::Mat R_inv = m_calibration_data.extrinsics.R.t(); // Transpose of rotation matrix is its inverse
//     cv::Mat t_transformed = -R_inv * m_calibration_data.extrinsics.t;
//     cv::Mat ray_in_thermal_space = R_inv * ray_in_zoom_space + t_transformed;

//     // 4. Project ray onto Thermal image plane
//     cv::Mat projected_homogeneous = m_calibration_data.thermal_intrinsics.K * ray_in_thermal_space;
//     double w = projected_homogeneous.at<double>(2, 0);

//     cv::Point2f distorted_point(projected_homogeneous.at<double>(0, 0) / w,
//                                 projected_homogeneous.at<double>(1, 0) / w);

                                
//     return distorted_point;
// }


// void TrackingPipeline::stop() {
//     m_stop_flag.store(true);
//     m_preprocessed_queue_eo->stop();
//     m_preprocessed_queue_ir->stop();
//     m_results_queue_eo->stop();
//     m_results_queue_ir->stop();
//     m_fused_queue->stop();
// }

// void TrackingPipeline::release_resources() {
//     if (m_config.save_output_video) m_video_writer.release();
//     m_capture_eo.release();
//     m_capture_ir.release();
//     if (m_config.enable_visualization) cv::destroyAllWindows();
// }

// // Main run method launches all parallel threads
// // void TrackingPipeline::run() {

// //     // Create the pipeline strings based on the config
// //     std::string eo_pipeline_str = create_gstreamer_pipeline(m_config.use_live_stream, 
// //                                                             m_config.use_live_stream ? m_config.eo_device_path : m_config.eo_video_path);
// //     std::string ir_pipeline_str = create_gstreamer_pipeline(m_config.use_live_stream,
// //                                                             m_config.use_live_stream ? m_config.ir_device_path : m_config.ir_video_path);

// //     std::cout << "[GSTREAMER EO] " << eo_pipeline_str << std::endl;
// //     std::cout << "[GSTREAMER IR] " << ir_pipeline_str << std::endl;

// //     // Launch all threads
// //     auto capture_eo_thread = std::async(std::launch::async, &TrackingPipeline::capture_worker, this, eo_pipeline_str, m_raw_frames_queue_eo);
// //     auto capture_ir_thread = std::async(std::launch::async, &TrackingPipeline::capture_worker, this, ir_pipeline_str, m_raw_frames_queue_ir);

// //     // FIX: Removed unused t_start and t_end variables
// //     auto preprocess_eo_thread = std::async(std::launch::async, &TrackingPipeline::preprocess_worker_eo, this);
// //     auto preprocess_ir_thread = std::async(std::launch::async, &TrackingPipeline::preprocess_worker_ir, this);
// //     auto inference_eo_thread = std::async(std::launch::async, &TrackingPipeline::inference_worker_eo, this);
// //     auto inference_ir_thread = std::async(std::launch::async, &TrackingPipeline::inference_worker_ir, this);
// //     auto fusion_thread = std::async(std::launch::async, &TrackingPipeline::fusion_worker, this);
// //     auto postprocess_thread = std::async(std::launch::async, &TrackingPipeline::postprocess_worker, this);

// //     postprocess_thread.wait();
// //     stop(); 
    
// //     preprocess_eo_thread.wait();
// //     preprocess_ir_thread.wait();
// //     inference_eo_thread.wait();
// //     inference_ir_thread.wait();
// //     fusion_thread.wait();


// //     std::cout << "Pipeline finished." << std::endl;
// // }


// void TrackingPipeline::run() {

//     if (m_config.mirror_single_camera && m_config.use_live_stream) {
//         auto t_start = std::chrono::high_resolution_clock::now();
//         // --- SINGLE CAMERA MIRROR MODE ---
//         std::cout << "[INFO] Mirroring single camera mode is ACTIVE." << std::endl;
//         std::cout << "[INFO] Using '" << m_config.eo_device_path << "' for both EO and IR streams." << std::endl;

//         // Create one GStreamer pipeline using the EO device path.
//         std::string single_pipeline_str = create_gstreamer_pipeline(true, m_config.eo_device_path);
        
//         // Launch a single capture thread that pushes to BOTH queues.
//         auto capture_thread = std::async(std::launch::async, &TrackingPipeline::mirror_capture_worker, this, single_pipeline_str, m_raw_frames_queue_eo, m_raw_frames_queue_ir);
        
//         // The rest of the pipeline runs as normal, unaware of the mirrored source.
//         auto preprocess_eo_thread = std::async(std::launch::async, &TrackingPipeline::preprocess_worker_eo, this);
//         auto preprocess_ir_thread = std::async(std::launch::async, &TrackingPipeline::preprocess_worker_ir, this);
        
//         auto inference_eo_thread = std::async(std::launch::async, &TrackingPipeline::inference_worker_eo, this);
//         auto inference_ir_thread = std::async(std::launch::async, &TrackingPipeline::inference_worker_ir, this);
//         auto fusion_thread = std::async(std::launch::async, &TrackingPipeline::fusion_worker, this);
        
//         auto postprocess_thread = std::async(std::launch::async, &TrackingPipeline::postprocess_worker, this);

//         postprocess_thread.wait();
//         stop(); 
        
//         preprocess_eo_thread.wait();
//         preprocess_ir_thread.wait();
//         inference_eo_thread.wait();
//         inference_ir_thread.wait();
//         fusion_thread.wait();
//         capture_thread.wait(); 

//         auto t_end = std::chrono::high_resolution_clock::now();
//     } 
//     else {
//         auto t_start = std::chrono::high_resolution_clock::now();
//         // --- DUAL STREAM (NORMAL) MODE ---
//         if (m_config.mirror_single_camera && !m_config.use_live_stream) {
//              std::cout << "[WARNING] 'mirror_single_camera' is true but 'use_live_stream' is false. Mirroring is only for live cameras. Running in normal dual video file mode." << std::endl;
//         }
//         std::cout << "[INFO] Running in normal dual-stream mode." << std::endl;

//         // Create two separate pipeline strings.
//         std::string eo_pipeline_str = create_gstreamer_pipeline(m_config.use_live_stream, 
//                                                                 m_config.use_live_stream ? m_config.eo_device_path : m_config.eo_video_path);
//         std::string ir_pipeline_str = create_gstreamer_pipeline(m_config.use_live_stream,
//                                                                 m_config.use_live_stream ? m_config.ir_device_path : m_config.ir_video_path);

//         std::cout << "[GSTREAMER EO] " << eo_pipeline_str << std::endl;
//         std::cout << "[GSTREAMER IR] " << ir_pipeline_str << std::endl;

//         // Launch two separate capture threads.
//         auto capture_eo_thread = std::async(std::launch::async, &TrackingPipeline::capture_worker, this, eo_pipeline_str, m_raw_frames_queue_eo);
//         auto capture_ir_thread = std::async(std::launch::async, &TrackingPipeline::capture_worker, this, ir_pipeline_str, m_raw_frames_queue_ir);

//         auto preprocess_eo_thread = std::async(std::launch::async, &TrackingPipeline::preprocess_worker_eo, this);
//         auto preprocess_ir_thread = std::async(std::launch::async, &TrackingPipeline::preprocess_worker_ir, this);
        
//         auto inference_eo_thread = std::async(std::launch::async, &TrackingPipeline::inference_worker_eo, this);
//         auto inference_ir_thread = std::async(std::launch::async, &TrackingPipeline::inference_worker_ir, this);
//         auto fusion_thread = std::async(std::launch::async, &TrackingPipeline::fusion_worker, this);
        
//         auto postprocess_thread = std::async(std::launch::async, &TrackingPipeline::postprocess_worker, this);

//         postprocess_thread.wait();
//         stop(); 
        
//         preprocess_eo_thread.wait();
//         preprocess_ir_thread.wait();
//         inference_eo_thread.wait();
//         inference_ir_thread.wait();
//         fusion_thread.wait();
//         capture_eo_thread.wait();
//         capture_ir_thread.wait();

//         auto t_end = std::chrono::high_resolution_clock::now();
       
//     }

//     std::cout << "Pipeline finished." << std::endl;
// }

// // worker function that reads from one camera and writes to two queues.
// void TrackingPipeline::mirror_capture_worker(const std::string& pipeline_str,
//                                              std::shared_ptr<BoundedTSQueue<cv::Mat>> queue_eo,
//                                              std::shared_ptr<BoundedTSQueue<cv::Mat>> queue_ir) {
//     gst_init(nullptr, nullptr);
//     GError *error = nullptr;
//     GstElement *pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    
//     if (!pipeline || error) { /* handle error */ stop(); return; }

//     GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
//     if (!appsink) { /* handle error */ stop(); return; }

//     gst_element_set_state(pipeline, GST_STATE_PLAYING);

//     while (!m_stop_flag.load()) {
//         GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
//         if (!sample) {
//             if (gst_app_sink_is_eos(GST_APP_SINK(appsink))) {
//                 std::cout << "GStreamer: End-Of-Stream reached." << std::endl;
//             } else {
//                 std::cerr << "GStreamer: Error pulling sample." << std::endl;
//             }
//             break;
//         }

//         GstBuffer *buffer = gst_sample_get_buffer(sample);
//         GstCaps *caps = gst_sample_get_caps(sample);
//         GstVideoInfo video_info;
//         if (!buffer || !caps || !gst_video_info_from_caps(&video_info, caps)) {
//             gst_sample_unref(sample);
//             continue;
//         }

//         GstMapInfo map_info;
//         if (gst_buffer_map(buffer, &map_info, GST_MAP_READ)) {
//             cv::Mat frame_wrapper(
//                 GST_VIDEO_INFO_HEIGHT(&video_info),
//                 GST_VIDEO_INFO_WIDTH(&video_info),
//                 CV_8UC3,
//                 map_info.data
//             );
            
//             // --- CORE MIRRORING LOGIC ---
//             // Push a clone of the frame to BOTH queues.
//             queue_eo->push(frame_wrapper.clone());
//             queue_ir->push(frame_wrapper.clone());
//             // --- END CORE MIRRORING LOGIC ---

//             gst_buffer_unmap(buffer, &map_info);
//         }
//         gst_sample_unref(sample);
//     }

//     // Cleanup
//     gst_element_set_state(pipeline, GST_STATE_NULL);
//     gst_object_unref(appsink);
//     gst_object_unref(pipeline);
    
//     // Signal to BOTH downstream workers that this stream has ended.
//     queue_eo->stop();
//     queue_ir->stop();
// }

// // Generic GStreamer Capture Worker
// void TrackingPipeline::capture_worker(const std::string& pipeline_str, std::shared_ptr<BoundedTSQueue<cv::Mat>> queue) {
//     gst_init(nullptr, nullptr);
//     GError *error = nullptr;
//     GstElement *pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    
//     if (!pipeline || error) { /* handle error */ stop(); return; }

//     GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
//     if (!appsink) { /* handle error */ stop(); return; }

//     gst_element_set_state(pipeline, GST_STATE_PLAYING);

//     while (!m_stop_flag.load()) {
//         GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
//         if (!sample) {
//             if (gst_app_sink_is_eos(GST_APP_SINK(appsink))) {
//                 std::cout << "GStreamer: End-Of-Stream reached." << std::endl;
//                 break;
//             } else {
//                 std::cerr << "GStreamer: Error pulling sample." << std::endl;
//             }
//             break;
//         }

//         GstBuffer *buffer = gst_sample_get_buffer(sample);
//         GstCaps *caps = gst_sample_get_caps(sample);
//         GstVideoInfo video_info;
//         if (!buffer || !caps || !gst_video_info_from_caps(&video_info, caps)) {
//             gst_sample_unref(sample);
//             continue;
//         }

//         GstMapInfo map_info;
//         if (gst_buffer_map(buffer, &map_info, GST_MAP_READ)) {
//             cv::Mat frame_wrapper(
//                 GST_VIDEO_INFO_HEIGHT(&video_info),
//                 GST_VIDEO_INFO_WIDTH(&video_info),
//                 CV_8UC3, // We requested BGR from the pipeline
//                 map_info.data
//             );
//             queue->push(frame_wrapper.clone());
//             gst_buffer_unmap(buffer, &map_info);
//         }
//         gst_sample_unref(sample);
//     }

//     // Cleanup
//     gst_element_set_state(pipeline, GST_STATE_NULL);
//     gst_object_unref(appsink);
//     gst_object_unref(pipeline);
//     // Do not call gst_deinit() here if another pipeline is running
    
//     queue->stop(); // Signal to the next stage that this stream has ended
// }

// // SIMPLIFIED Preprocess Worker for EO
// void TrackingPipeline::preprocess_worker_eo() {
//     int frame_counter = 0;
//     while (!m_stop_flag.load()) {
//         cv::Mat processed_frame;
//         if (!m_raw_frames_queue_eo->pop(processed_frame)) break;

//         SingleStreamFrameData item;
//         item.frame_id = frame_counter++;
//         item.t_capture_start = std::chrono::high_resolution_clock::now();
        
//         // The frame from GStreamer is already the correct processing size (e.g., 512x512)
//         item.org_frame = processed_frame.clone();

//         // The only remaining task is resizing to the model's input size.
//         // No stabilization or cropping logic is needed here anymore.
//         cv::resize(processed_frame, item.resized_for_infer, 
//                    cv::Size(m_config.model_input_width, m_config.model_input_height), 
//                    0, 0, cv::INTER_LINEAR);
        
//         // Since stabilization is disabled, the affine matrix is identity.
//         item.affine_matrix = cv::Mat::eye(2, 3, CV_64F);
//         item.crop_offset = cv::Point(0, 0);

//         item.t_preprocess_end = std::chrono::high_resolution_clock::now();
//         m_preprocessed_queue_eo->push(std::move(item));
//     }
//     m_preprocessed_queue_eo->stop();
// }

// void TrackingPipeline::preprocess_worker_ir() {
//     int frame_counter = 0;
//     while (!m_stop_flag.load()) {
//         cv::Mat processed_frame;
//         if (!m_raw_frames_queue_ir->pop(processed_frame)) break;

//         SingleStreamFrameData item;
//         item.frame_id = frame_counter++;
//         item.t_capture_start = std::chrono::high_resolution_clock::now();

//         item.org_frame = processed_frame.clone();
//         cv::resize(processed_frame, item.resized_for_infer, 
//                    cv::Size(m_config.model_input_width, m_config.model_input_height), 
//                    0, 0, cv::INTER_LINEAR);

//         item.affine_matrix = cv::Mat::eye(2, 3, CV_64F);
//         item.crop_offset = cv::Point(0, 0);

//         item.t_preprocess_end = std::chrono::high_resolution_clock::now();
//         m_preprocessed_queue_ir->push(std::move(item));
//     }
//     m_preprocessed_queue_ir->stop();
// }

// void TrackingPipeline::inference_worker_eo() {
//     while (!m_stop_flag.load()) {
//         SingleStreamFrameData item;
//         if (!m_preprocessed_queue_eo->pop(item)) break;
//         // m_model_eo->infer(std::make_shared<cv::Mat>(item.resized_for_infer),
//         m_model->infer(std::make_shared<cv::Mat>(item.resized_for_infer),
//             [item = std::move(item), queue = m_results_queue_eo](const auto&, const auto& outs, const auto& guards) mutable {
//                 item.t_inference_end = std::chrono::high_resolution_clock::now();
//                 item.output_data_and_infos = outs;
//                 item.output_guards = guards;
//                 queue->push(std::move(item));
//             });
//     }
//     m_results_queue_eo->stop();
// }

// void TrackingPipeline::inference_worker_ir() {
//     while (!m_stop_flag.load()) {
//         SingleStreamFrameData item;
//         if (!m_preprocessed_queue_ir->pop(item)) break;
//         // m_model_ir->infer(std::make_shared<cv::Mat>(item.resized_for_infer),
//         m_model->infer(std::make_shared<cv::Mat>(item.resized_for_infer),
//             [item = std::move(item), queue = m_results_queue_ir](const auto&, const auto& outs, const auto& guards) mutable {
//                 item.t_inference_end = std::chrono::high_resolution_clock::now();
//                 item.output_data_and_infos = outs;
//                 item.output_guards = guards;
//                 queue->push(std::move(item));
//             });
//     }
//     m_results_queue_ir->stop();
// }

// void TrackingPipeline::fusion_worker() {
//     while (!m_stop_flag.load()) {
//         SingleStreamFrameData eo_item, ir_item;


//         if (!m_results_queue_eo->pop(eo_item)) {
//             break; // EO stream ended, break the loop.
//         }
//         if (!m_results_queue_ir->pop(ir_item)) {
//             break; // IR stream ended, break the loop.
//         }

//         // Prepare the output data structure for this frame pair.
//         FusedFrameData fused_item;
//         fused_item.frame_id = eo_item.frame_id; // Use EO frame ID as the reference for the fused frame.
//         fused_item.display_frame = eo_item.org_frame.clone(); // Use the original EO frame for display.

//         // Copy all timestamps from both streams for detailed profiling.
//         fused_item.t_eo_capture_start = eo_item.t_capture_start;
//         fused_item.t_eo_preprocess_end = eo_item.t_preprocess_end;
//         fused_item.t_eo_inference_end = eo_item.t_inference_end;
//         fused_item.t_ir_capture_start = ir_item.t_capture_start;
//         fused_item.t_ir_preprocess_end = ir_item.t_preprocess_end;
//         fused_item.t_ir_inference_end = ir_item.t_inference_end;

//         // --- EO->THERMAL HANDOVER & SCORE FUSION STRATEGY ---

//         // Get current zoom/focus from the camera. For this demo, we use default values from the config.
//         // In a real system, you would get these values from the camera's SDK each frame.
//         double current_zoom = m_config.default_zoom;
//         double current_focus = m_config.default_focus;

//         // 1. Get native EO detections (Primary Sensor) and convert to full frame coordinates.
//         int eo_proc_width = m_config.enable_center_crop ? m_config.crop_size : eo_item.org_frame.cols;
//         int eo_proc_height = m_config.enable_center_crop ? m_config.crop_size : eo_item.org_frame.rows;
//         auto eo_bboxes = parse_nms_data(eo_item.output_data_and_infos[0].first, m_config.class_count);
//         std::vector<byte_track::Object> eo_objects = detections_to_bytetrack_objects(eo_bboxes, eo_proc_width, eo_proc_height);
//         for (auto& obj : eo_objects) {
//             obj.rect.x() += eo_item.crop_offset.x;
//             obj.rect.y() += eo_item.crop_offset.y;
//         }

//         // 2. Get native IR detections (Secondary Sensor) and convert to full frame coordinates.
//         int ir_proc_width = m_config.enable_center_crop ? m_config.crop_size : ir_item.org_frame.cols;
//         int ir_proc_height = m_config.enable_center_crop ? m_config.crop_size : ir_item.org_frame.rows;
//         auto ir_bboxes = parse_nms_data(ir_item.output_data_and_infos[0].first, m_config.class_count);
//         std::vector<byte_track::Object> ir_objects = detections_to_bytetrack_objects(ir_bboxes, ir_proc_width, ir_proc_height);
//         for (auto& obj : ir_objects) {
//             obj.rect.x() += ir_item.crop_offset.x;
//             obj.rect.y() += ir_item.crop_offset.y;
//         }

//         // 3. For each EO detection, search for a corresponding IR detection to validate and boost its score.
//         const float search_roi_size = 80.0f; // Search radius in pixels (e.g., +/- 80px), as suggested in the doc.
//         const float alpha = 0.7;             // Weight for EO score (e.g., for daytime). Can be scheduled.

//         // This loop modifies the `eo_objects` vector in place.
//         for (auto& eo_obj : eo_objects) {
//             // Handover: Project the EO detection's center point into the IR image space.
//             cv::Point2f eo_center(eo_obj.rect.x() + eo_obj.rect.width() / 2.0f,
//                                   eo_obj.rect.y() + eo_obj.rect.height() / 2.0f);
            
//             cv::Point2f projected_ir_center = transformEoToIr(eo_center, current_zoom, current_focus);

//             // Seed: Search for a matching IR detection within the defined ROI around the projected center.
//             byte_track::Object* best_ir_match = nullptr;
            
//             // Define the square search Region of Interest (ROI) in the IR image.
//             cv::Rect2f search_roi(projected_ir_center.x - search_roi_size / 2.0f,
//                                   projected_ir_center.y - search_roi_size / 2.0f,
//                                   search_roi_size, search_roi_size);

//             for (auto& ir_obj : ir_objects) {
//                 // Check if the center of the current IR object falls within our search ROI.
//                 cv::Point2f ir_center(ir_obj.rect.x() + ir_obj.rect.width() / 2.0f,
//                                       ir_obj.rect.y() + ir_obj.rect.height() / 2.0f);

//                 if (search_roi.contains(ir_center)) {
//                      // We found a potential match in the secondary sensor.
//                      // A simple strategy is to take the first one found.
//                      // A more advanced strategy could find the one with the highest score or closest center.
//                      best_ir_match = &ir_obj;
//                      break; // Found a match, no need to search further for this EO object.
//                 }
//             }

//             // Score Fusion: If a corresponding IR detection was found, update the EO object's score.
//             if (best_ir_match != nullptr) {
//                 // Apply the score fusion formula from the document.
//                 eo_obj.prob = alpha * eo_obj.prob + (1.0f - alpha) * best_ir_match->prob;
//             }
//         }

//         // 4. The final list of detections to be sent to the tracker consists of the original EO detections.
//         //    The key difference is that their confidence scores (`prob`) may have been boosted
//         //    if they were confirmed by the thermal sensor.
//         //    The geometry (bounding box) is still determined by the primary EO sensor.
//         fused_item.fused_detections = eo_objects;
        
//         // --- END OF FUSION LOGIC ---
        
//         fused_item.t_fusion_end = std::chrono::high_resolution_clock::now();
//         m_fused_queue->push(std::move(fused_item));
//     }
//     m_fused_queue->stop();
// }

// void TrackingPipeline::postprocess_worker() {
//     // --- INITIAL SETUP ---
//     bool video_writer_initialized = false;
//     if (m_config.enable_visualization) {
//         cv::namedWindow("Object Detection", cv::WINDOW_AUTOSIZE);
//     }
    
//     auto prev_time = std::chrono::high_resolution_clock::now();
//     size_t frame_index = 0;


//     // --- NEW: Variables for Average FPS Calculation ---
//     std::chrono::high_resolution_clock::time_point processing_start_time;
//     std::chrono::high_resolution_clock::time_point processing_end_time;
//     bool first_frame = true;
//     // --- END NEW ---


//     // --- MAIN PROCESSING LOOP ---
//     while (!m_stop_flag.load()) {
//         if (!m_config.use_live_stream) {
//             show_progress_helper(frame_index, m_total_frames);
//         }
        
//         FusedFrameData item;
//         if (!m_fused_queue->pop(item)) {
//             break; // Exit if the queue is stopped and empty
//         }

//         // --- NEW: Record start time on the very first frame ---
//         if (first_frame) {
//             processing_start_time = std::chrono::high_resolution_clock::now();
//             first_frame = false;
//         }
//         // --- END NEW ---

//         // Get a reference to the frame we will draw on. This is the full-resolution EO frame.
//         auto& frame_to_draw = item.display_frame;
        
//         // Initialize VideoWriter on the first valid frame
//         if (m_config.save_output_video && !video_writer_initialized && !frame_to_draw.empty()) {
//             // double fps = m_capture_eo.get(cv::CAP_PROP_FPS);
//             double fps = 30.0;
//             init_video_writer(m_config.output_video_path, m_video_writer, fps, frame_to_draw.cols, frame_to_draw.rows);
//             video_writer_initialized = true;
//         }

//         // Run the tracker on the fused detections
//         std::vector<std::shared_ptr<byte_track::STrack>> tracked_objects = m_tracker->update(item.fused_detections, cv::Mat());

//         // --- TIMESTAMP 4: Postprocess End (after tracking, before drawing) ---
//         auto t_postprocess_end = std::chrono::high_resolution_clock::now();

//         // --- NEW: Update the end time on every processed frame ---
//         processing_end_time = t_postprocess_end;
//         // --- END NEW ---

//         // --- PROFILING LOG ---
//         if (m_config.enable_profiling_log) {
//             // ... (profiling calculation logic is the same)
//             auto dur_eo_pre = std::chrono::duration<double, std::milli>(item.t_eo_preprocess_end - item.t_eo_capture_start).count();
//             auto dur_eo_inf = std::chrono::duration<double, std::milli>(item.t_eo_inference_end - item.t_eo_preprocess_end).count();
//             auto dur_ir_pre = std::chrono::duration<double, std::milli>(item.t_ir_preprocess_end - item.t_ir_capture_start).count();
//             auto dur_ir_inf = std::chrono::duration<double, std::milli>(item.t_ir_inference_end - item.t_ir_preprocess_end).count();
//             auto slowest_inference_end = std::max(item.t_eo_inference_end, item.t_ir_inference_end);
//             auto dur_fusion = std::chrono::duration<double, std::milli>(item.t_fusion_end - slowest_inference_end).count();
//             auto dur_post = std::chrono::duration<double, std::milli>(t_postprocess_end - item.t_fusion_end).count();
//             auto earliest_capture_start = std::min(item.t_eo_capture_start, item.t_ir_capture_start);
//             auto dur_total = std::chrono::duration<double, std::milli>(t_postprocess_end - earliest_capture_start).count();

//             std::cout << "[PROFILE] Frame " << item.frame_id << ":"
//                       << " EO(Pre:" << std::fixed << std::setprecision(1) << dur_eo_pre << " Inf:" << dur_eo_inf << ")"
//                       << " IR(Pre:" << dur_ir_pre << " Inf:" << dur_ir_inf << ")"
//                       << " | Fusion: " << dur_fusion << "ms"
//                       << " | Postproc: " << dur_post << "ms"
//                       << " | Total E2E: " << dur_total << "ms"
//                       << std::endl;
//         }
        
//         // --- THIS IS THE FIX: Perform all drawing operations unconditionally ---
//         // --- before checking if visualization is enabled. ---

//         // PTZ Guidance and Target Crosshair Drawing
//         if (!tracked_objects.empty()) {
//             auto target_track_it = std::min_element(tracked_objects.begin(), tracked_objects.end(), 
//                 [](const auto& a, const auto& b) { return a->getTrackId() < b->getTrackId(); });
            
//             if ((*target_track_it)->isActivated()) {
//                 const byte_track::Rect<float>& rect = (*target_track_it)->getRect();
//                 cv::Point2f stabilized_center(rect.x() + rect.width() / 2.0f, rect.y() + rect.height() / 2.0f);
//                 cv::drawMarker(frame_to_draw, stabilized_center, cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 25, 2);
//             }
//         }

//         // Drawing all tracked bounding boxes and IDs
//         for (const auto& track : tracked_objects) {
//             const byte_track::Rect<float>& rect = track->getRect();
//             cv::Rect2f tracked_bbox(rect.x(), rect.y(), rect.width(), rect.height());
//             cv::rectangle(frame_to_draw, tracked_bbox, cv::Scalar(0, 255, 0), 2);
//             std::string label = std::to_string(track->getTrackId());
//             cv::putText(frame_to_draw, label, cv::Point(tracked_bbox.x, tracked_bbox.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
//         }

//         // FPS Display Drawing
//         auto current_time = std::chrono::high_resolution_clock::now();
//         auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(current_time - prev_time).count();
//         prev_time = current_time;
//         double fps_current = frame_time > 0 ? 1e6 / frame_time : 0;
//         std::string fps_text = "FPS: " + std::to_string(static_cast<int>(fps_current + 0.5));
//         cv::putText(frame_to_draw, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
        
//         // --- END OF UNCONDITIONAL DRAWING ---

//         // Now, the 'frame_to_draw' has all the overlays.

//         // Display the modified frame if visualization is enabled
//         if (m_config.enable_visualization) {
//             cv::imshow("Object Detection", frame_to_draw);
//             if (cv::waitKey(1) == 'q') {
//                 stop();
//             }
//         }

//         // Save the modified frame to video if saving is enabled
//         if (m_config.save_output_video && video_writer_initialized) {
//             m_video_writer.write(frame_to_draw);
//         }
//         // m_processed_frames_count++;
//         frame_index++;

//     }
//     //  Calculate and Print Average FPS after the loop finishes ---
//     if (frame_index > 0) {
//         auto total_duration = std::chrono::duration<double>(processing_end_time - processing_start_time).count();
//         double average_fps = (total_duration > 0) ? (frame_index / total_duration) : 0.0;
        
//         std::cout << "\n----------------------------------------" << std::endl;
//         std::cout << "           Processing Summary           " << std::endl;
//         std::cout << "----------------------------------------" << std::endl;
//         std::cout << "Total Frames Processed: " << frame_index << std::endl;
//         std::cout << "Total Processing Time: " << std::fixed << std::setprecision(2) << total_duration << " seconds" << std::endl;
//         std::cout << "Average End-to-End FPS: " << std::fixed << std::setprecision(2) << average_fps << std::endl;
//         std::cout << "----------------------------------------" << std::endl;

//     }
// }

// // Bridge function (no changes needed)
// std::vector<byte_track::Object> TrackingPipeline::detections_to_bytetrack_objects(
//     const std::vector<NamedBbox>& bboxes, int frame_width, int frame_height) 
// {
//     std::vector<byte_track::Object> objects;
//     objects.reserve(bboxes.size());
//     for (const auto& bbox : bboxes) {
//         float x1 = bbox.bbox.x_min * frame_width;
//         float y1 = bbox.bbox.y_min * frame_height;
//         float w = (bbox.bbox.x_max - bbox.bbox.x_min) * frame_width;
//         float h = (bbox.bbox.y_max - bbox.bbox.y_min) * frame_height;
//         objects.emplace_back(byte_track::Rect<float>(x1, y1, w, h), bbox.class_id, bbox.bbox.score);
//     }
//     return objects;
// }



// //SINGLE INFERENCE AND FUSION
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
    // m_model_eo = std::make_unique<AsyncModelInfer>(m_config.hef_path);
    // std::shared_ptr<hailort::VDevice> shared_vdevice = m_model_eo->get_vdevice();
    // m_model_ir = std::make_unique<AsyncModelInfer>(shared_vdevice, m_config.hef_path);
    
    m_model = std::make_unique<AsyncModelInfer>(m_config.hef_path);
    std::cout << "[INFO] Single Hailo model instance created for both streams." << std::endl;
    
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


// Destructor  
TrackingPipeline::~TrackingPipeline() {
    stop();
    release_resources();
}

// std::string TrackingPipeline::create_gstreamer_pipeline(bool is_live, const std::string& source_path) {
//     std::string source_element;
//     if (is_live) {
//         source_element = "v4l2src device=" + source_path;
//     } else {
//         source_element = "filesrc location=" + source_path + " ! qtdemux ! h264parse ! mppvideodec";
//     }

//     return source_element + " ! rgaconvert ! " +
//            "video/x-raw,format=BGR,width=" + std::to_string(m_config.processing_width) +
//            ",height=" + std::to_string(m_config.processing_height) + " ! " +
//            "queue ! appsink name=sink drop=true max-buffers=1";
// }


std::string TrackingPipeline::create_gstreamer_pipeline(bool is_live, const std::string& source_path) {
    std::string source_element;
    std::string fps_enforcement = " ! videorate ! video/x-raw,framerate=30/1";

    if (is_live) {
        // Live Camera: Enforce FPS immediately after capturing
        source_element = "v4l2src device=" + source_path + fps_enforcement;
    } else {
        // Video File: Decode first, then enforce FPS (drops extra frames here)
        source_element = "filesrc location=" + source_path + 
                         " ! qtdemux ! h264parse ! mppvideodec" + fps_enforcement;
    }

    return source_element + " ! rgaconvert ! " +
           "video/x-raw,format=BGR,width=" + std::to_string(m_config.processing_width) +
           ",height=" + std::to_string(m_config.processing_height) + " ! " +
           "queue ! appsink name=sink drop=true max-buffers=1";
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

void TrackingPipeline::run() {

    if (m_config.mirror_single_camera && m_config.use_live_stream) {
        auto t_start = std::chrono::high_resolution_clock::now();
        // --- SINGLE CAMERA MIRROR MODE ---
        std::cout << "[INFO] Mirroring single camera mode is ACTIVE." << std::endl;
        std::cout << "[INFO] Using '" << m_config.eo_device_path << "' for both EO and IR streams." << std::endl;

        // Create one GStreamer pipeline using the EO device path.
        std::string single_pipeline_str = create_gstreamer_pipeline(true, m_config.eo_device_path);
        
        // Launch a single capture thread that pushes to BOTH queues.
        auto capture_thread = std::async(std::launch::async, &TrackingPipeline::mirror_capture_worker, this, single_pipeline_str, m_raw_frames_queue_eo, m_raw_frames_queue_ir);
        
        // The rest of the pipeline runs as normal, unaware of the mirrored source.
        auto preprocess_eo_thread = std::async(std::launch::async, &TrackingPipeline::preprocess_worker_eo, this);
        auto preprocess_ir_thread = std::async(std::launch::async, &TrackingPipeline::preprocess_worker_ir, this);
        
        auto fusion_inference_thread = std::async(std::launch::async, &TrackingPipeline::fusion_and_inference_worker, this);
        
        auto postprocess_thread = std::async(std::launch::async, &TrackingPipeline::postprocess_worker, this);

        postprocess_thread.wait();
        stop(); 
        
        preprocess_eo_thread.wait();
        preprocess_ir_thread.wait();

        fusion_inference_thread.wait();
        capture_thread.wait(); // Wait for the single capture thread.

        auto t_end = std::chrono::high_resolution_clock::now();
    } 
    else {
        auto t_start = std::chrono::high_resolution_clock::now();
        // --- DUAL STREAM (NORMAL) MODE ---
        if (m_config.mirror_single_camera && !m_config.use_live_stream) {
             std::cout << "[WARNING] 'mirror_single_camera' is true but 'use_live_stream' is false. Mirroring is only for live cameras. Running in normal dual video file mode." << std::endl;
        }
        std::cout << "[INFO] Running in normal dual-stream mode." << std::endl;

        // Create two separate pipeline strings.
        std::string eo_pipeline_str = create_gstreamer_pipeline(m_config.use_live_stream, 
                                                                m_config.use_live_stream ? m_config.eo_device_path : m_config.eo_video_path);
        std::string ir_pipeline_str = create_gstreamer_pipeline(m_config.use_live_stream,
                                                                m_config.use_live_stream ? m_config.ir_device_path : m_config.ir_video_path);

        std::cout << "[GSTREAMER EO] " << eo_pipeline_str << std::endl;
        std::cout << "[GSTREAMER IR] " << ir_pipeline_str << std::endl;

        // Launch two separate capture threads.
        auto capture_eo_thread = std::async(std::launch::async, &TrackingPipeline::capture_worker, this, eo_pipeline_str, m_raw_frames_queue_eo);
        auto capture_ir_thread = std::async(std::launch::async, &TrackingPipeline::capture_worker, this, ir_pipeline_str, m_raw_frames_queue_ir);

        auto preprocess_eo_thread = std::async(std::launch::async, &TrackingPipeline::preprocess_worker_eo, this);
        auto preprocess_ir_thread = std::async(std::launch::async, &TrackingPipeline::preprocess_worker_ir, this);
        
        auto fusion_inference_thread = std::async(std::launch::async, &TrackingPipeline::fusion_and_inference_worker, this);
        
        auto postprocess_thread = std::async(std::launch::async, &TrackingPipeline::postprocess_worker, this);

        postprocess_thread.wait();
        stop(); 
        
        preprocess_eo_thread.wait();
        preprocess_ir_thread.wait();
   
        fusion_inference_thread.wait();
        capture_eo_thread.wait();
        capture_ir_thread.wait();

        auto t_end = std::chrono::high_resolution_clock::now();
       
    }

    std::cout << "Pipeline finished." << std::endl;
}

// worker function that reads from one camera and writes to two queues.
void TrackingPipeline::mirror_capture_worker(const std::string& pipeline_str,
                                             std::shared_ptr<BoundedTSQueue<cv::Mat>> queue_eo,
                                             std::shared_ptr<BoundedTSQueue<cv::Mat>> queue_ir) {
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
                CV_8UC3,
                map_info.data
            );
            
            // --- CORE MIRRORING LOGIC ---
            // Push a clone of the frame to BOTH queues.
            queue_eo->push(frame_wrapper.clone());
            queue_ir->push(frame_wrapper.clone());
            // --- END CORE MIRRORING LOGIC ---

            gst_buffer_unmap(buffer, &map_info);
        }
        gst_sample_unref(sample);
    }

    // Cleanup
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(appsink);
    gst_object_unref(pipeline);
    
    // Signal to BOTH downstream workers that this stream has ended.
    queue_eo->stop();
    queue_ir->stop();
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
                break;
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


float TrackingPipeline::calculate_adaptive_alpha(const cv::Mat& eo_frame) {
    constexpr float low_contrast_threshold = 20.0f;
    constexpr float high_contrast_threshold = 60.0f;
    constexpr float min_alpha = 0.4f;
    constexpr float max_alpha = 0.8f;

    cv::Mat gray_frame;
    cv::cvtColor(eo_frame, gray_frame, cv::COLOR_BGR2GRAY);

    cv::Scalar mean, stddev;
    cv::meanStdDev(gray_frame, mean, stddev);
    float current_contrast = static_cast<float>(stddev[0]);

    float alpha = min_alpha + (current_contrast - low_contrast_threshold) * 
                               (max_alpha - min_alpha) / 
                               (high_contrast_threshold - low_contrast_threshold);

    return std::clamp(alpha, min_alpha, max_alpha);
}


void TrackingPipeline::fusion_and_inference_worker() {
    const auto max_sync_delta = std::chrono::milliseconds(20);
    while (!m_stop_flag.load()) {
        // Step 1: Pop a synchronized pair of preprocessed frames.
        SingleStreamFrameData eo_item, ir_item;
        // if (!m_preprocessed_queue_eo->pop(eo_item) || !m_preprocessed_queue_ir->pop(ir_item)) {
        //     break; // A stream has ended, so we can't form any more pairs.
        // }


        // --- [CORRECTED SYNCHRONIZATION LOGIC] ---
        
        // Step 1: Peek at the heads of both queues without removing the items.
        bool has_eo = m_preprocessed_queue_eo->try_peek(eo_item);
        bool has_ir = m_preprocessed_queue_ir->try_peek(ir_item);

        // Step 2: Handle cases where one or both queues are empty.
        if (!has_eo || !has_ir) {
            // If either stream has permanently ended (stopped and empty), we can't make any more pairs, so exit.
            if ((m_preprocessed_queue_eo->is_stopped() && !has_eo) || 
                (m_preprocessed_queue_ir->is_stopped() && !has_ir)) {
                break;
            }
            // One queue is temporarily empty. Wait briefly to avoid a busy-loop consuming 100% CPU.
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue; // Go back to the start of the while loop and check again.
        }

        // Step 3: We have a frame from each queue. Compare their timestamps.
        auto time_diff = eo_item.t_capture_start - ir_item.t_capture_start;

        if (time_diff > max_sync_delta) {
            // EO is too far ahead of IR. Discard the older IR frame to let it catch up.
            m_preprocessed_queue_ir->pop(ir_item); // Pop and discard the old frame.
            continue; // Loop immediately to compare the new IR frame against the same EO frame.
        } 
        else if (time_diff < -max_sync_delta) {
            // IR is too far ahead of EO. Discard the older EO frame to let it catch up.
            m_preprocessed_queue_eo->pop(eo_item); // Pop and discard the old frame.
            continue; // Loop immediately to compare the new EO frame against the same IR frame.
        }
        else {
            // The frames are a good match! Now we can officially pop them from the queues.
            m_preprocessed_queue_eo->pop(eo_item);
            m_preprocessed_queue_ir->pop(ir_item);
        }
        
        // --- [END OF CORRECTION] ---

        // Step 2: Use std::promise and std::future for synchronization.
        std::promise<void> eo_promise, ir_promise;
        std::future<void> eo_future = eo_promise.get_future();
        std::future<void> ir_future = ir_promise.get_future();

        // Step 3: Submit EO inference job.
        m_model->infer(
            std::make_shared<cv::Mat>(eo_item.resized_for_infer),
            [&eo_item, &eo_promise](const auto& info, const auto& outs, const auto& guards) mutable {
                if (info.status == HAILO_SUCCESS) {
                    eo_item.t_inference_end = std::chrono::high_resolution_clock::now();
                    eo_item.output_data_and_infos = outs;
                    eo_item.output_guards = guards;
                } else {
                    std::cerr << "[ERROR] EO Inference failed with status: " << info.status << std::endl;
                }
                eo_promise.set_value();
            });

        // Step 4: Submit IR inference job.
        m_model->infer(
            std::make_shared<cv::Mat>(ir_item.resized_for_infer),
            [&ir_item, &ir_promise](const auto& info, const auto& outs, const auto& guards) mutable {
                if (info.status == HAILO_SUCCESS) {
                    ir_item.t_inference_end = std::chrono::high_resolution_clock::now();
                    ir_item.output_data_and_infos = outs;
                    ir_item.output_guards = guards;
                } else {
                    std::cerr << "[ERROR] IR Inference failed with status: " << info.status << std::endl;
                }
                ir_promise.set_value();
            });

        // Step 5: Wait for both inference jobs to complete.
        eo_future.wait();
        ir_future.wait();

        // Step 6: Proceed with fusion logic.
        FusedFrameData fused_item;
        fused_item.frame_id = eo_item.frame_id;
        fused_item.display_frame = eo_item.org_frame.clone();

        // Copy all timestamps from both completed items.
        fused_item.t_eo_capture_start = eo_item.t_capture_start;
        fused_item.t_eo_preprocess_end = eo_item.t_preprocess_end;
        fused_item.t_eo_inference_end = eo_item.t_inference_end;
        fused_item.t_ir_capture_start = ir_item.t_capture_start;
        fused_item.t_ir_preprocess_end = ir_item.t_preprocess_end;
        fused_item.t_ir_inference_end = ir_item.t_inference_end;

        // Check if inference results are valid before parsing
        if (eo_item.output_data_and_infos.empty() || ir_item.output_data_and_infos.empty()) {
            std::cerr << "[WARNING] Missing inference results for frame " << eo_item.frame_id << ". Skipping fusion for this frame." << std::endl;
            fused_item.t_fusion_end = std::chrono::high_resolution_clock::now();
            fused_item.alpha_calculation_duration = std::chrono::milliseconds(0); // Set duration to zero
            m_fused_queue->push(std::move(fused_item)); // Push an item with empty detections
            continue;
        }
        
        // --- EO->THERMAL HANDOVER & SCORE FUSION LOGIC ---

        double current_zoom = m_config.default_zoom;
        double current_focus = m_config.default_focus;

        // Get native EO detections
        int eo_proc_width = m_config.enable_center_crop ? m_config.crop_size : eo_item.org_frame.cols;
        int eo_proc_height = m_config.enable_center_crop ? m_config.crop_size : eo_item.org_frame.rows;
        auto eo_bboxes = parse_nms_data(eo_item.output_data_and_infos[0].first, m_config.class_count);
        std::vector<byte_track::Object> eo_objects = detections_to_bytetrack_objects(eo_bboxes, eo_proc_width, eo_proc_height);
        for (auto& obj : eo_objects) {
            obj.rect.x() += eo_item.crop_offset.x;
            obj.rect.y() += eo_item.crop_offset.y;
        }

        // Get native IR detections
        int ir_proc_width = m_config.enable_center_crop ? m_config.crop_size : ir_item.org_frame.cols;
        int ir_proc_height = m_config.enable_center_crop ? m_config.crop_size : ir_item.org_frame.rows;
        auto ir_bboxes = parse_nms_data(ir_item.output_data_and_infos[0].first, m_config.class_count);
        std::vector<byte_track::Object> ir_objects = detections_to_bytetrack_objects(ir_bboxes, ir_proc_width, ir_proc_height);
        for (auto& obj : ir_objects) {
            obj.rect.x() += ir_item.crop_offset.x;
            obj.rect.y() += ir_item.crop_offset.y;
        }

        const float search_roi_size = 80.0f;
        
        // This block handles the adaptive alpha logic based on the feature flag.
        // float alpha;
        // auto t_alpha_start = std::chrono::high_resolution_clock::now();

        // if (m_config.enable_adaptive_alpha) {
        //     // Calculate alpha dynamically based on EO frame contrast
        //     alpha = calculate_adaptive_alpha(eo_item.org_frame);
        // } else {
        //     //  default alpha when the feature is disabled
        //     alpha = 0.7f;
        // }

        float alpha = eo_item.adaptive_alpha_value;

        auto t_alpha_end = std::chrono::high_resolution_clock::now();
        // fused_item.alpha_calculation_duration = std::chrono::duration<double, std::milli>(t_alpha_end - t_alpha_start);
        fused_item.alpha_calculation_duration = std::chrono::milliseconds(0); 


        for (auto& eo_obj : eo_objects) {
            cv::Point2f eo_center(eo_obj.rect.x() + eo_obj.rect.width() / 2.0f,
                                  eo_obj.rect.y() + eo_obj.rect.height() / 2.0f);
            
            cv::Point2f projected_ir_center = transformEoToIr(eo_center, current_zoom, current_focus);

            byte_track::Object* best_ir_match = nullptr;
            cv::Rect2f search_roi(projected_ir_center.x - search_roi_size / 2.0f,
                                  projected_ir_center.y - search_roi_size / 2.0f,
                                  search_roi_size, search_roi_size);

            for (auto& ir_obj : ir_objects) {
                cv::Point2f ir_center(ir_obj.rect.x() + ir_obj.rect.width() / 2.0f,
                                      ir_obj.rect.y() + ir_obj.rect.height() / 2.0f);

                if (search_roi.contains(ir_center)) {
                     best_ir_match = &ir_obj;
                     break;
                }
            }

            if (best_ir_match != nullptr) {
                eo_obj.prob = alpha * eo_obj.prob + (1.0f - alpha) * best_ir_match->prob;
            }
        }

        fused_item.fused_detections = eo_objects;
        
        // --- END OF FUSION LOGIC ---
        
        fused_item.t_fusion_end = std::chrono::high_resolution_clock::now();
        m_fused_queue->push(std::move(fused_item));
    }
    m_fused_queue->stop();
}

// void TrackingPipeline::fusion_and_inference_worker() {
//     while (!m_stop_flag.load()) {
//         // Step 1: Pop a synchronized pair of preprocessed frames.
//         // This will block until both an EO and an IR frame are ready.
//         SingleStreamFrameData eo_item, ir_item;
//         if (!m_preprocessed_queue_eo->pop(eo_item) || !m_preprocessed_queue_ir->pop(ir_item)) {
//             break; // A stream has ended, so we can't form any more pairs.
//         }

//         // Step 2: Use std::promise and std::future to manage the completion of the two parallel inference jobs.
//         std::promise<void> eo_promise, ir_promise;
//         std::future<void> eo_future = eo_promise.get_future();
//         std::future<void> ir_future = ir_promise.get_future();

//         // Step 3: Submit the EO inference job to the single, shared model instance.
//         m_model->infer(
//             std::make_shared<cv::Mat>(eo_item.resized_for_infer),
//             // The lambda callback will execute when the EO inference is complete.
//             [&eo_item, &eo_promise](const auto& info, const auto& outs, const auto& guards) mutable {
//                 if (info.status == HAILO_SUCCESS) {
//                     // Populate the item with the results.
//                     eo_item.t_inference_end = std::chrono::high_resolution_clock::now();
//                     eo_item.output_data_and_infos = outs;
//                     eo_item.output_guards = guards;
//                 } else {
//                     std::cerr << "[ERROR] EO Inference failed with status: " << info.status << std::endl;
//                 }
//                 // Notify the future that this job is done (successfully or not).
//                 eo_promise.set_value();
//             });

//         // Step 4: Submit the IR inference job to the same shared model instance.
//         m_model->infer(
//             std::make_shared<cv::Mat>(ir_item.resized_for_infer),
//             // The lambda callback will execute when the IR inference is complete.
//             [&ir_item, &ir_promise](const auto& info, const auto& outs, const auto& guards) mutable {
//                 if (info.status == HAILO_SUCCESS) {
//                     ir_item.t_inference_end = std::chrono::high_resolution_clock::now();
//                     ir_item.output_data_and_infos = outs;
//                     ir_item.output_guards = guards;
//                 } else {
//                     std::cerr << "[ERROR] IR Inference failed with status: " << info.status << std::endl;
//                 }
//                 // Notify the future that this job is done.
//                 ir_promise.set_value();
//             });

//         // Step 5: Wait here until BOTH inference jobs for this frame pair are complete.
//         // This is the key synchronization point that provides backpressure to the pipeline.
//         eo_future.wait();
//         ir_future.wait();

//         // Step 6: Now that both inferences are done, proceed with the fusion logic.
        
//         FusedFrameData fused_item;
//         fused_item.frame_id = eo_item.frame_id;
//         fused_item.display_frame = eo_item.org_frame.clone();

//         // Copy all timestamps from both completed items.
//         fused_item.t_eo_capture_start = eo_item.t_capture_start;
//         fused_item.t_eo_preprocess_end = eo_item.t_preprocess_end;
//         fused_item.t_eo_inference_end = eo_item.t_inference_end;
//         fused_item.t_ir_capture_start = ir_item.t_capture_start;
//         fused_item.t_ir_preprocess_end = ir_item.t_preprocess_end;
//         fused_item.t_ir_inference_end = ir_item.t_inference_end;

//         // Check if inference results are valid before parsing
//         if (eo_item.output_data_and_infos.empty() || ir_item.output_data_and_infos.empty()) {
//             std::cerr << "[WARNING] Missing inference results for frame " << eo_item.frame_id << ". Skipping fusion for this frame." << std::endl;
//             fused_item.t_fusion_end = std::chrono::high_resolution_clock::now();
//             m_fused_queue->push(std::move(fused_item)); // Push an item with empty detections
//             continue;
//         }
        
//         // --- EO->THERMAL HANDOVER & SCORE FUSION LOGIC ---

//         double current_zoom = m_config.default_zoom;
//         double current_focus = m_config.default_focus;

//         // Get native EO detections
//         int eo_proc_width = m_config.enable_center_crop ? m_config.crop_size : eo_item.org_frame.cols;
//         int eo_proc_height = m_config.enable_center_crop ? m_config.crop_size : eo_item.org_frame.rows;
//         auto eo_bboxes = parse_nms_data(eo_item.output_data_and_infos[0].first, m_config.class_count);
//         std::vector<byte_track::Object> eo_objects = detections_to_bytetrack_objects(eo_bboxes, eo_proc_width, eo_proc_height);
//         for (auto& obj : eo_objects) {
//             obj.rect.x() += eo_item.crop_offset.x;
//             obj.rect.y() += eo_item.crop_offset.y;
//         }

//         // Get native IR detections
//         int ir_proc_width = m_config.enable_center_crop ? m_config.crop_size : ir_item.org_frame.cols;
//         int ir_proc_height = m_config.enable_center_crop ? m_config.crop_size : ir_item.org_frame.rows;
//         auto ir_bboxes = parse_nms_data(ir_item.output_data_and_infos[0].first, m_config.class_count);
//         std::vector<byte_track::Object> ir_objects = detections_to_bytetrack_objects(ir_bboxes, ir_proc_width, ir_proc_height);
//         for (auto& obj : ir_objects) {
//             obj.rect.x() += ir_item.crop_offset.x;
//             obj.rect.y() += ir_item.crop_offset.y;
//         }

//         const float search_roi_size = 80.0f;
//         const float alpha = 0.7f;

//         for (auto& eo_obj : eo_objects) {
//             cv::Point2f eo_center(eo_obj.rect.x() + eo_obj.rect.width() / 2.0f,
//                                   eo_obj.rect.y() + eo_obj.rect.height() / 2.0f);
            
//             cv::Point2f projected_ir_center = transformEoToIr(eo_center, current_zoom, current_focus);

//             byte_track::Object* best_ir_match = nullptr;
//             cv::Rect2f search_roi(projected_ir_center.x - search_roi_size / 2.0f,
//                                   projected_ir_center.y - search_roi_size / 2.0f,
//                                   search_roi_size, search_roi_size);

//             for (auto& ir_obj : ir_objects) {
//                 cv::Point2f ir_center(ir_obj.rect.x() + ir_obj.rect.width() / 2.0f,
//                                       ir_obj.rect.y() + ir_obj.rect.height() / 2.0f);

//                 if (search_roi.contains(ir_center)) {
//                      best_ir_match = &ir_obj;
//                      break;
//                 }
//             }

//             if (best_ir_match != nullptr) {
//                 eo_obj.prob = alpha * eo_obj.prob + (1.0f - alpha) * best_ir_match->prob;
//             }
//         }

//         fused_item.fused_detections = eo_objects;
        
//         // --- END OF FUSION LOGIC ---
        
//         fused_item.t_fusion_end = std::chrono::high_resolution_clock::now();
//         m_fused_queue->push(std::move(fused_item));
//     }
//     m_fused_queue->stop();
// }

// void TrackingPipeline::preprocess_worker_eo() {
//     int frame_counter = 0;
//     while (!m_stop_flag.load()) {
//         cv::Mat processed_frame;
//         if (!m_raw_frames_queue_eo->pop(processed_frame)) break;

//         SingleStreamFrameData item;
//         item.frame_id = frame_counter++;
//         item.t_capture_start = std::chrono::high_resolution_clock::now();
        
//         // The frame from GStreamer is already the correct processing size (e.g., 512x512)
//         item.org_frame = processed_frame.clone();

//         // The only remaining task is resizing to the model's input size.
//         // No stabilization or cropping logic is needed here anymore.
//         cv::resize(processed_frame, item.resized_for_infer, 
//                    cv::Size(m_config.model_input_width, m_config.model_input_height), 
//                    0, 0, cv::INTER_LINEAR);
        
//         // Since stabilization is disabled, the affine matrix is identity.
//         item.affine_matrix = cv::Mat::eye(2, 3, CV_64F);
//         item.crop_offset = cv::Point(0, 0);

//         item.t_preprocess_end = std::chrono::high_resolution_clock::now();
//         m_preprocessed_queue_eo->push(std::move(item));
//     }
//     m_preprocessed_queue_eo->stop();
// }

void TrackingPipeline::preprocess_worker_eo() {
    int frame_counter = 0;
    while (!m_stop_flag.load()) {
        cv::Mat processed_frame;
        if (!m_raw_frames_queue_eo->pop(processed_frame)) break;

        SingleStreamFrameData item;
        item.frame_id = frame_counter++;
        item.t_capture_start = std::chrono::high_resolution_clock::now();
        
        // Store the original frame for display/fusion later
        item.org_frame = processed_frame.clone();

        // --- CENTER CROP LOGIC ---
        if (m_config.enable_center_crop) {
            int img_w = processed_frame.cols;
            int img_h = processed_frame.rows;
            int crop_size = m_config.crop_size;

            // Ensure crop size isn't larger than the image
            if (crop_size > img_w) crop_size = img_w;
            if (crop_size > img_h) crop_size = img_h;

            // Calculate Top-Left corner (The Offset)
            // Using floating point math for center precision, then casting to int
            int x_off = static_cast<int>((img_w - crop_size) / 2.0f);
            int y_off = static_cast<int>((img_h - crop_size) / 2.0f);

            // Store this EXACT offset for the fusion worker to add back later
            item.crop_offset = cv::Point(x_off, y_off);

            // Create the Region of Interest (ROI)
            cv::Rect roi(x_off, y_off, crop_size, crop_size);
            
            // Extract the crop
            cv::Mat crop_img = processed_frame(roi);

            // Resize ONLY the crop to the model input size (e.g., 640x640)
            // This ensures the model sees the "zoomed in" view full scale
            cv::resize(crop_img, item.resized_for_infer, 
                       cv::Size(m_config.model_input_width, m_config.model_input_height), 
                       0, 0, cv::INTER_LINEAR);
        } 
        else {
            // Standard Resize (No Crop)
            item.crop_offset = cv::Point(0, 0);
            cv::resize(processed_frame, item.resized_for_infer, 
                       cv::Size(m_config.model_input_width, m_config.model_input_height), 
                       0, 0, cv::INTER_LINEAR);
        }

        // --- ADD THIS BLOCK HERE ---
        if (m_config.enable_adaptive_alpha) {
            // This runs on CPU. Doing it here prevents blocking the Fusion thread later.
            item.adaptive_alpha_value = calculate_adaptive_alpha(item.org_frame);
        } else {
            item.adaptive_alpha_value = 0.7f; // Default static value
        }
        // ---------------------------

        // No stabilization in this step, so identity matrix
        item.affine_matrix = cv::Mat::eye(2, 3, CV_64F);

        item.t_preprocess_end = std::chrono::high_resolution_clock::now();
        m_preprocessed_queue_eo->push(std::move(item));
    }
    m_preprocessed_queue_eo->stop();
}

// void TrackingPipeline::preprocess_worker_ir() {
//     int frame_counter = 0;
//     while (!m_stop_flag.load()) {
//         cv::Mat processed_frame;
//         if (!m_raw_frames_queue_ir->pop(processed_frame)) break;

//         SingleStreamFrameData item;
//         item.frame_id = frame_counter++;
//         item.t_capture_start = std::chrono::high_resolution_clock::now();

//         item.org_frame = processed_frame.clone();
//         cv::resize(processed_frame, item.resized_for_infer, 
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

        // --- [REVERTED] ---
        // The frame from the new GStreamer pipeline is already clean BGR.
        // No extra copy or conversion is needed.
        item.org_frame = processed_frame.clone();
        // --- END REVERTED ---

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
        // m_model_eo->infer(std::make_shared<cv::Mat>(item.resized_for_infer),
        m_model->infer(std::make_shared<cv::Mat>(item.resized_for_infer),
            [item = std::move(item), queue = m_results_queue_eo](const auto&, const auto& outs, const auto& guards) mutable {
                item.t_inference_end = std::chrono::high_resolution_clock::now();
                item.output_data_and_infos = outs;
                item.output_guards = guards;
                queue->push(std::move(item));
            }
        );
    }
    m_results_queue_eo->stop();
}

void TrackingPipeline::inference_worker_ir() {
    while (!m_stop_flag.load()) {
        SingleStreamFrameData item;
        if (!m_preprocessed_queue_ir->pop(item)) break;
        // m_model_ir->infer(std::make_shared<cv::Mat>(item.resized_for_infer),
        m_model->infer(std::make_shared<cv::Mat>(item.resized_for_infer),
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


//         if (!m_results_queue_eo->pop(eo_item)) {
//             break; // EO stream ended, break the loop.
//         }
//         if (!m_results_queue_ir->pop(ir_item)) {
//             break; // IR stream ended, break the loop.
//         }

//         // Prepare the output data structure for this frame pair.
//         FusedFrameData fused_item;
//         fused_item.frame_id = eo_item.frame_id; // Use EO frame ID as the reference for the fused frame.
//         fused_item.display_frame = eo_item.org_frame.clone(); // Use the original EO frame for display.

//         // Copy all timestamps from both streams for detailed profiling.
//         fused_item.t_eo_capture_start = eo_item.t_capture_start;
//         fused_item.t_eo_preprocess_end = eo_item.t_preprocess_end;
//         fused_item.t_eo_inference_end = eo_item.t_inference_end;
//         fused_item.t_ir_capture_start = ir_item.t_capture_start;
//         fused_item.t_ir_preprocess_end = ir_item.t_preprocess_end;
//         fused_item.t_ir_inference_end = ir_item.t_inference_end;

//         // --- EO->THERMAL HANDOVER & SCORE FUSION STRATEGY ---

//         // Get current zoom/focus from the camera. For this demo, we use default values from the config.
//         // In a real system, you would get these values from the camera's SDK each frame.
//         double current_zoom = m_config.default_zoom;
//         double current_focus = m_config.default_focus;

//         // 1. Get native EO detections (Primary Sensor) and convert to full frame coordinates.
//         int eo_proc_width = m_config.enable_center_crop ? m_config.crop_size : eo_item.org_frame.cols;
//         int eo_proc_height = m_config.enable_center_crop ? m_config.crop_size : eo_item.org_frame.rows;
//         auto eo_bboxes = parse_nms_data(eo_item.output_data_and_infos[0].first, m_config.class_count);
//         std::vector<byte_track::Object> eo_objects = detections_to_bytetrack_objects(eo_bboxes, eo_proc_width, eo_proc_height);
//         for (auto& obj : eo_objects) {
//             obj.rect.x() += eo_item.crop_offset.x;
//             obj.rect.y() += eo_item.crop_offset.y;
//         }

//         // 2. Get native IR detections (Secondary Sensor) and convert to full frame coordinates.
//         int ir_proc_width = m_config.enable_center_crop ? m_config.crop_size : ir_item.org_frame.cols;
//         int ir_proc_height = m_config.enable_center_crop ? m_config.crop_size : ir_item.org_frame.rows;
//         auto ir_bboxes = parse_nms_data(ir_item.output_data_and_infos[0].first, m_config.class_count);
//         std::vector<byte_track::Object> ir_objects = detections_to_bytetrack_objects(ir_bboxes, ir_proc_width, ir_proc_height);
//         for (auto& obj : ir_objects) {
//             obj.rect.x() += ir_item.crop_offset.x;
//             obj.rect.y() += ir_item.crop_offset.y;
//         }

//         // 3. For each EO detection, search for a corresponding IR detection to validate and boost its score.
//         const float search_roi_size = 80.0f; // Search radius in pixels (e.g., +/- 80px), as suggested in the doc.
//         const float alpha = 0.7;             // Weight for EO score (e.g., for daytime). Can be scheduled.

//         // This loop modifies the `eo_objects` vector in place.
//         for (auto& eo_obj : eo_objects) {
//             // Handover: Project the EO detection's center point into the IR image space.
//             cv::Point2f eo_center(eo_obj.rect.x() + eo_obj.rect.width() / 2.0f,
//                                   eo_obj.rect.y() + eo_obj.rect.height() / 2.0f);
            
//             cv::Point2f projected_ir_center = transformEoToIr(eo_center, current_zoom, current_focus);

//             // Seed: Search for a matching IR detection within the defined ROI around the projected center.
//             byte_track::Object* best_ir_match = nullptr;
            
//             // Define the square search Region of Interest (ROI) in the IR image.
//             cv::Rect2f search_roi(projected_ir_center.x - search_roi_size / 2.0f,
//                                   projected_ir_center.y - search_roi_size / 2.0f,
//                                   search_roi_size, search_roi_size);

//             for (auto& ir_obj : ir_objects) {
//                 // Check if the center of the current IR object falls within our search ROI.
//                 cv::Point2f ir_center(ir_obj.rect.x() + ir_obj.rect.width() / 2.0f,
//                                       ir_obj.rect.y() + ir_obj.rect.height() / 2.0f);

//                 if (search_roi.contains(ir_center)) {
//                      // We found a potential match in the secondary sensor.
//                      // A simple strategy is to take the first one found.
//                      // A more advanced strategy could find the one with the highest score or closest center.
//                      best_ir_match = &ir_obj;
//                      break; // Found a match, no need to search further for this EO object.
//                 }
//             }

//             // Score Fusion: If a corresponding IR detection was found, update the EO object's score.
//             if (best_ir_match != nullptr) {
//                 // Apply the score fusion formula from the document.
//                 eo_obj.prob = alpha * eo_obj.prob + (1.0f - alpha) * best_ir_match->prob;
//             }
//         }

//         // 4. The final list of detections to be sent to the tracker consists of the original EO detections.
//         //    The key difference is that their confidence scores (`prob`) may have been boosted
//         //    if they were confirmed by the thermal sensor.
//         //    The geometry (bounding box) is still determined by the primary EO sensor.
//         fused_item.fused_detections = eo_objects;
        
//         // --- END OF FUSION LOGIC ---
        
//         fused_item.t_fusion_end = std::chrono::high_resolution_clock::now();
//         m_fused_queue->push(std::move(fused_item));
//     }
//     m_fused_queue->stop();
// }

void TrackingPipeline::postprocess_worker() {
    // --- INITIAL SETUP ---
    bool video_writer_initialized = false;
    if (m_config.enable_visualization) {
        cv::namedWindow("Object Detection", cv::WINDOW_AUTOSIZE);
    }
    
    auto prev_time = std::chrono::high_resolution_clock::now();
    size_t frame_index = 0;


    // --- NEW: Variables for Average FPS Calculation ---
    std::chrono::high_resolution_clock::time_point processing_start_time;
    std::chrono::high_resolution_clock::time_point processing_end_time;
    bool first_frame = true;
    // --- END NEW ---


    // --- MAIN PROCESSING LOOP ---
    while (!m_stop_flag.load()) {
        if (!m_config.use_live_stream) {
            show_progress_helper(frame_index, m_total_frames);
        }
        
        FusedFrameData item;
        if (!m_fused_queue->pop(item)) {
            break; // Exit if the queue is stopped and empty
        }

        // --- NEW: Record start time on the very first frame ---
        if (first_frame) {
            processing_start_time = std::chrono::high_resolution_clock::now();
            first_frame = false;
        }
        // --- END NEW ---

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

        // --- NEW: Update the end time on every processed frame ---
        processing_end_time = t_postprocess_end;
        // --- END NEW ---

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
        // m_processed_frames_count++;
        frame_index++;

    }
    //  Calculate and Print Average FPS after the loop finishes ---
    if (frame_index > 0) {
        auto total_duration = std::chrono::duration<double>(processing_end_time - processing_start_time).count();
        double average_fps = (total_duration > 0) ? (frame_index / total_duration) : 0.0;
        
        std::cout << "\n----------------------------------------" << std::endl;
        std::cout << "           Processing Summary           " << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Total Frames Processed: " << frame_index << std::endl;
        std::cout << "Total Processing Time: " << std::fixed << std::setprecision(2) << total_duration << " seconds" << std::endl;
        std::cout << "Average End-to-End FPS: " << std::fixed << std::setprecision(2) << average_fps << std::endl;
        std::cout << "----------------------------------------" << std::endl;

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
