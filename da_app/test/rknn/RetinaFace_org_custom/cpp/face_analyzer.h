// #ifndef RKNN_FACE_ANALYZER_H
// #define RKNN_FACE_ANALYZER_H

// #include "rknn_api.h"
// #include "common.h" // Assuming common.h includes image_buffer_t, letterbox_t etc.
// #include <vector>

// #define MAX_FACE_RESULTS 64 // Maximum faces to process
// #define NUM_LANDMARKS 468   // Number of landmarks from the new model

// // Basic point structure
// typedef struct point_t {
//     int x;
//     int y;
// } point_t;

// // Basic bounding box structure
// typedef struct box_rect_t {
//     int left;
//     int top;
//     int right;
//     int bottom;
// } box_rect_t;

// // Structure to hold results for a single detected face
// typedef struct face_object_t {
//     box_rect_t box;         // Bounding box
//     float score;            // Detection confidence
//     point_t landmarks[NUM_LANDMARKS]; // 468 Landmarks
//     bool landmarks_valid;   // Flag indicating if landmarks were successfully computed
// } face_object_t;

// // Structure to hold all results for an image
// typedef struct face_analyzer_result_t {
//     int count;                             // Number of detected faces
//     face_object_t faces[MAX_FACE_RESULTS]; // Array of face results
// } face_analyzer_result_t;


// // Application context holding information for BOTH models
// typedef struct rknn_app_context_t {
//     rknn_context rknn_ctx_detect;      // Context for Face Detection model
//     rknn_input_output_num io_num_detect;
//     rknn_tensor_attr *input_attrs_detect;
//     rknn_tensor_attr *output_attrs_detect;
//     int model1_channel;
//     int model1_width;
//     int model1_height;
//     rknn_tensor_format input_fmt_detect; // Store input format

//     rknn_context rknn_ctx_landmark;    // Context for Face Landmark model
//     rknn_input_output_num io_num_landmark;
//     rknn_tensor_attr *input_attrs_landmark;
//     rknn_tensor_attr *output_attrs_landmark;
//     int model2_channel;
//     int model2_width;
//     int model2_height;
//     rknn_tensor_format input_fmt_landmark; // Store input format

// } rknn_app_context_t;


// /**
//  * @brief Initialize both Face Detection and Face Landmark RKNN models.
//  *
//  * @param detection_model_path Path to the Face Detection RKNN model.
//  * @param landmark_model_path Path to the Face Landmark RKNN model.
//  * @param app_ctx Pointer to the application context structure to be filled.
//  * @return int 0 if successful, non-zero otherwise.
//  */
// int init_face_analyzer_models(const char *detection_model_path, const char* landmark_model_path, rknn_app_context_t *app_ctx);

// /**
//  * @brief Release resources associated with both RKNN models.
//  *
//  * @param app_ctx Pointer to the application context.
//  * @return int 0 if successful, non-zero otherwise.
//  */
// int release_face_analyzer_models(rknn_app_context_t *app_ctx);

// /**
//  * @brief Run inference using both models: Detect faces, then find landmarks for each face.
//  *
//  * @param app_ctx Pointer to the initialized application context.
//  * @param img Pointer to the input image buffer.
//  * @param out_result Pointer to the structure where results will be stored.
//  * @return int 0 if successful, non-zero otherwise.
//  */
// int inference_face_analyzer_models(rknn_app_context_t *app_ctx, image_buffer_t *img, face_analyzer_result_t *out_result);

// #endif // RKNN_FACE_ANALYZER_H




#ifndef RKNN_FACE_ANALYZER_H
#define RKNN_FACE_ANALYZER_H

#include "rknn_api.h"
#include "common.h" // Assuming common.h includes image_buffer_t, letterbox_t etc.
#include <vector>

#define MAX_FACE_RESULTS 64 // Maximum faces to process
#define NUM_FACE_LANDMARKS 468 // Number of landmarks from the face landmark model
#define NUM_EYE_CONTOUR_LANDMARKS 71 // Number of eye contour landmarks from iris model
#define NUM_IRIS_LANDMARKS 5       // Number of iris landmarks from iris model

// Basic point structure
typedef struct point_t {
    int x;
    int y;
} point_t;

// Basic bounding box structure
typedef struct box_rect_t {
    int left;
    int top;
    int right;
    int bottom;
} box_rect_t;

// Structure to hold results for a single detected face
typedef struct face_object_t {
    box_rect_t box;         // Bounding box
    float score;            // Detection confidence

    point_t face_landmarks[NUM_FACE_LANDMARKS]; // 468 Face Landmarks
    bool face_landmarks_valid;   // Flag indicating if face landmarks were computed

    point_t eye_landmarks_left[NUM_EYE_CONTOUR_LANDMARKS]; // 71 eye contour points
    point_t iris_landmarks_left[NUM_IRIS_LANDMARKS];       // 5 iris points
    bool eye_landmarks_left_valid; // separate flags for left/right eye results
    bool iris_landmarks_left_valid;

    point_t eye_landmarks_right[NUM_EYE_CONTOUR_LANDMARKS];
    point_t iris_landmarks_right[NUM_IRIS_LANDMARKS];
    bool eye_landmarks_right_valid;
    bool iris_landmarks_right_valid;

} face_object_t;

// Structure to hold all results for an image
typedef struct face_analyzer_result_t {
    int count;                             // Number of detected faces
    face_object_t faces[MAX_FACE_RESULTS]; // Array of face results
} face_analyzer_result_t;


// Application context holding information for ALL THREE models
typedef struct rknn_app_context_t {
    // Model 1: Face Detection
    rknn_context rknn_ctx_detect;
    rknn_input_output_num io_num_detect;
    rknn_tensor_attr *input_attrs_detect;
    rknn_tensor_attr *output_attrs_detect;
    int model1_channel;
    int model1_width;
    int model1_height;
    rknn_tensor_format input_fmt_detect;

    // Model 2: Face Landmarks
    rknn_context rknn_ctx_landmark;
    rknn_input_output_num io_num_landmark;
    rknn_tensor_attr *input_attrs_landmark;
    rknn_tensor_attr *output_attrs_landmark;
    int model2_channel;
    int model2_width;
    int model2_height;
    rknn_tensor_format input_fmt_landmark;

    // Model 3: Iris Landmarks
    rknn_context rknn_ctx_iris;
    rknn_input_output_num io_num_iris;
    rknn_tensor_attr *input_attrs_iris;
    rknn_tensor_attr *output_attrs_iris; // Will point to an array of 2 attrs
    int model3_channel;
    int model3_width;
    int model3_height;
    rknn_tensor_format input_fmt_iris;

} rknn_app_context_t;


/**
 * @brief Initialize Face Detection, Face Landmark, and Iris Landmark RKNN models.
 *
 * @param detection_model_path Path to the Face Detection RKNN model.
 * @param landmark_model_path Path to the Face Landmark RKNN model.
 * @param iris_model_path Path to the Iris Landmark RKNN model.
 * @param app_ctx Pointer to the application context structure to be filled.
 * @return int 0 if successful, non-zero otherwise.
 */
int init_face_analyzer_models(const char *detection_model_path,
                              const char* landmark_model_path,
                              const char* iris_model_path,
                              rknn_app_context_t *app_ctx);

/**
 * @brief Release resources associated with all RKNN models.
 *
 * @param app_ctx Pointer to the application context.
 * @return int 0 if successful, non-zero otherwise.
 */
int release_face_analyzer_models(rknn_app_context_t *app_ctx);

/**
 * @brief Run inference using all models: Detect faces, find face landmarks, find iris/eye landmarks.
 *
 * @param app_ctx Pointer to the initialized application context.
 * @param img Pointer to the input image buffer.
 * @param out_result Pointer to the structure where results will be stored.
 * @return int 0 if successful, non-zero otherwise.
 */
int inference_face_analyzer_models(rknn_app_context_t *app_ctx, image_buffer_t *img, face_analyzer_result_t *out_result);

#endif // RKNN_FACE_ANALYZER_H