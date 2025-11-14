#include "tracking_pipeline.hpp"
#include "global_stabilizer.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>

// --- GStreamer Callback Data Structure ---
struct GstCallbackData {
    std::shared_ptr<BoundedTSQueue<cv::Mat>> queue;
    std::atomic<bool>* stop_flag;                   // A flag to signal the callback to stop
    int width;                                      // Expected frame width from pipeline
    int height;                                     // Expected frame height from pipeline
};

// This C-style function is called by GStreamer from its own internal thread whenever a new frame is ready.
static GstFlowReturn on_new_sample(GstAppSink *appsink, gpointer user_data) {
    GstCallbackData* data = static_cast<GstCallbackData*>(user_data);
    if (data->stop_flag->load()) {
        return GST_FLOW_EOS; // Signal End-Of-Stream if we are stopping
    }

    GstSample *sample = gst_app_sink_pull_sample(appsink);
    if (!sample) {
        return GST_FLOW_ERROR;
    }

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {

        cv::Mat frame(cv::Size(data->width, data->height), CV_8UC3, (void*)map.data);
        
        data->queue->push(frame.clone()); 
        
        gst_buffer_unmap(buffer, &map);
    }
    
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

// Constructor
TrackingPipeline::TrackingPipeline(const PipelineConfig& config)
    : m_config(config),
      m_stop_flag(false),
      m_total_frames(0)
{
    // In TrackingPipeline constructor
    m_raw_frames_queue = std::make_shared<BoundedTSQueue<cv::Mat>>(2); 
    m_preprocessed_queue = std::make_shared<BoundedTSQueue<FrameData>>(10); 
    m_results_queue = std::make_shared<BoundedTSQueue<FrameData>>(10); 

    // Initialize Hailo model
    m_model = std::make_unique<AsyncModelInfer>(m_config.hef_path);

    // Initialize tracker
    double fps = 30.0; // Default for live stream
    // if (!m_config.use_live_stream) {
    //     m_capture.open(m_config.input_video_path);
    //     if (!m_capture.isOpened()) {
    //         throw std::runtime_error("Error: Could not open video file " + m_config.input_video_path);
    //     }
    //     m_total_frames = m_capture.get(cv::CAP_PROP_FRAME_COUNT);
    //     fps = m_capture.get(cv::CAP_PROP_FPS);
    // }
    if (!m_config.use_live_stream) {
        cv::VideoCapture temp_cap(m_config.input_video_path);
        if (!temp_cap.isOpened()) {
            throw std::runtime_error("Error: Could not open video file for metadata: " + m_config.input_video_path);
        }
        m_total_frames = temp_cap.get(cv::CAP_PROP_FRAME_COUNT);
        fps = temp_cap.get(cv::CAP_PROP_FPS);
        temp_cap.release(); // Close it immediately
    }
    m_tracker = std::make_unique<byte_track::BYTETracker>(fps, m_config.track_buffer_frames);
}

// Destructor
TrackingPipeline::~TrackingPipeline() {
    stop();
    release_resources();
}

std::string TrackingPipeline::create_gstreamer_pipeline(bool is_live, const std::string& source_path) {
    std::string source_element;
    if (is_live) {
        // High-performance pipeline for a live V4L2 camera
        source_element = "v4l2src device=" + source_path + " ! "
                         "video/x-raw,format=YUY2,width=640,height=480,framerate=30/1";
    } else {
        // High-performance pipeline for a video file (runs as fast as possible)
        source_element = "filesrc location=" + source_path + " ! qtdemux ! h264parse ! avdec_h264";
    }

    // Common elements to convert to BGR and push to the app
    return source_element + " ! videoconvert ! " +
           "video/x-raw,format=BGR ! " +
           "queue ! " +
           "appsink name=sink drop=true max-buffers=1 sync=false"; // sync=false is the key for performance!
}

void TrackingPipeline::stop() {
    m_stop_flag.store(true);
    // Stopping the queues will unblock any threads waiting on them
    m_raw_frames_queue->stop();
    m_preprocessed_queue->stop();
    m_results_queue->stop();
}

void TrackingPipeline::release_resources() {
    if (m_config.save_output_video) {
        m_video_writer.release();
    }
    if (!m_config.use_live_stream && m_capture.isOpened()) {
        m_capture.release();
    }
    if (m_config.enable_visualization) {
        cv::destroyAllWindows();
    }
}

// Main run method that launches all worker threads
void TrackingPipeline::run() {
    auto t_start = std::chrono::high_resolution_clock::now();

    // Launch all four worker threads asynchronously
    auto capture_thread = std::async(std::launch::async, &TrackingPipeline::capture_thread_worker, this);
    auto preprocess_thread = std::async(std::launch::async, &TrackingPipeline::preprocess_worker, this);
    auto inference_thread = std::async(std::launch::async, &TrackingPipeline::inference_worker, this);
    auto postprocess_thread = std::async(std::launch::async, &TrackingPipeline::postprocess_worker, this);

    // The main thread will block here until the post-processing thread finishes
    postprocess_thread.wait();
    
    // Once post-processing is done, signal all other threads to stop and wait for them to exit cleanly.
    stop(); 
    
    capture_thread.wait();
    preprocess_thread.wait();
    inference_thread.wait();

    auto t_end = std::chrono::high_resolution_clock::now();
    if (!m_config.use_live_stream) {
        print_inference_statistics(m_inference_time, m_config.hef_path, m_total_frames, t_end - t_start);
    }
}

// This thread is dedicated to capturing frames using the robust GStreamer method.
// void TrackingPipeline::capture_thread_worker() {
//     if (m_config.use_live_stream) {
//         // --- GStreamer Live Capture ---
//         gst_init(nullptr, nullptr);
//         GError *error = nullptr;
//         GstElement *pipeline = gst_parse_launch(m_config.gstreamer_pipeline.c_str(), &error);
        
//         if (!pipeline || error) {
//             std::cerr << "FATAL: GStreamer pipeline failed to parse: " << (error ? error->message : "Unknown error") << std::endl;
//             if (error) g_error_free(error);
//             stop(); // Signal all threads to stop
//             return;
//         }

//         GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
//         if (!appsink) {
//             std::cerr << "FATAL: Could not find 'appsink' element named 'sink' in GStreamer pipeline." << std::endl;
//             gst_object_unref(pipeline);
//             stop();
//             return;
//         }

//         // Set the pipeline to the PLAYING state
//         gst_element_set_state(pipeline, GST_STATE_PLAYING);

//         // --- Main GStreamer Loop ---
//         while (!m_stop_flag.load()) {
//             GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
//             if (!sample) {
//                 if (gst_app_sink_is_eos(GST_APP_SINK(appsink))) {
//                     std::cout << "GStreamer: End-Of-Stream reached." << std::endl;
//                 } else {
//                     std::cerr << "GStreamer: Error pulling sample." << std::endl;
//                 }
//                 break; // Exit loop on EOS or error
//             }

//             GstBuffer *buffer = gst_sample_get_buffer(sample);
//             GstCaps *caps = gst_sample_get_caps(sample);
//             GstVideoInfo video_info;

//             if (!buffer || !caps || !gst_video_info_from_caps(&video_info, caps)) {
//                 std::cerr << "GStreamer: Failed to get valid buffer/caps/info from sample." << std::endl;
//                 gst_sample_unref(sample);
//                 continue;
//             }

//             GstMapInfo map_info;
//             if (!gst_buffer_map(buffer, &map_info, GST_MAP_READ)) {
//                 std::cerr << "GStreamer: Failed to map buffer." << std::endl;
//                 gst_sample_unref(sample);
//                 continue;
//             }

//             // Create a cv::Mat that WRAPS the GStreamer buffer data without copying.
//             cv::Mat frame_wrapper(
//                 GST_VIDEO_INFO_HEIGHT(&video_info),
//                 GST_VIDEO_INFO_WIDTH(&video_info),
//                 CV_8UC3, // Assuming BGR format as requested in the pipeline
//                 map_info.data,
//                 GST_VIDEO_INFO_PLANE_STRIDE(&video_info, 0)
//             );

//             // This decouples the memory from the GStreamer buffer, which is essential for multi-threading.
//             m_raw_frames_queue->push(frame_wrapper.clone());

//             // Cleanup for this frame
//             gst_buffer_unmap(buffer, &map_info);
//             gst_sample_unref(sample);
//         }
        
//         // Cleanup GStreamer
//         gst_element_set_state(pipeline, GST_STATE_NULL);
//         gst_object_unref(appsink);
//         gst_object_unref(pipeline);
//         gst_deinit();
        
//     } else {
//         // --- Video File Capture (Unchanged) ---
//         while (!m_stop_flag.load()) {
//             cv::Mat frame;
//             if (!m_capture.read(frame)) {
//                 break; // End of file
//             }
//             m_raw_frames_queue->push(frame);
//         }
//     }
//     // Signal to the next stage that capture is finished
//     m_raw_frames_queue->stop();
// }


void TrackingPipeline::capture_thread_worker() {
    // Determine which source path to use
    std::string source_path = m_config.use_live_stream ? m_config.camera_device_path : m_config.input_video_path;
    
    // Build the appropriate GStreamer pipeline
    std::string pipeline_str = create_gstreamer_pipeline(m_config.use_live_stream, source_path);
    std::cout << "[INFO] Using GStreamer pipeline: " << pipeline_str << std::endl;

    // --- GStreamer Execution (now used for both modes) ---
    gst_init(nullptr, nullptr);
    GError *error = nullptr;
    GstElement *pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    
    if (!pipeline || error) {
        std::cerr << "FATAL: GStreamer pipeline failed to parse: " << (error ? error->message : "Unknown error") << std::endl;
        if (error) g_error_free(error);
        stop();
        return;
    }

    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    if (!appsink) {
        std::cerr << "FATAL: Could not find 'appsink' element named 'sink' in GStreamer pipeline." << std::endl;
        gst_object_unref(pipeline);
        stop();
        return;
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // --- Main GStreamer Loop ---
    while (!m_stop_flag.load()) {
        GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
        if (!sample) {
            if (gst_app_sink_is_eos(GST_APP_SINK(appsink))) {
                std::cout << "GStreamer: End-Of-Stream reached." << std::endl;
            } else {
                std::cerr << "GStreamer: Error pulling sample." << std::endl;
            }
            break; // Exit loop on EOS or error
        }

        GstBuffer *buffer = gst_sample_get_buffer(sample);
        GstMapInfo map_info;
        if (!gst_buffer_map(buffer, &map_info, GST_MAP_READ)) {
            std::cerr << "GStreamer: Failed to map buffer." << std::endl;
            gst_sample_unref(sample);
            continue;
        }

        // Use GstVideoInfo to correctly get dimensions and stride
        GstCaps *caps = gst_sample_get_caps(sample);
        GstVideoInfo video_info;
        if (caps && gst_video_info_from_caps(&video_info, caps)) {
            cv::Mat frame_wrapper(
                GST_VIDEO_INFO_HEIGHT(&video_info),
                GST_VIDEO_INFO_WIDTH(&video_info),
                CV_8UC3, // We requested BGR format in the pipeline
                map_info.data,
                GST_VIDEO_INFO_PLANE_STRIDE(&video_info, 0)
            );
            m_raw_frames_queue->push(frame_wrapper.clone());
        }

        gst_buffer_unmap(buffer, &map_info);
        gst_sample_unref(sample);
    }
    
    // Cleanup GStreamer
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(appsink);
    gst_object_unref(pipeline);
    gst_deinit();
    
    // Signal to the next stage that capture is finished
    m_raw_frames_queue->stop();
}

void TrackingPipeline::preprocess_worker() {
    GlobalStabilizer stabilizer;
    int frame_counter = 0;

    while (!m_stop_flag.load()) {
        cv::Mat raw_frame;
        if (!m_raw_frames_queue->pop(raw_frame)) {
            break; // Exit if capture queue is stopped and empty
        }

        FrameData item;
        item.frame_id = frame_counter++;
        item.t_capture_start = std::chrono::high_resolution_clock::now();
        
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
        item.crop_offset = cv::Point(0, 0); // Not used, but set to default
        
        item.t_preprocess_end = std::chrono::high_resolution_clock::now();
        
        m_preprocessed_queue->push(std::move(item));
    }
    m_preprocessed_queue->stop();
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


void TrackingPipeline::postprocess_worker() { 
    // --- INITIAL SETUP ---
    bool video_writer_initialized = false;

    if (m_config.enable_visualization) {
        cv::namedWindow("Object Detection", cv::WINDOW_AUTOSIZE);
    }

    if (m_config.save_processed_frames) {
        if (!fs::exists(m_config.processed_frames_dir)) {
            try {
                fs::create_directory(m_config.processed_frames_dir);
                std::cout << "-I- Created directory for processed frames: " << m_config.processed_frames_dir << std::endl;
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Error creating directory " << m_config.processed_frames_dir << ": " << e.what() << std::endl;
                m_config.save_processed_frames = false; // Disable if creation fails
            }
        }
    }
    
    auto prev_time = std::chrono::high_resolution_clock::now();
    size_t frame_index = 0;
    
    // --- MAIN PROCESSING LOOP ---
    while (!m_stop_flag.load()) {
        if (!m_config.use_live_stream) {
            show_progress_helper(m_total_frames > 0 ? (size_t)m_capture.get(cv::CAP_PROP_POS_FRAMES) : 0, m_total_frames);
        }
        
        FrameData item;
        if (!m_results_queue->pop(item)) {
            break; // Exit if the queue is stopped and empty
        }

        if (!m_config.use_live_stream) {
            show_progress_helper(item.frame_id, m_total_frames);
        }
        
        auto& frame_to_draw = item.org_frame;

        // if (m_config.save_output_video && !video_writer_initialized) {
        //     double fps = m_config.use_live_stream ? 30.0 : m_capture.get(cv::CAP_PROP_FPS);
        //     init_video_writer(m_config.output_video_path, m_video_writer, fps, frame_to_draw.cols, frame_to_draw.rows);
        //     video_writer_initialized = true;
        // }

        if (m_config.save_output_video && !video_writer_initialized) {
            // --- [FIX FOR FPS] ---
            // The m_capture object might be invalid here. Use a sensible default or pre-calculated FPS.
            // Let's get the FPS in the constructor and store it as a member variable.
            // For now, let's assume 30.0 for robustness.
            double fps = 30.0; 
            if(!m_config.use_live_stream && m_capture.isOpened()) {
                 fps = m_capture.get(cv::CAP_PROP_FPS);
            }
            init_video_writer(m_config.output_video_path, m_video_writer, fps, frame_to_draw.cols, frame_to_draw.rows);
            // --- END FIX FOR FPS ---
            video_writer_initialized = true;
        }

        auto bboxes = parse_nms_data(item.output_data_and_infos[0].first, m_config.class_count);
        
        std::vector<byte_track::Object> objects = detections_to_bytetrack_objects(bboxes, frame_to_draw.cols, frame_to_draw.rows);
        std::vector<std::shared_ptr<byte_track::STrack>> tracked_objects = m_tracker->update(objects, item.affine_matrix);

        item.t_postprocess_end = std::chrono::high_resolution_clock::now();
        

        // --- PROFILING LOG ---
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
        
        // ========================================================================
        // --- DRAWING LOGIC (MOVED OUTSIDE VISUALIZATION BLOCK) ---
        // ========================================================================
        
        // PTZ Guidance Visualization (Crosshair)
        if (!tracked_objects.empty()) {
            auto target_track_it = std::min_element(tracked_objects.begin(), tracked_objects.end(), 
                [](const auto& a, const auto& b) { return a->getTrackId() < b->getTrackId(); });
            
            if ((*target_track_it)->isActivated()) {
                const byte_track::Rect<float>& rect = (*target_track_it)->getRect();
                cv::Point2f stabilized_center(rect.x() + rect.width() / 2.0f, rect.y() + rect.height() / 2.0f);
                cv::drawMarker(frame_to_draw, stabilized_center, cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 25, 2);
            }
        }

        // Drawing all tracked objects
        for (const auto& track : tracked_objects) {
            if (track->isActivated()) {
                const byte_track::Rect<float>& rect = track->getRect();
                cv::Rect2f tracked_bbox(rect.x(), rect.y(), rect.width(), rect.height());
                cv::rectangle(frame_to_draw, tracked_bbox, cv::Scalar(0, 255, 0), 2);
                std::string label = std::to_string(track->getTrackId());
                cv::putText(frame_to_draw, label, cv::Point(tracked_bbox.x, tracked_bbox.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
            }
        }

        // FPS Display Calculation
        auto current_time = std::chrono::high_resolution_clock::now();
        auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(current_time - prev_time).count();
        prev_time = current_time;
        double fps_current = frame_time > 0 ? 1e6 / frame_time : 0;
        std::string fps_text = "FPS: " + std::to_string(static_cast<int>(fps_current + 0.5));
        cv::putText(frame_to_draw, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

        // --- END OF DRAWING LOGIC ---
        if (m_config.enable_visualization) {
            cv::imshow("Object Detection", frame_to_draw);
            if (cv::waitKey(1) == 'q') {
                stop();
            }
        }

        // --- SAVE OUTPUT VIDEO  ---
        if (m_config.save_output_video && video_writer_initialized) {
            m_video_writer.write(frame_to_draw);
        }

        // SAVE INDIVIDUAL PROCESSED FRAMES ---
        if (m_config.save_processed_frames && !tracked_objects.empty()) {
            std::string filename = m_config.processed_frames_dir + "/frame_" + std::to_string(item.frame_id) + ".jpg";
            cv::imwrite(filename, frame_to_draw);
        }
    }
}


//"Bridge" function implementation
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