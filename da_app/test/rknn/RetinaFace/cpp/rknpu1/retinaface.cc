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
//            attr->index, attr->name, attr->n_dims, attr->dims[3], attr->dims[2], attr->dims[1], attr->dims[0],
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
//         result->object[last_count].score = props[i];  // Confidence

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

//     ret = rknn_init(&ctx, model, model_len, 0);
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
//         app_ctx->model_channel = input_attrs[0].dims[2];
//         app_ctx->model_height  = input_attrs[0].dims[1];
//         app_ctx->model_width   = input_attrs[0].dims[0];
//     } else {
//         printf("model is NHWC input fmt\n");
//         app_ctx->model_height = input_attrs[0].dims[2];
//         app_ctx->model_width = input_attrs[0].dims[1];
//         app_ctx->model_channel = input_attrs[0].dims[0];
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
#include "common.h" // Assuming this contains your utility functions
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
    }
    return low;
}

static int filterValidResult(float *scores, float *loc, float *landms, const float boxPriors[][4], int model_in_h, int model_in_w,
                             int filter_indice[], float *props, float threshold, const int num_results) {
    int validCount = 0;
    const float VARIANCES[2] = {0.1, 0.2};
    // Scale them back to the input size.
    for (int i = 0; i < num_results; ++i) {
        float face_score = scores[i * 2 + 1];
        if (face_score > threshold) {
            filter_indice[validCount] = i;
            props[validCount] = face_score;
            //decode location to origin position
            float xcenter = loc[i * 4 + 0] * VARIANCES[0] * boxPriors[i][2] + boxPriors[i][0];
            float ycenter = loc[i * 4 + 1] * VARIANCES[0] * boxPriors[i][3] + boxPriors[i][1];
            float w = (float) expf(loc[i * 4 + 2] * VARIANCES[1] ) * boxPriors[i][2];
            float h = (float) expf(loc[i * 4 + 3] * VARIANCES[1]) * boxPriors[i][3];

            float xmin = xcenter - w * 0.5f;
            float ymin = ycenter - h * 0.5f;
            float xmax = xmin + w;
            float ymax = ymin + h;

            loc[i * 4 + 0] = xmin ;
            loc[i * 4 + 1] = ymin ;
            loc[i * 4 + 2] = xmax ;
            loc[i * 4 + 3] = ymax ;
            ++validCount;
        }
    }
    return validCount;
}

// Initialize both retinaface and facenet models.
int init_faceD_faceL_model(const char *model_path, const char *model_path2, rknn_app_context_t *app_faceD_ctx, rknn_app_context_t *app_faceL_ctx) {
    int ret;
    int model_len = 0;
    char *model;
    rknn_context ctx_faceD = 0;
    rknn_context ctx_faceL = 0;

    // Face Detection (faceD) Model Initialization
    ret = rknn_init(&ctx_faceD, (char *)model_path, 0, 0, NULL);
    if (ret < 0) {
        printf("rknn_init for faceD fail! ret=%d\n", ret);
        return -1;
    }
    // Get Model Input Output Number
    rknn_input_output_num io_num_faceD;
    ret = rknn_query(ctx_faceD, RKNN_QUERY_IN_OUT_NUM, &io_num_faceD, sizeof(io_num_faceD));
    if (ret != RKNN_SUCC) {
        printf("rknn_query fail! ret=%d\n", ret);
        return -1;
    }
    printf("faceD model input num: %d, output num: %d\n", io_num_faceD.n_input, io_num_faceD.n_output);

    // Get Model Input Info
    printf("faceD input tensors:\n");
    rknn_tensor_attr input_attrs_faceD[io_num_faceD.n_input];
    memset(input_attrs_faceD, 0, sizeof(input_attrs_faceD));
    for (int i = 0; i < io_num_faceD.n_input; i++) {
        input_attrs_faceD[i].index = i;
        ret = rknn_query(ctx_faceD, RKNN_QUERY_INPUT_ATTR, &(input_attrs_faceD[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs_faceD[i]));
    }

    // Get Model Output Info
    printf("faceD output tensors:\n");
    rknn_tensor_attr output_attrs_faceD[io_num_faceD.n_output];
    memset(output_attrs_faceD, 0, sizeof(output_attrs_faceD));
    for (int i = 0; i < io_num_faceD.n_output; i++) {
        output_attrs_faceD[i].index = i;
        //When using the zero-copy API interface, query the native output tensor attribute
        ret = rknn_query(ctx_faceD, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &(output_attrs_faceD[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_attrs_faceD[i]));
    }

    // default input type is int8 (normalize and quantize need compute in outside)
    // if set uint8, will fuse normalize and quantize to npu
    input_attrs_faceD[0].type = RKNN_TENSOR_UINT8;
    // default fmt is NHWC,1106 npu only support NHWC in zero copy mode
    input_attrs_faceD[0].fmt = RKNN_TENSOR_NHWC;
    printf("input_attrs_faceD[0].size_with_stride=%d\n", input_attrs_faceD[0].size_with_stride);
    app_faceD_ctx->input_mems[0] = rknn_create_mem(ctx_faceD, input_attrs_faceD[0].size_with_stride);
    // Set input tensor memory
    ret = rknn_set_io_mem(ctx_faceD, app_faceD_ctx->input_mems[0], &input_attrs_faceD[0]);
    if (ret < 0) {
        printf("input_mems rknn_set_io_mem fail! ret=%d\n", ret);
        return -1;
    }

    // Set output tensor memory
    for (uint32_t i = 0; i < io_num_faceD.n_output; ++i) {
        app_faceD_ctx->output_mems[i] = rknn_create_mem(ctx_faceD, output_attrs_faceD[i].size_with_stride);
        printf("output mem [%d] = %d \n",i ,output_attrs_faceD[i].size_with_stride);
        ret = rknn_set_io_mem(ctx_faceD, app_faceD_ctx->output_mems[i], &output_attrs_faceD[i]);
        if (ret < 0) {
            printf("output_mems rknn_set_io_mem fail! ret=%d\n", ret);
            return -1;
        }
    }

    // Set to context
    app_faceD_ctx->rknn_ctx = ctx_faceD;

    app_faceD_ctx->io_num = io_num_faceD;
    app_faceD_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num_faceD.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_faceD_ctx->input_attrs, input_attrs_faceD, io_num_faceD.n_input * sizeof(rknn_tensor_attr));
    app_faceD_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num_faceD.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_faceD_ctx->output_attrs, output_attrs_faceD, io_num_faceD.n_output * sizeof(rknn_tensor_attr));
    app_faceD_ctx->model_height  = input_attrs_faceD[0].dims[2];
    app_faceD_ctx->model_width   = input_attrs_faceD[0].dims[3];
    app_faceD_ctx->model_channel = input_attrs_faceD[0].dims[0];

    //Face Landmark Model Initialization
    printf("init faceL model\n");
    ret = rknn_init(&ctx_faceL, (char *)model_path2, 0, 0, NULL);
    if (ret < 0)
    {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }
    rknn_input_output_num io_num_faceL;
    ret = rknn_query(ctx_faceL, RKNN_QUERY_IN_OUT_NUM, &io_num_faceL, sizeof(io_num_faceL));
    if (ret != RKNN_SUCC) {
        printf("rknn_query fail! ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num_faceL.n_input, io_num_faceL.n_output);

    // Get Model Input Info
    printf("faceL input tensors:\n");
    rknn_tensor_attr input_attrs_faceL[io_num_faceL.n_input];
    memset(input_attrs_faceL, 0, sizeof(input_attrs_faceL));
    for (int i = 0; i < io_num_faceL.n_input; i++) {
        input_attrs_faceL[i].index = i;
        ret = rknn_query(ctx_faceL, RKNN_QUERY_INPUT_ATTR, &(input_attrs_faceL[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs_faceL[i]));
    }

    // Get Model Output Info
    printf("faceL output tensors:\n");
    rknn_tensor_attr output_attrs_faceL[io_num_faceL.n_output];
    memset(output_attrs_faceL, 0, sizeof(output_attrs_faceL));
    for (int i = 0; i < io_num_faceL.n_output; i++) {
        output_attrs_faceL[i].index = i;
        //When using the zero-copy API interface, query the native output tensor attribute
        ret = rknn_query(ctx_faceL, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &(output_attrs_faceL[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_attrs_faceL[i]));
    }
    
    // default input type is int8 (normalize and quantize need compute in outside)
    // if set uint8, will fuse normalize and quantize to npu
    input_attrs_faceL[0].type = RKNN_TENSOR_UINT8;
    // default fmt is NHWC,1106 npu only support NHWC in zero copy mode
    input_attrs_faceL[0].fmt = RKNN_TENSOR_NHWC;
    printf("input_attrs_faceL[0].size_with_stride=%d\n", input_attrs_faceL[0].size_with_stride);
    app_faceL_ctx->input_mems[0] = rknn_create_mem(ctx_faceL, input_attrs_faceL[0].size_with_stride);

    // Set input tensor memory
    ret = rknn_set_io_mem(ctx_faceL, app_faceL_ctx->input_mems[0], &input_attrs_faceL[0]);
    if (ret < 0) {
        printf("input_mems rknn_set_io_mem fail! ret=%d\n", ret);
        return -1;
    }

    // Set output tensor memory
    for (uint32_t i = 0; i < io_num_faceL.n_output; ++i) {
        app_faceL_ctx->output_mems[i] = rknn_create_mem(ctx_faceL, output_attrs_faceL[i].size_with_stride);
        printf("output mem [%d] = %d \n",i ,output_attrs_faceL[i].size_with_stride);
        ret = rknn_set_io_mem(ctx_faceL, app_faceL_ctx->output_mems[i], &output_attrs_faceL[i]);
        if (ret < 0) {
            printf("output_mems rknn_set_io_mem fail! ret=%d\n", ret);
            return -1;
        }
    }

    // Set to context
    app_faceL_ctx->rknn_ctx = ctx_faceL;
    app_faceL_ctx->io_num = io_num_faceL;
    app_faceL_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num_faceL.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_faceL_ctx->input_attrs, input_attrs_faceL, io_num_faceL.n_input * sizeof(rknn_tensor_attr));
    app_faceL_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num_faceL.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_faceL_ctx->output_attrs, output_attrs_faceL, io_num_faceL.n_output * sizeof(rknn_tensor_attr));

    printf("model is NHWC input fmt\n");
    app_faceL_ctx->model_height  = input_attrs_faceL[0].dims[2];
    app_faceL_ctx->model_width   = input_attrs_faceL[0].dims[3];
    app_faceL_ctx->model_channel = input_attrs_faceL[0].dims[0];

    return 0;
}

int release_retinaface_model(rknn_app_context_t *app_ctx) {
    // Release resources for faceD model
    release_faceD_model(app_ctx);
    return 0;
}

int release_facenet_model(rknn_app_context_t *app_ctx) {
    // Release resources for faceL model
    release_faceL_model(app_ctx);
    return 0;
}

int inference_faceD_faceL_model(rknn_app_context_t *app_faceD_ctx, rknn_app_context_t *app_faceL_ctx, image_buffer_t *src_img, retinaface_result *result) {
    int ret;
    image_buffer_t img;
    letterbox_t letter_box;
    rknn_input inputs[1];
    rknn_output outputs_faceD[3];  // Correct for the number of faceD outputs
    rknn_output outputs_faceL[1];  //Correct for the number of faceL outputs

    memset(&img, 0, sizeof(image_buffer_t));
    memset(inputs, 0, sizeof(inputs));
    memset(outputs_faceD, 0, sizeof(rknn_output) * 3); // Correct for the number of faceD outputs
    memset(outputs_faceL, 0, sizeof(rknn_output)); // Correct for the number of faceL outputs
    memset(&letter_box, 0, sizeof(letterbox_t));

    // Pre Process for faceD
    img.width = app_faceD_ctx->model_width;
    img.height = app_faceD_ctx->model_height;
    img.format = IMAGE_FORMAT_RGB888;
    img.size = get_image_size(&img);
    img.virt_addr = (unsigned char *)malloc(img.size);

    if (img.virt_addr == NULL) {
        printf("malloc buffer size:%d fail!\n", img.size);
        return -1;
    }

    ret = convert_image_with_letterbox(src_img, &img, &letter_box, 114);
    if (ret < 0) {
        printf("convert_image fail! ret=%d\n", ret);
        return -1;
    }

    // Set Input Data for faceD
    inputs[0].index = 0;
    inputs[0].type  = RKNN_TENSOR_UINT8;
    inputs[0].fmt   = RKNN_TENSOR_NHWC;
    inputs[0].size  = app_faceD_ctx->model_width * app_faceD_ctx->model_height * app_faceD_ctx->model_channel;
    inputs[0].buf   = img.virt_addr;

    ret = rknn_inputs_set(app_faceD_ctx->rknn_ctx, 1, inputs);
    if (ret < 0) {
        printf("rknn_input_set fail! ret=%d\n", ret);
        return -1;
    }
    // Run faceD
    ret = rknn_run(app_faceD_ctx->rknn_ctx, nullptr);
    if (ret < 0) {
        printf("rknn_run fail! ret=%d\n", ret);
        return -1;
    }
    // Get Output
    for (int i = 0; i < app_faceD_ctx->io_num.n_output; i++) {
        outputs_faceD[i].index = i;
        outputs_faceD[i].want_float = 1;
    }
    ret = rknn_outputs_get(app_faceD_ctx->rknn_ctx, app_faceD_ctx->io_num.n_output, outputs_faceD, NULL); // Get faceD output
    if (ret < 0) {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        goto out;
    }

    ret = post_process_retinaface(app_faceD_ctx, src_img, outputs_faceD, result, &letter_box);
    if (ret < 0) {
        printf("post_process_retinaface fail! ret=%d\n", ret);
        return -1;
    }
    // Remeber to release rknn output
    //rknn_outputs_release(app_faceD_ctx->rknn_ctx, app_faceD_ctx->io_num.n_output, outputs_faceD);

    // faceL run, we already have the face detected with the faceD model
    for (int i = 0; i < result->count; i++) {
        // Get the face box
        object_detect_result *det_result = &(result->object[i]);
        cv::Rect roi(det_result->box.left, det_result->box.top,
                     (det_result->box.right - det_result->box.left),
                     (det_result->box.bottom - det_result->box.top));
        cv::Mat face_img = src_img->mat_data(roi);

        // Preprocess the face image (resize it)
        cv::Mat faceL_input;
        cv::resize(face_img, faceL_input, cv::Size(app_faceL_ctx->model_width, app_faceL_ctx->model_height));

        // Set Input Data for faceL
        inputs[0].index = 0;
        inputs[0].type = RKNN_TENSOR_UINT8;
        inputs[0].fmt = RKNN_TENSOR_NHWC;
        inputs[0].size = app_faceL_ctx->model_width * app_faceL_ctx->model_height * app_faceL_ctx->model_channel;
        inputs[0].buf = (unsigned char *)faceL_input.data;

        ret = rknn_inputs_set(app_faceL_ctx->rknn_ctx, 1, inputs);
        if (ret < 0) {
            printf("rknn_input_set fail! ret=%d\n", ret);
            return -1;
        }

        // Run faceL
        ret = rknn_run(app_faceL_ctx->rknn_ctx, nullptr);
        if (ret < 0) {
            printf("rknn_run fail! ret=%d\n", ret);
            return -1;
        }
        // Get Output
        outputs_faceL[0].index = 0;
        outputs_faceL[0].want_float = 1;

        ret = rknn_outputs_get(app_faceL_ctx->rknn_ctx, app_faceL_ctx->io_num.n_output, outputs_faceL, NULL); //Get faceL output
        if (ret < 0) {
            printf("rknn_outputs_get fail! ret=%d\n", ret);
            goto out;
        }

        // Process the landmark data (assuming a specific output format)
        float *landms = (float *)outputs_faceL[0].buf; // Assuming the landmark data is in outputs[0]
        for (int j = 0; j < 468; ++j) {
            // Assuming the 468 mesh points are stored sequentially as x, y pairs.
            det_result->point[j].x = (int)(landms[j * 2] * src_img->width / (float)app_faceL_ctx->model_width);
            det_result->point[j].y = (int)(landms[j * 2 + 1] * src_img->height / (float)app_faceL_ctx->model_height);
        }
    }
out:
    if (img.virt_addr != NULL) {
        free(img.virt_addr);
    }

    return ret;
}