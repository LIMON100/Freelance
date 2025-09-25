// #include "tracking_pipeline.hpp"
// #include "global_stabilizer.hpp"
// #include <iostream>
// #include <iomanip>
// #include <algorithm>

// // Constructor: Initializes all members based on the config
// TrackingPipeline::TrackingPipeline(const PipelineConfig& config)
//     : m_config(config),
//       m_stop_flag(false),
//       m_total_frames(0)
// {
//     // Initialize thread-safe queues
//     m_preprocessed_queue = std::make_shared<BoundedTSQueue<PreprocessedFrameItem>>(2);
//     m_results_queue = std::make_shared<BoundedTSQueue<InferenceOutputItem>>(2);

//     // Initialize Hailo model
//     m_model = std::make_unique<AsyncModelInfer>(m_config.hef_path);

//     // Initialize Video Capture
//     if (m_config.use_live_stream) {
//         std::cout << "-I- Opening GStreamer pipeline for live camera..." << std::endl;
//         m_capture.open(m_config.gstreamer_pipeline, cv::CAP_GSTREAMER);
//     } else {
//         std::cout << "-I- Opening video file: " << m_config.input_video_path << std::endl;
//         m_capture.open(m_config.input_video_path);
//     }

//     if (!m_capture.isOpened()) {
//         std::string error_msg = m_config.use_live_stream ? 
//             "Error: Could not open GStreamer pipeline." : 
//             "Error: Could not open video file " + m_config.input_video_path;
//         throw std::runtime_error(error_msg);
//     }

//     m_total_frames = m_config.use_live_stream ? -1 : m_capture.get(cv::CAP_PROP_FRAME_COUNT);
//     double fps = m_config.use_live_stream ? 30.0 : m_capture.get(cv::CAP_PROP_FPS);

//     // Initialize the tracker
//     m_tracker = std::make_unique<byte_track::BYTETracker>(fps, m_config.track_buffer_frames);
// }

// // Destructor: Ensures resources are released
// TrackingPipeline::~TrackingPipeline() {
//     stop();
//     release_resources();
// }

// void TrackingPipeline::stop() {
//     m_stop_flag.store(true);
//     m_preprocessed_queue->stop();
//     m_results_queue->stop();
// }

// void TrackingPipeline::release_resources() {
//     if (m_config.save_output_video) {
//         m_video_writer.release();
//     }
//     if (!m_config.use_live_stream) {
//         m_capture.release();
//     }
//     if (m_config.enable_visualization) {
//         cv::destroyAllWindows();
//     }
// }

// // Main run method that launches threads
// void TrackingPipeline::run() {
//     auto t_start = std::chrono::high_resolution_clock::now();

//     auto preprocess_thread = std::async(std::launch::async, &TrackingPipeline::preprocess_worker, this);
//     auto inference_thread = std::async(std::launch::async, &TrackingPipeline::inference_worker, this);
//     auto postprocess_thread = std::async(std::launch::async, &TrackingPipeline::postprocess_worker, this);

//     // Wait for the pipeline to finish
//     postprocess_thread.wait();
//     preprocess_thread.wait();
//     inference_thread.wait();

//     auto t_end = std::chrono::high_resolution_clock::now();
//     if (!m_config.use_live_stream) {
//         print_inference_statistics(m_inference_time, m_config.hef_path, m_total_frames, t_end - t_start);
//     }
// }


// void TrackingPipeline::preprocess_worker() {
//     // Create an instance of our stabilizer class. All its state is self-contained.
//     GlobalStabilizer stabilizer;

//     while (!m_stop_flag.load()) {
//         try {
//             cv::Mat raw_frame, org_frame;
//             m_capture >> raw_frame;
//             if (raw_frame.empty()) {
//                 m_preprocessed_queue->stop();
//                 break;
//             }
            
//             if (m_config.processing_width > 0 && m_config.processing_height > 0) {
//                 cv::resize(raw_frame, org_frame, cv::Size(m_config.processing_width, m_config.processing_height));
//             } else {
//                 org_frame = raw_frame.clone();
//             }

//             cv::Mat frame_for_inference;
//             cv::Mat final_transform;

//             // --- THIS IS THE CORRECT ON/OFF SWITCH LOGIC ---
//             if (m_config.enable_global_stabilization) {
//                 // If the flag is true, run the stabilization logic.
//                 stabilizer.stabilize(org_frame, frame_for_inference, final_transform);
//             } else {
//                 // If the flag is false, just copy the original frame and use an identity matrix.
//                 frame_for_inference = org_frame.clone();
//                 final_transform = cv::Mat::eye(2, 3, CV_64F);
//             }
//             // --- END SWITCH LOGIC ---

//             cv::Mat resized_frame;
//             cv::resize(frame_for_inference, resized_frame, cv::Size(m_config.model_input_width, m_config.model_input_height), 0, 0, cv::INTER_LINEAR);
            
//             PreprocessedFrameItem item;
//             item.org_frame = org_frame.clone();
//             item.resized_for_infer = resized_frame;
            
//             cv::Mat inverse_transform;
//             cv::invertAffineTransform(final_transform, inverse_transform);
//             item.affine_matrix = inverse_transform;
            
//             m_preprocessed_queue->push(item);

//         } catch (const cv::Exception& e) {
//             std::cerr << "!!! OpenCV Exception in preprocess thread: " << e.what() << std::endl;
//             // No need to reset state here, the stabilizer class manages its own state.
//             continue;
//         }
//     }
// }

// void TrackingPipeline::inference_worker() {
//     auto start_time = std::chrono::high_resolution_clock::now();
//     while (!m_stop_flag.load()) {
//         PreprocessedFrameItem item;
//         if (!m_preprocessed_queue->pop(item)) {
//             break;
//         }

//         m_model->infer(
//             std::make_shared<cv::Mat>(item.resized_for_infer),
//             [org_frame = item.org_frame, 
//              affine_matrix = item.affine_matrix,
//              queue = m_results_queue](
//                 const hailort::AsyncInferCompletionInfo &/*info*/,
//                 const std::vector<std::pair<uint8_t*, hailo_vstream_info_t>> &output_data_and_infos,
//                 const std::vector<std::shared_ptr<uint8_t>> &output_guards)
//             {
//                 InferenceOutputItem output_item;
//                 output_item.org_frame = org_frame;
//                 output_item.affine_matrix = affine_matrix;
//                 output_item.output_data_and_infos = output_data_and_infos;
//                 output_item.output_guards = output_guards;
//                 queue->push(output_item);
//             });
//     }
//     m_results_queue->stop();
//     auto end_time = std::chrono::high_resolution_clock::now();
//     m_inference_time = end_time - start_time;
// }

// void TrackingPipeline::postprocess_worker() {
//     int org_width = m_config.processing_width > 0 ? m_config.processing_width : m_capture.get(cv::CAP_PROP_FRAME_WIDTH);
//     int org_height = m_config.processing_height > 0 ? m_config.processing_height : m_capture.get(cv::CAP_PROP_FRAME_HEIGHT);
//     double fps = m_config.use_live_stream ? 30.0 : m_capture.get(cv::CAP_PROP_FPS);

//     if (m_config.save_output_video) {
//         init_video_writer(m_config.output_video_path, m_video_writer, fps, org_width, org_height);
//     }
//     if (m_config.enable_visualization) {
//         cv::namedWindow("Object Detection", cv::WINDOW_AUTOSIZE);
//     }

//     int i = 0;
//     auto prev_time = std::chrono::high_resolution_clock::now();
    
//     while (!m_stop_flag.load()) {
//         if (!m_config.use_live_stream) show_progress_helper(i, m_total_frames);
        
//         InferenceOutputItem output_item;
//         if (!m_results_queue->pop(output_item)) {
//             break;
//         }
//         auto& frame_to_draw = output_item.org_frame;
//         auto bboxes = parse_nms_data(output_item.output_data_and_infos[0].first, m_config.class_count);

//         std::vector<byte_track::Object> objects = detections_to_bytetrack_objects(bboxes, org_width, org_height);
        
//         // The affine matrix is now correctly either the motion matrix or an identity matrix
//         std::vector<std::shared_ptr<byte_track::STrack>> tracked_objects = m_tracker->update(objects, output_item.affine_matrix);

//         if (!tracked_objects.empty()) {
//             auto target_track_it = std::min_element(tracked_objects.begin(), tracked_objects.end(), 
//                 [](const auto& a, const auto& b) { return a->getTrackId() < b->getTrackId(); });
            
//             const byte_track::Rect<float>& rect = (*target_track_it)->getRect();
//             cv::Point2f stabilized_center(rect.x() + rect.width() / 2.0f, rect.y() + rect.height() / 2.0f);
            
//             if (m_config.enable_visualization) {
//                 cv::drawMarker(frame_to_draw, stabilized_center, cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 25, 2);
//             }
//         }

//         if (m_config.enable_visualization) {
//             for (const auto& track : tracked_objects) {
//                 const byte_track::Rect<float>& rect = track->getRect();
//                 cv::Rect2f tracked_bbox(rect.x(), rect.y(), rect.width(), rect.height());
//                 cv::rectangle(frame_to_draw, tracked_bbox, cv::Scalar(0, 255, 0), 2);
//                 std::string label = std::to_string(track->getTrackId());
//                 cv::putText(frame_to_draw, label, cv::Point(tracked_bbox.x, tracked_bbox.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
//             }

//             auto current_time = std::chrono::high_resolution_clock::now();
//             auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(current_time - prev_time).count();
//             double fps_current = frame_time > 0 ? 1e6 / frame_time : 0;
//             prev_time = current_time;
//             std::string fps_text = "FPS: " + std::to_string(static_cast<int>(fps_current + 0.5));
//             cv::putText(frame_to_draw, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

//             cv::imshow("Object Detection", frame_to_draw);
//             if (cv::waitKey(1) == 'q') {
//                 stop();
//             }
//         }

//         if (m_config.save_output_video) {
//             m_video_writer.write(frame_to_draw);
//         }
//         i++;
//     }
// }

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


#include "tracking_pipeline.hpp"
#include "global_stabilizer.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

// Constructor: Initializes all members based on the config
TrackingPipeline::TrackingPipeline(const PipelineConfig& config)
    : m_config(config),
      m_stop_flag(false),
      m_total_frames(0)
{
    // Initialize thread-safe queues using the new FrameData struct
    m_preprocessed_queue = std::make_shared<BoundedTSQueue<FrameData>>(2);
    m_results_queue = std::make_shared<BoundedTSQueue<FrameData>>(2);

    // Initialize Hailo model
    m_model = std::make_unique<AsyncModelInfer>(m_config.hef_path);

    // Initialize Video Capture
    if (m_config.use_live_stream) {
        std::cout << "-I- Opening GStreamer pipeline for live camera..." << std::endl;
        m_capture.open(m_config.gstreamer_pipeline, cv::CAP_GSTREAMER);
    } else {
        std::cout << "-I- Opening video file: " << m_config.input_video_path << std::endl;
        m_capture.open(m_config.input_video_path);
    }

    if (!m_capture.isOpened()) {
        std::string error_msg = m_config.use_live_stream ? 
            "Error: Could not open GStreamer pipeline." : 
            "Error: Could not open video file " + m_config.input_video_path;
        throw std::runtime_error(error_msg);
    }

    m_total_frames = m_config.use_live_stream ? -1 : m_capture.get(cv::CAP_PROP_FRAME_COUNT);
    double fps = m_config.use_live_stream ? 30.0 : m_capture.get(cv::CAP_PROP_FPS);

    // Initialize the tracker
    m_tracker = std::make_unique<byte_track::BYTETracker>(fps, m_config.track_buffer_frames);
}

// Destructor: Ensures resources are released
TrackingPipeline::~TrackingPipeline() {
    stop();
    release_resources();
}

void TrackingPipeline::stop() {
    m_stop_flag.store(true);
    m_preprocessed_queue->stop();
    m_results_queue->stop();
}

void TrackingPipeline::release_resources() {
    if (m_config.save_output_video) {
        m_video_writer.release();
    }
    if (!m_config.use_live_stream) {
        m_capture.release();
    }
    if (m_config.enable_visualization) {
        cv::destroyAllWindows();
    }
}

// Main run method that launches threads
void TrackingPipeline::run() {
    auto t_start = std::chrono::high_resolution_clock::now();

    auto preprocess_thread = std::async(std::launch::async, &TrackingPipeline::preprocess_worker, this);
    auto inference_thread = std::async(std::launch::async, &TrackingPipeline::inference_worker, this);
    auto postprocess_thread = std::async(std::launch::async, &TrackingPipeline::postprocess_worker, this);

    // Wait for the pipeline to finish
    postprocess_thread.wait();
    preprocess_thread.wait();
    inference_thread.wait();

    auto t_end = std::chrono::high_resolution_clock::now();
    if (!m_config.use_live_stream) {
        print_inference_statistics(m_inference_time, m_config.hef_path, m_total_frames, t_end - t_start);
    }
}

// --- WORKER THREAD IMPLEMENTATIONS ---

void TrackingPipeline::preprocess_worker() {
    GlobalStabilizer stabilizer;
    int frame_counter = 0;

    while (!m_stop_flag.load()) {
        // Create a new FrameData object for this frame
        FrameData item;
        item.frame_id = frame_counter++;
        
        // --- TIMESTAMP 1: Capture Start ---
        item.t_capture_start = std::chrono::high_resolution_clock::now();

        cv::Mat raw_frame;
        m_capture >> raw_frame;
        if (raw_frame.empty()) {
            m_preprocessed_queue->stop();
            break;
        }
        
        if (m_config.processing_width > 0 && m_config.processing_height > 0) {
            cv::resize(raw_frame, item.org_frame, cv::Size(m_config.processing_width, m_config.processing_height));
        } else {
            item.org_frame = raw_frame.clone();
        }

        cv::Mat frame_for_inference;
        cv::Mat final_transform;

        if (m_config.enable_global_stabilization) {
            stabilizer.stabilize(item.org_frame, frame_for_inference, final_transform);
        } else {
            frame_for_inference = item.org_frame.clone();
            final_transform = cv::Mat::eye(2, 3, CV_64F);
        }

        cv::resize(frame_for_inference, item.resized_for_infer, cv::Size(m_config.model_input_width, m_config.model_input_height), 0, 0, cv::INTER_LINEAR);
        
        cv::invertAffineTransform(final_transform, item.affine_matrix);
        
        // --- TIMESTAMP 2: Preprocess End ---
        item.t_preprocess_end = std::chrono::high_resolution_clock::now();
        
        m_preprocessed_queue->push(std::move(item));
    }
}

void TrackingPipeline::inference_worker() {
    auto start_time = std::chrono::high_resolution_clock::now();
    while (!m_stop_flag.load()) {
        FrameData item;
        if (!m_preprocessed_queue->pop(item)) {
            break;
        }

        m_model->infer(
            std::make_shared<cv::Mat>(item.resized_for_infer),
            [item = std::move(item), queue = m_results_queue](
                const hailort::AsyncInferCompletionInfo &/*info*/,
                const std::vector<std::pair<uint8_t*, hailo_vstream_info_t>> &output_data_and_infos,
                const std::vector<std::shared_ptr<uint8_t>> &output_guards) mutable
            {
                // --- TIMESTAMP 3: Inference End ---
                item.t_inference_end = std::chrono::high_resolution_clock::now();

                item.output_data_and_infos = output_data_and_infos;
                item.output_guards = output_guards;
                
                queue->push(std::move(item));
            });
    }
    m_results_queue->stop();
    auto end_time = std::chrono::high_resolution_clock::now();
    m_inference_time = end_time - start_time;
}

// void TrackingPipeline::postprocess_worker() {
//     int org_width = m_config.processing_width > 0 ? m_config.processing_width : m_capture.get(cv::CAP_PROP_FRAME_WIDTH);
//     int org_height = m_config.processing_height > 0 ? m_config.processing_height : m_capture.get(cv::CAP_PROP_FRAME_HEIGHT);
//     double fps = m_config.use_live_stream ? 30.0 : m_capture.get(cv::CAP_PROP_FPS);

//     if (m_config.save_output_video) {
//         init_video_writer(m_config.output_video_path, m_video_writer, fps, org_width, org_height);
//     }
//     if (m_config.enable_visualization) {
//         cv::namedWindow("Object Detection", cv::WINDOW_AUTOSIZE);
//     }
    
//     while (!m_stop_flag.load()) {
//         if (!m_config.use_live_stream) show_progress_helper(m_total_frames > 0 ? (size_t)m_capture.get(cv::CAP_PROP_POS_FRAMES) : 0, m_total_frames);
        
//         FrameData item;
//         if (!m_results_queue->pop(item)) {
//             break;
//         }
//         auto& frame_to_draw = item.org_frame;
//         auto bboxes = parse_nms_data(item.output_data_and_infos[0].first, m_config.class_count);

//         std::vector<byte_track::Object> objects = detections_to_bytetrack_objects(bboxes, org_width, org_height);
//         std::vector<std::shared_ptr<byte_track::STrack>> tracked_objects = m_tracker->update(objects, item.affine_matrix);

//         if (!tracked_objects.empty()) {
//             auto target_track_it = std::min_element(tracked_objects.begin(), tracked_objects.end(), 
//                 [](const auto& a, const auto& b) { return a->getTrackId() < b->getTrackId(); });
            
//             const byte_track::Rect<float>& rect = (*target_track_it)->getRect();
//             cv::Point2f stabilized_center(rect.x() + rect.width() / 2.0f, rect.y() + rect.height() / 2.0f);
            
//             if (m_config.enable_visualization) {
//                 cv::drawMarker(frame_to_draw, stabilized_center, cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 25, 2);
//             }
//         }

//         // --- TIMESTAMP 4: Postprocess End (before visualization) ---
//         item.t_postprocess_end = std::chrono::high_resolution_clock::now();

//         // --- DURATION CALCULATION (Moved outside the 'if' block) ---
//         auto dur_preprocess = std::chrono::duration<double, std::milli>(item.t_preprocess_end - item.t_capture_start).count();
//         auto dur_inference = std::chrono::duration<double, std::milli>(item.t_inference_end - item.t_preprocess_end).count();
//         auto dur_postprocess = std::chrono::duration<double, std::milli>(item.t_postprocess_end - item.t_inference_end).count();
//         auto dur_total = std::chrono::duration<double, std::milli>(item.t_postprocess_end - item.t_capture_start).count();

//         // --- PROFILING LOG ---
//         if (m_config.enable_profiling_log) {
//             std::cout << "[PROFILE] Frame " << item.frame_id
//                       << ": Preproc: " << std::fixed << std::setprecision(1) << dur_preprocess << "ms"
//                       << " | Infer: " << std::fixed << std::setprecision(1) << dur_inference << "ms"
//                       << " | Postproc: " << std::fixed << std::setprecision(1) << dur_postprocess << "ms"
//                       << " | Total E2E: " << std::fixed << std::setprecision(1) << dur_total << "ms"
//                       << std::endl;
//         }
        
//         // --- VISUALIZATION LOGIC ---
//         if (m_config.enable_visualization) {
//             for (const auto& track : tracked_objects) {
//                 const byte_track::Rect<float>& rect = track->getRect();
//                 cv::Rect2f tracked_bbox(rect.x(), rect.y(), rect.width(), rect.height());
//                 cv::rectangle(frame_to_draw, tracked_bbox, cv::Scalar(0, 255, 0), 2);
//                 std::string label = std::to_string(track->getTrackId());
//                 cv::putText(frame_to_draw, label, cv::Point(tracked_bbox.x, tracked_bbox.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
//             }

//             // FPS calculation now uses the 'dur_total' which is in the correct scope
//             double fps_current = dur_total > 0 ? 1000.0 / dur_total : 0;
//             std::string fps_text = "FPS: " + std::to_string(static_cast<int>(fps_current + 0.5));
//             cv::putText(frame_to_draw, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

//             cv::imshow("Object Detection", frame_to_draw);
//             if (cv::waitKey(1) == 'q') {
//                 stop();
//             }
//         }

//         if (m_config.save_output_video) {
//             m_video_writer.write(frame_to_draw);
//         }
//     }
// }


void TrackingPipeline::postprocess_worker() {
    int org_width = m_config.processing_width > 0 ? m_config.processing_width : m_capture.get(cv::CAP_PROP_FRAME_WIDTH);
    int org_height = m_config.processing_height > 0 ? m_config.processing_height : m_capture.get(cv::CAP_PROP_FRAME_HEIGHT);
    double fps = m_config.use_live_stream ? 30.0 : m_capture.get(cv::CAP_PROP_FPS);

    if (m_config.save_output_video) {
        init_video_writer(m_config.output_video_path, m_video_writer, fps, org_width, org_height);
    }
    if (m_config.enable_visualization) {
        cv::namedWindow("Object Detection", cv::WINDOW_AUTOSIZE);
    }
    
    // --- FPS FIX: Restore the timer for calculating display FPS ---
    auto prev_time = std::chrono::high_resolution_clock::now();
    
    while (!m_stop_flag.load()) {
        if (!m_config.use_live_stream) show_progress_helper(m_total_frames > 0 ? (size_t)m_capture.get(cv::CAP_PROP_POS_FRAMES) : 0, m_total_frames);
        
        FrameData item;
        if (!m_results_queue->pop(item)) {
            break;
        }
        auto& frame_to_draw = item.org_frame;
        auto bboxes = parse_nms_data(item.output_data_and_infos[0].first, m_config.class_count);

        std::vector<byte_track::Object> objects = detections_to_bytetrack_objects(bboxes, org_width, org_height);
        std::vector<std::shared_ptr<byte_track::STrack>> tracked_objects = m_tracker->update(objects, item.affine_matrix);

        if (!tracked_objects.empty()) {
            auto target_track_it = std::min_element(tracked_objects.begin(), tracked_objects.end(), 
                [](const auto& a, const auto& b) { return a->getTrackId() < b->getTrackId(); });
            
            const byte_track::Rect<float>& rect = (*target_track_it)->getRect();
            cv::Point2f stabilized_center(rect.x() + rect.width() / 2.0f, rect.y() + rect.height() / 2.0f);
            
            if (m_config.enable_visualization) {
                cv::drawMarker(frame_to_draw, stabilized_center, cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 25, 2);
            }
        }

        // --- TIMESTAMP 4: Postprocess End (before visualization) ---
        item.t_postprocess_end = std::chrono::high_resolution_clock::now();

        // --- PROFILING LOG (This part remains the same) ---
        if (m_config.enable_profiling_log) {
            auto dur_preprocess = std::chrono::duration<double, std::milli>(item.t_preprocess_end - item.t_capture_start).count();
            auto dur_inference = std::chrono::duration<double, std::milli>(item.t_inference_end - item.t_preprocess_end).count();
            auto dur_postprocess = std::chrono::duration<double, std::milli>(item.t_postprocess_end - item.t_inference_end).count();
            auto dur_total = std::chrono::duration<double, std::milli>(item.t_postprocess_end - item.t_capture_start).count();

            std::cout << "[PROFILE] Frame " << item.frame_id
                      << ": Preproc: " << std::fixed << std::setprecision(1) << dur_preprocess << "ms"
                      << " | Infer: " << std::fixed << std::setprecision(1) << dur_inference << "ms"
                      << " | Postproc: " << std::fixed << std::setprecision(1) << dur_postprocess << "ms"
                      << " | Total E2E: " << std::fixed << std::setprecision(1) << dur_total << "ms"
                      << std::endl;
        }
        
        // --- VISUALIZATION LOGIC ---
        if (m_config.enable_visualization) {
            for (const auto& track : tracked_objects) {
                const byte_track::Rect<float>& rect = track->getRect();
                cv::Rect2f tracked_bbox(rect.x(), rect.y(), rect.width(), rect.height());
                cv::rectangle(frame_to_draw, tracked_bbox, cv::Scalar(0, 255, 0), 2);
                std::string label = std::to_string(track->getTrackId());
                cv::putText(frame_to_draw, label, cv::Point(tracked_bbox.x, tracked_bbox.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
            }

            // --- FPS FIX: Use the old, smoother calculation method for display ---
            auto current_time = std::chrono::high_resolution_clock::now();
            auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(current_time - prev_time).count();
            prev_time = current_time; // Update the timer for the next frame
            double fps_current = frame_time > 0 ? 1e6 / frame_time : 0;
            
            std::string fps_text = "FPS: " + std::to_string(static_cast<int>(fps_current + 0.5));
            cv::putText(frame_to_draw, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

            cv::imshow("Object Detection", frame_to_draw);
            if (cv::waitKey(1) == 'q') {
                stop();
            }
        }

        if (m_config.save_output_video) {
            m_video_writer.write(frame_to_draw);
        }
    }
}

// "Bridge" function implementation
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