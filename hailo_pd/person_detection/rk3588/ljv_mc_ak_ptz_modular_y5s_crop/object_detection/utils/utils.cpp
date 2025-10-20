// #include "utils.hpp"

// std::vector<cv::Scalar> COLORS = {
//     cv::Scalar(255,   0,   0),  // Red
//     cv::Scalar(  0, 255,   0),  // Green
//     cv::Scalar(  0,   0, 255),  // Blue
//     cv::Scalar(255, 255,   0),  // Cyan
//     cv::Scalar(255,   0, 255),  // Magenta
//     cv::Scalar(  0, 255, 255),  // Yellow
//     cv::Scalar(255, 128,   0),  // Orange
//     cv::Scalar(128,   0, 128),  // Purple
//     cv::Scalar(128, 128,   0),  // Olive
//     cv::Scalar(128,   0, 255),  // Violet
//     cv::Scalar(  0, 128, 255),  // Sky Blue
//     cv::Scalar(255,   0, 128),  // Pink
//     cv::Scalar(  0, 128,   0),  // Dark Green
//     cv::Scalar(128, 128, 128),  // Gray
//     cv::Scalar(255, 255, 255)   // White
// };

// std::string get_coco_name_from_int(int cls)
// {
//     std::string result = "N/A";
//     switch(cls) {
//         case 1:  result = "person"; break;
//         case 2:  result = "drone";  break; 
//     }
//     return result;
// }


// CommandLineArgs parse_command_line_arguments(int argc, char** argv) {
//     return {
//         getCmdOption(argc, argv, "-hef="),
//         getCmdOption(argc, argv, "-input="),
//         has_flag(argc, argv, "-s")
//     };
// }


// void print_inference_statistics(std::chrono::duration<double> inference_time,
//                                 const std::string &hef_file,
//                                 double frame_count,
//                                 std::chrono::duration<double> total_time)
// {
//     std::cout << BOLDGREEN << "\n-I-----------------------------------------------" << std::endl;
//     std::cout << "-I- Inference & Postprocess                        " << std::endl;
//     std::cout << "-I-----------------------------------------------" << std::endl;
//     std::cout << "-I- Average FPS:  " << frame_count / (inference_time.count()) << std::endl;
//     std::cout << "-I- Total time:   " << inference_time.count() << " sec" << std::endl;
//     //std::cout << "-I- Latency:      "<< 1.0 / (frame_count / (inference_time.count()) / 1000) << " ms" << std::endl;
//     std::cout << "-I-----------------------------------------------" << std::endl;

//     std::cout << BOLDBLUE << "\n-I- Application finished successfully" << RESET << std::endl;
//     std::cout << BOLDBLUE << "-I- Total application run time: " << (double)total_time.count() << " sec" << RESET << std::endl;
    
// }

// std::string getCmdOption(int argc, char *argv[], const std::string &option)
// {
//     std::string cmd;
//     for (int i = 1; i < argc; ++i) {
//         std::string arg = argv[i];
//         if (0 == arg.find(option, 0)) {
//             std::size_t found = arg.find("=", 0) + 1;
//             cmd = arg.substr(found);
//             return cmd;
//         }
//     }
//     return cmd;
// }


// bool is_image_file(const std::string& path) {
//     static const std::vector<std::string> image_extensions = {".jpg", ".jpeg", ".png", ".bmp", ".tiff", ".webp"};
//     std::string extension = fs::path(path).extension().string();
//     std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
//     return std::find(image_extensions.begin(), image_extensions.end(), extension) != image_extensions.end();
// }

// bool is_video_file(const std::string& path) {
//     static const std::vector<std::string> video_extensions = {".mp4", ".avi", ".mov", ".mkv", ".wmv", ".flv", ".webm"};
//     std::string extension = fs::path(path).extension().string();
//     std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
//     return std::find(video_extensions.begin(), video_extensions.end(), extension) != video_extensions.end();
// }

// bool is_directory_of_images(const std::string& path, int &entry_count) {
//     entry_count = 0;
//     if (fs::exists(path) && fs::is_directory(path)) {
//         bool has_images = false;
//         for (const auto& entry : fs::directory_iterator(path)) {
//             if (fs::is_regular_file(entry)) {
//                 entry_count++;
//                 if (!is_image_file(entry.path().string())) {
//                     // Found a non-image file
//                     return false;
//                 }
//                 has_images = true; 
//             }
//         }
//         return has_images; 
//     }
//     return false;
// }

// bool is_image(const std::string& path) {
//     return fs::exists(path) && fs::is_regular_file(path) && is_image_file(path);
// }

// bool is_video(const std::string& path) {
//     return fs::exists(path) && fs::is_regular_file(path) && is_video_file(path);
// }

// InputType determine_input_type(const std::string& input_path, cv::VideoCapture &capture,
//                                double &org_height, double &org_width, size_t &frame_count) {

//     InputType input_type;
//     int directory_entry_count;
//     if (is_directory_of_images(input_path, directory_entry_count)) {
//         input_type.is_directory = true;
//         input_type.directory_entry_count = directory_entry_count;
//     } else if (is_image(input_path)) {
//         input_type.is_image = true;
//     } else if (is_video(input_path)) {
//         input_type.is_video = true;
//         capture = open_video_capture(input_path, std::ref(capture), org_height, org_width, frame_count);
//     } else {
//         std::cout << "Input is not an image or video, trying to open as camera" << std::endl;
//         input_type.is_camera = true;
//         capture = open_video_capture(input_path, std::ref(capture), org_height, org_width, frame_count);
//     }
//     return input_type;
// }


// void print_net_banner(const std::string &detection_model_name,
//                       const std::vector<hailort::InferModel::InferStream> &inputs,
//                       const std::vector<hailort::InferModel::InferStream> &outputs)
// {
//     std::cout << BOLDMAGENTA << "-I-----------------------------------------------" << std::endl << RESET;
//     std::cout << BOLDMAGENTA << "-I-  Network Name                               " << std::endl << RESET;
//     std::cout << BOLDMAGENTA << "-I-----------------------------------------------" << std::endl << RESET;
//     std::cout << BOLDMAGENTA << "-I   " << detection_model_name << std::endl << RESET;
//     std::cout << BOLDMAGENTA << "-I-----------------------------------------------" << std::endl << RESET;
//     for (auto &input : inputs) {
//         auto shape = input.shape();
//         std::cout << MAGENTA << "-I-  Input: " << input.name()
//                   << ", Shape: (" << shape.height << ", " << shape.width << ", " << shape.features << ")"
//                   << std::endl << RESET;
//     }
//     std::cout << BOLDMAGENTA << "-I-----------------------------------------------" << std::endl << RESET;
//     for (auto &output : outputs) {
//         auto shape = output.shape();
//         std::cout << MAGENTA << "-I-  Output: " << output.name()
//                   << ", Shape: (" << shape.height << ", " << shape.width << ", " << shape.features << ")"
//                   << std::endl << RESET;
//     }
//     std::cout << BOLDMAGENTA << "-I-----------------------------------------------\n" << std::endl << RESET;
// }

// void init_video_writer(const std::string &output_path, cv::VideoWriter &video, double fps, int org_width, int org_height) {

//     int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
    
//     video.open(output_path, fourcc, fps, cv::Size(org_width, org_height));
    
//     if (!video.isOpened()) {
//         std::cerr << "************************************************************" << std::endl;
//         std::cerr << "ERROR: Could not open VideoWriter for codec 'avc1'." << std::endl;
//         std::cerr << "Your system may be missing the required H.264 encoder." << std::endl;
//         std::cerr << "Trying fallback codec 'H264'..." << std::endl;

//         // --- Fallback Option ---
//         fourcc = cv::VideoWriter::fourcc('H', '2', '6', '4');
//         video.open(output_path, fourcc, fps, cv::Size(org_width, org_height));

//         if (!video.isOpened()) {
//             std::cerr << "ERROR: Fallback codec 'H264' also failed." << std::endl;
//             std::cerr << "Please ensure you have a valid H.264 encoder installed (e.g., via ffmpeg or gstreamer-plugins)." << std::endl;
//             std::cerr << "************************************************************" << std::endl;
//             throw std::runtime_error("Error when writing video: Could not open VideoWriter with any H.264 codec.");
//         }
//     }
// }

// PreprocessedFrameItem create_preprocessed_frame_item(const cv::Mat &frame,
//                                                             uint32_t width,
//                                                             uint32_t height)
// {
//     PreprocessedFrameItem item;
//     item.org_frame = frame.clone(); 
//     cv::resize(frame, item.resized_for_infer, cv::Size(width, height));
//     return item;
// }

// void initialize_class_colors(std::unordered_map<int, cv::Scalar>& class_colors) {
//     for (int cls = 0; cls <= 80; ++cls) {
//         class_colors[cls] = COLORS[cls % COLORS.size()]; 
//     }
// }

// std::string get_hef_name(const std::string &path)
// {
//     size_t pos = path.find_last_of("/\\");
//     if (pos == std::string::npos) {
//         return path;
//     }
//     return path.substr(pos + 1);
// }

// bool has_flag(int argc, char *argv[], const std::string &flag) {
//     for (int i = 1; i < argc; ++i) {
//         if (argv[i] == flag) {
//             return true;
//         }
//     }
//     return false;
// }

// hailo_status check_status(const hailo_status &status, const std::string &message) {
//     if (HAILO_SUCCESS != status) {
//         std::cerr << message << " with status " << status << std::endl;
//         return status;
//     }
//     return HAILO_SUCCESS;
// }

// void show_progress_helper(size_t current, size_t total)
// {
//     int progress = static_cast<int>((static_cast<float>(current + 1) / static_cast<float>(total)) * 100);
//     int bar_width = 50; 
//     int pos = static_cast<int>(bar_width * (current + 1) / total);

//     std::cout << "\rProgress: [";
//     for (int j = 0; j < bar_width; ++j) {
//         if (j < pos) std::cout << "=";
//         else if (j == pos) std::cout << ">";
//         else std::cout << " ";
//     }
//     std::cout << "] " << std::setw(3) << progress
//               << "% (" << std::setw(3) << (current + 1) << "/" << total << ")" << std::flush;
// }

// void show_progress(InputType &input_type, int progress, size_t frame_count) {
//     if (input_type.is_video) {
//         show_progress_helper(progress, frame_count);
//     } else if (input_type.is_directory) {
//         show_progress_helper(progress, input_type.directory_entry_count);
//     }
// }

// cv::Rect get_bbox_coordinates(const hailo_bbox_float32_t& bbox, int frame_width, int frame_height) {
//     int x1 = static_cast<int>(bbox.x_min * frame_width);
//     int y1 = static_cast<int>(bbox.y_min * frame_height);
//     int x2 = static_cast<int>(bbox.x_max * frame_width);
//     int y2 = static_cast<int>(bbox.y_max * frame_height);
//     return cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2));
// }

// void draw_label(cv::Mat& frame, const std::string& label, const cv::Point& top_left, const cv::Scalar& color) {
//     int baseLine = 0;
//     cv::Size label_size = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
//     int top = std::max(top_left.y, label_size.height);
//     cv::rectangle(frame, cv::Point(top_left.x, top + baseLine), 
//                   cv::Point(top_left.x + label_size.width, top - label_size.height), color, cv::FILLED);
//     cv::putText(frame, label, cv::Point(top_left.x, top), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
// }

// void draw_single_bbox(cv::Mat& frame, const NamedBbox& named_bbox, const cv::Scalar& color) {
//     auto bbox_rect = get_bbox_coordinates(named_bbox.bbox, frame.cols, frame.rows);
//     cv::rectangle(frame, bbox_rect, color, 2);

//     std::string score_str = std::to_string(named_bbox.bbox.score * 100).substr(0, 4) + "%";
//     std::string label = get_coco_name_from_int(static_cast<int>(named_bbox.class_id)) + " " + score_str;
//     draw_label(frame, label, bbox_rect.tl(), color);
// }

// void draw_bounding_boxes(cv::Mat& frame, const std::vector<NamedBbox>& bboxes) {
//     std::unordered_map<int, cv::Scalar> class_colors;
//     initialize_class_colors(class_colors);
//     for (const auto& named_bbox : bboxes) {
//         const auto& color = class_colors[named_bbox.class_id];
//         draw_single_bbox(frame, named_bbox, color);
//     }
// }

// std::vector<NamedBbox> parse_nms_data(uint8_t* data, size_t max_class_count) {
//     std::vector<NamedBbox> bboxes;
//     size_t offset = 0;

//     // THIS LOOP IS THE KEY
//     for (size_t class_id = 0; class_id < max_class_count; class_id++) {
//         // Reads the number of detections for this specific class
//         auto det_count = static_cast<uint32_t>(*reinterpret_cast<float32_t*>(data + offset));
//         offset += sizeof(float32_t);

//         // Reads the data for each of those detections
//         for (size_t j = 0; j < det_count; j++) {
//             hailo_bbox_float32_t bbox_data = *reinterpret_cast<hailo_bbox_float32_t*>(data + offset);
//             offset += sizeof(hailo_bbox_float32_t);

//             NamedBbox named_bbox;
//             named_bbox.bbox = bbox_data;
//             named_bbox.class_id = class_id + 1; // Assigns the class ID
//             bboxes.push_back(named_bbox);
//         }
//     }
//     return bboxes;
// }

// cv::VideoCapture open_video_capture(const std::string &input_path, cv::VideoCapture capture,
//                                     double &org_height, double &org_width, size_t &frame_count) {
//     capture.open(input_path, cv::CAP_ANY); 
//     if (!capture.isOpened()) {
//         throw std::runtime_error("Unable to read input file");
//     }
//     org_height = capture.get(cv::CAP_PROP_FRAME_HEIGHT);
//     org_width = capture.get(cv::CAP_PROP_FRAME_WIDTH);
//     frame_count = capture.get(cv::CAP_PROP_FRAME_COUNT);
//     return capture;
// }
// bool show_frame(const InputType &input_type, const cv::Mat &frame_to_draw)
// {
//     if (input_type.is_camera) {
//         cv::imshow("Inference", frame_to_draw);
//         if (cv::waitKey(1) == 'q') {
//             std::cout << "Exiting" << std::endl;
//             return false;
//         }
//     }
//     return true;
// }

// hailo_status wait_and_check_threads(
//     std::future<hailo_status> &f1, const std::string &name1,
//     std::future<hailo_status> &f2, const std::string &name2,
//     std::future<hailo_status> &f3, const std::string &name3)
// {
//     hailo_status status = f1.get();
//     if (HAILO_SUCCESS != status) {
//         std::cerr << name1 << " failed with status " << status << std::endl;
//         return status;
//     }

//     status = f2.get();
//     if (HAILO_SUCCESS != status) {
//         std::cerr << name2 << " failed with status " << status << std::endl;
//         return status;
//     }

//     status = f3.get();
//     if (HAILO_SUCCESS != status) {
//         std::cerr << name3 << " failed with status " << status << std::endl;
//         return status;
//     }

//     return HAILO_SUCCESS;
// }









// File: utils.cpp
#include "utils.hpp"

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>

#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc.hpp> // For cvtColor if needed, but prefer GStreamer
#include <opencv2/dnn.hpp>     // For NMSBoxes

#include <boost/format.hpp>

#include <atomic>
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <future>
#include <queue>
#include <stdexcept>
#include <filesystem>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <mutex>
#include <unordered_map>

// --- GStreamer Globals ---
// Using a global bus is generally okay for a single pipeline managed by one main app.
static GstBus* g_bus = NULL;

// --- COLORS Vector ---
std::vector<cv::Scalar> COLORS = {
    cv::Scalar(255,   0,   0), cv::Scalar(  0, 255,   0), cv::Scalar(  0,   0, 255),
    cv::Scalar(255, 255,   0), cv::Scalar(255,   0, 255), cv::Scalar(  0, 255, 255),
    cv::Scalar(255, 128,   0), cv::Scalar(128,   0, 128), cv::Scalar(128, 128,   0),
    cv::Scalar(128,   0, 255), cv::Scalar(  0, 128, 255), cv::Scalar(255,   0, 128),
    cv::Scalar(  0, 128,   0), cv::Scalar(128, 128, 128), cv::Scalar(255, 255, 255)
};

// --- COCO Class Mapping ---
std::string get_coco_name_from_int(int cls)
{
    std::string result = "N/A";
    if (cls == 1) result = "person";
    if (cls == 2) result = "drone";
    return result;
}

// --- Command Line Parsing ---
std::string getCmdOption(int argc, char *argv[], const std::string &option)
{
    std::string cmd;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find(option) == 0) { // Use find() for substring matching
            std::size_t found = arg.find("=", 0);
            if (found != std::string::npos && found + 1 < arg.length()) {
                cmd = arg.substr(found + 1);
                return cmd;
            } else {
                return ""; // Option found but no value provided
            }
        }
    }
    return cmd; // Option not found
}

bool has_flag(int argc, char *argv[], const std::string &flag) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == flag) { // Direct string comparison is fine for flags
            return true;
        }
    }
    return false;
}

CommandLineArgs parse_command_line_arguments(int argc, char **argv) {
    return {
        getCmdOption(argc, argv, "-hef="),
        getCmdOption(argc, argv, "-input="),
        has_flag(argc, argv, "-s")
    };
}

// --- Input Type Detection ---
bool is_image_file(const std::string& path) {
    static const std::vector<std::string> image_extensions = {".jpg", ".jpeg", ".png", ".bmp", ".tiff", ".webp"};
    std::string extension = fs::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    return std::find(image_extensions.begin(), image_extensions.end(), extension) != image_extensions.end();
}

bool is_video_file(const std::string& path) {
    static const std::vector<std::string> video_extensions = {".mp4", ".avi", ".mov", ".mkv", ".wmv", ".flv", ".webm"};
    std::string extension = fs::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    return std::find(video_extensions.begin(), video_extensions.end(), extension) != video_extensions.end();
}

bool is_directory_of_images(const std::string& path, int &entry_count) {
    entry_count = 0;
    if (fs::exists(path) && fs::is_directory(path)) {
        bool has_images = false;
        for (const auto& entry : fs::directory_iterator(path)) {
            if (fs::is_regular_file(entry)) {
                entry_count++;
                if (!is_image_file(entry.path().string())) {
                    // Found a non-image file, consider it not a pure image directory
                    return false;
                }
                has_images = true;
            }
        }
        // Return true only if it's a directory and contains at least one image file
        return has_images;
    }
    return false;
}

bool is_image(const std::string& path) {
    return fs::exists(path) && fs::is_regular_file(path) && is_image_file(path);
}

bool is_video(const std::string& path) {
    return fs::exists(path) && fs::is_regular_file(path) && is_video_file(path);
}

// --- GStreamer Initialization and Management ---
// Corrected: Removed 'static' as it's a globally accessible callback for GStreamer.
GstFlowReturn on_new_buffer(GstAppSink *appsink, gpointer user_data) {
    AppSinkData* data = (AppSinkData*)user_data;

    GstSample *sample = gst_app_sink_pull_sample(appsink);
    if (!sample) {
        g_warning("Failed to pull sample from appsink.");
        return GST_FLOW_ERROR;
    }

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstCaps *caps = gst_sample_get_caps(sample);
    if (!caps) {
        g_warning("Failed to get caps for sample.");
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    GstStructure *structure = gst_caps_get_structure(caps, 0);
    const gchar *format_str = gst_structure_get_string(structure, "format");
    gint width, height;
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);

    // Determine GStreamer format code
    int gst_format_code = 0;
    if (format_str) {
        if (strcmp(format_str, "RGB") == 0) gst_format_code = GST_VIDEO_FORMAT_RGB;
        else if (strcmp(format_str, "BGR") == 0) gst_format_code = GST_VIDEO_FORMAT_BGR;
        else if (strcmp(format_str, "BGRA") == 0) gst_format_code = GST_VIDEO_FORMAT_BGRA;
        // Add more format mappings if your pipeline uses them (e.g., YUY2, NV12)
        else g_warning("Unsupported GStreamer format: %s. Treating as generic.", format_str);
    } else {
        g_warning("GStreamer format string not found.");
    }

    GstMapFlags map_flags = GST_MAP_READ;
    GstMapInfo map_info;
    if (!gst_buffer_map(buffer, &map_info, map_flags)) {
        g_warning("Failed to map buffer.");
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    // Safely copy buffer data to our AppSinkData
    {
        std::lock_guard<std::mutex> lock(data->buffer_mutex);
        data->current_frame_buffer.assign(
            reinterpret_cast<const char*>(map_info.data),
            reinterpret_cast<const char*>(map_info.data) + map_info.size);
        data->frame_width = width;
        data->frame_height = height;
        data->frame_format = gst_format_code;
        data->frame_ready.store(true); // Signal that a frame is ready
        data->frame_id++;              // Increment frame ID for ordering
    }

    gst_buffer_unmap(buffer, &map_info);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

void print_inference_statistics(std::chrono::duration<double> inference_time,
                                const std::string &hef_file,
                                double frame_count,
                                std::chrono::duration<double> total_time)
{
    std::cout << BOLDGREEN << "\n-I-----------------------------------------------" << std::endl;
    std::cout << "-I- Inference & Postprocess                        " << std::endl;
    std::cout << "-I-----------------------------------------------" << std::endl;
    std::cout << "-I- Average FPS:  " << frame_count / (inference_time.count()) << std::endl;
    std::cout << "-I- Total time:   " << inference_time.count() << " sec" << std::endl;
    std::cout << "-I-----------------------------------------------" << std::endl;
    std::cout << BOLDBLUE << "\n-I- Application finished successfully" << RESET << std::endl;
    std::cout << BOLDBLUE << "-I- Total application run time: " << (double)total_time.count() << " sec" << RESET << std::endl;
}

bool init_gstreamer_pipeline(AppSinkData* appsink_data, const std::string& pipeline_str, std::function<void(const std::vector<char>&, int, int, int)> callback) {
    gst_init(NULL, NULL); // Initialize GStreamer

    GError *error = NULL;
    appsink_data->pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (!appsink_data->pipeline) {
        g_printerr("Failed to create GStreamer pipeline: %s\n", error->message);
        g_error_free(error);
        return false;
    }

    // Find the appsink element to attach our callback
    appsink_data->app_sink = gst_bin_get_by_name(GST_BIN(appsink_data->pipeline), "appsink");
    if (!appsink_data->app_sink) {
        g_printerr("Failed to find appsink element in the pipeline. Ensure your pipeline string has an element named 'appsink'.\n");
        gst_object_unref(appsink_data->pipeline);
        return false;
    }

    // Configure appsink callbacks
    GstAppSinkCallbacks callbacks = {NULL}; // Initialize all members to NULL/zero
    callbacks.new_sample = on_new_buffer;
    gst_app_sink_set_callbacks(GST_APP_SINK(appsink_data->app_sink), &callbacks, appsink_data, NULL);

    // Configure appsink properties for buffer retrieval
    // GstAppSinkProperties struct is deprecated. Use individual property setters.
    gst_app_sink_set_drop(GST_APP_SINK(appsink_data->app_sink), TRUE);
    gst_app_sink_set_max_buffers(GST_APP_SINK(appsink_data->app_sink), 2);

    // Store the frame callback
    appsink_data->frame_callback = callback;
    appsink_data->frame_id = 0; // Initialize frame counter

    // Start the pipeline
    GstStateChangeReturn ret = gst_element_set_state(appsink_data->pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to set pipeline to playing state.\n");
        gst_object_unref(appsink_data->app_sink);
        gst_object_unref(appsink_data->pipeline);
        return false;
    }

    // Get the bus and start monitoring it in a separate thread
    g_bus = gst_element_get_bus(appsink_data->pipeline);
    std::thread([=]() { // Use lambda to capture appsink_data and g_bus
        GstMessage *msg = NULL;
        while (!appsink_data->stop_flag.load()) {
            msg = gst_bus_timed_pop(g_bus, GST_CLOCK_TIME_NONE); // Wait indefinitely
            if (!msg) { // If bus is NULL, likely pipeline is gone
                g_warning("GStreamer bus returned NULL, stopping.");
                appsink_data->stop_flag.store(true);
                break;
            }

            if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
                GError *error;
                gchar *debug;
                gst_message_parse_error(msg, &error, &debug);
                g_printerr("Error received from pipeline: %s\n", error->message);
                g_printerr("Debugging info: %s\n", (debug) ? debug : "none");
                g_error_free(error);
                g_free(debug);
                appsink_data->stop_flag.store(true); // Signal pipeline error
            } else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS) {
                // End of stream reached (e.g., video file finished)
                g_print("End of stream reached.\n");
                appsink_data->stop_flag.store(true); // Signal pipeline completion
            } else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_STATE_CHANGED) {
                // Optional: Monitor state changes
                GstState old_state, new_state, pending_state;
                gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                // g_print("Pipeline state changed from %s to %s\n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
            }

            gst_message_unref(msg);
            if (appsink_data->stop_flag.load()) break; // Exit loop if stop requested
        }
        g_print("GStreamer bus monitoring thread exiting.\n");
    }).detach(); // Detach the bus monitor thread

    return true;
}

void stop_gstreamer_pipeline(AppSinkData* appsink_data) {
    if (!appsink_data) return;

    appsink_data->stop_flag.store(true); // Signal the callback and bus threads to stop

    // Properly shut down appsink callbacks and pipeline
    if (appsink_data->app_sink) {
        gst_app_sink_set_callbacks(GST_APP_SINK(appsink_data->app_sink), NULL, NULL, NULL); // Remove callbacks
        gst_element_set_state(appsink_data->app_sink, GST_STATE_NULL); // Set appsink to NULL
        gst_object_unref(appsink_data->app_sink);
        appsink_data->app_sink = NULL;
    }

    if (appsink_data->pipeline) {
        gst_element_set_state(appsink_data->pipeline, GST_STATE_NULL); // Set pipeline to NULL
        gst_object_unref(appsink_data->pipeline);
        appsink_data->pipeline = NULL;
    }

    // Unref the bus
    if (g_bus) {
        gst_object_unref(g_bus);
        g_bus = NULL;
    }
}

// Function to retrieve the latest frame safely
bool get_latest_frame_from_gstreamer(AppSinkData* appsink_data, std::vector<char>& frame_buffer, int& width, int& height, int& format) {
    if (!appsink_data || !appsink_data->frame_ready.load() || appsink_data->stop_flag.load()) {
        return false; // No frame ready or pipeline stopped
    }

    std::lock_guard<std::mutex> lock(appsink_data->buffer_mutex);
    if (!appsink_data->current_frame_buffer.empty()) {
        frame_buffer = appsink_data->current_frame_buffer; // Copy the buffer
        width = appsink_data->frame_width;
        height = appsink_data->frame_height;
        format = appsink_data->frame_format;
        appsink_data->frame_ready.store(false); // Mark frame as consumed
        return true;
    }
    return false;
}


// --- Display Utilities ---
void print_net_banner(const std::string &detection_model_name,
                      const std::vector<hailort::InferModel::InferStream> &inputs,
                      const std::vector<hailort::InferModel::InferStream> &outputs)
{
    std::cout << BOLDMAGENTA << "-I-----------------------------------------------" << std::endl << RESET;
    std::cout << BOLDMAGENTA << "-I-  Network Name                               " << std::endl << RESET;
    std::cout << BOLDMAGENTA << "-I-----------------------------------------------" << std::endl << RESET;
    std::cout << BOLDMAGENTA << "-I   " << detection_model_name << std::endl << RESET;
    std::cout << BOLDMAGENTA << "-I-----------------------------------------------" << std::endl << RESET;
    for (const auto &input : inputs) { // Use const auto& for read-only access
        auto shape = input.shape();
        std::cout << MAGENTA << "-I-  Input: " << input.name()
                  << ", Shape: (" << shape.height << ", " << shape.width << ", " << shape.features << ")"
                  << std::endl << RESET;
    }
    std::cout << BOLDMAGENTA << "-I-----------------------------------------------" << std::endl << RESET;
    for (const auto &output : outputs) { // Use const auto& for read-only access
        auto shape = output.shape();
        std::cout << MAGENTA << "-I-  Output: " << output.name()
                  << ", Shape: (" << shape.height << ", " << shape.width << ", " << shape.features << ")"
                  << std::endl << RESET;
    }
    std::cout << BOLDMAGENTA << "-I-----------------------------------------------\n" << std::endl << RESET;
}

void show_progress_helper(size_t current, size_t total)
{
    if (total == 0) return; // Avoid division by zero

    int progress = static_cast<int>((static_cast<float>(current + 1) / static_cast<float>(total)) * 100);
    int bar_width = 50;
    int pos = static_cast<int>(bar_width * static_cast<float>(current + 1) / total);

    std::cout << "\rProgress: [";
    for (int j = 0; j < bar_width; ++j) {
        if (j < pos) std::cout << "=";
        else if (j == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << std::setw(3) << progress
              << "% (" << std::setw(3) << (current + 1) << "/" << total << ")" << std::flush;
}

void show_progress(InputType &input_type, int progress, size_t frame_count) {
    if (input_type.is_video || input_type.is_camera) { // Camera also means progress is relevant
        if (frame_count > 0) { // Only show progress if total frame count is known
            show_progress_helper(progress, frame_count);
        }
    } else if (input_type.is_directory) {
        show_progress_helper(progress, input_type.directory_entry_count);
    }
}

// --- Video Writer Utility ---
// void init_video_writer(const std::string &output_path, cv::VideoWriter &video, double fps, int org_width, int org_height) {
//     // Try AVC1 codec first (H.264, common on many platforms)
//     int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
//     video.open(output_path, fourcc, fps, cv::Size(org_width, org_height));

//     if (!video.isOpened()) {
//         std::cerr << "************************************************************" << std::endl;
//         std::cerr << "ERROR: Could not open VideoWriter for codec 'avc1'." << std::endl;
//         std::cerr << "Your system may be missing the required H.264 encoder." << std::endl;
//         std::cerr << "Trying fallback codec 'H264'..." << std::endl;

//         // Fallback to another common H.264 codec
//         fourcc = cv::VideoWriter::fourcc('H', '2', '6', '4');
//         video.open(output_path, fourcc, fps, cv::Size(org_width, org_height));

//         if (!video.isOpened()) {
//             std::cerr << "ERROR: Fallback codec 'H264' also failed." << std::endl;
//             std::cerr << "Please ensure you have a valid H.264 encoder installed (e.g., via ffmpeg or gstreamer-plugins)." << std::endl;
//             std::cerr << "************************************************************" << std::endl;
//             throw std::runtime_error("Error when writing video: Could not open VideoWriter with any H.264 codec.");
//         }
//     }
//      std::cout << "-I- Video writer initialized for output: " << output_path << std::endl;
// }

void init_video_writer(const std::string &output_path, cv::VideoWriter &video, double fps, int org_width, int org_height) {

    // --- THE FIX: Use a highly compatible AVI container with a basic MJPEG codec ---
    // This is a very robust combination that relies on software encoding and should
    // work on almost any system with a standard OpenCV installation.
    int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
    
    // Change the output file extension to .avi to match the container format.
    std::string final_output_path = "processed_video.avi";

    video.open(final_output_path, fourcc, fps, cv::Size(org_width, org_height));
    
    if (!video.isOpened()) {
        std::cerr << "************************************************************" << std::endl;
        std::cerr << "FATAL ERROR: Could not open VideoWriter for codec 'MJPG' in AVI container." << std::endl;
        std::cerr << "This indicates a fundamental issue with the OpenCV video I/O backend (e.g., FFMPEG)." << std::endl;
        std::cerr << "Please ensure OpenCV was built with FFMPEG support and that basic codecs are available." << std::endl;
        std::cerr << "************************************************************" << std::endl;
        throw std::runtime_error("Error when writing video: Could not initialize VideoWriter.");
    } else {
        std::cout << "-I- Video writer initialized for output: " << final_output_path << " with codec 'MJPG'." << std::endl;
    }
}

// --- Postprocessing Utilities ---
cv::Rect get_bbox_coordinates(const hailo_bbox_float32_t& bbox, int frame_width, int frame_height) {
    // These coordinates are normalized (0.0 to 1.0) and are relative to the frame size passed to this function.
    // We need to convert them to pixel coordinates.
    int x1 = static_cast<int>(bbox.x_min * frame_width);
    int y1 = static_cast<int>(bbox.y_min * frame_height);
    int x2 = static_cast<int>(bbox.x_max * frame_width);
    int y2 = static_cast<int>(bbox.y_max * frame_height);
    return cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2));
}

void draw_label(cv::Mat& frame, const std::string& label, const cv::Point& top_left, const cv::Scalar& color) {
    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
    // Adjust top to ensure label stays within frame boundaries
    int top = std::max(top_left.y - label_size.height, 0);
    cv::rectangle(frame, cv::Point(top_left.x, top),
                  cv::Point(top_left.x + label_size.width, top + label_size.height + baseLine), color, cv::FILLED);
    // Position text slightly above the background rectangle
    cv::putText(frame, label, cv::Point(top_left.x, top + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
}

void draw_single_bbox(cv::Mat& frame, const NamedBbox& named_bbox, const cv::Scalar& color) {
    // The frame_width/height here are the dimensions of the `frame` object (item.org_frame).
    // The coordinates from NamedBbox.bbox are normalized relative to the model input size.
    // We need to map them back to the original frame's dimensions, considering any GStreamer scaling.
    // This mapping is handled in tracking_pipeline.cpp. So, bbox coordinates are already adjusted.
    auto bbox_rect = get_bbox_coordinates(named_bbox.bbox, frame.cols, frame.rows);

    // Ensure the bbox is within the frame boundaries before drawing
    bbox_rect.x = std::max(0, bbox_rect.x);
    bbox_rect.y = std::max(0, bbox_rect.y);
    bbox_rect.width = std::min(bbox_rect.width, frame.cols - bbox_rect.x);
    bbox_rect.height = std::min(bbox_rect.height, frame.rows - bbox_rect.y);

    cv::rectangle(frame, bbox_rect, color, 2);

    std::string score_str = std::to_string(static_cast<int>(named_bbox.bbox.score * 100.0f));
    // Truncate score to avoid overly long strings if needed, e.g., to "85%"
    if (score_str.length() > 3) score_str = score_str.substr(0, 3);
    score_str += "%";

    std::string label = get_coco_name_from_int(static_cast<int>(named_bbox.class_id)) + " " + score_str;
    draw_label(frame, label, bbox_rect.tl(), color);
}

void draw_bounding_boxes(cv::Mat& frame, const std::vector<NamedBbox>& bboxes) {
    std::unordered_map<int, cv::Scalar> class_colors;
    initialize_class_colors(class_colors);
    for (const auto& named_bbox : bboxes) {
        // The class ID from the model might need adjustment if it's not 0-indexed or doesn't match get_coco_name_from_int's expectations.
        // Assuming here that NamedBbox.class_id is directly usable for color lookup.
        const auto& color = class_colors[named_bbox.class_id];
        draw_single_bbox(frame, named_bbox, color);
    }
}

// Parses the raw output buffer from the Hailo model for NMS-processed bounding boxes.
// Assumes the format described in the RKNN examples (count followed by bbox data).
// This parser MUST match the exact output structure of your specific .hef model.
// std::vector<NamedBbox> parse_nms_data(uint8_t* data, size_t max_class_count) {
//     std::vector<NamedBbox> bboxes;
//     size_t offset = 0;

//     // The structure of the RKNN output buffer is model-dependent.
//     // The examples usually show a pattern like:
//     // [num_detections_class_0, bbox_data_class_0, ..., num_detections_class_1, bbox_data_class_1, ...]
//     // where bbox_data is typically [x_min, y_min, x_max, y_max, score].
//     // It's CRUCIAL that this parsing logic accurately reflects your model's output format.

//     for (size_t class_id = 0; class_id < max_class_count; ++class_id) {
//         // Read the number of detections for this class.
//         // RKNN models often output a float32_t count at the beginning of each class's output block.
//         if (data == nullptr) { // Safety check
//             g_warning("parse_nms_data received null data pointer.");
//             return bboxes;
//         }

//         // Ensure offset is within bounds before reading count.
//         // Use a reasonably large but not infinite limit for safety.
//         const size_t MAX_OFFSET = 10 * 1024 * 1024; // 10MB, adjust if your model output is larger
//         if (offset >= MAX_OFFSET) {
//              g_warning("Offset exceeded expected bounds (%zu) while parsing NMS data.", MAX_OFFSET);
//              break;
//         }

//         // Safely read the count (number of detections for this class)
//         // Assuming count is float32_t. If it's uint32_t or something else, adjust this.
//         float det_count_f32 = *reinterpret_cast<float32_t*>(data + offset);
//         auto det_count = static_cast<uint32_t>(det_count_f32);
//         offset += sizeof(float32_t); // Advance offset by the size of the count

//         // Process each detection for this class
//         for (uint32_t j = 0; j < det_count; ++j) {
//             hailo_bbox_float32_t bbox_data;
//             // The bbox data structure is typically [x_min, y_min, x_max, y_max, score], all float32_t.
//             // Ensure offset is within bounds before reading bbox data.
//             if (offset + sizeof(float32_t) * 5 > MAX_OFFSET) {
//                 g_warning("Offset exceeded expected bounds (%zu) while parsing bbox data for class %zu.", MAX_OFFSET, class_id);
//                 break; // Stop processing for this class
//             }
//             float *bbox_f32 = reinterpret_cast<float*>(data + offset);
//             bbox_data.x_min = bbox_f32[0];
//             bbox_data.y_min = bbox_f32[1];
//             bbox_data.x_max = bbox_f32[2];
//             bbox_data.y_max = bbox_f32[3];
//             float score = bbox_f32[4]; // The score is the last element

//             offset += sizeof(float32_t) * 5; // Advance offset by the size of 5 floats

//             NamedBbox named_bbox;
//             named_bbox.bbox = bbox_data;
//             // The class_id from the model might be 0-indexed or 1-indexed.
//             // Ensure this matches how get_coco_name_from_int expects it.
//             // For now, assume it's 0-indexed and matches the loop variable.
//             named_bbox.class_id = class_id;
//             named_bbox.bbox.score = score; // Store the score

//             bboxes.push_back(named_bbox);
//         }
//     }
//     return bboxes;
// }


std::vector<NamedBbox> parse_nms_data(uint8_t* data, size_t max_class_count) {
    std::vector<NamedBbox> bboxes;
    size_t offset = 0;

    // THIS LOOP IS THE KEY
    for (size_t class_id = 0; class_id < max_class_count; class_id++) {
        // Reads the number of detections for this specific class
        auto det_count = static_cast<uint32_t>(*reinterpret_cast<float32_t*>(data + offset));
        offset += sizeof(float32_t);

        // Reads the data for each of those detections
        for (size_t j = 0; j < det_count; j++) {
            hailo_bbox_float32_t bbox_data = *reinterpret_cast<hailo_bbox_float32_t*>(data + offset);
            offset += sizeof(hailo_bbox_float32_t);

            NamedBbox named_bbox;
            named_bbox.bbox = bbox_data;
            named_bbox.class_id = class_id + 1; // Assigns the class ID
            bboxes.push_back(named_bbox);
        }
    }
    return bboxes;
}

// --- Deprecated Video Capture (for reference) ---
// This function is no longer used if use_live_stream is true.
cv::VideoCapture open_video_capture(const std::string &input_path, cv::VideoCapture capture,
                                    double &org_height, double &org_width, size_t &frame_count) {
    capture.open(input_path, cv::CAP_ANY);
    if (!capture.isOpened()) {
        throw std::runtime_error("Unable to read input file: " + input_path);
    }
    org_height = capture.get(cv::CAP_PROP_FRAME_HEIGHT);
    org_width = capture.get(cv::CAP_PROP_FRAME_WIDTH);
    frame_count = capture.get(cv::CAP_PROP_FRAME_COUNT);
    std::cout << "Opened video file: " << input_path << " (W:" << org_width << " H:" << org_height << " Frames:" << frame_count << ")" << std::endl;
    return capture;
}

// --- Frame Display Utility (for standalone testing, not used by pipeline directly) ---
bool show_frame(const InputType &input_type, const cv::Mat &frame_to_draw)
{
    if (input_type.is_camera) { // This check is a bit redundant if pipeline handles capture
        cv::imshow("Inference", frame_to_draw);
        if (cv::waitKey(1) == 'q') {
            std::cout << "Exiting" << std::endl;
            return false;
        }
    }
    return true;
}

// --- Thread Synchronization ---
hailo_status wait_and_check_threads(
    std::future<hailo_status> &f1, const std::string &name1,
    std::future<hailo_status> &f2, const std::string &name2,
    std::future<hailo_status> &f3, const std::string &name3)
{
    hailo_status status;

    status = f1.get();
    if (HAILO_SUCCESS != status) {
        std::cerr << name1 << " failed with status " << status << std::endl;
        return status;
    }

    status = f2.get();
    if (HAILO_SUCCESS != status) {
        std::cerr << name2 << " failed with status " << status << std::endl;
        return status;
    }

    status = f3.get();
    if (HAILO_SUCCESS != status) {
        std::cerr << name3 << " failed with status " << status << std::endl;
        return status;
    }

    return HAILO_SUCCESS;
}

// --- Class Color Initialization ---
void initialize_class_colors(std::unordered_map<int, cv::Scalar>& class_colors) {
    // FIX: This function now works because OBJ_CLASS_NUM is defined in the header.
    for (int cls = 0; cls < OBJ_CLASS_NUM; ++cls) {
        class_colors[cls] = COLORS[cls % COLORS.size()];
    }
}