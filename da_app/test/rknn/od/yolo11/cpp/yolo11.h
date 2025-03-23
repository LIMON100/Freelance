#ifndef _YOLO11_H
#define _YOLO11_H

#include <stdint.h>
#include "rknn_api.h"
#include "image_utils.h"

// Define object detection result types
typedef struct {
    int left;
    int top;
    int right;
    int bottom;
} box_t;

typedef struct {
    int cls_id;
    float prop;
    box_t box;
} object_detect_result;

typedef struct {
    int count;
    object_detect_result results[100]; // Adjust size as needed
} object_detect_result_list;

typedef struct {
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr *input_attrs;
    rknn_tensor_attr *output_attrs;
    int model_width;
    int model_height;
    int model_channels;
} rknn_app_context_t;

int init_yolo11_model(const char* model_path, rknn_app_context_t* app_ctx);
int release_yolo11_model(rknn_app_context_t* app_ctx);
int inference_yolo11_model(rknn_app_context_t* app_ctx, image_buffer_t* img, object_detect_result_list* od_results);

#endif