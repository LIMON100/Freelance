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
    char *model_buf = NULL; // Use char* for model buffer
    rknn_context ctx = 0;

    // Ensure app_ctx is clean before starting
    memset(app_ctx, 0, sizeof(yolo11_app_context_t));

    // Load RKNN Model
    printf("Loading YOLO model: %s\n", model_path);
    model_len = read_data_from_file(model_path, &model_buf);
    if (model_buf == NULL) {
        printf("ERROR: load_model fail for YOLO!\n");
        return -1;
    }

    // Initialize RKNN context
    ret = rknn_init(&ctx, model_buf, model_len, 0, NULL);
    free(model_buf); // Free the buffer right after init
    model_buf = NULL;
    if (ret < 0) {
        printf("ERROR: rknn_init for YOLO model failed! ret=%d\n", ret);
        return -1; // Cannot proceed
    }

    // +++ Set Core Mask for YOLO Model +++
    ret = rknn_set_core_mask(ctx, RKNN_NPU_CORE_1); // Assign to Core 1
    if (ret < 0) {
        printf("ERROR: rknn_set_core_mask(yolo, CORE_1) failed! ret=%d\n", ret);
        rknn_destroy(ctx); // Clean up the initialized context before returning
        return -1; // Indicate failure
    }
    printf("INFO: YOLO Model assigned to NPU Core 1.\n");
    // +++++++++++++++++++++++++++++++++++++

    // --- Continue with querying model info ---
    app_ctx->rknn_ctx = ctx; // Store the context handle in our struct

    // Query I/O Number
    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC) {
        printf("ERROR: rknn_query(Num) for YOLO failed! ret=%d\n", ret);
        rknn_destroy(ctx); // Cleanup
        return -1;
    }
    // printf("YOLO model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);
    app_ctx->io_num = io_num;

    // Query Input Attributes
    // printf("YOLO input tensors:\n");
    app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    if (!app_ctx->input_attrs) { printf("ERROR: malloc fail for yolo input attrs\n"); rknn_destroy(ctx); return -1; }
    memset(app_ctx->input_attrs, 0, io_num.n_input * sizeof(rknn_tensor_attr));
    for (uint32_t i = 0; i < io_num.n_input; i++) {
        app_ctx->input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(app_ctx->input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("ERROR: rknn_query(Input %d) for YOLO failed! ret=%d\n", i, ret);
            free(app_ctx->input_attrs); // Free allocated memory
            rknn_destroy(ctx);
            return -1;
        }
        dump_tensor_attr(&(app_ctx->input_attrs[i]));
    }

    // Query Output Attributes
    // printf("YOLO output tensors:\n");
    app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    if (!app_ctx->output_attrs) { printf("ERROR: malloc fail for yolo output attrs\n"); free(app_ctx->input_attrs); rknn_destroy(ctx); return -1; }
    memset(app_ctx->output_attrs, 0, io_num.n_output * sizeof(rknn_tensor_attr));
    for (uint32_t i = 0; i < io_num.n_output; i++) {
        app_ctx->output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(app_ctx->output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("ERROR: rknn_query(Output %d) for YOLO failed! ret=%d\n", i, ret);
            free(app_ctx->output_attrs); // Free allocated memory
            free(app_ctx->input_attrs);
            rknn_destroy(ctx);
            return -1;
        }
        dump_tensor_attr(&(app_ctx->output_attrs[i]));
    }

    // Determine quantization status (check output type and qnt_type)
    // Adjust this logic based on your specific YOLO model's quantization details
    if (app_ctx->output_attrs[0].qnt_type != RKNN_TENSOR_QNT_NONE &&
        (app_ctx->output_attrs[0].type == RKNN_TENSOR_INT8 || app_ctx->output_attrs[0].type == RKNN_TENSOR_UINT8)) {
        app_ctx->is_quant = true;
        printf("INFO: YOLO model appears to be quantized (output 0 type: %s, qnt: %s)\n",
               get_type_string(app_ctx->output_attrs[0].type),
               get_qnt_type_string(app_ctx->output_attrs[0].qnt_type));
    } else {
        app_ctx->is_quant = false;
         printf("INFO: YOLO model appears to be FP32/FP16 (output 0 type: %s, qnt: %s)\n",
               get_type_string(app_ctx->output_attrs[0].type),
               get_qnt_type_string(app_ctx->output_attrs[0].qnt_type));
    }

    // Determine input dimensions based on format
    if (app_ctx->input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
        // printf("YOLO model is NCHW input fmt\n");
        app_ctx->model_channel = app_ctx->input_attrs[0].dims[1];
        app_ctx->model_height = app_ctx->input_attrs[0].dims[2];
        app_ctx->model_width = app_ctx->input_attrs[0].dims[3];
    } else { // Assume NHWC
        // printf("YOLO model is NHWC input fmt\n");
        app_ctx->model_height = app_ctx->input_attrs[0].dims[1];
        app_ctx->model_width = app_ctx->input_attrs[0].dims[2];
        app_ctx->model_channel = app_ctx->input_attrs[0].dims[3];
    }
    // printf("YOLO model input height=%d, width=%d, channel=%d\n",
    //        app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);

    return 0; // Success
}

int release_yolo11(yolo11_app_context_t *app_ctx) // <-- Renamed function and struct
{
    if (app_ctx->input_attrs != NULL)
    {
        free(app_ctx->input_attrs);
        app_ctx->input_attrs = NULL;
    }
    if (app_ctx->output_attrs != NULL)
    {
        free(app_ctx->output_attrs);
        app_ctx->output_attrs = NULL;
    }
    if (app_ctx->rknn_ctx != 0)
    {
        rknn_destroy(app_ctx->rknn_ctx);
        app_ctx->rknn_ctx = 0;
    }
    return 0;
}

int inference_yolo11(yolo11_app_context_t *app_ctx, image_buffer_t *img, object_detect_result_list *od_results) // <-- Renamed function and struct
{
    int ret;
    image_buffer_t dst_img;
    letterbox_t letter_box;
    rknn_input inputs[app_ctx->io_num.n_input];
    rknn_output outputs[app_ctx->io_num.n_output];
    const float nms_threshold = NMS_THRESH;      // 默认的NMS阈值
    const float box_conf_threshold = BOX_THRESH; // 默认的置信度阈值
    int bg_color = 114;

    if ((!app_ctx) || !(img) || (!od_results))
    {
        return -1;
    }

    memset(od_results, 0x00, sizeof(*od_results));
    memset(&letter_box, 0, sizeof(letterbox_t));
    memset(&dst_img, 0, sizeof(image_buffer_t));
    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));

    // Pre Process
    dst_img.width = app_ctx->model_width;
    dst_img.height = app_ctx->model_height;
    dst_img.format = IMAGE_FORMAT_RGB888;
    dst_img.size = get_image_size(&dst_img);
    dst_img.virt_addr = (unsigned char *)malloc(dst_img.size);
    if (dst_img.virt_addr == NULL)
    {
        // printf("malloc buffer size:%d fail!\n", dst_img.size);
        return -1;
    }

    // letterbox
    ret = convert_image_with_letterbox(img, &dst_img, &letter_box, bg_color);
    if (ret < 0)
    {
        // printf("convert_image_with_letterbox fail! ret=%d\n", ret);
        return -1;
    }

    // Set Input Data
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].size = app_ctx->model_width * app_ctx->model_height * app_ctx->model_channel;
    inputs[0].buf = dst_img.virt_addr;

    ret = rknn_inputs_set(app_ctx->rknn_ctx, app_ctx->io_num.n_input, inputs);
    if (ret < 0)
    {
        // printf("rknn_input_set fail! ret=%d\n", ret);
        return -1;
    }

    // Run
    printf("rknn_run\n");
    ret = rknn_run(app_ctx->rknn_ctx, nullptr);
    if (ret < 0)
    {
        // printf("rknn_run fail! ret=%d\n", ret);
        return -1;
    }

    // Get Output
    memset(outputs, 0, sizeof(outputs));
    for (int i = 0; i < app_ctx->io_num.n_output; i++)
    {
        outputs[i].index = i;
        outputs[i].want_float = (!app_ctx->is_quant);
    }
    ret = rknn_outputs_get(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs, NULL);
    if (ret < 0)
    {
        // printf("rknn_outputs_get fail! ret=%d\n", ret);
        goto out;
    }

    // Post Process
    post_process(app_ctx, outputs, &letter_box, box_conf_threshold, nms_threshold, od_results);

    // Remeber to release rknn output
    rknn_outputs_release(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs);

out:
    if (dst_img.virt_addr != NULL)
    {
        free(dst_img.virt_addr);
    }

    return ret;
}


// int inference_yolo11(yolo11_app_context_t *app_ctx, image_buffer_t *img, object_detect_result_list *od_results) {
//     int ret = 0;
//     image_buffer_t dst_img = {0};
//     letterbox_t letter_box = {0};
//     rknn_input inputs[app_ctx->io_num.n_input] = {0};
//     rknn_output outputs[app_ctx->io_num.n_output] = {0};
//     const float nms_threshold = NMS_THRESH;
//     const float box_conf_threshold = BOX_THRESH;
//     int bg_color = 114;

//     if (!app_ctx || !img || !od_results) {
//         return -1;
//     }

//     memset(od_results, 0, sizeof(*od_results));

//     // Preprocess: Set up destination image
//     dst_img.width = app_ctx->model_width;
//     dst_img.height = app_ctx->model_height;
//     dst_img.format = IMAGE_FORMAT_RGB888;
//     dst_img.size = get_image_size(&dst_img);

// #ifdef ZERO_COPY
//     rknn_tensor_mem *input_mem = rknn_create_mem(app_ctx->rknn_ctx, dst_img.size);
//     if (!input_mem) {
//         printf("ERROR: rknn_create_mem failed for input buffer!\n");
//         return -1;
//     }
//     dst_img.virt_addr = (unsigned char *)input_mem->virt_addr;
// #else
//     dst_img.virt_addr = (unsigned char *)malloc(dst_img.size);
//     if (!dst_img.virt_addr) {
//         printf("ERROR: malloc buffer size:%d failed!\n", dst_img.size);
//         return -1;
//     }
// #endif

//     // Use convert_image_with_letterbox for preprocessing
//     ret = convert_image_with_letterbox(img, &dst_img, &letter_box, bg_color);
//     if (ret < 0) {
//         printf("ERROR: convert_image_with_letterbox failed! ret=%d\n", ret);
// #ifdef ZERO_COPY
//         rknn_destroy_mem(app_ctx->rknn_ctx, input_mem);
// #else
//         free(dst_img.virt_addr);
// #endif
//         return ret;
//     }

//     // Set Input Data
//     inputs[0].index = 0;
//     inputs[0].type = RKNN_TENSOR_UINT8; // Adjust if model uses INT8 or FP32
//     inputs[0].fmt = RKNN_TENSOR_NHWC;   // Adjust based on model
//     // inputs[0].size = dst_img.size;
//     // inputs[0].buf = dst_img.virt_addr;
//     inputs[0].size = app_ctx->model_width * app_ctx->model_height * app_ctx->model_channel;
//     inputs[0].buf = dst_img.virt_addr;

//     ret = rknn_inputs_set(app_ctx->rknn_ctx, app_ctx->io_num.n_input, inputs);
//     if (ret < 0) {
//         printf("ERROR: rknn_inputs_set failed! ret=%d\n", ret);
// #ifdef ZERO_COPY
//         rknn_destroy_mem(app_ctx->rknn_ctx, input_mem);
// #else
//         free(dst_img.virt_addr);
// #endif
//         return ret;
//     }

//     // Run inference
//     printf("Running inference...\n");
//     ret = rknn_run(app_ctx->rknn_ctx, NULL);
//     if (ret < 0) {
//         printf("ERROR: rknn_run failed! ret=%d\n", ret);
// #ifdef ZERO_COPY
//         rknn_destroy_mem(app_ctx->rknn_ctx, input_mem);
// #else
//         free(dst_img.virt_addr);
// #endif
//         return ret;
//     }

//     // Get Output
//     for (int i = 0; i < app_ctx->io_num.n_output; i++) {
//         outputs[i].index = i;
//         outputs[i].want_float = (!app_ctx->is_quant);
//     }
//     ret = rknn_outputs_get(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs, NULL);
//     if (ret < 0) {
//         printf("ERROR: rknn_outputs_get failed! ret=%d\n", ret);
// #ifdef ZERO_COPY
//         rknn_destroy_mem(app_ctx->rknn_ctx, input_mem);
// #else
//         free(dst_img.virt_addr);
// #endif
//         return ret;
//     }

//     // Post Process
//     post_process(app_ctx, outputs, &letter_box, box_conf_threshold, nms_threshold, od_results);

//     // Cleanup
//     rknn_outputs_release(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs);
// #ifdef ZERO_COPY
//     rknn_destroy_mem(app_ctx->rknn_ctx, input_mem);
// #else
//     free(dst_img.virt_addr);
// #endif

//     return 0;
// }