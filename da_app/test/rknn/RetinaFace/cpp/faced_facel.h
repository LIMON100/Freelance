// #ifndef _RKNN_DEMO_FACED_FACEL_H_
// #define _RKNN_DEMO_FACED_FACEL_H_

// #include "rknn_api.h"
// #include <stdint.h>
// #include <vector>
// #include <opencv2/core.hpp>
// #include <opencv2/highgui.hpp>
// #include <opencv2/imgproc.hpp>

// typedef struct {
//     char *dma_buf_virt_addr;
//     int dma_buf_fd;
//     int size;
// } rknn_dma_buf;

// typedef struct {
//     int left;
//     int top;
//     int right;
//     int bottom;
// } image_rect_t;

// typedef struct {
//     int x;
//     int y;
// } point_t;

// typedef struct {
//     image_rect_t box;
//     float prop;
//     point_t points[468]; // Assuming 468 points for face mesh
// } object_detect_result;

// typedef struct {
//     int count;
//     object_detect_result results[128];
// } object_detect_result_list;

// typedef struct {
//     rknn_context rknn_ctx;
//     rknn_tensor_mem* max_mem;
//     rknn_tensor_mem* net_mem;
//     rknn_input_output_num io_num;
//     rknn_tensor_attr* input_attrs;
//     rknn_tensor_attr* output_attrs;
//     rknn_tensor_mem* input_mems[1];  // Always define input_mems
//     rknn_tensor_mem* output_mems[2]; // Always define output_mems
//     rknn_dma_buf img_dma_buf;        // Always define img_dma_buf
//     int model_channel;
//     int model_width;
//     int model_height;
//     bool is_quant;
// } rknn_app_context_t;

// // Function declarations
// int init_faced_facel_model(const char *faced_model_path, const char *facel_model_path, rknn_app_context_t *app_faced_ctx, rknn_app_context_t *app_facel_ctx);
// int init_faced_model(const char* model_path, rknn_app_context_t* app_ctx);
// int init_facel_model(const char* model_path, rknn_app_context_t* app_ctx);
// int release_faced_facel_model(rknn_app_context_t* app_faced_ctx, rknn_app_context_t* app_facel_ctx);
// int inference_faced_model(rknn_app_context_t* app_ctx, object_detect_result_list* od_results);
// int inference_facel_model(rknn_app_context_t* app_ctx, cv::Mat face_img, point_t* landmarks);
// void mapCoordinates(cv::Mat input, cv::Mat output, int *x, int *y);

// #endif // _RKNN_DEMO_FACED_FACEL_H_


#ifndef _RKNN_DEMO_FACED_FACEL_H_
#define _RKNN_DEMO_FACED_FACEL_H_

#include "rknn_api.h"
#include "image_utils.h"  // For image_buffer_t, image_rect_t, and image utils
#include "image_drawing.h" // For drawing bounding boxes and landmarks
#include <stdint.h>
#include <vector>

typedef struct {
    char *dma_buf_virt_addr;
    int dma_buf_fd;
    int size;
} rknn_dma_buf;

typedef struct {
    int x;
    int y;
} point_t;

typedef struct {
    image_rect_t box; // Use image_rect_t from image_utils.h
    float prop;
    point_t points[468]; // Assuming 468 points for face mesh
} object_detect_result;

typedef struct {
    int count;
    object_detect_result results[128];
} object_detect_result_list;

typedef struct {
    rknn_context rknn_ctx;
    rknn_tensor_mem* max_mem;
    rknn_tensor_mem* net_mem;
    rknn_input_output_num io_num;
    rknn_tensor_attr* input_attrs;
    rknn_tensor_attr* output_attrs;
    rknn_tensor_mem* input_mems[1];
    rknn_tensor_mem* output_mems[2];
    rknn_dma_buf img_dma_buf;
    int model_channel;
    int model_width;
    int model_height;
    bool is_quant;
} rknn_app_context_t;

// Function declarations
int init_faced_facel_model(const char *faced_model_path, const char *facel_model_path, rknn_app_context_t *app_faced_ctx, rknn_app_context_t *app_facel_ctx);
int init_faced_model(const char* model_path, rknn_app_context_t* app_ctx);
int init_facel_model(const char* model_path, rknn_app_context_t* app_ctx);
int release_faced_facel_model(rknn_app_context_t* app_faced_ctx, rknn_app_context_t* app_facel_ctx);
int inference_faced_model(rknn_app_context_t* app_ctx, image_buffer_t* src_image, object_detect_result_list* od_results);
int inference_facel_model(rknn_app_context_t* app_ctx, image_buffer_t* face_img, point_t* landmarks);
void mapCoordinates(image_buffer_t* input, image_buffer_t* output, int *x, int *y);
int crop_image(image_buffer_t* src_image, image_buffer_t* dst_image, int x, int y, int width, int height);
int write_data_to_file(const char *path, const void *data, size_t size); // Declare the fallback

#endif // _RKNN_DEMO_FACED_FACEL_H_