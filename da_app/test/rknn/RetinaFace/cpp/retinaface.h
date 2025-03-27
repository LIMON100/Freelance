// #ifndef _RKNN_DEMO_MOBILENET_H_
// #define _RKNN_DEMO_MOBILENET_H_

// #include "rknn_api.h"
// #include "common.h"

// typedef struct {
//     rknn_context rknn_ctx;
//     rknn_input_output_num io_num;
//     rknn_tensor_attr *input_attrs;
//     rknn_tensor_attr *output_attrs;
//     int model_channel;
//     int model_width;
//     int model_height;
// } rknn_app_context_t;

// typedef struct box_rect_t {
//     int left;    ///< Most left coordinate
//     int top;     ///< Most top coordinate
//     int right;   ///< Most right coordinate
//     int bottom;  ///< Most bottom coordinate
// } box_rect_t;

// typedef struct ponit_t {
//     int x;
//     int y;
// } ponit_t;

// typedef struct retinaface_object_t {
//     int cls;       
//     box_rect_t box;  
//     float score;      
//     ponit_t ponit[5];
// } retinaface_object_t;

// typedef struct {
//     int count;
//     retinaface_object_t object[128];
// } retinaface_result;

// int init_retinaface_model(const char *model_path, rknn_app_context_t *app_ctx);

// int release_retinaface_model(rknn_app_context_t *app_ctx);

// int inference_retinaface_model(rknn_app_context_t *app_ctx, image_buffer_t *img, retinaface_result *out_result);

// #endif //_RKNN_DEMO_MOBILENET_H_


#ifndef _RKNN_DEMO_MOBILENET_H_
#define _RKNN_DEMO_MOBILENET_H_

#include "rknn_api.h"
#include "common.h" // Assuming this includes your utility functions

typedef struct {
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr *input_attrs;
    rknn_tensor_attr *output_attrs;
    int model_channel;
    int model_width;
    int model_height;
} rknn_app_context_t;

typedef struct box_rect_t {
    int left;    ///< Most left coordinate
    int top;     ///< Most top coordinate
    int right;   ///< Most right coordinate
    int bottom;  ///< Most bottom coordinate
} box_rect_t;

typedef struct ponit_t {
    int x;
    int y;
} ponit_t;

typedef struct retinaface_object_t {
    int cls;
    box_rect_t box;
    float score;
    ponit_t ponit[468]; // Assuming face mesh has 468 points
} retinaface_object_t;

typedef struct {
    int count;
    retinaface_object_t object[128];  // Assuming a maximum of 128 faces
} retinaface_result;

// Function Prototypes - Updated
int init_faceD_faceL_model(const char *model_path, const char *model_path2, rknn_app_context_t *app_faceD_ctx, rknn_app_context_t *app_faceL_ctx); // Function to init both models
int release_faceD_model(rknn_app_context_t *app_ctx); // Releasing face detection model
int release_faceL_model(rknn_app_context_t *app_ctx); // Releasing facial landmark model
int inference_faceD_faceL_model(rknn_app_context_t *app_faceD_ctx, rknn_app_context_t *app_faceL_ctx, image_buffer_t *img, retinaface_result *out_result);

#endif //_RKNN_DEMO_MOBILENET_H_