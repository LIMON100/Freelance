// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <math.h>

// #include "retinaface.h"
// #include "common.h"
// #include "file_utils.h"
// #include "image_utils.h"
// #include "rknn_box_priors.h"

// #define NMS_THRESHOLD 0.4
// #define CONF_THRESHOLD 0.5
// #define VIS_THRESHOLD 0.4

// static int clamp(int x, int min, int max) {
//     if (x > max) return max;
//     if (x < min) return min;
//     return x;
// }

// static void dump_tensor_attr(rknn_tensor_attr *attr) {
//     printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
//            "zp=%d, scale=%f\n",
//            attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3],
//            attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
//            get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
// }

// static float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1, float ymax1) {
//     float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1) + 1);
//     float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1) + 1);
//     float i = w * h;
//     float u = (xmax0 - xmin0 + 1) * (ymax0 - ymin0 + 1) + (xmax1 - xmin1 + 1) * (ymax1 - ymin1 + 1) - i;
//     return u <= 0.f ? 0.f : (i / u);
// }

// static int nms(int validCount, float *outputLocations, int order[], float threshold, int width, int height) {
//     for (int i = 0; i < validCount; ++i) {
//         if (order[i] == -1) {
//             continue;
//         }
//         int n = order[i];
//         for (int j = i + 1; j < validCount; ++j) {
//             int m = order[j];
//             if (m == -1) {
//                 continue;
//             }
//             float xmin0 = outputLocations[n * 4 + 0] * width;
//             float ymin0 = outputLocations[n * 4 + 1] * height;
//             float xmax0 = outputLocations[n * 4 + 2] * width;
//             float ymax0 = outputLocations[n * 4 + 3] * height;

//             float xmin1 = outputLocations[m * 4 + 0] * width;
//             float ymin1 = outputLocations[m * 4 + 1] * height;
//             float xmax1 = outputLocations[m * 4 + 2] * width;
//             float ymax1 = outputLocations[m * 4 + 3] * height;

//             float iou = CalculateOverlap(xmin0, ymin0, xmax0, ymax0, xmin1, ymin1, xmax1, ymax1);

//             if (iou > threshold) {
//                 order[j] = -1;
//             }
//         }
//     }
//     return 0;
// }

// static int quick_sort_indice_inverse(float *input, int left, int right, int *indices) {
//     float key;
//     int key_index;
//     int low = left;
//     int high = right;
//     if (left < right) {
//         key_index = indices[left];
//         key = input[left];
//         while (low < high) {
//             while (low < high && input[high] <= key) {
//                 high--;
//             }
//             input[low] = input[high];
//             indices[low] = indices[high];
//             while (low < high && input[low] >= key) {
//                 low++;
//             }
//             input[high] = input[low];
//             indices[high] = indices[low];
//         }
//         input[low] = key;
//         indices[low] = key_index;
//         quick_sort_indice_inverse(input, left, low - 1, indices);
//         quick_sort_indice_inverse(input, low + 1, right, indices);
//     }
//     return low;
// }

// static int filterValidResult(float *scores, float *loc, float *landms, const float boxPriors[][4], int model_in_h, int model_in_w,
//                              int filter_indice[], float *props, float threshold, const int num_results) {
//     int validCount = 0;
//     const float VARIANCES[2] = {0.1, 0.2};
//     // Scale them back to the input size.
//     for (int i = 0; i < num_results; ++i) {
//         float face_score = scores[i * 2 + 1];
//         if (face_score > threshold) {
//             filter_indice[validCount] = i;
//             props[validCount] = face_score;
//             //decode location to origin position
//             float xcenter = loc[i * 4 + 0] * VARIANCES[0] * boxPriors[i][2] + boxPriors[i][0];
//             float ycenter = loc[i * 4 + 1] * VARIANCES[0] * boxPriors[i][3] + boxPriors[i][1];
//             float w = (float) expf(loc[i * 4 + 2] * VARIANCES[1] ) * boxPriors[i][2];
//             float h = (float) expf(loc[i * 4 + 3] * VARIANCES[1]) * boxPriors[i][3];

//             float xmin = xcenter - w * 0.5f;
//             float ymin = ycenter - h * 0.5f;
//             float xmax = xmin + w;
//             float ymax = ymin + h;

//             loc[i * 4 + 0] = xmin ;
//             loc[i * 4 + 1] = ymin ;
//             loc[i * 4 + 2] = xmax ;
//             loc[i * 4 + 3] = ymax ;
//             for (int j = 0; j < 5; ++j) {
//                 landms[i * 10 + 2 * j] = landms[i * 10 + 2 * j] * VARIANCES[0] * boxPriors[i][2] + boxPriors[i][0];
//                 landms[i * 10 + 2 * j + 1] = landms[i * 10 + 2 * j + 1] * VARIANCES[0] * boxPriors[i][3] + boxPriors[i][1];
//             }
//             ++validCount;
//         }
//     }

//     return validCount;
// }

// static int post_process_retinaface(rknn_app_context_t *app_ctx, image_buffer_t *src_img, rknn_output outputs[], retinaface_result *result, letterbox_t *letter_box) {
//     float *location = (float *)outputs[0].buf;
//     float *scores = (float *)outputs[1].buf;
//     float *landms = (float *)outputs[2].buf;
//     const float (*prior_ptr)[4];
//     int num_priors = 0;
//     if (app_ctx->model_height == 320) {
//         num_priors = 4200;//anchors box number
//         prior_ptr = BOX_PRIORS_320;
//     } else if(app_ctx->model_height == 640){
//         num_priors = 16800;//anchors box number
//         prior_ptr = BOX_PRIORS_640;
//     }
//     else
//     {
//         printf("model_shape error!!!\n");
//         return -1;

//     }

//     int filter_indices[num_priors];
//     float props[num_priors];

//     memset(filter_indices, 0, sizeof(int)*num_priors);
//     memset(props, 0, sizeof(float)*num_priors);

//     int validCount = filterValidResult(scores, location, landms, prior_ptr, app_ctx->model_height, app_ctx->model_width,
//                                        filter_indices, props, CONF_THRESHOLD, num_priors);

//     quick_sort_indice_inverse(props, 0, validCount - 1, filter_indices);
//     nms(validCount, location, filter_indices, NMS_THRESHOLD, src_img->width, src_img->height);


//     int last_count = 0;
//     result->count = 0;
//     for (int i = 0; i < validCount; ++i) {
//         if (last_count >= 128) {
//             printf("Warning: detected more than 128 faces, can not handle that");
//             break;
//         }
//         if (filter_indices[i] == -1 || props[i] < VIS_THRESHOLD) {
//             continue;
//         }

//         int n = filter_indices[i];

//         float x1 = location[n * 4 + 0] * app_ctx->model_width - letter_box->x_pad;
//         float y1 = location[n * 4 + 1] * app_ctx->model_height - letter_box->y_pad;
//         float x2 = location[n * 4 + 2] * app_ctx->model_width - letter_box->x_pad;
//         float y2 = location[n * 4 + 3] * app_ctx->model_height - letter_box->y_pad;
//         int model_in_w = app_ctx->model_width;
//         int model_in_h = app_ctx->model_height;
//         result->object[last_count].box.left   = (int)(clamp(x1, 0, model_in_w) / letter_box->scale); // Face box
//         result->object[last_count].box.top    = (int)(clamp(y1, 0, model_in_h) / letter_box->scale);
//         result->object[last_count].box.right  = (int)(clamp(x2, 0, model_in_w) / letter_box->scale);
//         result->object[last_count].box.bottom = (int)(clamp(y2, 0, model_in_h) / letter_box->scale);
//         result->object[last_count].score = props[i]; // Confidence

//         for (int j = 0; j < 5; ++j) { // Facial feature points
//             float ponit_x = landms[n * 10 + 2 * j] * app_ctx->model_width - letter_box->x_pad;
//             float ponit_y = landms[n * 10 + 2 * j + 1] * app_ctx->model_height - letter_box->y_pad;
//             result->object[last_count].ponit[j].x = (int)(clamp(ponit_x, 0, model_in_w) / letter_box->scale);
//             result->object[last_count].ponit[j].y = (int)(clamp(ponit_y, 0, model_in_h) / letter_box->scale);
//         }
//         last_count++;
//     }

//     result->count = last_count;

//     return 0;
// }

// int init_retinaface_model(const char *model_path, rknn_app_context_t *app_ctx) {
//     int ret;
//     int model_len = 0;
//     char *model;
//     rknn_context ctx = 0;

//     // Load RKNN Model
//     model_len = read_data_from_file(model_path, &model);
//     if (model == NULL) {
//         printf("load_model fail!\n");
//         return -1;
//     }

//     ret = rknn_init(&ctx, model, model_len, 0, NULL);
//     free(model);
//     if (ret < 0) {
//         printf("rknn_init fail! ret=%d\n", ret);
//         return -1;
//     }

//     // Get Model Input Output Number
//     rknn_input_output_num io_num;
//     ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
//     if (ret != RKNN_SUCC) {
//         printf("rknn_query fail! ret=%d\n", ret);
//         return -1;
//     }
//     printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

//     // Get Model Input Info
//     printf("input tensors:\n");
//     rknn_tensor_attr input_attrs[io_num.n_input];
//     memset(input_attrs, 0, sizeof(input_attrs));
//     for (int i = 0; i < io_num.n_input; i++) {
//         input_attrs[i].index = i;
//         ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
//         if (ret != RKNN_SUCC) {
//             printf("rknn_query fail! ret=%d\n", ret);
//             return -1;
//         }
//         dump_tensor_attr(&(input_attrs[i]));
//     }

//     // Get Model Output Info
//     printf("output tensors:\n");
//     rknn_tensor_attr output_attrs[io_num.n_output];
//     memset(output_attrs, 0, sizeof(output_attrs));
//     for (int i = 0; i < io_num.n_output; i++) {
//         output_attrs[i].index = i;
//         ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
//         if (ret != RKNN_SUCC) {
//             printf("rknn_query fail! ret=%d\n", ret);
//             return -1;
//         }
//         dump_tensor_attr(&(output_attrs[i]));
//     }

//     // Set to context
//     app_ctx->rknn_ctx = ctx;
//     app_ctx->io_num = io_num;
//     app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
//     memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
//     app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
//     memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

//     if (input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
//         printf("model is NCHW input fmt\n");
//         app_ctx->model_channel = input_attrs[0].dims[1];
//         app_ctx->model_height  = input_attrs[0].dims[2];
//         app_ctx->model_width   = input_attrs[0].dims[3];
//     } else {
//         printf("model is NHWC input fmt\n");
//         app_ctx->model_height  = input_attrs[0].dims[1];
//         app_ctx->model_width   = input_attrs[0].dims[2];
//         app_ctx->model_channel = input_attrs[0].dims[3];
//     }
//     printf("model input height=%d, width=%d, channel=%d\n",
//            app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);

//     return 0;
// }

// int release_retinaface_model(rknn_app_context_t *app_ctx) {
//     if (app_ctx->input_attrs != NULL) {
//         free(app_ctx->input_attrs);
//         app_ctx->input_attrs = NULL;
//     }
//     if (app_ctx->output_attrs != NULL) {
//         free(app_ctx->output_attrs);
//         app_ctx->output_attrs = NULL;
//     }
//     if (app_ctx->rknn_ctx != 0) {
//         rknn_destroy(app_ctx->rknn_ctx);
//         app_ctx->rknn_ctx = 0;
//     }
//     return 0;
// }

// int inference_retinaface_model(rknn_app_context_t *app_ctx, image_buffer_t *src_img, retinaface_result *out_result) {
//     int ret;
//     image_buffer_t img;
//     letterbox_t letter_box;
//     rknn_input inputs[1];
//     rknn_output outputs[app_ctx->io_num.n_output];
//     memset(&img, 0, sizeof(image_buffer_t));
//     memset(inputs, 0, sizeof(inputs));
//     memset(outputs, 0, sizeof(rknn_output) * 3);
//     memset(&letter_box, 0, sizeof(letterbox_t));
//     int bg_color = 114;//letterbox background pixel

//     // Pre Process
//     img.width = app_ctx->model_width;
//     img.height = app_ctx->model_height;
//     img.format = IMAGE_FORMAT_RGB888;
//     img.size = get_image_size(&img);
//     img.virt_addr = (unsigned char *)malloc(img.size);

//     if (img.virt_addr == NULL) {
//         printf("malloc buffer size:%d fail!\n", img.size);
//         return -1;
//     }

//     ret = convert_image_with_letterbox(src_img, &img, &letter_box, bg_color);
//     if (ret < 0) {
//         printf("convert_image fail! ret=%d\n", ret);
//         return -1;
//     }

//     // Set Input Data
//     inputs[0].index = 0;
//     inputs[0].type  = RKNN_TENSOR_UINT8;
//     inputs[0].fmt   = RKNN_TENSOR_NHWC;
//     inputs[0].size  = app_ctx->model_width * app_ctx->model_height * app_ctx->model_channel;
//     inputs[0].buf   = img.virt_addr;

//     ret = rknn_inputs_set(app_ctx->rknn_ctx, 1, inputs);
//     if (ret < 0) {
//         printf("rknn_input_set fail! ret=%d\n", ret);
//         return -1;
//     }

//     // Run
//     printf("rknn_run\n");
//     ret = rknn_run(app_ctx->rknn_ctx, nullptr);
//     if (ret < 0) {
//         printf("rknn_run fail! ret=%d\n", ret);
//         return -1;
//     }

//     // Get Output
//     for (int i = 0; i < app_ctx->io_num.n_output; i++) {
//         outputs[i].index = i;
//         outputs[i].want_float = 1;
//     }
//     ret = rknn_outputs_get(app_ctx->rknn_ctx, 3, outputs, NULL);
//     if (ret < 0) {
//         printf("rknn_outputs_get fail! ret=%d\n", ret);
//         goto out;
//     }

//     ret = post_process_retinaface(app_ctx, src_img, outputs, out_result, &letter_box);
//     if (ret < 0) {
//         printf("post_process_retinaface fail! ret=%d\n", ret);
//         return -1;
//     }
//     // Remeber to release rknn output
//     rknn_outputs_release(app_ctx->rknn_ctx, 3, outputs);

// out:
//     if (img.virt_addr != NULL) {
//         free(img.virt_addr);
//     }

//     return ret;
// }


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "retinaface.h"
#include "common.h"
#include "file_utils.h"
#include "image_utils.h"
#include "rknn_box_priors.h"

#define NMS_THRESHOLD 0.4
#define CONF_THRESHOLD 0.5
#define VIS_THRESHOLD 0.4

static int clamp(int x, int min, int max) {
    if (x > max) return max;
    if (x < min) return min;
    return x;
}

static void dump_tensor_attr(rknn_tensor_attr *attr) {
    printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
           "zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, attr->dims[3], attr->dims[2], attr->dims[1], attr->dims[0],
           attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
           get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

static float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1, float ymax1) {
    float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1) + 1);
    float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1) + 1);
    float i = w * h;
    float u = (xmax0 - xmin0 + 1) * (ymax0 - ymin0 + 1) + (xmax1 - xmin1 + 1) * (ymax1 - ymin1 + 1) - i;
    return u <= 0.f ? 0.f : (i / u);
}

static int quick_sort_indice_inverse(float *input, int left, int right, int *indices) {
    float key;
    int key_index;
    int low = left;
    int high = right;
    if (left < right) {
        key_index = indices[left];
        key = input[left];
        while (low < high) {
            while (low < high && input[high] <= key) {
                high--;
            }
            input[low] = input[high];
            indices[low] = indices[high];
            while (low < high && input[low] >= key) {
                low++;
            }
            input[high] = key;
            indices[high] = key_index;
            quick_sort_indice_inverse(input, left, low - 1, indices);
            quick_sort_indice_inverse(input, low + 1, right, indices);
        }
        input[low] = key;
        indices[low] = key_index;
        quick_sort_indice_inverse(input, left, low - 1, indices);
        quick_sort_indice_inverse(input, low + 1, right, indices);
    }
    return low;
}

static int nms(int validCount, float *outputLocations, int order[], float threshold, int width, int height) {
    for (int i = 0; i < validCount; ++i) {
        if (order[i] == -1) {
            continue;
        }
        int n = order[i];
        for (int j = i + 1; j < validCount; ++j) {
            int m = order[j];
            if (m == -1) {
                continue;
            }
            float xmin0 = outputLocations[n * 4 + 0] * width;
            float ymin0 = outputLocations[n * 4 + 1] * height;
            float xmax0 = outputLocations[n * 4 + 2] * width;
            float ymax0 = outputLocations[n * 4 + 3] * height;

            float xmin1 = outputLocations[m * 4 + 0] * width;
            float ymin1 = outputLocations[m * 4 + 1] * height;
            float xmax1 = outputLocations[m * 4 + 2] * width;
            float ymax1 = outputLocations[m * 4 + 3] * height;

            float iou = CalculateOverlap(xmin0, ymin0, xmax0, ymax0, xmin1, ymin1, xmax1, ymax1);

            if (iou > threshold) {
                order[j] = -1;
            }
        }
    }
    return 0;
}

int init_retinaface_facenet_model(const char *model_path_d, const char *model_path_l, rknn_app_context_t *app_faced_ctx, rknn_app_context_t *app_facel_ctx)
{
    int ret;
    int model_len = 0;
    char *model;
    rknn_context ctx_faced = 0;
    rknn_context ctx_facel = 0;

    // init faced model
    ret = rknn_init(&ctx_faced, (char *)model_path_d, 0, 0, NULL);
    if (ret < 0) {
        printf("rknn_init for faceD fail! ret=%d\n", ret);
        return -1;
    }
    // init facel model
    ret = rknn_init(&ctx_facel, (char *)model_path_l, 0, 0, NULL);
    if (ret < 0) {
        printf("rknn_init for faceL fail! ret=%d\n", ret);
        return -1;
    }

    //-----------------------------------
    // faceD in/output Setting
    printf("faceD Info\n");
    rknn_input_output_num io_num_faced;
    ret = rknn_query(ctx_faced, RKNN_QUERY_IN_OUT_NUM, &io_num_faced, sizeof(io_num_faced));
    if (ret != RKNN_SUCC) {
        printf("rknn_query for faceD fail! ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num_faced.n_input, io_num_faced.n_output);

    // Get Model Input Info
    printf("input tensors:\n");
    rknn_tensor_attr input_attrs_faced[io_num_faced.n_input];
    memset(input_attrs_faced, 0, sizeof(input_attrs_faced));
    for (int i = 0; i < io_num_faced.n_input; i++) {
        input_attrs_faced[i].index = i;
        ret = rknn_query(ctx_faced,
                        RKNN_QUERY_NATIVE_INPUT_ATTR,
                        &(input_attrs_faced[i]),
                        sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs_faced[i]));
    }

    // Get Model Output Info
    printf("output tensors:\n");
    rknn_tensor_attr output_attrs_faced[io_num_faced.n_output];
    memset(output_attrs_faced, 0, sizeof(output_attrs_faced));
    for (int i = 0; i < io_num_faced.n_output; i++) {
        output_attrs_faced[i].index = i;
        ret = rknn_query(ctx_faced,
                        RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR,
                        &(output_attrs_faced[i]),
                        sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_attrs_faced[i]));
    }

    // default input type is int8 (normalize and quantize need compute in outside)
    // if set uint8, will fuse normalize and quantize to npu
    input_attrs_faced[0].type = RKNN_TENSOR_UINT8;
    // default fmt is NHWC,1106 npu only support NHWC in zero copy mode
    input_attrs_faced[0].fmt = RKNN_TENSOR_NHWC;
    printf("input_attrs[0].size_with_stride=%d\n", input_attrs_faced[0].size_with_stride);
    app_faced_ctx->input_mems[0] = rknn_create_mem(ctx_faced, input_attrs_faced[0].size_with_stride);
    // Set input tensor memory
    ret = rknn_set_io_mem(ctx_faced, app_faced_ctx->input_mems[0], &input_attrs_faced[0]);
    if (ret < 0) {
        printf("input_mems rknn_set_io_mem fail! ret=%d\n", ret);
        return -1;
    }

    // Set output tensor memory
    for (uint32_t i = 0; i < io_num_faced.n_output; ++i) {
        app_faced_ctx->output_mems[i] = rknn_create_mem(ctx_faced, output_attrs_faced[i].size_with_stride);
        printf("output mem [%d] = %d \n",i ,output_attrs_faced[i].size_with_stride);
        ret = rknn_set_io_mem(ctx_faced, app_faced_ctx->output_mems[i], &output_attrs_faced[i]);
        if (ret < 0) {
            printf("output_mems rknn_set_io_mem fail! ret=%d\n", ret);
            return -1;
        }
    }

    // Set to context
    app_faced_ctx->rknn_ctx = ctx_faced;

    // TODO
    if (output_attrs_faced[0].qnt_type == RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC) {
        app_faced_ctx->is_quant = true;
    } else {
        app_faced_ctx->is_quant = false;
    }

    app_faced_ctx->io_num = io_num_faced;
    app_faced_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num_faced.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_faced_ctx->input_attrs, input_attrs_faced, io_num_faced.n_input * sizeof(rknn_tensor_attr));
    app_faced_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num_faced.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_faced_ctx->output_attrs, output_attrs_faced, io_num_faced.n_output * sizeof(rknn_tensor_attr));

    printf("model is NHWC input fmt\n");
    app_faced_ctx->model_height = input_attrs_faced[0].dims[2];
    app_faced_ctx->model_width  = input_attrs_faced[0].dims[3];
    app_faced_ctx->model_channel = input_attrs_faced[0].dims[0];

    //-----------------------------------------
    // faceL in/output Setting
    printf("faceL Info\n");
    rknn_input_output_num io_num_facel;
    rknn_query(ctx_facel, RKNN_QUERY_IN_OUT_NUM, &io_num_facel, sizeof(io_num_facel));
    printf("model input num: %d, output num: %d\n", io_num_facel.n_input, io_num_facel.n_output);

    // Get Model Input Info
    printf("input tensors:\n");
    rknn_tensor_attr input_attrs_facel[io_num_facel.n_input];
    memset(input_attrs_facel, 0, sizeof(input_attrs_facel));
    for (int i = 0; i < io_num_facel.n_input; i++) {
        input_attrs_facel[i].index = i;
        ret = rknn_query(ctx_facel, RKNN_QUERY_NATIVE_INPUT_ATTR, &(input_attrs_facel[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs_facel[i]));
    }

    // Get Model Output Info
    printf("output tensors:\n");
    rknn_tensor_attr output_attrs_facel[io_num_facel.n_output];
    memset(output_attrs_facel, 0, sizeof(output_attrs_facel));
    for (int i = 0; i < io_num_facel.n_output; i++) {
        output_attrs_facel[i].index = i;
        //When using the zero-copy API interface, query the native output tensor attribute
        ret = rknn_query(ctx_facel, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &(output_attrs_facel[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_attrs_facel[i]));
    }
        // default input type is int8 (normalize and quantize need compute in outside)
    // if set uint8, will fuse normalize and quantize to npu
    input_attrs_facel[0].type = RKNN_TENSOR_UINT8;
    // default fmt is NHWC,1106 npu only support NHWC in zero copy mode
    input_attrs_facel[0].fmt = RKNN_TENSOR_NHWC;
    printf("input_attrs_facenet[0].size_with_stride=%d\n", input_attrs_facel[0].size_with_stride);
    app_facel_ctx->input_mems[0] = rknn_create_mem(ctx_facel, input_attrs_facel[0].size_with_stride);

    // Set input tensor memory
    ret = rknn_set_io_mem(ctx_facel, app_facel_ctx->input_mems[0], &input_attrs_facel[0]);
    if (ret < 0) {
        printf("input_mems rknn_set_io_mem fail! ret=%d\n", ret);
        return -1;
    }

    // Set output tensor memory
    for (uint32_t i = 0; i < io_num_facel.n_output; ++i) {
        app_facel_ctx->output_mems[i] = rknn_create_mem(ctx_facel, output_attrs_facel[i].size_with_stride);
        printf("output mem [%d] = %d \n",i ,output_attrs_facel[i].size_with_stride);
        ret = rknn_set_io_mem(ctx_facel, app_facel_ctx->output_mems[i], &output_attrs_facel[i]);
        if (ret < 0) {
            printf("output_mems rknn_set_io_mem fail! ret=%d\n", ret);
            return -1;
        }
    }
    // Set to context
    app_facel_ctx->rknn_ctx = ctx_facel;

    // TODO
    if (output_attrs_facel[0].qnt_type == RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC)
    {
        app_facel_ctx->is_quant = true;
    }
    else
    {
        app_facel_ctx->is_quant = false;
    }

    app_facel_ctx->io_num = io_num_facel;
    app_facel_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num_facel.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_facel_ctx->input_attrs, input_attrs_facel, io_num_facel.n_input * sizeof(rknn_tensor_attr));
    app_facel_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num_facel.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_facel_ctx->output_attrs, output_attrs_facel, io_num_facel.n_output * sizeof(rknn_tensor_attr));


    printf("Init success \n");
    return 0;
}
int release_retinaface_model(rknn_app_context_t *app_faced_ctx,rknn_app_context_t *app_facel_ctx) {
    // Release face detection model resources
    if (app_faced_ctx->input_attrs != NULL) {
        free(app_faced_ctx->input_attrs);
        app_faced_ctx->input_attrs = NULL;
    }
    if (app_faced_ctx->output_attrs != NULL) {
        free(app_faced_ctx->output_attrs);
        app_faced_ctx->output_attrs = NULL;
    }
    if (app_faced_ctx->rknn_ctx != 0) {
        rknn_destroy(app_faced_ctx->rknn_ctx);
        app_faced_ctx->rknn_ctx = 0;
    }

    // Release face landmark model resources
    if (app_facel_ctx->input_attrs != NULL) {
        free(app_facel_ctx->input_attrs);
        app_facel_ctx->input_attrs = NULL;
    }
    if (app_facel_ctx->output_attrs != NULL) {
        free(app_facel_ctx->output_attrs);
        app_facel_ctx->output_attrs = NULL;
    }
    if (app_facel_ctx->rknn_ctx != 0) {
        rknn_destroy(app_facel_ctx->rknn_ctx);
        app_facel_ctx->rknn_ctx = 0;
    }
    return 0;
}
int inference_retinaface_model(rknn_app_context_t *app_ctx, image_buffer_t *src_img, retinaface_result *out_result) {
    int ret;
    rknn_input inputs[1];
    rknn_output outputs_faced[2];  // Assuming 2 outputs for face detection (bounding boxes and scores)
    rknn_output outputs_facel[1];   // Assuming 1 output for facial landmarks
    memset(inputs, 0, sizeof(inputs));
    memset(outputs_faced, 0, sizeof(rknn_output) * 2);
    memset(outputs_facel, 0, sizeof(rknn_output));
    letterbox_t letter_box;
    memset(&letter_box, 0, sizeof(letterbox_t));

    // 1. Preprocess the image (common for both models)
    // Define the input size
    img.width = app_ctx->model_width;
    img.height = app_ctx->model_height;
    img.format = IMAGE_FORMAT_RGB888;
    img.size = get_image_size(&img);
    img.virt_addr = (unsigned char *)malloc(img.size);

    if (img.virt_addr == NULL) {
        printf("malloc buffer size:%d fail!\n", img.size);
        return -1;
    }
    ret = convert_image_with_letterbox(src_img, &img, &letter_box, 114); // Assuming 114 is the background color
    if (ret < 0) {
        printf("convert_image fail! ret=%d\n", ret);
        free(img.virt_addr);
        return -1;
    }

    // 2.  Prepare Input for face detection model
    inputs[0].index = 0;
    inputs[0].type  = RKNN_TENSOR_UINT8;
    inputs[0].fmt   = RKNN_TENSOR_NHWC;
    inputs[0].size  = app_ctx->model_width * app_ctx->model_height * app_ctx->model_channel;
    inputs[0].buf   = img.virt_addr;

    ret = rknn_inputs_set(app_ctx->rknn_ctx, 1, inputs); //using the context
    if (ret < 0) {
        printf("rknn_input_set fail! ret=%d\n", ret);
        free(img.virt_addr);
        return -1;
    }
    //3. Run Face Detection Model
    ret = rknn_run(app_ctx->rknn_ctx, nullptr); //using the context
    if (ret < 0) {
        printf("rknn_run fail! ret=%d\n", ret);
        free(img.virt_addr);
        return -1;
    }
    // 4. Get output from face detection
    for (int i = 0; i < 2; i++) { // Assuming two output tensors for face detection
        outputs_faced[i].index = i;
        outputs_faced[i].want_float = 1;  //  Assuming output is float
    }
    ret = rknn_outputs_get(app_ctx->rknn_ctx, 2, outputs_faced, NULL); //using the context
    if (ret < 0) {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        free(img.virt_addr);
        return -1;
    }
    // 5. Post-process face detection model
    ret = post_process_retinaface(app_ctx, src_img, outputs_faced, out_result, &letter_box); // using the context
    if (ret < 0) {
        printf("post_process_retinaface fail! ret=%d\n", ret);
        rknn_outputs_release(app_ctx->rknn_ctx, 2, outputs_faced); //using the context
        free(img.virt_addr);
        return -1;
    }

    // face detection output
    if (result->count > 0) {
      // 6. Face Landmarks Inference: For each detected face, we now do landmark extraction
      for (int i = 0; i < result->count; ++i) {
          // Cropping face and re-sizing for landmark model
          box_rect_t face_box = result->object[i].box;
          int crop_width = (face_box.right - face_box.left);
          int crop_height = (face_box.bottom - face_box.top);
          cv::Mat face_img = cv::Mat(src_img->height, src_img->width, CV_8UC3, src_img->virt_addr);
          cv::Rect roi(face_box.left, face_box.top,crop_width , crop_height);
          cv::Mat face_img_cropped = face_img(roi);
          cv::Mat face_img_resize;
          cv::resize(face_img_cropped, face_img_resize, cv::Size(192, 192)); // Assuming FaceNet input size is 192x192

          // Preparing the input for the Landmark Model
          inputs[0].index = 0;
          inputs[0].type = RKNN_TENSOR_UINT8;
          inputs[0].fmt = RKNN_TENSOR_NHWC;
          inputs[0].size = 192 * 192 * 3; // Assuming 192x192x3
          inputs[0].buf = face_img_resize.data;  // Input buffer points to the resized face image

          // Set Input Data
          ret = rknn_inputs_set(app_facel_ctx->rknn_ctx, 1, inputs);
          if (ret < 0) {
              printf("rknn_input_set fail! ret=%d\n", ret);
              return -1;
          }
          // 7. Run the face landmark extraction model.
          ret = rknn_run(app_facel_ctx->rknn_ctx, nullptr);
          if (ret < 0) {
              printf("rknn_run for faceL fail! ret=%d\n", ret);
              return -1;
          }
          // Get output
          for (int j = 0; j < 1; ++j) { // Assuming one output tensor for landmarks
              outputs_facel[j].index = j;
              outputs_facel[j].want_float = 1;  // Assuming output is float
          }
          ret = rknn_outputs_get(app_facel_ctx->rknn_ctx, 1, outputs_facel, NULL);
          if (ret < 0) {
            printf("rknn_outputs_get for faceL fail! ret=%d\n", ret);
            return -1;
          }
          // 8. Process landmark data
          // Assuming the 1404 output values represent 234 landmarks (x, y)
          // Modify this part based on your model's landmark output format.

          float* landms = (float*)outputs_facel[0].buf;

          for (int k = 0; k < 234; ++k) {  // Assuming 234 landmarks
                int point_x = (int)(landms[k * 2] * src_img->width);
                int point_y = (int)(landms[k * 2 + 1] * src_img->height);
                result->object[i].ponit[k].x =  clamp(point_x,0,src_img->width);
                result->object[i].ponit[k].y =  clamp(point_y,0,src_img->height);
          }

          rknn_outputs_release(app_facel_ctx->rknn_ctx, 1, outputs_facel); // Release  the landmark output
      }
    }


    // Clean up allocated memory
    rknn_outputs_release(app_ctx->rknn_ctx, 2, outputs_faced);  // Release the face detection output
    free(img.virt_addr);

    return 0;
}