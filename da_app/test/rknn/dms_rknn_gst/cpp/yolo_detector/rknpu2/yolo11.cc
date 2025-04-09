// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <math.h>

// #include "yolo11.h"
// #include "common.h"
// #include "file_utils.h"
// #include "image_utils.h"

// static void dump_tensor_attr(rknn_tensor_attr *attr)
// {
//     printf("");
//     // printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
//     //        "zp=%d, scale=%f\n",
//     //        attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3],
//     //        attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
//     //        get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
// }


// int init_yolo11(const char *model_path, yolo11_app_context_t *app_ctx)
// {
//     int ret;
//     int model_len = 0;
//     char *model_buf = NULL; // Use char* for model buffer
//     rknn_context ctx = 0;

//     // Ensure app_ctx is clean before starting
//     memset(app_ctx, 0, sizeof(yolo11_app_context_t));

//     // Load RKNN Model
//     printf("Loading YOLO model: %s\n", model_path);
//     model_len = read_data_from_file(model_path, &model_buf);
//     if (model_buf == NULL) {
//         printf("ERROR: load_model fail for YOLO!\n");
//         return -1;
//     }

//     // Initialize RKNN context
//     ret = rknn_init(&ctx, model_buf, model_len, 0, NULL);
//     free(model_buf); // Free the buffer right after init
//     model_buf = NULL;
//     if (ret < 0) {
//         printf("ERROR: rknn_init for YOLO model failed! ret=%d\n", ret);
//         return -1; // Cannot proceed
//     }

//     // +++ Set Core Mask for YOLO Model +++
//     ret = rknn_set_core_mask(ctx, RKNN_NPU_CORE_1); // Assign to Core 1
//     if (ret < 0) {
//         printf("ERROR: rknn_set_core_mask(yolo, CORE_1) failed! ret=%d\n", ret);
//         rknn_destroy(ctx); // Clean up the initialized context before returning
//         return -1; // Indicate failure
//     }
//     printf("INFO: YOLO Model assigned to NPU Core 1.\n");
//     // +++++++++++++++++++++++++++++++++++++

//     // --- Continue with querying model info ---
//     app_ctx->rknn_ctx = ctx; // Store the context handle in our struct

//     // Query I/O Number
//     rknn_input_output_num io_num;
//     ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
//     if (ret != RKNN_SUCC) {
//         printf("ERROR: rknn_query(Num) for YOLO failed! ret=%d\n", ret);
//         rknn_destroy(ctx); // Cleanup
//         return -1;
//     }
//     // printf("YOLO model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);
//     app_ctx->io_num = io_num;

//     // Query Input Attributes
//     // printf("YOLO input tensors:\n");
//     app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
//     if (!app_ctx->input_attrs) { printf("ERROR: malloc fail for yolo input attrs\n"); rknn_destroy(ctx); return -1; }
//     memset(app_ctx->input_attrs, 0, io_num.n_input * sizeof(rknn_tensor_attr));
//     for (uint32_t i = 0; i < io_num.n_input; i++) {
//         app_ctx->input_attrs[i].index = i;
//         ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(app_ctx->input_attrs[i]), sizeof(rknn_tensor_attr));
//         if (ret != RKNN_SUCC) {
//             printf("ERROR: rknn_query(Input %d) for YOLO failed! ret=%d\n", i, ret);
//             free(app_ctx->input_attrs); // Free allocated memory
//             rknn_destroy(ctx);
//             return -1;
//         }
//         dump_tensor_attr(&(app_ctx->input_attrs[i]));
//     }

//     // Query Output Attributes
//     // printf("YOLO output tensors:\n");
//     app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
//     if (!app_ctx->output_attrs) { printf("ERROR: malloc fail for yolo output attrs\n"); free(app_ctx->input_attrs); rknn_destroy(ctx); return -1; }
//     memset(app_ctx->output_attrs, 0, io_num.n_output * sizeof(rknn_tensor_attr));
//     for (uint32_t i = 0; i < io_num.n_output; i++) {
//         app_ctx->output_attrs[i].index = i;
//         ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(app_ctx->output_attrs[i]), sizeof(rknn_tensor_attr));
//         if (ret != RKNN_SUCC) {
//             printf("ERROR: rknn_query(Output %d) for YOLO failed! ret=%d\n", i, ret);
//             free(app_ctx->output_attrs); // Free allocated memory
//             free(app_ctx->input_attrs);
//             rknn_destroy(ctx);
//             return -1;
//         }
//         dump_tensor_attr(&(app_ctx->output_attrs[i]));
//     }

//     // Determine quantization status (check output type and qnt_type)
//     // Adjust this logic based on your specific YOLO model's quantization details
//     if (app_ctx->output_attrs[0].qnt_type != RKNN_TENSOR_QNT_NONE &&
//         (app_ctx->output_attrs[0].type == RKNN_TENSOR_INT8 || app_ctx->output_attrs[0].type == RKNN_TENSOR_UINT8)) {
//         app_ctx->is_quant = true;
//         printf("INFO: YOLO model appears to be quantized (output 0 type: %s, qnt: %s)\n",
//                get_type_string(app_ctx->output_attrs[0].type),
//                get_qnt_type_string(app_ctx->output_attrs[0].qnt_type));
//     } else {
//         app_ctx->is_quant = false;
//          printf("INFO: YOLO model appears to be FP32/FP16 (output 0 type: %s, qnt: %s)\n",
//                get_type_string(app_ctx->output_attrs[0].type),
//                get_qnt_type_string(app_ctx->output_attrs[0].qnt_type));
//     }

//     // Determine input dimensions based on format
//     if (app_ctx->input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
//         // printf("YOLO model is NCHW input fmt\n");
//         app_ctx->model_channel = app_ctx->input_attrs[0].dims[1];
//         app_ctx->model_height = app_ctx->input_attrs[0].dims[2];
//         app_ctx->model_width = app_ctx->input_attrs[0].dims[3];
//     } else { // Assume NHWC
//         // printf("YOLO model is NHWC input fmt\n");
//         app_ctx->model_height = app_ctx->input_attrs[0].dims[1];
//         app_ctx->model_width = app_ctx->input_attrs[0].dims[2];
//         app_ctx->model_channel = app_ctx->input_attrs[0].dims[3];
//     }
//     // printf("YOLO model input height=%d, width=%d, channel=%d\n",
//     //        app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);

//     return 0; // Success
// }

// int release_yolo11(yolo11_app_context_t *app_ctx) // <-- Renamed function and struct
// {
//     if (app_ctx->input_attrs != NULL)
//     {
//         free(app_ctx->input_attrs);
//         app_ctx->input_attrs = NULL;
//     }
//     if (app_ctx->output_attrs != NULL)
//     {
//         free(app_ctx->output_attrs);
//         app_ctx->output_attrs = NULL;
//     }
//     if (app_ctx->rknn_ctx != 0)
//     {
//         rknn_destroy(app_ctx->rknn_ctx);
//         app_ctx->rknn_ctx = 0;
//     }
//     return 0;
// }

// // int inference_yolo11(yolo11_app_context_t *app_ctx, image_buffer_t *img, object_detect_result_list *od_results) // <-- Renamed function and struct
// // {
// //     int ret;
// //     image_buffer_t dst_img;
// //     letterbox_t letter_box;
// //     rknn_input inputs[app_ctx->io_num.n_input];
// //     rknn_output outputs[app_ctx->io_num.n_output];
// //     const float nms_threshold = NMS_THRESH;      // 默认的NMS阈值
// //     const float box_conf_threshold = BOX_THRESH; // 默认的置信度阈值
// //     int bg_color = 114;

// //     if ((!app_ctx) || !(img) || (!od_results))
// //     {
// //         return -1;
// //     }

// //     memset(od_results, 0x00, sizeof(*od_results));
// //     memset(&letter_box, 0, sizeof(letterbox_t));
// //     memset(&dst_img, 0, sizeof(image_buffer_t));
// //     memset(inputs, 0, sizeof(inputs));
// //     memset(outputs, 0, sizeof(outputs));

// //     // Pre Process
// //     dst_img.width = app_ctx->model_width;
// //     dst_img.height = app_ctx->model_height;
// //     dst_img.format = IMAGE_FORMAT_RGB888;
// //     dst_img.size = get_image_size(&dst_img);
// //     dst_img.virt_addr = (unsigned char *)malloc(dst_img.size);
// //     if (dst_img.virt_addr == NULL)
// //     {
// //         // printf("malloc buffer size:%d fail!\n", dst_img.size);
// //         return -1;
// //     }

// //     // letterbox
// //     ret = convert_image_with_letterbox(img, &dst_img, &letter_box, bg_color);
// //     if (ret < 0)
// //     {
// //         // printf("convert_image_with_letterbox fail! ret=%d\n", ret);
// //         return -1;
// //     }

// //     // Set Input Data
// //     inputs[0].index = 0;
// //     inputs[0].type = RKNN_TENSOR_UINT8;
// //     inputs[0].fmt = RKNN_TENSOR_NHWC;
// //     inputs[0].size = app_ctx->model_width * app_ctx->model_height * app_ctx->model_channel;
// //     inputs[0].buf = dst_img.virt_addr;

// //     ret = rknn_inputs_set(app_ctx->rknn_ctx, app_ctx->io_num.n_input, inputs);
// //     if (ret < 0)
// //     {
// //         // printf("rknn_input_set fail! ret=%d\n", ret);
// //         return -1;
// //     }

// //     // Run
// //     printf("rknn_run\n");
// //     ret = rknn_run(app_ctx->rknn_ctx, nullptr);
// //     if (ret < 0)
// //     {
// //         // printf("rknn_run fail! ret=%d\n", ret);
// //         return -1;
// //     }

// //     // Get Output
// //     memset(outputs, 0, sizeof(outputs));
// //     for (int i = 0; i < app_ctx->io_num.n_output; i++)
// //     {
// //         outputs[i].index = i;
// //         outputs[i].want_float = (!app_ctx->is_quant);
// //     }
// //     ret = rknn_outputs_get(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs, NULL);
// //     if (ret < 0)
// //     {
// //         // printf("rknn_outputs_get fail! ret=%d\n", ret);
// //         goto out;
// //     }

// //     // Post Process
// //     post_process(app_ctx, outputs, &letter_box, box_conf_threshold, nms_threshold, od_results);

// //     // Remeber to release rknn output
// //     rknn_outputs_release(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs);

// // out:
// //     if (dst_img.virt_addr != NULL)
// //     {
// //         free(dst_img.virt_addr);
// //     }

// //     return ret;
// // }


// int inference_yolo11(yolo11_app_context_t *app_ctx,
//                      image_buffer_t *img, // Input is now preprocessed by RGA
//                      object_detect_result_list *od_results,
//                      letterbox_t* letter_box) // Receive letterbox info
// {
//     int ret;
//     // image_buffer_t dst_img; // REMOVED - No internal preprocessing buffer needed
//     // letterbox_t letter_box; // REMOVED - Use the passed-in one
//     rknn_input inputs[app_ctx->io_num.n_input];
//     rknn_output outputs[app_ctx->io_num.n_output];
//     const float nms_threshold = NMS_THRESH;
//     const float box_conf_threshold = BOX_THRESH;
//     // int bg_color = 114; // REMOVED - Not needed here anymore

//     if ((!app_ctx) || !(img) || (!od_results) || (!letter_box)) // Check letter_box too
//     {
//         printf("ERROR: Invalid arguments to inference_yolo11\n");
//         return -1;
//     }
//      // Ensure the input buffer is valid
//     if (!img->virt_addr) {
//         printf("ERROR: Input image buffer has NULL virt_addr in inference_yolo11\n");
//         return -1;
//     }

//     memset(od_results, 0x00, sizeof(*od_results));
//     // memset(&letter_box, 0, sizeof(letterbox_t)); // REMOVED
//     // memset(&dst_img, 0, sizeof(image_buffer_t)); // REMOVED
//     memset(inputs, 0, sizeof(inputs));
//     memset(outputs, 0, sizeof(outputs));

//     // Pre Process - REMOVED
//     /*
//     dst_img.width = app_ctx->model_width;
//     dst_img.height = app_ctx->model_height;
//     dst_img.format = IMAGE_FORMAT_RGB888;
//     dst_img.size = get_image_size(&dst_img);
//     dst_img.virt_addr = (unsigned char *)malloc(dst_img.size);
//     if (dst_img.virt_addr == NULL) { //... return -1 ... }
//     ret = convert_image_with_letterbox(img, &dst_img, &letter_box, bg_color);
//     if (ret < 0) { //... return -1 ... }
//     */

//     // Set Input Data - Use the 'img' buffer directly
//     inputs[0].index = 0;
//     inputs[0].type = RKNN_TENSOR_UINT8; // Assuming model input is UINT8
//     inputs[0].fmt = RKNN_TENSOR_NHWC;   // Assuming model input is NHWC
//     inputs[0].size = img->size;         // Use size from the input buffer
//     inputs[0].buf = img->virt_addr;     // Use virt_addr from the input buffer

//     ret = rknn_inputs_set(app_ctx->rknn_ctx, app_ctx->io_num.n_input, inputs);
//     if (ret < 0)
//     {
//         printf("ERROR: rknn_inputs_set fail! ret=%d\n", ret);
//         // goto out; // No buffer to free here anymore in the standard path
//         return -1;
//     }

//     // Run
//     // printf("rknn_run (YOLO)\n"); // Keep minimal logging if desired
//     ret = rknn_run(app_ctx->rknn_ctx, nullptr);
//     if (ret < 0)
//     {
//         printf("ERROR: rknn_run (YOLO) fail! ret=%d\n", ret);
//         return -1; // No buffer to free here anymore
//     }

//     // Get Output
//     memset(outputs, 0, sizeof(outputs));
//     for (int i = 0; i < app_ctx->io_num.n_output; i++)
//     {
//         outputs[i].index = i;
//         outputs[i].want_float = (!app_ctx->is_quant); // Request float if model isn't quant
//     }
//     ret = rknn_outputs_get(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs, NULL);
//     if (ret < 0)
//     {
//         printf("ERROR: rknn_outputs_get (YOLO) fail! ret=%d\n", ret);
//         // Still need to release outputs if get fails partially? API is unclear. Assume not.
//         return -1; // No buffer to free here anymore
//     }

//     // Post Process - Pass the letterbox info received from main thread
//     post_process(app_ctx, outputs, letter_box, box_conf_threshold, nms_threshold, od_results);

//     // Remember to release rknn output
//     rknn_outputs_release(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs);

// // out: // Label removed as internal buffer allocation is gone
// //     if (dst_img.virt_addr != NULL) // REMOVED
// //     {
// //         free(dst_img.virt_addr);
// //     }

//     return ret; // Return 0 on success, or error code
// }



// File: yolo11.cc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "yolo11.h"
#include "common.h"
#include "file_utils.h"
#include "image_utils.h"

static void dump_tensor_attr(rknn_tensor_attr *attr)
{
    // Simplified version - uncomment more details if needed
    printf("  Idx:%d Name:%s Dims:[%d,%d,%d,%d] Fmt:%s Type:%s Qnt:%s ZP:%d Scale:%.3f Stride:%d SzStride:%d\n",
           attr->index, attr->name,
           attr->n_dims > 0 ? attr->dims[0] : -1,
           attr->n_dims > 1 ? attr->dims[1] : -1,
           attr->n_dims > 2 ? attr->dims[2] : -1,
           attr->n_dims > 3 ? attr->dims[3] : -1,
           get_format_string(attr->fmt), get_type_string(attr->type),
           get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale,
           attr->w_stride, attr->size_with_stride);
}


int init_yolo11(const char *model_path, yolo11_app_context_t *app_ctx)
{
    int ret;
    int model_len = 0;
    char *model_buf = NULL;
    rknn_context ctx = 0;

    memset(app_ctx, 0, sizeof(yolo11_app_context_t));

    // Load RKNN Model
    printf("Loading YOLO model: %s\n", model_path);
    model_len = read_data_from_file(model_path, &model_buf);
    if (model_buf == NULL) { /* error */ return -1; }
    ret = rknn_init(&ctx, model_buf, model_len, 0, NULL);
    free(model_buf); model_buf = NULL;
    if (ret < 0) { /* error */ return -1; }
    ret = rknn_set_core_mask(ctx, RKNN_NPU_CORE_1);
    if(ret < 0) { printf("WARN: rknn_set_core_mask(yolo, CORE_1) failed! ret=%d\n", ret); }
    else { printf("INFO: YOLO Model assigned to NPU Core 1.\n"); }
    app_ctx->rknn_ctx = ctx;

    // Query I/O Number
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &app_ctx->io_num, sizeof(app_ctx->io_num));
    if (ret != RKNN_SUCC) { /* error */ goto cleanup; }
    printf("YOLO Model: inputs=%d, outputs=%d\n", app_ctx->io_num.n_input, app_ctx->io_num.n_output);

    // --- FIX: Check if io_num matches array sizes in struct ---
    if (app_ctx->io_num.n_input > 1 || app_ctx->io_num.n_output > 9) {
        printf("ERROR: Model I/O numbers (%u inputs, %u outputs) exceed struct array sizes (1 input, 9 outputs).\n",
               app_ctx->io_num.n_input, app_ctx->io_num.n_output);
        ret = RKNN_ERR_MODEL_INVALID; // Or a more specific error
        goto cleanup;
    }
    // --- END FIX ---

    // Query Input Attributes (Native for Zero-Copy)
    printf("YOLO Input Tensors (Native):\n");
    app_ctx->input_attrs = (rknn_tensor_attr *)malloc(app_ctx->io_num.n_input * sizeof(rknn_tensor_attr));
    if (!app_ctx->input_attrs) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup; }
    memset(app_ctx->input_attrs, 0, app_ctx->io_num.n_input * sizeof(rknn_tensor_attr));
    for (uint32_t i = 0; i < app_ctx->io_num.n_input; i++) {
        app_ctx->input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_NATIVE_INPUT_ATTR, &(app_ctx->input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) { /* error */ goto cleanup; }
        dump_tensor_attr(&(app_ctx->input_attrs[i]));
    }

    // Query Output Attributes (Native NHWC for Zero-Copy)
    printf("YOLO Output Tensors (Native NHWC):\n");
    app_ctx->output_attrs = (rknn_tensor_attr *)malloc(app_ctx->io_num.n_output * sizeof(rknn_tensor_attr));
    if (!app_ctx->output_attrs) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup; }
    memset(app_ctx->output_attrs, 0, app_ctx->io_num.n_output * sizeof(rknn_tensor_attr));
    for (uint32_t i = 0; i < app_ctx->io_num.n_output; i++) {
        app_ctx->output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &(app_ctx->output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) { /* error */ goto cleanup; }
        dump_tensor_attr(&(app_ctx->output_attrs[i]));
    }

    // Allocate and Bind Zero-Copy Input Buffers
    if (app_ctx->io_num.n_input > 0) {
        // Set format/type for the input buffer we'll fill using RGA
        app_ctx->input_attrs[0].type = RKNN_TENSOR_UINT8;
        app_ctx->input_attrs[0].fmt = RKNN_TENSOR_NHWC;
        // app_ctx->input_fmt_detect = RKNN_TENSOR_NHWC; // <-- DELETE THIS LINE

        for(uint32_t i = 0; i < app_ctx->io_num.n_input; ++i) {
            // Allocate the individual rknn_tensor_mem struct
            app_ctx->input_mems[i] = rknn_create_mem(ctx, app_ctx->input_attrs[i].size_with_stride);
            if(!app_ctx->input_mems[i]) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup; }
            // Bind it
            ret = rknn_set_io_mem(ctx, app_ctx->input_mems[i], &app_ctx->input_attrs[i]);
            if (ret < 0) { /* error */ goto cleanup; }
        }
    }

    // Allocate and Bind Zero-Copy Output Buffers
    if (app_ctx->io_num.n_output > 0) {
        // --- FIX: Don't malloc the array, just the elements ---
        // app_ctx->output_mems = (rknn_tensor_mem**)malloc(sizeof(rknn_tensor_mem*) * app_ctx->io_num.n_output); // REMOVED
        for(uint32_t i = 0; i < app_ctx->io_num.n_output; ++i) {
             // Allocate the individual rknn_tensor_mem struct
            app_ctx->output_mems[i] = rknn_create_mem(ctx, app_ctx->output_attrs[i].size_with_stride);
            if(!app_ctx->output_mems[i]) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup; }
            // Bind it
            ret = rknn_set_io_mem(ctx, app_ctx->output_mems[i], &app_ctx->output_attrs[i]);
            if (ret < 0) { /* error */ goto cleanup; }
        }
        // --- END FIX ---
    }

    // Determine quantization status (check output type and qnt_type)
    if (app_ctx->output_attrs[0].qnt_type != RKNN_TENSOR_QNT_NONE &&
        (app_ctx->output_attrs[0].type == RKNN_TENSOR_INT8 || app_ctx->output_attrs[0].type == RKNN_TENSOR_UINT8)) {
        app_ctx->is_quant = true;
        printf("INFO: YOLO model appears to be quantized (native output 0 type: %s, qnt: %s)\n", get_type_string(app_ctx->output_attrs[0].type), get_qnt_type_string(app_ctx->output_attrs[0].qnt_type));
    } else {
        app_ctx->is_quant = false;
         printf("INFO: YOLO model appears to be FP32/FP16 (native output 0 type: %s, qnt: %s)\n", get_type_string(app_ctx->output_attrs[0].type), get_qnt_type_string(app_ctx->output_attrs[0].qnt_type));
    }

    // Determine input dimensions based on format (use native attributes)
    if (app_ctx->input_attrs[0].fmt == RKNN_TENSOR_NCHW) { /* NCHW case */ } else { /* NHWC case */ } // Logic remains same
    app_ctx->model_height = app_ctx->input_attrs[0].dims[1]; app_ctx->model_width = app_ctx->input_attrs[0].dims[2]; app_ctx->model_channel = app_ctx->input_attrs[0].dims[3];
    printf("YOLO model input (native/target): H=%d, W=%d, C=%d\n", app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);

    return 0; // Success

cleanup:
    printf("ERROR: init_yolo11 failed during setup, cleaning up. ret=%d\n", ret);
    release_yolo11(app_ctx);
    return ret > 0 ? -ret : ret;
}


int release_yolo11(yolo11_app_context_t *app_ctx) {
    // Release allocated input tensor_mem buffers
    // --- FIX: Don't free the array pointer, just destroy elements ---
    // if(app_ctx->input_mems) { // Check if the pointer itself is valid (it won't be NULL here)
        for(uint32_t i=0; i<app_ctx->io_num.n_input; ++i) {
            // Check if the element pointer is valid before destroying
            if(app_ctx->input_mems[i]) {
                rknn_destroy_mem(app_ctx->rknn_ctx, app_ctx->input_mems[i]);
                app_ctx->input_mems[i] = NULL; // Set to NULL after destroying
            }
        }
        // free(app_ctx->input_mems); // REMOVED - Do not free the fixed-size array member
        // app_ctx->input_mems = NULL; // REMOVED - Cannot assign NULL to array
    // }
    // --- END FIX ---

    // Release allocated output tensor_mem buffers
    // --- FIX: Don't free the array pointer, just destroy elements ---
    // if(app_ctx->output_mems) {
        for(uint32_t i=0; i<app_ctx->io_num.n_output; ++i) {
             // Check if the element pointer is valid before destroying
            if(app_ctx->output_mems[i]) {
                rknn_destroy_mem(app_ctx->rknn_ctx, app_ctx->output_mems[i]);
                app_ctx->output_mems[i] = NULL; // Set to NULL after destroying
            }
        }
        // free(app_ctx->output_mems); // REMOVED
        // app_ctx->output_mems = NULL; // REMOVED
    // }
    // --- END FIX ---

    // Release attributes
    if (app_ctx->input_attrs != NULL) { free(app_ctx->input_attrs); app_ctx->input_attrs = NULL; }
    if (app_ctx->output_attrs != NULL) { free(app_ctx->output_attrs); app_ctx->output_attrs = NULL; }

    // Destroy context
    if (app_ctx->rknn_ctx != 0) { rknn_destroy(app_ctx->rknn_ctx); app_ctx->rknn_ctx = 0; }
    return 0;
}


int inference_yolo11(yolo11_app_context_t *app_ctx, object_detect_result_list *od_results) {
    int ret;
    rknn_output outputs[app_ctx->io_num.n_output];
    const float nms_threshold = NMS_THRESH;
    const float box_conf_threshold = BOX_THRESH;
    
    if ((!app_ctx) || (!od_results)) {
        printf("ERROR: Invalid arguments to inference_yolo11\n");
        return -1;
    }
    
    if (!app_ctx->input_mems || !app_ctx->input_mems[0] || !app_ctx->output_mems) {
        printf("ERROR: YOLO input/output mems not initialized in context.\n");
        return -1;
    }
    
    memset(od_results, 0x00, sizeof(*od_results));
    memset(outputs, 0, sizeof(outputs));
    
    #ifdef RKNN_MEMORY_SYNC_TO_DEVICE
    ret = rknn_mem_sync(app_ctx->rknn_ctx, app_ctx->input_mems[0], RKNN_MEMORY_SYNC_TO_DEVICE);
    if (ret < 0) {
        printf("ERROR: rknn_mem_sync (YOLO input) failed! ret=%d\n", ret);
        return ret;
    }
    #endif
    
    ret = rknn_run(app_ctx->rknn_ctx, nullptr);
    if (ret < 0) {
        printf("ERROR: rknn_run (YOLO) fail! ret=%d\n", ret);
        return ret;
    }
    
    #ifdef RKNN_MEMORY_SYNC_FROM_DEVICE
    for (uint32_t i = 0; i < app_ctx->io_num.n_output; ++i) {
        if (app_ctx->output_mems[i]) {
            ret = rknn_mem_sync(app_ctx->rknn_ctx, app_ctx->output_mems[i], RKNN_MEMORY_SYNC_FROM_DEVICE);
            if (ret < 0) {
                printf("ERROR: rknn_mem_sync (YOLO output %u) failed! ret=%d\n", i, ret);
                return ret;
            }
        } else {
            printf("ERROR: YOLO output_mems[%u] is NULL!\n", i);
            return -1;
        }
    }
    #endif
    
    post_process(app_ctx, app_ctx->output_mems, nullptr, box_conf_threshold, nms_threshold, od_results);
    return 0;
}