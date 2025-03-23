#include "yolo11.h"
#include "postprocess.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int init_yolo11_model(const char* model_path, rknn_app_context_t* app_ctx) {
    int ret;
    ret = rknn_init(&app_ctx->rknn_ctx, (void*)model_path, 0, 0, nullptr);
    if (ret < 0) {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }

    ret = rknn_query(app_ctx->rknn_ctx, RKNN_QUERY_IN_OUT_NUM, &app_ctx->io_num, sizeof(app_ctx->io_num));
    if (ret < 0) {
        printf("rknn_query fail! ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", app_ctx->io_num.n_input, app_ctx->io_num.n_output);

    app_ctx->input_attrs = (rknn_tensor_attr*)calloc(app_ctx->io_num.n_input, sizeof(rknn_tensor_attr));
    for (int i = 0; i < app_ctx->io_num.n_input; i++) {
        app_ctx->input_attrs[i].index = i;
        ret = rknn_query(app_ctx->rknn_ctx, RKNN_QUERY_INPUT_ATTR, &(app_ctx->input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret < 0) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        printf("input tensors: [%d, %d, %d, %d]\n",
               app_ctx->input_attrs[i].dims[0], app_ctx->input_attrs[i].dims[1],
               app_ctx->input_attrs[i].dims[2], app_ctx->input_attrs[i].dims[3]);
    }

    app_ctx->output_attrs = (rknn_tensor_attr*)calloc(app_ctx->io_num.n_output, sizeof(rknn_tensor_attr));
    for (int i = 0; i < app_ctx->io_num.n_output; i++) {
        app_ctx->output_attrs[i].index = i;
        ret = rknn_query(app_ctx->rknn_ctx, RKNN_QUERY_OUTPUT_ATTR, &(app_ctx->output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret < 0) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        printf("output tensors: [%d, %d, %d]\n",
               app_ctx->output_attrs[i].dims[0], app_ctx->output_attrs[i].dims[1],
               app_ctx->output_attrs[i].dims[2]);
    }

    app_ctx->model_width = 648;
    app_ctx->model_height = 648;
    app_ctx->model_channels = 3;

    return 0;
}

int release_yolo11_model(rknn_app_context_t* app_ctx) {
    if (app_ctx->rknn_ctx != 0) {
        rknn_destroy(app_ctx->rknn_ctx);
        app_ctx->rknn_ctx = 0;
    }
    if (app_ctx->input_attrs != nullptr) {
        free(app_ctx->input_attrs);
        app_ctx->input_attrs = nullptr;
    }
    if (app_ctx->output_attrs != nullptr) {
        free(app_ctx->output_attrs);
        app_ctx->output_attrs = nullptr;
    }
    return 0;
}

int inference_yolo11_model(rknn_app_context_t *app_ctx, image_buffer_t *img, object_detect_result_list *od_results) {
    int ret;
    image_buffer_t dst_img;
    letterbox_t letter_box;
    int model_width = app_ctx->model_width;
    int model_height = app_ctx->model_height;

    // Preprocess: Convert image to model input size (640x640)
    memset(&dst_img, 0, sizeof(image_buffer_t));
    uint32_t bg_color = 0x72727272U; // Define as a variable to suppress warning
    ret = convert_image_with_letterbox(img, &dst_img, &letter_box, bg_color);
    if (ret < 0) {
        printf("convert_image_with_letterbox fail! ret=%d\n", ret);
        return -1;
    }

    // Debug: Save preprocessed image
    write_image("preprocessed_input.png", &dst_img);

    // Prepare input
    rknn_input inputs[1];
    memset(inputs, 0, sizeof(inputs));
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8; // Input is UINT8 after preprocessing
    inputs[0].size = model_width * model_height * 3;
    inputs[0].fmt = RKNN_TENSOR_NCHW; // Model expects NCHW
    inputs[0].buf = dst_img.virt_addr;

    // Convert NHWC to NCHW
    unsigned char *input_data = (unsigned char *)malloc(inputs[0].size);
    if (!input_data) {
        printf("Failed to allocate input buffer\n");
        return -1;
    }
    int hwc_size = model_width * model_height;
    for (int c = 0; c < 3; c++) {
        for (int h = 0; h < model_height; h++) {
            for (int w = 0; w < model_width; w++) {
                input_data[c * hwc_size + h * model_width + w] = dst_img.virt_addr[h * model_width * 3 + w * 3 + c];
            }
        }
    }
    inputs[0].buf = input_data;

    // Set inputs
    ret = rknn_inputs_set(app_ctx->rknn_ctx, app_ctx->io_num.n_input, inputs);
    if (ret < 0) {
        printf("rknn_input_set fail! ret=%d\n", ret);
        free(input_data);
        return -1;
    }

    // Run inference
    printf("rknn_run\n");
    ret = rknn_run(app_ctx->rknn_ctx, nullptr);
    if (ret < 0) {
        printf("rknn_run fail! ret=%d\n", ret);
        free(input_data);
        return -1;
    }

    // Get output
    rknn_output outputs[app_ctx->io_num.n_output];
    memset(outputs, 0, sizeof(outputs));
    for (int i = 0; i < app_ctx->io_num.n_output; i++) {
        outputs[i].want_float = 0; // Keep as INT8
        outputs[i].index = i;
    }
    ret = rknn_outputs_get(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs, nullptr);
    if (ret < 0) {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        free(input_data);
        return -1;
    }

    // Post-process
    ret = post_process(app_ctx, outputs[0].buf, &letter_box, BOX_THRESH, NMS_THRESH, od_results);

    // Release outputs
    rknn_outputs_release(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs);
    free(input_data);
    return ret;
}