#ifndef RKNN_FACE_ANALYZER_H
#define RKNN_FACE_ANALYZER_H

#include "rknn_api.h"
#include "common.h"
#include <vector>

// Constants remain the same
#define MAX_FACE_RESULTS 64
#define NUM_FACE_LANDMARKS 468
#define NUM_EYE_CONTOUR_LANDMARKS 71
#define NUM_IRIS_LANDMARKS 5

// Structs point_t, box_rect_t, face_object_t, face_analyzer_result_t remain the same
typedef struct point_t { int x; int y; } point_t;
typedef struct box_rect_t { int left; int top; int right; int bottom; } box_rect_t;
typedef struct face_object_t { /* ... as before ... */
    box_rect_t box;
    float score;
    point_t face_landmarks[NUM_FACE_LANDMARKS];
    bool face_landmarks_valid;
    point_t eye_landmarks_left[NUM_EYE_CONTOUR_LANDMARKS];
    point_t iris_landmarks_left[NUM_IRIS_LANDMARKS];
    bool eye_landmarks_left_valid;
    bool iris_landmarks_left_valid;
    point_t eye_landmarks_right[NUM_EYE_CONTOUR_LANDMARKS];
    point_t iris_landmarks_right[NUM_IRIS_LANDMARKS];
    bool eye_landmarks_right_valid;
    bool iris_landmarks_right_valid;
} face_object_t;
typedef struct face_analyzer_result_t { int count; face_object_t faces[MAX_FACE_RESULTS]; } face_analyzer_result_t;


// *** RENAME the context struct ***
typedef struct face_analyzer_app_context_t { // <-- Renamed
    // Model 1: Face Detection
    rknn_context rknn_ctx_detect;
    rknn_input_output_num io_num_detect;
    rknn_tensor_attr *input_attrs_detect;
    rknn_tensor_attr *output_attrs_detect;
    int model1_channel; int model1_width; int model1_height; rknn_tensor_format input_fmt_detect;
    // Model 2: Face Landmarks
    rknn_context rknn_ctx_landmark;
    rknn_input_output_num io_num_landmark;
    rknn_tensor_attr *input_attrs_landmark;
    rknn_tensor_attr *output_attrs_landmark;
    int model2_channel; int model2_width; int model2_height; rknn_tensor_format input_fmt_landmark;
    // Model 3: Iris Landmarks
    rknn_context rknn_ctx_iris;
    rknn_input_output_num io_num_iris;
    rknn_tensor_attr *input_attrs_iris;
    rknn_tensor_attr *output_attrs_iris;
    int model3_channel; int model3_width; int model3_height; rknn_tensor_format input_fmt_iris;
} face_analyzer_app_context_t; // <-- Renamed


// *** RENAME functions for clarity ***
int init_face_analyzer(const char *detection_model_path, // <-- Renamed
                       const char* landmark_model_path,
                       const char* iris_model_path,
                       face_analyzer_app_context_t *app_ctx); // <-- Use renamed struct

int release_face_analyzer(face_analyzer_app_context_t *app_ctx); // <-- Renamed

int inference_face_analyzer(face_analyzer_app_context_t *app_ctx, // <-- Renamed
                            image_buffer_t *img,
                            face_analyzer_result_t *out_result);

#endif // RKNN_FACE_ANALYZER_H