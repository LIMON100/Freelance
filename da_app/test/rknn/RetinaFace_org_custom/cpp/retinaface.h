#ifndef RKNN_DEMO_RETINAFACE_H
#define RKNN_DEMO_RETINAFACE_H

#include "rknn_api.h"
#include "common.h" // Make sure common.h defines image_buffer_t, letterbox_t

// Context structure to hold BOTH model details
typedef struct {
    // --- Face Detection Model ---
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr *input_attrs;
    rknn_tensor_attr *output_attrs;
    int model_channel;
    int model_width;
    int model_height;

    // --- Landmark Model ---
    rknn_context landmark_ctx;
    rknn_input_output_num landmark_io_num;
    rknn_tensor_attr *landmark_input_attrs;
    rknn_tensor_attr *landmark_output_attrs;
    int landmark_model_channel;
    int landmark_model_width;
    int landmark_model_height;

} rknn_app_context_t;

// Bounding Box structure
typedef struct box_rect_t {
    int left;
    int top;
    int right;
    int bottom;
} box_rect_t;

// Landmark Point structure
typedef struct ponit_t {
    int x;
    int y;
} ponit_t;

// Structure to hold results for a single detected face
typedef struct retinaface_object_t {
    int cls;
    box_rect_t box;
    float score;
    ponit_t landmarks[468]; // Correctly named for 468 points
} retinaface_object_t;

// Structure to hold all detection results for an image
typedef struct {
    int count;                          // Number of faces detected
    retinaface_object_t object[128];    // Array to store results for up to 128 faces
} retinaface_result;

// Function Prototypes
int init_retinaface_model(const char *face_model_path, const char *landmark_model_path, rknn_app_context_t *app_ctx);
int release_retinaface_model(rknn_app_context_t *app_ctx);
// Renamed the main inference function for clarity
int inference_retinaface_pipeline(rknn_app_context_t *app_ctx, image_buffer_t *img, retinaface_result *out_result);

#endif // RKNN_DEMO_RETINAFACE_H