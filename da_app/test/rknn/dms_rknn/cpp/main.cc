// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <vector>        // For std::vector
// #include <string>        // For std::string
// #include <sstream>       // For std::stringstream
// #include <iomanip>       // For std::setprecision
// #include <chrono>        // For calibration timer
// #include <stdexcept>     // For std::stod exception handling
// #include <cmath>         // For std::stod dependency if not implicitly included

// // Include headers for both pipelines
// #include "face_analyzer/face_analyzer.h"
// #include "yolo_detector/yolo11.h"

// // Include Behavior Analysis Headers
// #include "behavior_analysis/BlinkDetector.hpp"
// #include "behavior_analysis/YawnDetector.hpp"
// #include "behavior_analysis/HeadPoseTracker.hpp"
// #include "behavior_analysis/KSSCalculator.hpp"

// // Common utilities
// #include "image_utils.h"
// #include "file_utils.h"
// #include "image_drawing.h"

// // Define additional colors manually if needed
// // Assuming the drawing functions expect 0xRRGGBB format (common for simple drawing)
// // Check image_drawing.h for the actual format used by COLOR_RED/GREEN/BLUE
// #ifndef COLOR_MAGENTA
// #define COLOR_MAGENTA (0xFF00FF)
// #endif
// #ifndef COLOR_CYAN
// #define COLOR_CYAN (0x00FFFF)
// #endif
// #ifndef COLOR_YELLOW
// #define COLOR_YELLOW (0xFFFF00)
// #endif
// #ifndef COLOR_BLUE
// #define COLOR_BLUE (0x0000FF)
// #endif
// #ifndef COLOR_ORANGE
// #define COLOR_ORANGE (0xFFA500)
// #endif
// #ifndef COLOR_WHITE // Define white if not already present
// #define COLOR_WHITE (0xFFFFFF)
// #endif
// // COLOR_RED and COLOR_GREEN should be defined in image_drawing.h

// // Define a top-level context structure
// typedef struct app_context_t {
//     face_analyzer_app_context_t face_ctx;
//     yolo11_app_context_t        yolo_ctx;
// } app_context_t;

// // Helper to convert point_t array to std::vector<cv::Point>
// // ASSUMPTION: cv::Point is available OR point_t is directly compatible/used
// #include <opencv2/core/types.hpp> // Keep if HeadPoseTracker uses cv::Point explicitly
// std::vector<cv::Point> convert_landmarks_to_cvpoint(const point_t landmarks[], int count) {
//     std::vector<cv::Point> cv_landmarks;
//     cv_landmarks.reserve(count);
//     for (int i = 0; i < count; ++i) {
//         // Implicit conversion might work if point_t has members x, y
//         // Otherwise, explicitly construct:
//         cv_landmarks.emplace_back(cv::Point(landmarks[i].x, landmarks[i].y));
//     }
//     return cv_landmarks;
// }


// /*-------------------------------------------
//                   Main Function
// -------------------------------------------*/
// int main(int argc, char **argv)
// {
//     if (argc != 6)
//     {
//         printf("Usage: %s <face_detect_model> <face_lmk_model> <iris_lmk_model> <yolo_model> <image_path>\n", argv[0]);
//         return -1;
//     }

//     const char *detection_model_path = argv[1];
//     const char *landmark_model_path  = argv[2];
//     const char *iris_model_path      = argv[3];
//     const char *yolo_model_path      = argv[4];
//     const char *image_path           = argv[5];

//     int ret = 0; // Initialize ret
//     app_context_t app_ctx;
//     memset(&app_ctx, 0, sizeof(app_context_t));
//     image_buffer_t src_image;
//     memset(&src_image, 0, sizeof(image_buffer_t));

//     // --- Declare results and analysis objects BEFORE potential goto ---
//     face_analyzer_result_t face_results;
//     memset(&face_results, 0, sizeof(face_results)); // Initialize
//     object_detect_result_list yolo_results;
//     memset(&yolo_results, 0, sizeof(yolo_results)); // Initialize

//     my::BlinkDetector blinkDetector;
//     YawnDetector yawnDetector;
//     my::HeadPoseTracker headPoseTracker;
//     KSSCalculator kssCalculator;

//     // --- Calibration Variables ---
//     bool calibration_done = false;
//     const int CALIBRATION_TIME_SEC = 5;
//     auto calibration_start_time = std::chrono::steady_clock::now();

//     // --- Declare KSS and drawing variables ---
//     int compositeKSS = 1; // Default KSS score
//     std::string kssStatus = "Initializing"; // Default status
//     YawnDetector::YawnMetrics yawnMetrics = {}; // Default init
//     my::HeadPoseTracker::HeadPoseResults headPoseResults = {}; // Default init
//     std::vector<std::string> detectedObjects;
//     std::stringstream text_stream;
//     int text_y = 30; // Starting Y position for text block
//     const int line_height = 18;
//     const double text_scale = 0.6;
//     const int thickness = 1;
//     unsigned int status_color_uint = COLOR_GREEN; // Default to green


//     // --- Initialize Modules ---
//     ret = init_post_process(); // YOLO labels
//     if (ret != 0) { printf("Error initializing YOLO postprocess.\n"); ret = -1; goto out;} // Use ret=-1 for consistency
//     ret = init_face_analyzer(detection_model_path, landmark_model_path, iris_model_path, &app_ctx.face_ctx);
//     if (ret != 0) { printf("init_face_analyzer fail! ret=%d\n", ret); goto out; }
//     ret = init_yolo11(yolo_model_path, &app_ctx.yolo_ctx);
//     if (ret != 0) { printf("init_yolo11 fail! ret=%d model_path=%s\n", ret, yolo_model_path); goto out; }

//     // --- Read Source Image ---
//     ret = read_image(image_path, &src_image);
//     if (ret != 0) { printf("read image fail! ret=%d image_path=%s\n", ret, image_path); goto out; }


//     // --- Initial Face Inference for Calibration ---
//     ret = inference_face_analyzer(&app_ctx.face_ctx, &src_image, &face_results);
//     if (ret != 0) { printf("Initial inference_face_analyzer fail! ret=%d\n", ret); goto out;}


//     // --- Calibration Logic (Simplified for single image) ---
//     printf("Attempting calibration...\n");
//     if (face_results.count > 0 && face_results.faces[0].face_landmarks_valid) {
//          std::vector<cv::Point> faceLandmarksCv = convert_landmarks_to_cvpoint(
//              face_results.faces[0].face_landmarks, NUM_FACE_LANDMARKS
//          );
//          headPoseResults = headPoseTracker.run(faceLandmarksCv); // Run head pose once
//          if (headPoseTracker.isCalibrated()) {
//              calibration_done = true;
//              printf("Calibration Complete!\n");
//          } else {
//              printf("Calibration Failed (Head pose not suitable?). Running without full KSS.\n");
//              // Proceed without full calibration for this demo
//              calibration_done = true; // Allow processing to continue
//              kssStatus = "Low Risk (Cal. Failed)";
//          }
//     } else {
//          printf("Calibration Failed (No face or landmarks found).\n");
//          calibration_done = true; // Allow processing to continue
//          kssStatus = "Low Risk (No Face)";
//     }


//     // --- Main Processing (Run YOLO & Behavior Analysis) ---
//     // Run YOLO inference regardless of calibration state for demo
//     ret = inference_yolo11(&app_ctx.yolo_ctx, &src_image, &yolo_results);
//     if (ret != 0) { printf("inference_yolo11 fail! ret=%d\n", ret); } // Log error but continue


//     // --- Run Behavior Analysis if face is valid ---
//     if (face_results.count > 0 && face_results.faces[0].face_landmarks_valid) {
//         face_object_t *face = &face_results.faces[0]; // Process first face
//         std::vector<cv::Point> faceLandmarksCv = convert_landmarks_to_cvpoint(
//             face->face_landmarks, NUM_FACE_LANDMARKS
//         );

//         // Run detectors using refactored signatures
//         blinkDetector.run(faceLandmarksCv, src_image.width, src_image.height);
//         yawnMetrics = yawnDetector.run(faceLandmarksCv, src_image.width, src_image.height);
//         // Re-run head pose to get latest values if needed (already ran once for calibration)
//         headPoseResults = headPoseTracker.run(faceLandmarksCv);

//         // Extract YOLO results for KSS
//         detectedObjects.clear(); // Clear previous results
//         for (int j = 0; j < yolo_results.count; ++j) {
//             object_detect_result *det_result = &(yolo_results.results[j]);
//             detectedObjects.push_back(coco_cls_to_name(det_result->cls_id));
//         }

//         // --- KSS Calculation ---
//         kssCalculator.setPerclos(blinkDetector.getPerclos());
//         if (headPoseResults.rows.size() >= 3) {
//             try {
//                 // Basic parsing, assumes "Value deg" format - adjust if needed
//                 double yaw = std::stod(headPoseResults.rows[0][1]);
//                 double pitch = std::stod(headPoseResults.rows[1][1]);
//                 double roll = std::stod(headPoseResults.rows[2][1]);
//                 kssCalculator.setHeadPose(yaw, pitch, roll);
//             } catch (const std::invalid_argument& e) {
//                 kssCalculator.setHeadPose(0.0, 0.0, 0.0);
//                  printf("WARN: Error parsing head pose: %s\n", e.what());
//             } catch (const std::out_of_range& e) {
//                  kssCalculator.setHeadPose(0.0, 0.0, 0.0);
//                  printf("WARN: Error parsing head pose: %s\n", e.what());
//             }
//         } else {
//              kssCalculator.setHeadPose(0.0, 0.0, 0.0);
//         }
//         kssCalculator.setYawnMetrics(yawnMetrics.isYawning, yawnMetrics.yawnFrequency, yawnMetrics.yawnDuration);
//         kssCalculator.setDetectedObjects(detectedObjects);

//         compositeKSS = kssCalculator.calculateCompositeKSS();
//         // Only update status if it wasn't set during failed calibration/no face
//         if (kssStatus == "Initializing" || kssStatus == "Low Risk (Not Calibrated)") {
//            kssStatus = kssCalculator.getKSSAlertStatus(compositeKSS);
//         }
//         // --- End KSS Calculation ---

//     } // else: keep default KSS score/status if no valid face


//     // --- Draw Results ---

//     // Draw Face Analysis Results
//     printf("Drawing %d faces.\n", face_results.count);
//     for (int i = 0; i < face_results.count; ++i) {
//         face_object_t *face = &face_results.faces[i];
//         int rx = face->box.left; int ry = face->box.top; int rw = face->box.right - face->box.left; int rh = face->box.bottom - face->box.top;
//         draw_rectangle(&src_image, rx, ry, rw, rh, COLOR_GREEN, 3);
//         char score_text[20]; snprintf(score_text, 20, "%.2f", face->score);
//         draw_text(&src_image, score_text, rx, ry - 20 > 0 ? ry - 20 : ry, COLOR_RED, 20);

//         if (face->face_landmarks_valid) { for (int j = 0; j < NUM_FACE_LANDMARKS; j++) { draw_circle(&src_image, face->face_landmarks[j].x, face->face_landmarks[j].y, 1, COLOR_ORANGE, 2); } }
//         if (face->eye_landmarks_left_valid) { for (int j = 0; j < NUM_EYE_CONTOUR_LANDMARKS; j++) { draw_circle(&src_image, face->eye_landmarks_left[j].x, face->eye_landmarks_left[j].y, 1, COLOR_BLUE, 2); } }
//         if (face->iris_landmarks_left_valid) { for (int j = 0; j < NUM_IRIS_LANDMARKS; j++) { draw_circle(&src_image, face->iris_landmarks_left[j].x, face->iris_landmarks_left[j].y, 2, COLOR_YELLOW, 3); } }
//         if (face->eye_landmarks_right_valid) { for (int j = 0; j < NUM_EYE_CONTOUR_LANDMARKS; j++) { draw_circle(&src_image, face->eye_landmarks_right[j].x, face->eye_landmarks_right[j].y, 1, COLOR_BLUE, 2); } }
//         if (face->iris_landmarks_right_valid) { for (int j = 0; j < NUM_IRIS_LANDMARKS; j++) { draw_circle(&src_image, face->iris_landmarks_right[j].x, face->iris_landmarks_right[j].y, 2, COLOR_YELLOW, 3); } }
//     }

//     // Draw YOLO Detection Results
//     printf("Drawing %d YOLO objects.\n", yolo_results.count);
//     for (int i = 0; i < yolo_results.count; i++) {
//         object_detect_result *det_result = &(yolo_results.results[i]);
//         int x1 = det_result->box.left; int y1 = det_result->box.top; int x2 = det_result->box.right; int y2 = det_result->box.bottom;
//         draw_rectangle(&src_image, x1, y1, x2 - x1, y2 - y1, COLOR_MAGENTA, 3);
//         char text[256]; sprintf(text, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
//         draw_text(&src_image, text, x1, y1 - 40 > 0 ? y1 - 40 : y1, COLOR_CYAN, 15);
//     }


//     // --- Draw Behavior Metrics & KSS ---
//     status_color_uint = (compositeKSS >= 4) ? COLOR_RED : COLOR_GREEN; // Determine color based on final KSS

//     // Start drawing text lower down if YOLO boxes are present near the top
//     text_y = 60; // Reset starting Y position

//     draw_text(&src_image, kssStatus.c_str(), 10, text_y, status_color_uint, 20); // Larger text for status
//     text_y += 30;

//     text_stream.str(""); text_stream << "PERCLOS: " << std::fixed << std::setprecision(2) << blinkDetector.getPerclos() << "%";
//     draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, 15); text_y += line_height;

//     text_stream.str(""); text_stream << "Blink Rate: " << std::fixed << std::setprecision(1) << blinkDetector.getBlinkRate() << " BPM";
//     draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, 15); text_y += line_height;

//     text_stream.str(""); text_stream << "Yawn Count (last min): " << static_cast<int>(yawnMetrics.yawnFrequency);
//     draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, 15); text_y += line_height;

//     if (calibration_done && headPoseResults.rows.size() >= 3) {
//         // Draw Yaw, Pitch, Roll values more compactly
//         std::string headpose_text = "Yaw:" + headPoseResults.rows[0][1] +
//                                     " Pitch:" + headPoseResults.rows[1][1] +
//                                     " Roll:" + headPoseResults.rows[2][1];
//         draw_text(&src_image, headpose_text.c_str(), 10, text_y, COLOR_WHITE, 15);
//         text_y += line_height;
//     }

//     text_stream.str(""); text_stream << "KSS Score: " << compositeKSS;
//     draw_text(&src_image, text_stream.str().c_str(), 10, text_y, status_color_uint, 15); text_y += line_height;
//     // --- End Drawing Behavior Metrics ---

//     write_image("out_final.jpg", &src_image);
//     printf("Final result image saved to out_final.jpg\n");

// out:
//     // Release all resources
//     release_face_analyzer(&app_ctx.face_ctx);
//     release_yolo11(&app_ctx.yolo_ctx);
//     deinit_post_process();

//     if (src_image.virt_addr != NULL) { free(src_image.virt_addr); }
//     printf("Exiting (ret = %d)\n", ret);
//     return ret; // Return final status
// }


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <stdexcept>
#include <cmath>
#include <opencv2/core/types.hpp> // For cv::Point

#include "face_analyzer/face_analyzer.h"
#include "yolo_detector/yolo11.h"
#include "behavior_analysis/BlinkDetector.hpp"
#include "behavior_analysis/YawnDetector.hpp"
#include "behavior_analysis/HeadPoseTracker.hpp"
#include "behavior_analysis/KSSCalculator.hpp"
#include "image_utils.h"
#include "file_utils.h"
#include "image_drawing.h"

// --- Color Defines ---
#ifndef COLOR_MAGENTA
#define COLOR_MAGENTA (0xFF00FF)
#endif
// ... other color defines ...
#ifndef COLOR_CYAN
#define COLOR_CYAN (0x00FFFF)
#endif
#ifndef COLOR_YELLOW
#define COLOR_YELLOW (0xFFFF00)
#endif
#ifndef COLOR_BLUE
#define COLOR_BLUE (0x0000FF)
#endif
#ifndef COLOR_ORANGE
#define COLOR_ORANGE (0xFFA500)
#endif
#ifndef COLOR_WHITE
#define COLOR_WHITE (0xFFFFFF)
#endif
// --- End Color Defines ---

// --- Top Level Context ---
typedef struct app_context_t {
    face_analyzer_app_context_t face_ctx;
    yolo11_app_context_t        yolo_ctx;
} app_context_t;
// --- End Top Level Context ---

// --- Helper Functions ---
std::vector<cv::Point> convert_landmarks_to_cvpoint(const point_t landmarks[], int count) {
    std::vector<cv::Point> cv_landmarks; cv_landmarks.reserve(count);
    for (int i = 0; i < count; ++i) { cv_landmarks.emplace_back(cv::Point(landmarks[i].x, landmarks[i].y)); }
    return cv_landmarks;
}
double parse_head_pose_value(const std::string& s) {
    try { size_t first_space = s.find_first_of(" \t"); if (first_space == std::string::npos) { return std::stod(s); } std::string value_part = s.substr(first_space + 1); return std::stod(value_part); } catch (...) { printf("WARN: Could not parse head pose value from string: '%s'\n", s.c_str()); return 0.0; }
}


/*-------------------------------------------
                  Main Function
-------------------------------------------*/
int main(int argc, char **argv)
{
    if (argc != 6) { printf("Usage: %s <face_detect_model> <face_lmk_model> <iris_lmk_model> <yolo_model> <image_path>\n", argv[0]); return -1; }

    // --- Variable Declarations (Moved UP) ---
    int ret = 0;
    const char *detection_model_path = argv[1];
    const char *landmark_model_path  = argv[2];
    const char *iris_model_path      = argv[3];
    const char *yolo_model_path      = argv[4];
    const char *image_path           = argv[5];

    app_context_t app_ctx;
    memset(&app_ctx, 0, sizeof(app_context_t));
    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t)); // Important: init virt_addr to NULL

    face_analyzer_result_t face_results;
    memset(&face_results, 0, sizeof(face_results));
    object_detect_result_list yolo_results;
    memset(&yolo_results, 0, sizeof(yolo_results));

    // Behavior Analysis Objects
    my::BlinkDetector blinkDetector;
    YawnDetector yawnDetector;
    my::HeadPoseTracker headPoseTracker;
    KSSCalculator kssCalculator;

    // Calibration & KSS Variables
    bool calibration_done = false;
    const int CALIBRATION_TIME_SEC = 5; // Not used in single image, but keep for structure
    auto calibration_start_time = std::chrono::steady_clock::now(); // Not used in single image
    int compositeKSS = 1;
    std::string kssStatus = "Initializing";
    YawnDetector::YawnMetrics yawnMetrics = {};
    my::HeadPoseTracker::HeadPoseResults headPoseResults = {};
    std::vector<std::string> detectedObjects;

    // Drawing Variables (declare early, initialize later if needed)
    std::stringstream text_stream;
    int text_y = 0; // Will be reset before drawing
    const int line_height = 16;
    const int text_size = 7;
    const int status_text_size = 18;
    const int thickness = 1; // Use 1 for smaller text
    unsigned int status_color_uint = COLOR_GREEN;
    // --- End Variable Declarations ---


    // --- Initialize Modules ---
    ret = init_post_process(); if (ret != 0) { printf("Error init YOLO postprocess.\n"); ret = -1; goto out; }
    ret = init_face_analyzer(detection_model_path, landmark_model_path, iris_model_path, &app_ctx.face_ctx); if (ret != 0) { printf("init_face_analyzer fail! ret=%d\n", ret); goto out; }
    ret = init_yolo11(yolo_model_path, &app_ctx.yolo_ctx); if (ret != 0) { printf("init_yolo11 fail! ret=%d\n", ret); goto out; }

    // --- Read Source Image ---
    ret = read_image(image_path, &src_image); if (ret != 0) { printf("read image fail! ret=%d\n", ret); goto out; }


    // --- Initial Face Inference for Calibration ---
    ret = inference_face_analyzer(&app_ctx.face_ctx, &src_image, &face_results); if (ret != 0) { printf("Initial inference_face_analyzer fail! ret=%d\n", ret); goto out;}


    // --- Calibration Logic ---
    printf("Attempting calibration...\n");
    if (face_results.count > 0 && face_results.faces[0].face_landmarks_valid) {
         std::vector<cv::Point> faceLandmarksCv = convert_landmarks_to_cvpoint(face_results.faces[0].face_landmarks, NUM_FACE_LANDMARKS);
         headPoseResults = headPoseTracker.run(faceLandmarksCv);
         if (headPoseTracker.isCalibrated()) { calibration_done = true; printf("Calibration Complete!\n"); }
         else { printf("Calibration Failed (Head pose not suitable?).\n"); calibration_done = true; kssStatus = "Low Risk (Cal. Failed)"; }
    } else { printf("Calibration Failed (No face/landmarks).\n"); calibration_done = true; kssStatus = "Low Risk (No Face)"; }


    // --- Main Processing (Run YOLO & Behavior Analysis) ---
    ret = inference_yolo11(&app_ctx.yolo_ctx, &src_image, &yolo_results); if (ret != 0) { printf("inference_yolo11 fail! ret=%d\n", ret); } // Log error, continue

    if (face_results.count > 0 && face_results.faces[0].face_landmarks_valid) {
        face_object_t *face = &face_results.faces[0];
        std::vector<cv::Point> faceLandmarksCv = convert_landmarks_to_cvpoint(face->face_landmarks, NUM_FACE_LANDMARKS);
        blinkDetector.run(faceLandmarksCv, src_image.width, src_image.height); // Refactored call
        yawnMetrics = yawnDetector.run(faceLandmarksCv, src_image.width, src_image.height); // Refactored call
        headPoseResults = headPoseTracker.run(faceLandmarksCv);

        detectedObjects.clear();
        for (int j = 0; j < yolo_results.count; ++j) { detectedObjects.push_back(coco_cls_to_name(yolo_results.results[j].cls_id)); }

        // --- KSS Calculation ---
        kssCalculator.setPerclos(blinkDetector.getPerclos());
        if (calibration_done && headPoseResults.rows.size() >= 3) {
             try { kssCalculator.setHeadPose(parse_head_pose_value(headPoseResults.rows[0][1]), parse_head_pose_value(headPoseResults.rows[1][1]), parse_head_pose_value(headPoseResults.rows[2][1])); } catch (...) { kssCalculator.setHeadPose(0.0, 0.0, 0.0); }
        } else { kssCalculator.setHeadPose(0.0, 0.0, 0.0); }
        kssCalculator.setYawnMetrics(yawnMetrics.isYawning, yawnMetrics.yawnFrequency, yawnMetrics.yawnDuration);
        kssCalculator.setDetectedObjects(detectedObjects);
        compositeKSS = kssCalculator.calculateCompositeKSS();
        if (kssStatus == "Initializing" || kssStatus == "Low Risk (Not Calibrated)") { kssStatus = kssCalculator.getKSSAlertStatus(compositeKSS); }
        // --- End KSS Calculation ---
    } // else: keep default KSS if no valid face


    // --- Draw Results ---
    printf("Drawing %d faces.\n", face_results.count);
    for (int i = 0; i < face_results.count; ++i) {
        face_object_t *face = &face_results.faces[i];
        int rx = face->box.left; int ry = face->box.top; int rw = face->box.right - face->box.left; int rh = face->box.bottom - face->box.top;
        draw_rectangle(&src_image, rx, ry, rw, rh, COLOR_GREEN, 3);
        char score_text[20]; snprintf(score_text, 20, "%.2f", face->score);
        draw_text(&src_image, score_text, rx, ry - 20 > 0 ? ry - 20 : ry, COLOR_RED, 20);

        if (face->face_landmarks_valid) { for (int j = 0; j < NUM_FACE_LANDMARKS; j++) { draw_circle(&src_image, face->face_landmarks[j].x, face->face_landmarks[j].y, 1, COLOR_ORANGE, 2); } }
        if (face->eye_landmarks_left_valid) { for (int j = 0; j < NUM_EYE_CONTOUR_LANDMARKS; j++) { draw_circle(&src_image, face->eye_landmarks_left[j].x, face->eye_landmarks_left[j].y, 1, COLOR_BLUE, 2); } }
        if (face->iris_landmarks_left_valid) { for (int j = 0; j < NUM_IRIS_LANDMARKS; j++) { draw_circle(&src_image, face->iris_landmarks_left[j].x, face->iris_landmarks_left[j].y, 2, COLOR_YELLOW, 3); } }
        if (face->eye_landmarks_right_valid) { for (int j = 0; j < NUM_EYE_CONTOUR_LANDMARKS; j++) { draw_circle(&src_image, face->eye_landmarks_right[j].x, face->eye_landmarks_right[j].y, 1, COLOR_BLUE, 2); } }
        if (face->iris_landmarks_right_valid) { for (int j = 0; j < NUM_IRIS_LANDMARKS; j++) { draw_circle(&src_image, face->iris_landmarks_right[j].x, face->iris_landmarks_right[j].y, 2, COLOR_YELLOW, 3); } }
    }

    // Draw YOLO Detection Results
    printf("Drawing %d YOLO objects.\n", yolo_results.count);
    for (int i = 0; i < yolo_results.count; i++) {
        object_detect_result *det_result = &(yolo_results.results[i]);
        int x1 = det_result->box.left; int y1 = det_result->box.top; int x2 = det_result->box.right; int y2 = det_result->box.bottom;
        draw_rectangle(&src_image, x1, y1, x2 - x1, y2 - y1, COLOR_MAGENTA, 3);
        char text[256]; sprintf(text, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
        draw_text(&src_image, text, x1, y1 - 40 > 0 ? y1 - 40 : y1, COLOR_CYAN, 15);
    }

    // --- Draw Behavior Metrics & KSS ---
    status_color_uint = (compositeKSS >= 4) ? COLOR_RED : COLOR_GREEN;
    text_y = 6; // Reset Y for drawing

    draw_text(&src_image, kssStatus.c_str(), 10, text_y, status_color_uint, status_text_size); text_y += (int)(line_height * 1.5);
    text_stream.str(""); text_stream << "PERCLOS: " << std::fixed << std::setprecision(2) << blinkDetector.getPerclos() << "%"; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;
    text_stream.str(""); text_stream << "Blink Count: " << blinkDetector.getBlinkCount(); draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;
    text_stream.str(""); text_stream << "Last Blink Dur: " << std::fixed << std::setprecision(2) << blinkDetector.getLastBlinkDuration() << " s"; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;
    if (calibration_done && headPoseResults.rows.size() >= 3) { std::string headpose_text = "Yaw:" + headPoseResults.rows[0][1] + " Pitch:" + headPoseResults.rows[1][1] + " Roll:" + headPoseResults.rows[2][1]; draw_text(&src_image, headpose_text.c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height; } else { draw_text(&src_image, "Head Pose: N/A", 10, text_y, COLOR_WHITE, text_size); text_y += line_height; }
    text_stream.str(""); text_stream << "Yawning: " << (yawnMetrics.isYawning ? "Yes" : "No"); draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;
    text_stream.str(""); text_stream << "Yawn Freq (last min): " << static_cast<int>(yawnMetrics.yawnFrequency); draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;
    text_stream.str(""); text_stream << "Last Yawn Dur: " << std::fixed << std::setprecision(2) << yawnMetrics.yawnDuration << " s"; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, COLOR_WHITE, text_size); text_y += line_height;
    text_stream.str(""); text_stream << "KSS Score: " << compositeKSS; draw_text(&src_image, text_stream.str().c_str(), 10, text_y, status_color_uint, text_size); text_y += line_height;
    // --- End Drawing Behavior Metrics ---

    write_image("out_final.jpg", &src_image);
    printf("Final result image saved to out_final.jpg\n");

out:
    printf("Cleaning up...\n");
    release_face_analyzer(&app_ctx.face_ctx);
    release_yolo11(&app_ctx.yolo_ctx);
    deinit_post_process();
    if (src_image.virt_addr != NULL) { free(src_image.virt_addr); }
    printf("Exiting (ret = %d)\n", ret);
    return ret;
}