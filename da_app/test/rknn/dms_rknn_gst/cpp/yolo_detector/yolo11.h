// #ifndef _RKNN_DEMO_YOLO11_H_
// #define _RKNN_DEMO_YOLO11_H_

// #include "rknn_api.h"
// #include "common.h"

// #if defined(RV1106_1103)
//     typedef struct { /* ... dma buf ... */ } rknn_dma_buf;
// #endif

// // *** RENAME the context struct ***
// typedef struct yolo11_app_context_t { 
//     rknn_context rknn_ctx;
//     rknn_input_output_num io_num;
//     rknn_tensor_attr* input_attrs;
//     rknn_tensor_attr* output_attrs;
// #if defined(RV1106_1103)
//     rknn_tensor_mem* input_mems[1];
//     rknn_tensor_mem* output_mems[9]; // Assuming max 9 outputs for YOLO? Adjust if needed.
//     rknn_dma_buf img_dma_buf;
// #endif
// #if defined(ZERO_COPY)
//     rknn_tensor_mem* input_mems[1];
//     rknn_tensor_mem* output_mems[9]; // Assuming max 9 outputs
//     rknn_tensor_attr* input_native_attrs;
//     rknn_tensor_attr* output_native_attrs;
// #endif
//     int model_channel;
//     int model_width;
//     int model_height;
//     bool is_quant;
// } yolo11_app_context_t; 


// #include "postprocess.h" // Include YOLO postprocess header


// // *** RENAME functions for clarity ***
// int init_yolo11(const char* model_path, yolo11_app_context_t* app_ctx); 

// int release_yolo11(yolo11_app_context_t* app_ctx); 

// int inference_yolo11(yolo11_app_context_t* app_ctx,
//                      image_buffer_t* img,
//                      object_detect_result_list* od_results);

// #endif //_RKNN_DEMO_YOLO11_H_


#ifndef _RKNN_DEMO_YOLO11_H_
#define _RKNN_DEMO_YOLO11_H_

#include "rknn_api.h"
#include "common.h" // Make sure this includes letterbox_t

// Context struct remains the same as your provided one
typedef struct yolo11_app_context_t {
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr* input_attrs;
    rknn_tensor_attr* output_attrs;
#if defined(RV1106_1103)
    rknn_tensor_mem* input_mems[1];
    rknn_tensor_mem* output_mems[9]; // Assuming max 9 outputs
    // rknn_dma_buf img_dma_buf; // Type 'rknn_dma_buf' might be specific, ensure defined if used
#endif
#if defined(ZERO_COPY) // Use Zero-Copy structures
    rknn_tensor_mem* input_mems[1]; // Array for input mem handles
    rknn_tensor_mem* output_mems[9]; // Array for output mem handles
    rknn_tensor_attr* input_native_attrs;
    rknn_tensor_attr* output_native_attrs;
#endif
    int model_channel;
    int model_width;
    int model_height;
    bool is_quant;
} yolo11_app_context_t;


#include "postprocess.h" // Include YOLO postprocess header


int init_yolo11(const char* model_path, yolo11_app_context_t* app_ctx);

int release_yolo11(yolo11_app_context_t* app_ctx);

// *** MODIFIED Signature: Removed img buffer, added scaling info ***
int inference_yolo11(yolo11_app_context_t* app_ctx,
                     object_detect_result_list* od_results,
                     float scale_w, // Scaling factor width used during preprocessing
                     float scale_h, // Scaling factor height used during preprocessing
                     int offset_x,  // X offset of the crop in original image
                     int offset_y); // Y offset of the crop in original image

#endif //_RKNN_DEMO_YOLO11_H_