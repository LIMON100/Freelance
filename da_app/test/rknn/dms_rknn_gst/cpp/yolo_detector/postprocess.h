// #ifndef _RKNN_YOLO11_DEMO_POSTPROCESS_H_
// #define _RKNN_YOLO11_DEMO_POSTPROCESS_H_

// #include <stdint.h>
// #include <vector>
// #include "rknn_api.h"
// #include "common.h"
// #include "image_utils.h"

// #define OBJ_NAME_MAX_SIZE 64
// #define OBJ_NUMB_MAX_SIZE 128
// #define OBJ_CLASS_NUM 4
// #define NMS_THRESH 0.5
// #define BOX_THRESH 0.6  // Lowered from 0.25 to 0.1 to be less strict

// // class rknn_app_context_t;

// typedef struct {
//     image_rect_t box;
//     float prop;
//     int cls_id;
// } object_detect_result;

// typedef struct {
//     int id;
//     int count;
//     object_detect_result results[OBJ_NUMB_MAX_SIZE];
// } object_detect_result_list;

// int init_post_process();
// void deinit_post_process();
// char *coco_cls_to_name(int cls_id);
// // int post_process(rknn_app_context_t *app_ctx, void *outputs, letterbox_t *letter_box, float conf_threshold, float nms_threshold, object_detect_result_list *od_results);
// int post_process(yolo11_app_context_t *app_ctx, void *outputs, letterbox_t *letter_box, float conf_threshold, float nms_threshold, object_detect_result_list *od_results); 

// void deinitPostProcess();
// #endif //_RKNN_YOLO11_DEMO_POSTPROCESS_H_


// File: yolo_detector/postprocess.h
#ifndef _RKNN_YOLO11_DEMO_POSTPROCESS_H_
#define _RKNN_YOLO11_DEMO_POSTPROCESS_H_

#include <stdint.h>
#include <vector>
#include "rknn_api.h"
#include "common.h"
#include "image_utils.h" // Keep if needed elsewhere

// Keep definitions for yolo11_app_context_t forward declaration needed
// Or include yolo11.h if postprocess.h is used independently
// Forward declaration is generally safer if yolo11.h includes postprocess.h
struct yolo11_app_context_t; // Forward declaration

#define OBJ_NAME_MAX_SIZE 64
#define OBJ_NUMB_MAX_SIZE 128
#define OBJ_CLASS_NUM 4 // Make sure this matches your model
#define NMS_THRESH 0.5
#define BOX_THRESH 0.6

// Struct definitions remain the same
typedef struct {
    image_rect_t box;
    float prop;
    int cls_id;
} object_detect_result;

typedef struct {
    int id;
    int count;
    object_detect_result results[OBJ_NUMB_MAX_SIZE];
} object_detect_result_list;

// Function declarations
int init_post_process();
void deinit_post_process();

// *** FIX: Change return type to const char* ***
const char *coco_cls_to_name(int cls_id);

// Signature remains the same, caller passes rknn_tensor_mem** as void*
int post_process(yolo11_app_context_t *app_ctx,
                 void *outputs, // Keep as void* for flexibility
                 letterbox_t *letter_box, // Keep or remove based on final implementation
                 float conf_threshold, float nms_threshold,
                 object_detect_result_list *od_results);

// Remove redundant declaration if present:
// void deinitPostProcess();

#endif //_RKNN_YOLO11_DEMO_POSTPROCESS_H_