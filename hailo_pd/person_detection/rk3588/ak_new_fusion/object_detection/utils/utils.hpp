#ifndef _EXAMPLE_COMMON_H_
#define _EXAMPLE_COMMON_H_

#include <iostream>
#include <string>
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

#include <opencv2/highgui.hpp>
#include <opencv2/core/matx.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp> 

#include <boost/format.hpp>

#include "hailo/infer_model.hpp" 
#include "hailo/hailort.h"
#include "ByteTrack/Object.h"


// --- ADD NLOHMANN JSON HEADER ---
#include <nlohmann/json.hpp>

// --- ADD LUT/CALIB DATA HEADERS ---
#include "lut.hpp"
#include "calib_data_io.hpp"


// -------------------------- COLOR MACROS --------------------------
#define RESET         "\033[0m"
#define MAGENTA       "\033[35m"
#define BOLDGREEN     "\033[1m\033[32m"
#define BOLDBLUE      "\033[1m\033[34m"
#define BOLDMAGENTA   "\033[1m\033[35m"

extern std::vector<cv::Scalar> COLORS;
namespace fs = std::filesystem;

// --------------------------- FUNCTION DECLARATIONS ---------------------------


// --- NEW: DECLARATIONS FOR JSON PARSING FUNCTIONS ---
void loadThermalIntrinsics(const std::string& path, cv::Mat& K, cv::Mat& dist);
void loadZoomLUT(const std::string& path, LUTGrid& grid, LUTParam& lut);
void loadExtrinsics(const std::string& path, cv::Mat& R, cv::Mat& t);

struct SingleStreamFrameData {
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
};

struct FusedFrameData {
    int frame_id;
    cv::Mat display_frame;
    std::vector<byte_track::Object> fused_detections;

    // --- THIS IS THE FIX ---
    // All timestamps are now correctly included in this struct.
    std::chrono::high_resolution_clock::time_point t_eo_capture_start;
    std::chrono::high_resolution_clock::time_point t_eo_preprocess_end;
    std::chrono::high_resolution_clock::time_point t_eo_inference_end;
    
    std::chrono::high_resolution_clock::time_point t_ir_capture_start;
    std::chrono::high_resolution_clock::time_point t_ir_preprocess_end;
    std::chrono::high_resolution_clock::time_point t_ir_inference_end;

    std::chrono::high_resolution_clock::time_point t_fusion_end;
};

struct CommandLineArgs {
    std::string detection_hef;
    std::string input_path;
    bool save;
};

struct PreprocessedFrameItem {
    cv::Mat org_frame;    
    cv::Mat resized_for_infer; 
    cv::Mat affine_matrix;
};

struct InferenceOutputItem {
    cv::Mat org_frame;
    std::vector<std::pair<uint8_t*, hailo_vstream_info_t>> output_data_and_infos;
    std::vector<std::shared_ptr<uint8_t>> output_guards;
    cv::Mat affine_matrix; // <-- ADD THIS LINE BACK
};

struct NamedBbox {
    hailo_bbox_float32_t bbox;
    size_t class_id;
};

struct InputType {
    bool is_image = false;
    bool is_video = false;
    bool is_directory = false;
    int directory_entry_count = 0;
    bool is_camera = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// COMMAND-LINE PARSING / FLAGS
// ─────────────────────────────────────────────────────────────────────────────

std::string getCmdOption(int argc, char *argv[], const std::string &option);
bool has_flag(int argc, char *argv[], const std::string &flag);
CommandLineArgs parse_command_line_arguments(int argc, char **argv);

// ─────────────────────────────────────────────────────────────────────────────
// INPUT DETECTION
// ─────────────────────────────────────────────────────────────────────────────

bool is_image_file(const std::string &path);
bool is_video_file(const std::string &path);
bool is_directory_of_images(const std::string &path, int &entry_count);
bool is_image(const std::string &path);
bool is_video(const std::string &path);
InputType determine_input_type(const std::string &input_path, cv::VideoCapture &capture,
                               double &org_height, double &org_width, size_t &frame_count);

// ─────────────────────────────────────────────────────────────────────────────
// DISPLAY
// ─────────────────────────────────────────────────────────────────────────────

void print_net_banner(const std::string &detection_model_name,
                      const std::vector<hailort::InferModel::InferStream> &inputs,
                      const std::vector<hailort::InferModel::InferStream> &outputs);
void show_progress_helper(size_t current, size_t total);
void show_progress(InputType &input_type, int progress, size_t frame_count);
void print_inference_statistics(std::chrono::duration<double> inference_time,
                                const std::string &hef_file,
                                double frame_count,
                                std::chrono::duration<double> total_time);

// ─────────────────────────────────────────────────────────────────────────────
// VIDEO
// ─────────────────────────────────────────────────────────────────────────────

void init_video_writer(const std::string &output_path, cv::VideoWriter &video,
                       double fps, int org_width, int org_height);
cv::VideoCapture open_video_capture(const std::string &input_path, cv::VideoCapture capture,
                                    double &org_height, double &org_width, size_t &rame_count);
bool show_frame(const InputType &input_type, const cv::Mat &frame_to_draw);

// ─────────────────────────────────────────────────────────────────────────────
// POSTPROCESSING
// ─────────────────────────────────────────────────────────────────────────────

cv::Rect get_bbox_coordinates(const hailo_bbox_float32_t &bbox, int frame_width, int frame_height);
void draw_label(cv::Mat &frame, const std::string &label, const cv::Point &top_left, const cv::Scalar &color);
void draw_single_bbox(cv::Mat &frame, const NamedBbox &named_bbox, const cv::Scalar &color);
void draw_bounding_boxes(cv::Mat &frame, const std::vector<NamedBbox> &bboxes);
std::vector<NamedBbox> parse_nms_data(uint8_t *data, size_t max_class_count);

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS
// ─────────────────────────────────────────────────────────────────────────────

hailo_status check_status(const hailo_status &status, const std::string &message);
std::string get_hef_name(const std::string &path);
hailo_status wait_and_check_threads(std::future<hailo_status> &f1, const std::string &name1,
                                    std::future<hailo_status> &f2, const std::string &name2,
                                    std::future<hailo_status> &f3, const std::string &name3);
PreprocessedFrameItem create_preprocessed_frame_item(const cv::Mat &frame, uint32_t width, uint32_t height);
void initialize_class_colors(std::unordered_map<int, cv::Scalar> &class_colors);
std::string get_coco_name_from_int(int cls);

#endif