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




#ifndef _RKNN_YOLO11_DEMO_POSTPROCESS_H_
#define _RKNN_YOLO11_DEMO_POSTPROCESS_H_

#include <stdint.h>
#include <vector>
#include "rknn_api.h"
#include "common.h"
#include "image_utils.h"
#include "yolo11.h" // Include yolo11 header to get yolo11_app_context_t

#define OBJ_NAME_MAX_SIZE 64
#define OBJ_NUMB_MAX_SIZE 128
#define OBJ_CLASS_NUM 4 // Make sure this matches your custom_class.txt
#define NMS_THRESH 0.5
#define BOX_THRESH 0.6

// Structs remain the same
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

int init_post_process();
void deinit_post_process();
char *coco_cls_to_name(int cls_id);

// *** MODIFIED Signature: Removed letter_box, added scaling info ***
int post_process(yolo11_app_context_t *app_ctx,
                 void *outputs, // Still void* to handle rknn_output or rknn_tensor_mem**
                 float scale_w, // Scaling factor width used during preprocessing
                 float scale_h, // Scaling factor height used during preprocessing
                 int offset_x,  // X offset of the crop in original image
                 int offset_y,  // Y offset of the crop in original image
                 float conf_threshold,
                 float nms_threshold,
                 object_detect_result_list *od_results);

// void deinitPostProcess(); // Duplicate? Use deinit_post_process()

#endif //_RKNN_YOLO11_DEMO_POSTPROCESS_H_