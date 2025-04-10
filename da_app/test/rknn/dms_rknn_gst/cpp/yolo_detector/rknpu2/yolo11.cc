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

// int inference_yolo11(yolo11_app_context_t *app_ctx, image_buffer_t *img, object_detect_result_list *od_results) // <-- Renamed function and struct
// {
//     int ret;
//     image_buffer_t dst_img;
//     letterbox_t letter_box;
//     rknn_input inputs[app_ctx->io_num.n_input];
//     rknn_output outputs[app_ctx->io_num.n_output];
//     const float nms_threshold = NMS_THRESH;      // 默认的NMS阈值
//     const float box_conf_threshold = BOX_THRESH; // 默认的置信度阈值
//     int bg_color = 114;

//     if ((!app_ctx) || !(img) || (!od_results))
//     {
//         return -1;
//     }

//     memset(od_results, 0x00, sizeof(*od_results));
//     memset(&letter_box, 0, sizeof(letterbox_t));
//     memset(&dst_img, 0, sizeof(image_buffer_t));
//     memset(inputs, 0, sizeof(inputs));
//     memset(outputs, 0, sizeof(outputs));

//     // Pre Process
//     dst_img.width = app_ctx->model_width;
//     dst_img.height = app_ctx->model_height;
//     dst_img.format = IMAGE_FORMAT_RGB888;
//     dst_img.size = get_image_size(&dst_img);
//     dst_img.virt_addr = (unsigned char *)malloc(dst_img.size);
//     if (dst_img.virt_addr == NULL)
//     {
//         // printf("malloc buffer size:%d fail!\n", dst_img.size);
//         return -1;
//     }

//     // letterbox
//     ret = convert_image_with_letterbox(img, &dst_img, &letter_box, bg_color);
//     if (ret < 0)
//     {
//         // printf("convert_image_with_letterbox fail! ret=%d\n", ret);
//         return -1;
//     }

//     // Set Input Data
//     inputs[0].index = 0;
//     inputs[0].type = RKNN_TENSOR_UINT8;
//     inputs[0].fmt = RKNN_TENSOR_NHWC;
//     inputs[0].size = app_ctx->model_width * app_ctx->model_height * app_ctx->model_channel;
//     inputs[0].buf = dst_img.virt_addr;

//     ret = rknn_inputs_set(app_ctx->rknn_ctx, app_ctx->io_num.n_input, inputs);
//     if (ret < 0)
//     {
//         // printf("rknn_input_set fail! ret=%d\n", ret);
//         return -1;
//     }

//     // Run
//     printf("rknn_run\n");
//     ret = rknn_run(app_ctx->rknn_ctx, nullptr);
//     if (ret < 0)
//     {
//         // printf("rknn_run fail! ret=%d\n", ret);
//         return -1;
//     }

//     // Get Output
//     memset(outputs, 0, sizeof(outputs));
//     for (int i = 0; i < app_ctx->io_num.n_output; i++)
//     {
//         outputs[i].index = i;
//         outputs[i].want_float = (!app_ctx->is_quant);
//     }
//     ret = rknn_outputs_get(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs, NULL);
//     if (ret < 0)
//     {
//         // printf("rknn_outputs_get fail! ret=%d\n", ret);
//         goto out;
//     }

//     // Post Process
//     post_process(app_ctx, outputs, &letter_box, box_conf_threshold, nms_threshold, od_results);

//     // Remeber to release rknn output
//     rknn_outputs_release(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs);

// out:
//     if (dst_img.virt_addr != NULL)
//     {
//         free(dst_img.virt_addr);
//     }

//     return ret;
// }


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
    printf("");
    // printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
    //        "zp=%d, scale=%f\n",
    //        attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3],
    //        attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
    //        get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}



int init_yolo11(const char *model_path, yolo11_app_context_t *app_ctx)
{
    int ret;
    int model_len = 0;
    char *model_buf = NULL;
    rknn_context ctx = 0;

    memset(app_ctx, 0, sizeof(yolo11_app_context_t));

    printf("Loading YOLO model: %s\n", model_path);
    model_len = read_data_from_file(model_path, &model_buf);
    if (model_buf == NULL) { printf("ERROR: load_model fail for YOLO!\n"); return -1; }
    ret = rknn_init(&ctx, model_buf, model_len, 0, NULL);
    free(model_buf); model_buf = NULL;
    if (ret < 0) { printf("ERROR: rknn_init for YOLO model failed! ret=%d\n", ret); return -1; }
    ret = rknn_set_core_mask(ctx, RKNN_NPU_CORE_1);
    if(ret < 0) { printf("WARN: rknn_set_core_mask(yolo, CORE_1) failed! ret=%d\n", ret); }
    else { printf("INFO: YOLO Model assigned to NPU Core 1.\n"); }
    app_ctx->rknn_ctx = ctx;

    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &app_ctx->io_num, sizeof(app_ctx->io_num));
    if (ret != RKNN_SUCC) { printf("ERROR: rknn_query(Num) failed! ret=%d\n", ret); goto error_cleanup; }

    // Allocate and query GENERAL input attributes
    app_ctx->input_attrs = (rknn_tensor_attr *)malloc(app_ctx->io_num.n_input * sizeof(rknn_tensor_attr));
    if (!app_ctx->input_attrs) { printf("ERROR: malloc fail input_attrs\n"); ret = RKNN_ERR_MALLOC_FAIL; goto error_cleanup; }
    memset(app_ctx->input_attrs, 0, app_ctx->io_num.n_input * sizeof(rknn_tensor_attr));
    for (uint32_t i = 0; i < app_ctx->io_num.n_input; i++) {
        app_ctx->input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(app_ctx->input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) { printf("ERROR: rknn_query(Input %d) failed! ret=%d\n", i, ret); goto error_cleanup; }
        dump_tensor_attr(&(app_ctx->input_attrs[i])); // Dump general attributes
    }

    // Allocate and query GENERAL output attributes
    app_ctx->output_attrs = (rknn_tensor_attr *)malloc(app_ctx->io_num.n_output * sizeof(rknn_tensor_attr));
    if (!app_ctx->output_attrs) { printf("ERROR: malloc fail output_attrs\n"); ret = RKNN_ERR_MALLOC_FAIL; goto error_cleanup; }
    memset(app_ctx->output_attrs, 0, app_ctx->io_num.n_output * sizeof(rknn_tensor_attr));
    for (uint32_t i = 0; i < app_ctx->io_num.n_output; i++) {
        app_ctx->output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(app_ctx->output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) { printf("ERROR: rknn_query(Output %d) failed! ret=%d\n", i, ret); goto error_cleanup; }
        dump_tensor_attr(&(app_ctx->output_attrs[i])); // Dump general attributes
    }

    // Determine quantization status using general attributes
    app_ctx->is_quant = (app_ctx->output_attrs[0].qnt_type != RKNN_TENSOR_QNT_NONE);
    printf("INFO: YOLO model quantization: %s\n", app_ctx->is_quant ? "YES" : "NO");

    // --- Conditional Zero-Copy Memory Allocation and Setup ---
#if defined(ZERO_COPY) || defined(RV1106_1103)
    printf("INFO: Initializing YOLO with Zero-Copy/RV1106 memory.\n");
    memset(app_ctx->input_mems, 0, sizeof(app_ctx->input_mems));
    memset(app_ctx->output_mems, 0, sizeof(app_ctx->output_mems));

    if (app_ctx->io_num.n_input > sizeof(app_ctx->input_mems) / sizeof(app_ctx->input_mems[0])) {
        printf("ERROR: Model has more inputs (%u) than allocated input_mems array size (%zu)!\n",
               app_ctx->io_num.n_input, sizeof(app_ctx->input_mems) / sizeof(app_ctx->input_mems[0]));
        ret = RKNN_ERR_PARAM_INVALID; goto error_cleanup;
    }
    // Query NATIVE input attributes and create/set input memory
    for (uint32_t i = 0; i < app_ctx->io_num.n_input; i++) {
        // Query NATIVE attributes into the input_attrs struct
        ret = rknn_query(ctx, RKNN_QUERY_NATIVE_INPUT_ATTR, &(app_ctx->input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) { printf("ERROR: rknn_query(Native Input %d) failed! ret=%d\n", i, ret); goto error_cleanup; }
        printf("INFO: YOLO Native Input Attr %d:\n", i);
        dump_tensor_attr(&(app_ctx->input_attrs[i])); // Dump native attributes

        // *** Set model dims AFTER native query, using potentially updated dims ***
        if (i == 0) { // Assuming we use dims from the first input
             if (app_ctx->input_attrs[0].fmt == RKNN_TENSOR_NCHW) { // Check format again after native query
                 app_ctx->model_channel = app_ctx->input_attrs[0].dims[1];
                 app_ctx->model_height = app_ctx->input_attrs[0].dims[2];
                 app_ctx->model_width = app_ctx->input_attrs[0].dims[3];
             } else { // Assume NHWC
                 app_ctx->model_height = app_ctx->input_attrs[0].dims[1];
                 app_ctx->model_width = app_ctx->input_attrs[0].dims[2];
                 app_ctx->model_channel = app_ctx->input_attrs[0].dims[3];
             }
              printf("INFO: YOLO model input (using native attrs): H=%d, W=%d, C=%d\n",
                     app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);
        }

        // Set the type/fmt we will provide data in (for rknn_set_io_mem)
        app_ctx->input_attrs[i].type = RKNN_TENSOR_UINT8;
        app_ctx->input_attrs[i].fmt = RKNN_TENSOR_NHWC;

        // Create memory based on native size
        app_ctx->input_mems[i] = rknn_create_mem(ctx, app_ctx->input_attrs[i].size_with_stride);
        if (!app_ctx->input_mems[i]) { printf("ERROR: rknn_create_mem (Input %d) failed\n", i); ret = RKNN_ERR_MALLOC_FAIL; goto error_cleanup; }

        // Set the I/O memory
        ret = rknn_set_io_mem(ctx, app_ctx->input_mems[i], &app_ctx->input_attrs[i]);
        if (ret < 0) { printf("ERROR: rknn_set_io_mem (Input %d) failed! ret=%d\n", i, ret); goto error_cleanup; }
    }

     if (app_ctx->io_num.n_output > sizeof(app_ctx->output_mems) / sizeof(app_ctx->output_mems[0])) {
        printf("ERROR: Model has more outputs (%u) than allocated output_mems array size (%zu)!\n",
               app_ctx->io_num.n_output, sizeof(app_ctx->output_mems) / sizeof(app_ctx->output_mems[0]));
        ret = RKNN_ERR_PARAM_INVALID; goto error_cleanup;
     }
     // Query NATIVE output attributes and create/set output memory
    for (uint32_t i = 0; i < app_ctx->io_num.n_output; i++) {
        // Query NATIVE attributes into output_attrs struct
        ret = rknn_query(ctx, RKNN_QUERY_NATIVE_OUTPUT_ATTR, &(app_ctx->output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) { printf("ERROR: rknn_query(Native Output %d) failed! ret=%d\n", i, ret); goto error_cleanup; }
        printf("INFO: YOLO Native Output Attr %d:\n", i);
        dump_tensor_attr(&(app_ctx->output_attrs[i])); // Dump native attributes

        // Create memory based on native size
        app_ctx->output_mems[i] = rknn_create_mem(ctx, app_ctx->output_attrs[i].size_with_stride);
         if (!app_ctx->output_mems[i]) { printf("ERROR: rknn_create_mem (Output %d) failed\n", i); ret = RKNN_ERR_MALLOC_FAIL; goto error_cleanup; }

        // Set the I/O memory
        ret = rknn_set_io_mem(ctx, app_ctx->output_mems[i], &app_ctx->output_attrs[i]);
        if (ret < 0) { printf("ERROR: rknn_set_io_mem (Output %d) failed! ret=%d\n", i, ret); goto error_cleanup; }
    }
#else // Not using Zero-Copy / RV1106
    // Determine model dimensions from general attributes if not using zero-copy
    if (app_ctx->input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
        app_ctx->model_channel = app_ctx->input_attrs[0].dims[1];
        app_ctx->model_height = app_ctx->input_attrs[0].dims[2];
        app_ctx->model_width = app_ctx->input_attrs[0].dims[3];
    } else { // Assume NHWC
        app_ctx->model_height = app_ctx->input_attrs[0].dims[1];
        app_ctx->model_width = app_ctx->input_attrs[0].dims[2];
        app_ctx->model_channel = app_ctx->input_attrs[0].dims[3];
    }
     printf("INFO: YOLO model input (using general attrs): H=%d, W=%d, C=%d\n",
            app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);
#endif
    // --- End Conditional Zero-Copy Setup ---

    // Final check if model dimensions were set
    if (app_ctx->model_width <= 0 || app_ctx->model_height <= 0) {
        printf("ERROR: Failed to determine valid model input dimensions (W=%d, H=%d)\n", app_ctx->model_width, app_ctx->model_height);
        ret = RKNN_ERR_MODEL_INVALID; // Or appropriate error
        goto error_cleanup;
    }

    printf("INFO: init_yolo11 successful.\n");
    return 0; // Success

error_cleanup:
     // ... (Error cleanup code remains the same as previous version) ...
     printf("ERROR: init_yolo11 failed with ret=%d\n", ret);
     if (app_ctx->input_attrs) free(app_ctx->input_attrs);
     if (app_ctx->output_attrs) free(app_ctx->output_attrs);
 #if defined(ZERO_COPY) || defined(RV1106_1103)
     if (app_ctx->input_mems) { uint32_t input_array_size = sizeof(app_ctx->input_mems) / sizeof(app_ctx->input_mems[0]); for (uint32_t i = 0; i < std::min(app_ctx->io_num.n_input, input_array_size); ++i) { if (app_ctx->input_mems[i]) rknn_destroy_mem(ctx, app_ctx->input_mems[i]); } }
      if (app_ctx->output_mems) { uint32_t output_array_size = sizeof(app_ctx->output_mems) / sizeof(app_ctx->output_mems[0]); for (uint32_t i = 0; i < std::min(app_ctx->io_num.n_output, output_array_size); ++i) { if (app_ctx->output_mems[i]) rknn_destroy_mem(ctx, app_ctx->output_mems[i]); } }
 #endif
     if (ctx != 0) rknn_destroy(ctx);
     memset(app_ctx, 0, sizeof(yolo11_app_context_t));
     return ret > 0 ? -ret : ret;
}

// *** CORRECTED release_yolo11 ***
int release_yolo11(yolo11_app_context_t *app_ctx)
{
    // Free input/output attributes first
    if (app_ctx->input_attrs != NULL) { free(app_ctx->input_attrs); app_ctx->input_attrs = NULL; }
    if (app_ctx->output_attrs != NULL) { free(app_ctx->output_attrs); app_ctx->output_attrs = NULL; }

    // Conditionally free zero-copy memory buffers
#if defined(ZERO_COPY) || defined(RV1106_1103)
    // We only need to destroy the buffers pointed to by the array elements
    // *** Check array bounds before accessing ***
    uint32_t input_array_size = sizeof(app_ctx->input_mems) / sizeof(app_ctx->input_mems[0]);
    for (uint32_t i = 0; i < std::min(app_ctx->io_num.n_input, input_array_size); ++i) {
        if (app_ctx->input_mems[i] != NULL) { // Check individual buffer pointer
            rknn_destroy_mem(app_ctx->rknn_ctx, app_ctx->input_mems[i]);
            app_ctx->input_mems[i] = NULL; // Clear the pointer in the array
        }
    }
    // *** DO NOT free(app_ctx->input_mems); ***

    uint32_t output_array_size = sizeof(app_ctx->output_mems) / sizeof(app_ctx->output_mems[0]);
    for (uint32_t i = 0; i < std::min(app_ctx->io_num.n_output, output_array_size); ++i) {
         if (app_ctx->output_mems[i] != NULL) { // Check individual buffer pointer
             rknn_destroy_mem(app_ctx->rknn_ctx, app_ctx->output_mems[i]);
             app_ctx->output_mems[i] = NULL; // Clear the pointer in the array
         }
    }
    // *** DO NOT free(app_ctx->output_mems); ***

    // Free native attrs if they were allocated separately
    // ...
#endif

    // Destroy context last
    if (app_ctx->rknn_ctx != 0) { rknn_destroy(app_ctx->rknn_ctx); app_ctx->rknn_ctx = 0; }
    return 0;
}


int inference_yolo11(yolo11_app_context_t *app_ctx,
                     object_detect_result_list *od_results,
                     float scale_w, // Scaling factor width used during preprocessing
                     float scale_h, // Scaling factor height used during preprocessing
                     int offset_x,  // X offset of the crop in original image
                     int offset_y)  // Y offset of the crop in original image
{
    int ret;
    rknn_output outputs[app_ctx->io_num.n_output]; // Keep outputs struct array for non-ZC case
    const float nms_threshold = NMS_THRESH;
    const float box_conf_threshold = BOX_THRESH;

    // Basic validation
    if ((!app_ctx) || (!od_results) || !app_ctx->rknn_ctx) {
        printf("ERROR: Invalid arguments to inference_yolo11\n");
        return -1;
    }

    bool use_zero_copy_input = false;
    bool use_zero_copy_output = false;

    // Determine if zero-copy is enabled based on defines AND if mems are allocated
#if defined(ZERO_COPY) || defined(RV1106_1103)
    use_zero_copy_input = (app_ctx->input_mems != nullptr && app_ctx->input_mems[0] != nullptr);
    use_zero_copy_output = (app_ctx->output_mems != nullptr); // Check if the array pointer exists

    if (use_zero_copy_input && !app_ctx->input_mems[0]->virt_addr) {
         printf("ERROR: YOLO input memory handle exists but virt_addr is NULL (zero-copy).\n");
         return -1;
    }
    // We also need to check output mems pointers later before using them
#endif

    memset(od_results, 0x00, sizeof(*od_results));
    memset(outputs, 0, sizeof(outputs)); // Zero outputs struct array

    // Sync input memory before run (only if using zero-copy input)
    if (use_zero_copy_input) {
#ifdef RKNN_MEMORY_SYNC_TO_DEVICE
        ret = rknn_mem_sync(app_ctx->rknn_ctx, app_ctx->input_mems[0], RKNN_MEMORY_SYNC_TO_DEVICE);
        if (ret < 0) {
            printf("ERROR: rknn_mem_sync (yolo input) failed! ret=%d\n", ret);
            return ret;
        }
#else
       // printf("WARN: RKNN_MEMORY_SYNC_TO_DEVICE not defined, skipping sync for YOLO input.\n");
#endif
    } else {
        // If NOT using zero-copy input, the caller (main.cc) should have prepared
        // a CPU buffer and we would need to use rknn_inputs_set here.
        // Since the main.cc is now modified for RGA->NPU buffer, this path
        // currently assumes zero-copy input is enabled and prepared.
        // If you need a non-zero-copy fallback, main.cc would need modification too.
        printf("ERROR: inference_yolo11 called without zero-copy input prepared. This path is not supported by current main.cc logic.\n");
        return -1;
    }

    // Run
    ret = rknn_run(app_ctx->rknn_ctx, nullptr);
    if (ret < 0) {
        printf("ERROR: rknn_run (YOLO) fail! ret=%d\n", ret);
        return -1;
    }

    // Prepare to get outputs / Sync outputs
    if (use_zero_copy_output) {
        // For zero-copy output, we *might* still need to call get to ensure completion.
        // Let's try calling it but mark want_float=0.
        for (int i = 0; i < app_ctx->io_num.n_output; i++) {
            outputs[i].index = i;
            outputs[i].want_float = 0; // Don't need float copy
        }
        ret = rknn_outputs_get(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs, NULL);
        if (ret < 0) {
            printf("ERROR: rknn_outputs_get (YOLO, zero-copy sync?) fail! ret=%d\n", ret);
            return -1;
        }

        // Sync output memory before CPU access
#ifdef RKNN_MEMORY_SYNC_FROM_DEVICE
        for (int i = 0; i < app_ctx->io_num.n_output; ++i) {
            // *** CRITICAL CHECK: Ensure output_mems[i] is valid ***
            if (app_ctx->output_mems[i] != nullptr) {
                ret = rknn_mem_sync(app_ctx->rknn_ctx, app_ctx->output_mems[i], RKNN_MEMORY_SYNC_FROM_DEVICE);
                if (ret < 0) {
                    printf("ERROR: rknn_mem_sync (yolo output %d) failed! ret=%d\n", i, ret);
                    return ret;
                }
            } else {
                printf("ERROR: output_mems[%d] is NULL during sync (zero-copy path)!\n", i);
                return -1; // Cannot proceed if a mem handle is missing
            }
        }
#else
        // printf("WARN: RKNN_MEMORY_SYNC_FROM_DEVICE not defined, skipping sync for YOLO output.\n");
#endif
    } else { // Not using zero-copy output
        for (int i = 0; i < app_ctx->io_num.n_output; i++) {
            outputs[i].index = i;
            outputs[i].want_float = (!app_ctx->is_quant);
        }
        ret = rknn_outputs_get(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs, NULL);
        if (ret < 0) {
            printf("ERROR: rknn_outputs_get (YOLO, non-zero-copy) fail! ret=%d\n", ret);
            return -1;
        }
    }

    // Post Process
    void* output_buffers_ptr = nullptr;
    if (use_zero_copy_output) {
        // *** CRITICAL CHECK before passing to post_process ***
        if (!app_ctx->output_mems[0] || !app_ctx->output_mems[0]->virt_addr) {
             printf("ERROR: app_ctx->output_mems is NULL or invalid before post_process!\n");
             // Release outputs obtained from `get` if any (only relevant if get was called in ZC path)
             // rknn_outputs_release(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs); // Unnecessary if get doesn't allocate
             return -1;
        }
        output_buffers_ptr = app_ctx->output_mems; // Pass the array of rknn_tensor_mem pointers
    } else {
         // Pass the array of rknn_output structs
        output_buffers_ptr = outputs;
         if (!outputs[0].buf) { // Check the buffer pointer from rknn_outputs_get
             printf("ERROR: outputs[0].buf is NULL before post_process!\n");
             rknn_outputs_release(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs); // Release potentially allocated buffers
             return -1;
         }
    }

    // Call post_process with the correct buffer pointer type and scaling info
    post_process(app_ctx, output_buffers_ptr,
                 scale_w, scale_h, offset_x, offset_y,
                 box_conf_threshold, nms_threshold, od_results);

    // Release rknn output resources *only if not* using zero-copy output
    if (!use_zero_copy_output) {
        rknn_outputs_release(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs);
    }

    return 0; // Success
}