// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <math.h>
// #include "faced_facel.h"
// #include "faced_box_priors.h"

// #define USE_RKNN_MEM_SHARE 0

// int clamp(float x, int min, int max) {
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

// int init_faced_model(const char *model_path, rknn_app_context_t *app_ctx) {
//     int ret;
//     rknn_context ctx = 0;

//     ret = rknn_init(&ctx, (char *)model_path, 0, 0, NULL);
//     if (ret < 0) {
//         printf("rknn_init fail! ret=%d\n", ret);
//         return -1;
//     }

//     rknn_input_output_num io_num;
//     ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
//     if (ret != RKNN_SUCC) {
//         printf("rknn_query fail! ret=%d\n", ret);
//         return -1;
//     }
//     printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

//     printf("input tensors:\n");
//     rknn_tensor_attr input_attrs[io_num.n_input];
//     memset(input_attrs, 0, sizeof(input_attrs));
//     for (int i = 0; i < io_num.n_input; i++) {
//         input_attrs[i].index = i;
//         ret = rknn_query(ctx, RKNN_QUERY_NATIVE_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
//         if (ret != RKNN_SUCC) {
//             printf("rknn_query fail! ret=%d\n", ret);
//             return -1;
//         }
//         dump_tensor_attr(&(input_attrs[i]));
//     }

//     printf("output tensors:\n");
//     rknn_tensor_attr output_attrs[io_num.n_output];
//     memset(output_attrs, 0, sizeof(output_attrs));
//     for (int i = 0; i < io_num.n_output; i++) {
//         output_attrs[i].index = i;
//         ret = rknn_query(ctx, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
//         if (ret != RKNN_SUCC) {
//             printf("rknn_query fail! ret=%d\n", ret);
//             return -1;
//         }
//         dump_tensor_attr(&(output_attrs[i]));
//     }

//     input_attrs[0].type = RKNN_TENSOR_UINT8;
//     input_attrs[0].fmt = RKNN_TENSOR_NCHW; 

//     printf("input_attrs[0].size_with_stride=%d\n", input_attrs[0].size_with_stride);
//     app_ctx->input_mems[0] = rknn_create_mem(ctx, input_attrs[0].size_with_stride);

//     ret = rknn_set_io_mem(ctx, app_ctx->input_mems[0], &input_attrs[0]);
//     if (ret < 0) {
//         printf("input_mems rknn_set_io_mem fail! ret=%d\n", ret);
//         return -1;
//     }

//     for (uint32_t i = 0; i < io_num.n_output; ++i) {
//         app_ctx->output_mems[i] = rknn_create_mem(ctx, output_attrs[i].size_with_stride);
//         printf("output mem [%d] = %d \n", i, output_attrs[i].size_with_stride);
//         ret = rknn_set_io_mem(ctx, app_ctx->output_mems[i], &output_attrs[i]);
//         if (ret < 0) {
//             printf("output_mems rknn_set_io_mem fail! ret=%d\n", ret);
//             return -1;
//         }
//     }

//     app_ctx->rknn_ctx = ctx;
//     if (output_attrs[0].qnt_type == RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC) {
//         app_ctx->is_quant = true;
//     } else {
//         app_ctx->is_quant = false;
//     }

//     app_ctx->io_num = io_num;
//     app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
//     memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
//     app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
//     memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

//     printf("model is NCHW input fmt\n");
//     // Input is NHWC: [1, H, W, C]
//     app_ctx->model_height  = input_attrs[0].dims[1]; // H = 128
//     app_ctx->model_width   = input_attrs[0].dims[2]; // W = 128
//     app_ctx->model_channel = input_attrs[0].dims[3]; // C = 3

//     printf("model input height=%d, width=%d, channel=%d\n",
//            app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);

//     return 0;
// }

// int init_facel_model(const char *model_path, rknn_app_context_t *app_ctx) {
//     int ret;
//     rknn_context ctx = 0;

//     ret = rknn_init(&ctx, (char *)model_path, 0, 0, NULL);
//     if (ret < 0) {
//         printf("rknn_init fail! ret=%d\n", ret);
//         return -1;
//     }

//     rknn_input_output_num io_num;
//     ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
//     if (ret != RKNN_SUCC) {
//         printf("rknn_query fail! ret=%d\n", ret);
//         return -1;
//     }
//     printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

//     printf("input tensors:\n");
//     rknn_tensor_attr input_attrs[io_num.n_input];
//     memset(input_attrs, 0, sizeof(input_attrs));
//     for (int i = 0; i < io_num.n_input; i++) {
//         input_attrs[i].index = i;
//         ret = rknn_query(ctx, RKNN_QUERY_NATIVE_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
//         if (ret != RKNN_SUCC) {
//             printf("rknn_query fail! ret=%d\n", ret);
//             return -1;
//         }
//         dump_tensor_attr(&(input_attrs[i]));
//     }

//     printf("output tensors:\n");
//     rknn_tensor_attr output_attrs[io_num.n_output];
//     memset(output_attrs, 0, sizeof(output_attrs));
//     for (int i = 0; i < io_num.n_output; i++) {
//         output_attrs[i].index = i;
//         ret = rknn_query(ctx, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
//         if (ret != RKNN_SUCC) {
//             printf("rknn_query fail! ret=%d\n", ret);
//             return -1;
//         }
//         dump_tensor_attr(&(output_attrs[i]));
//     }

//     input_attrs[0].type = RKNN_TENSOR_UINT8;
//     input_attrs[0].fmt = RKNN_TENSOR_NCHW; // faceL.rknn uses NCHW internally, but input is NHWC
//     printf("input_attrs[0].size_with_stride=%d\n", input_attrs[0].size_with_stride);
//     app_ctx->input_mems[0] = rknn_create_mem(ctx, input_attrs[0].size_with_stride);

//     ret = rknn_set_io_mem(ctx, app_ctx->input_mems[0], &input_attrs[0]);
//     if (ret < 0) {
//         printf("input_mems rknn_set_io_mem fail! ret=%d\n", ret);
//         return -1;
//     }

//     for (uint32_t i = 0; i < io_num.n_output; ++i) {
//         app_ctx->output_mems[i] = rknn_create_mem(ctx, output_attrs[i].size_with_stride);
//         printf("output mem [%d] = %d \n", i, output_attrs[i].size_with_stride);
//         ret = rknn_set_io_mem(ctx, app_ctx->output_mems[i], &output_attrs[i]);
//         if (ret < 0) {
//             printf("output_mems rknn_set_io_mem fail! ret=%d\n", ret);
//             return -1;
//         }
//     }

//     app_ctx->rknn_ctx = ctx;
//     if (output_attrs[0].qnt_type == RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC) {
//         app_ctx->is_quant = true;
//     } else {
//         app_ctx->is_quant = false;
//     }

//     app_ctx->io_num = io_num;
//     app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
//     memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
//     app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
//     memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

//     printf("model is NCHW input fmt\n");
//     // Input is NHWC: [1, H, W, C]
//     app_ctx->model_height  = input_attrs[0].dims[1]; // H = 192
//     app_ctx->model_width   = input_attrs[0].dims[2]; // W = 192
//     app_ctx->model_channel = input_attrs[0].dims[3]; // C = 3

//     printf("model input height=%d, width=%d, channel=%d\n",
//            app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);

//     return 0;
// }

// int init_faced_facel_model(const char *faced_model_path, const char *facel_model_path, rknn_app_context_t *app_faced_ctx, rknn_app_context_t *app_facel_ctx) {
//     printf("init faced\n");
//     int ret = init_faced_model(faced_model_path, app_faced_ctx);
//     if (ret != 0) {
//         printf("init_faced_model fail! ret=%d\n", ret);
//         return -1;
//     }

//     printf("init facel\n");
//     ret = init_facel_model(facel_model_path, app_facel_ctx);
//     if (ret != 0) {
//         printf("init_facel_model fail! ret=%d\n", ret);
//         return -1;
//     }

//     printf("Init success\n");
//     return 0;
// }

// int release_faced_facel_model(rknn_app_context_t *app_faced_ctx, rknn_app_context_t *app_facel_ctx) {
//     if (app_faced_ctx->rknn_ctx != 0) {
//         rknn_destroy(app_faced_ctx->rknn_ctx);
//         app_faced_ctx->rknn_ctx = 0;
//     }
//     if (app_faced_ctx->input_attrs != NULL) {
//         free(app_faced_ctx->input_attrs);
//         app_faced_ctx->input_attrs = NULL;
//     }
//     if (app_faced_ctx->output_attrs != NULL) {
//         free(app_faced_ctx->output_attrs);
//         app_faced_ctx->output_attrs = NULL;
//     }
//     for (int i = 0; i < app_faced_ctx->io_num.n_input; i++) {
//         if (app_faced_ctx->input_mems[i] != NULL) {
//             rknn_destroy_mem(app_faced_ctx->rknn_ctx, app_faced_ctx->input_mems[i]);
//             free(app_faced_ctx->input_mems[i]);
//         }
//     }
//     for (int i = 0; i < app_faced_ctx->io_num.n_output; i++) {
//         if (app_faced_ctx->output_mems[i] != NULL) {
//             rknn_destroy_mem(app_faced_ctx->rknn_ctx, app_faced_ctx->output_mems[i]);
//             free(app_faced_ctx->output_mems[i]);
//         }
//     }

//     if (app_facel_ctx->rknn_ctx != 0) {
//         rknn_destroy(app_facel_ctx->rknn_ctx);
//         app_facel_ctx->rknn_ctx = 0;
//     }
//     if (app_facel_ctx->input_attrs != NULL) {
//         free(app_facel_ctx->input_attrs);
//         app_facel_ctx->input_attrs = NULL;
//     }
//     if (app_facel_ctx->output_attrs != NULL) {
//         free(app_facel_ctx->output_attrs);
//         app_facel_ctx->output_attrs = NULL;
//     }
//     for (int i = 0; i < app_facel_ctx->io_num.n_input; i++) {
//         if (app_facel_ctx->input_mems[i] != NULL) {
//             rknn_destroy_mem(app_facel_ctx->rknn_ctx, app_facel_ctx->input_mems[i]);
//             free(app_facel_ctx->input_mems[i]);
//         }
//     }
//     for (int i = 0; i < app_facel_ctx->io_num.n_output; i++) {
//         if (app_facel_ctx->output_mems[i] != NULL) {
//             rknn_destroy_mem(app_facel_ctx->rknn_ctx, app_facel_ctx->output_mems[i]);
//             free(app_facel_ctx->output_mems[i]);
//         }
//     }

//     return 0;
// }

// static float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1, float ymax1) {
//     float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1) + 1);
//     float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1) + 1);
//     float i = w * h;
//     float u = (xmax0 - xmin0 + 1) * (ymax0 - ymin0 + 1) + (xmax1 - xmin1 + 1) * (ymax1 - ymin1 + 1) - i;
//     return u <= 0.f ? 0.f : (i / u);
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

// static float deqnt_affine_to_f32(int8_t qnt, int32_t zp, float scale) {
//     return ((float)qnt - (float)zp) * scale;
// }

// int inference_faced_model(rknn_app_context_t *app_ctx, image_buffer_t* src_image, object_detect_result_list *od_results) {
//     int ret;

//     // Convert image to the required format (128x128, NCHW)
//     image_buffer_t dst_image;
//     memset(&dst_image, 0, sizeof(image_buffer_t));

//     // Set up the destination image
//     dst_image.width = app_ctx->model_width;   // Should be 128
//     dst_image.height = app_ctx->model_height; // Should be 128
//     dst_image.format = IMAGE_FORMAT_RGB888;   // Set a valid format (RGB888 is commonly supported)
//     dst_image.size = dst_image.width * dst_image.height * 3; // 128 * 128 * 3 = 49152
//     printf("Allocating dst_image.virt_addr: size=%zu\n", dst_image.size);
//     dst_image.virt_addr = (unsigned char*)malloc(dst_image.size);
//     if (dst_image.virt_addr == NULL) {
//         printf("Failed to allocate memory for dst_image\n");
//         return -1;
//     }

//     // Define source and destination rectangles
//     image_rect_t src_box = {0, 0, src_image->width, src_image->height};
//     image_rect_t dst_box = {0, 0, app_ctx->model_width, app_ctx->model_height};

//     ret = convert_image(src_image, &dst_image, &src_box, &dst_box, 0);
//     if (ret != 0) {
//         printf("convert image fail! ret=%d\n", ret);
//         if (dst_image.virt_addr != NULL) {
//             free(dst_image.virt_addr);
//         }
//         return -1;
//     }

//     // Copy image data to RKNN input memory (convert from RGB to NCHW if needed)
//     unsigned char* input_data = (unsigned char*)app_ctx->input_mems[0]->virt_addr;
//     for (int c = 0; c < 3; c++) { // NCHW: channels first
//         for (int h = 0; h < dst_image.height; h++) {
//             for (int w = 0; w < dst_image.width; w++) {
//                 int src_idx = (h * dst_image.width + w) * 3 + c; // RGB
//                 int dst_idx = c * (dst_image.width * dst_image.height) + h * dst_image.width + w; // NCHW
//                 input_data[dst_idx] = dst_image.virt_addr[src_idx];
//             }
//         }
//     }

//     ret = rknn_run(app_ctx->rknn_ctx, nullptr);
//     if (ret < 0) {
//         printf("rknn_run fail! ret=%d\n", ret);
//         if (dst_image.virt_addr != NULL) {
//             free(dst_image.virt_addr);
//         }
//         return -1;
//     }

//     uint8_t *locations = (uint8_t *)(app_ctx->output_mems[0]->virt_addr); // norm_tensor1: [1, 896, 16]
//     uint8_t *scores = (uint8_t *)(app_ctx->output_mems[1]->virt_addr);   // norm_tensor2: [1, 896, 1]

//     const float (*prior_ptr)[4];
//     int num_priors = 896;
//     prior_ptr = BOX_PRIORS_128;

//     int filter_indices[num_priors];
//     float props[num_priors];
//     uint32_t location_size = app_ctx->output_mems[0]->size / sizeof(uint8_t);
//     printf("Allocating loc_fp32: size=%zu\n", location_size * sizeof(float));
//     float *loc_fp32 = (float*)malloc(location_size * sizeof(float)); // Dynamically allocate to avoid stack overflow
//     if (loc_fp32 == NULL) {
//         printf("Failed to allocate memory for loc_fp32\n");
//         if (dst_image.virt_addr != NULL) {
//             free(dst_image.virt_addr);
//         }
//         return -1;
//     }
//     memset(loc_fp32, 0, location_size * sizeof(float));
//     memset(filter_indices, 0, sizeof(int) * num_priors);
//     memset(props, 0, sizeof(float) * num_priors);

//     int validCount = 0;
//     const float VARIANCES[2] = {0.1, 0.2};

//     int loc_zp = app_ctx->output_attrs[0].zp;
//     float loc_scale = app_ctx->output_attrs[0].scale;
//     int scores_zp = app_ctx->output_attrs[1].zp;
//     float scores_scale = app_ctx->output_attrs[1].scale;

//     // Decode bounding boxes
//     for (int i = 0; i < num_priors; i++) {
//         float face_score = deqnt_affine_to_f32(scores[i], scores_zp, scores_scale);
//         if (face_score > 0.5) {
//             filter_indices[validCount] = i;
//             props[validCount] = face_score;
//             int offset = i * 16; // 16 values per anchor
//             uint8_t *bbox = locations + offset;

//             float dx = deqnt_affine_to_f32(bbox[0], loc_zp, loc_scale);
//             float dy = deqnt_affine_to_f32(bbox[1], loc_zp, loc_scale);
//             float dw = deqnt_affine_to_f32(bbox[2], loc_zp, loc_scale);
//             float dh = deqnt_affine_to_f32(bbox[3], loc_zp, loc_scale);

//             float box_x = dx * VARIANCES[0] * prior_ptr[i][2] + prior_ptr[i][0];
//             float box_y = dy * VARIANCES[0] * prior_ptr[i][3] + prior_ptr[i][1];
//             float box_w = expf(dw * VARIANCES[1]) * prior_ptr[i][2];
//             float box_h = expf(dh * VARIANCES[1]) * prior_ptr[i][3];

//             float xmin = box_x - box_w * 0.5f;
//             float ymin = box_y - box_h * 0.5f;
//             float xmax = xmin + box_w;
//             float ymax = ymin + box_h;

//             loc_fp32[i * 4 + 0] = xmin;
//             loc_fp32[i * 4 + 1] = ymin;
//             loc_fp32[i * 4 + 2] = xmax;
//             loc_fp32[i * 4 + 3] = ymax;

//             ++validCount;
//         }
//     }

//     quick_sort_indice_inverse(props, 0, validCount - 1, filter_indices);
//     nms(validCount, loc_fp32, filter_indices, 0.2, app_ctx->model_width, app_ctx->model_height);

//     uint8_t num_face_count = 0;
//     for (int i = 0; i < validCount; ++i) {
//         if (num_face_count >= 128) {
//             printf("Warning: detected more than 128 faces, cannot handle that\n");
//             break;
//         }
//         if (filter_indices[i] == -1 || props[i] < 0.5) {
//             continue;
//         }

//         int n = filter_indices[i];
//         float x1 = loc_fp32[n * 4 + 0] * app_ctx->model_width;
//         float y1 = loc_fp32[n * 4 + 1] * app_ctx->model_height;
//         float x2 = loc_fp32[n * 4 + 2] * app_ctx->model_width;
//         float y2 = loc_fp32[n * 4 + 3] * app_ctx->model_height;

//         od_results->results[num_face_count].box.left   = clamp(x1, 0, app_ctx->model_width);
//         od_results->results[num_face_count].box.top    = clamp(y1, 0, app_ctx->model_height);
//         od_results->results[num_face_count].box.right  = clamp(x2, 0, app_ctx->model_width);
//         od_results->results[num_face_count].box.bottom = clamp(y2, 0, app_ctx->model_height);
//         od_results->results[num_face_count].prop = props[i];
//         num_face_count++;
//     }

//     od_results->count = num_face_count;

//     // Free allocated memory
//     if (loc_fp32 != NULL) {
//         free(loc_fp32);
//     }
//     if (dst_image.virt_addr != NULL) {
//         free(dst_image.virt_addr);
//     }

//     return 0;
// }

// int inference_facel_model(rknn_app_context_t *app_ctx, image_buffer_t* face_img, point_t* landmarks) {
//     int ret;

//     // Convert face image to the required format (192x192, NCHW)
//     image_buffer_t dst_image;
//     memset(&dst_image, 0, sizeof(image_buffer_t));

//     // Set up the destination image
//     dst_image.width = app_ctx->model_width;   // Should be 192
//     dst_image.height = app_ctx->model_height; // Should be 192
//     dst_image.format = IMAGE_FORMAT_RGB888;   // Set a valid format
//     dst_image.size = dst_image.width * dst_image.height * 3; // 192 * 192 * 3 = 110592
//     printf("Allocating dst_image.virt_addr: size=%zu\n", dst_image.size);
//     dst_image.virt_addr = (unsigned char*)malloc(dst_image.size);
//     if (dst_image.virt_addr == NULL) {
//         printf("Failed to allocate memory for dst_image\n");
//         return -1;
//     }

//     // Define source and destination rectangles
//     image_rect_t src_box = {0, 0, face_img->width, face_img->height};
//     image_rect_t dst_box = {0, 0, app_ctx->model_width, app_ctx->model_height};

//     ret = convert_image(face_img, &dst_image, &src_box, &dst_box, 0);
//     if (ret != 0) {
//         printf("convert image fail! ret=%d\n", ret);
//         if (dst_image.virt_addr != NULL) {
//             free(dst_image.virt_addr);
//         }
//         return -1;
//     }

//     // Copy image data to RKNN input memory (convert from RGB to NCHW)
//     unsigned char* input_data = (unsigned char*)app_ctx->input_mems[0]->virt_addr;
//     for (int c = 0; c < 3; c++) { // NCHW: channels first
//         for (int h = 0; h < dst_image.height; h++) {
//             for (int w = 0; w < dst_image.width; w++) {
//                 int src_idx = (h * dst_image.width + w) * 3 + c; // RGB
//                 int dst_idx = c * (dst_image.width * dst_image.height) + h * dst_image.width + w; // NCHW
//                 input_data[dst_idx] = dst_image.virt_addr[src_idx];
//             }
//         }
//     }

//     ret = rknn_run(app_ctx->rknn_ctx, nullptr);
//     if (ret < 0) {
//         printf("rknn_run fail! ret=%d\n", ret);
//         if (dst_image.virt_addr != NULL) {
//             free(dst_image.virt_addr);
//         }
//         return -1;
//     }

//     uint8_t *landmarks_data = (uint8_t *)(app_ctx->output_mems[0]->virt_addr); // conv2d_21: [1, 1, 1404, 1]
//     uint8_t *confidences = (uint8_t *)(app_ctx->output_mems[1]->virt_addr);    // conv2d_31: [1, 1, 1, 1]

//     int landmarks_zp = app_ctx->output_attrs[0].zp;
//     float landmarks_scale = app_ctx->output_attrs[0].scale;
//     int conf_zp = app_ctx->output_attrs[1].zp;
//     float conf_scale = app_ctx->output_attrs[1].scale;

//     // Assuming 468 points, each with x, y, z coordinates (1404 / 3 = 468 points)
//     for (int i = 0; i < 468; i++) {
//         float conf = deqnt_affine_to_f32(confidences[0], conf_zp, conf_scale); // Single confidence value
//         if (conf < 0.5) continue; // Skip low-confidence points

//         int offset = i * 3; // x, y, z coordinates
//         float x = deqnt_affine_to_f32(landmarks_data[offset + 0], landmarks_zp, landmarks_scale);
//         float y = deqnt_affine_to_f32(landmarks_data[offset + 1], landmarks_zp, landmarks_scale);
//         // Ignore z coordinate for now

//         landmarks[i].x = clamp(x * app_ctx->model_width, 0, app_ctx->model_width);
//         landmarks[i].y = clamp(y * app_ctx->model_height, 0, app_ctx->model_height);
//     }

//     // Free the converted image buffer
//     if (dst_image.virt_addr != NULL) {
//         free(dst_image.virt_addr);
//     }

//     return 0;
// }

// void mapCoordinates(image_buffer_t* input, image_buffer_t* output, int *x, int *y) {
//     float scaleX = (float)output->width / (float)input->width;
//     float scaleY = (float)output->height / (float)input->height;
//     *x = (int)((float)*x / scaleX);
//     *y = (int)((float)*y / scaleY);
// }

// int crop_image(image_buffer_t* src_image, image_buffer_t* dst_image, int x, int y, int width, int height) {
//     image_rect_t src_box = {x, y, x + width, y + height};
//     image_rect_t dst_box = {0, 0, width, height};
//     return convert_image(src_image, dst_image, &src_box, &dst_box, 0);
// }

// // Fallback implementation of write_data_to_file
// int write_data_to_file(const char *path, const void *data, size_t size) {
//     FILE *fp = fopen(path, "wb");
//     if (fp == NULL) {
//         printf("Failed to open file %s for writing\n", path);
//         return -1;
//     }
//     size_t written = fwrite(data, 1, size, fp);
//     fclose(fp);
//     if (written != size) {
//         printf("Failed to write %zu bytes to file %s, wrote %zu\n", size, path, written);
//         return -1;
//     }
//     return 0;
// }


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "faced_facel.h"
#include "faced_box_priors.h"

#define USE_RKNN_MEM_SHARE 0

int clamp(float x, int min, int max) {
    if (x > max) return max;
    if (x < min) return min;
    return x;
}

static void dump_tensor_attr(rknn_tensor_attr *attr) {
    printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
           "zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3],
           attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
           get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

int init_faced_model(const char *model_path, rknn_app_context_t *app_ctx) {
    int ret;
    rknn_context ctx = 0;

    ret = rknn_init(&ctx, (char *)model_path, 0, 0, NULL);
    if (ret < 0) {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }

    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC) {
        printf("rknn_query fail! ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

    printf("input tensors:\n");
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++) {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_NATIVE_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs[i]));
    }

    printf("output tensors:\n");
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++) {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_attrs[i]));
    }

    input_attrs[0].type = RKNN_TENSOR_UINT8;
    input_attrs[0].fmt = RKNN_TENSOR_NHWC;

    printf("input_attrs[0].size_with_stride=%d\n", input_attrs[0].size_with_stride);
    app_ctx->input_mems[0] = rknn_create_mem(ctx, input_attrs[0].size_with_stride);

    ret = rknn_set_io_mem(ctx, app_ctx->input_mems[0], &input_attrs[0]);
    if (ret < 0) {
        printf("input_mems rknn_set_io_mem fail! ret=%d\n", ret);
        return -1;
    }

    for (uint32_t i = 0; i < io_num.n_output; ++i) {
        app_ctx->output_mems[i] = rknn_create_mem(ctx, output_attrs[i].size_with_stride);
        printf("output mem [%d] = %d \n", i, output_attrs[i].size_with_stride);
        ret = rknn_set_io_mem(ctx, app_ctx->output_mems[i], &output_attrs[i]);
        if (ret < 0) {
            printf("output_mems rknn_set_io_mem fail! ret=%d\n", ret);
            return -1;
        }
    }

    app_ctx->rknn_ctx = ctx;
    if (output_attrs[0].qnt_type == RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC) {
        app_ctx->is_quant = true;
    } else {
        app_ctx->is_quant = false;
    }

    app_ctx->io_num = io_num;
    app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

    printf("model is NHWC input fmt\n");
    app_ctx->model_height  = input_attrs[0].dims[1]; // H = 128
    app_ctx->model_width   = input_attrs[0].dims[2]; // W = 128
    app_ctx->model_channel = input_attrs[0].dims[3]; // C = 3

    printf("model input height=%d, width=%d, channel=%d\n",
           app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);

    return 0;
}

int init_facel_model(const char *model_path, rknn_app_context_t *app_ctx) {
    int ret;
    rknn_context ctx = 0;

    ret = rknn_init(&ctx, (char *)model_path, 0, 0, NULL);
    if (ret < 0) {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }

    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC) {
        printf("rknn_query fail! ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

    printf("input tensors:\n");
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++) {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_NATIVE_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs[i]));
    }

    printf("output tensors:\n");
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++) {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_attrs[i]));
    }

    input_attrs[0].type = RKNN_TENSOR_UINT8;
    input_attrs[0].fmt = RKNN_TENSOR_NHWC;

    printf("input_attrs[0].size_with_stride=%d\n", input_attrs[0].size_with_stride);
    app_ctx->input_mems[0] = rknn_create_mem(ctx, input_attrs[0].size_with_stride);

    ret = rknn_set_io_mem(ctx, app_ctx->input_mems[0], &input_attrs[0]);
    if (ret < 0) {
        printf("input_mems rknn_set_io_mem fail! ret=%d\n", ret);
        return -1;
    }

    for (uint32_t i = 0; i < io_num.n_output; ++i) {
        app_ctx->output_mems[i] = rknn_create_mem(ctx, output_attrs[i].size_with_stride);
        printf("output mem [%d] = %d \n", i, output_attrs[i].size_with_stride);
        ret = rknn_set_io_mem(ctx, app_ctx->output_mems[i], &output_attrs[i]);
        if (ret < 0) {
            printf("output_mems rknn_set_io_mem fail! ret=%d\n", ret);
            return -1;
        }
    }

    app_ctx->rknn_ctx = ctx;
    if (output_attrs[0].qnt_type == RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC) {
        app_ctx->is_quant = true;
    } else {
        app_ctx->is_quant = false;
    }

    app_ctx->io_num = io_num;
    app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

    printf("model is NHWC input fmt\n");
    app_ctx->model_height  = input_attrs[0].dims[1]; // H = 192
    app_ctx->model_width   = input_attrs[0].dims[2]; // W = 192
    app_ctx->model_channel = input_attrs[0].dims[3]; // C = 3

    printf("model input height=%d, width=%d, channel=%d\n",
           app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);

    return 0;
}

int init_faced_facel_model(const char *faced_model_path, const char *facel_model_path, rknn_app_context_t *app_faced_ctx, rknn_app_context_t *app_facel_ctx) {
    printf("init faced\n");
    int ret = init_faced_model(faced_model_path, app_faced_ctx);
    if (ret != 0) {
        printf("init_faced_model fail! ret=%d\n", ret);
        return -1;
    }

    printf("init facel\n");
    ret = init_facel_model(facel_model_path, app_facel_ctx);
    if (ret != 0) {
        printf("init_facel_model fail! ret=%d\n", ret);
        return -1;
    }

    printf("Init success\n");
    return 0;
}

int release_faced_facel_model(rknn_app_context_t *app_faced_ctx, rknn_app_context_t *app_facel_ctx) {
    if (app_faced_ctx->rknn_ctx != 0) {
        rknn_destroy(app_faced_ctx->rknn_ctx);
        app_faced_ctx->rknn_ctx = 0;
    }
    if (app_faced_ctx->input_attrs != NULL) {
        free(app_faced_ctx->input_attrs);
        app_faced_ctx->input_attrs = NULL;
    }
    if (app_faced_ctx->output_attrs != NULL) {
        free(app_faced_ctx->output_attrs);
        app_faced_ctx->output_attrs = NULL;
    }
    for (int i = 0; i < app_faced_ctx->io_num.n_input; i++) {
        if (app_faced_ctx->input_mems[i] != NULL) {
            rknn_destroy_mem(app_faced_ctx->rknn_ctx, app_faced_ctx->input_mems[i]);
            free(app_faced_ctx->input_mems[i]);
        }
    }
    for (int i = 0; i < app_faced_ctx->io_num.n_output; i++) {
        if (app_faced_ctx->output_mems[i] != NULL) {
            rknn_destroy_mem(app_faced_ctx->rknn_ctx, app_faced_ctx->output_mems[i]);
            free(app_faced_ctx->output_mems[i]);
        }
    }

    if (app_facel_ctx->rknn_ctx != 0) {
        rknn_destroy(app_facel_ctx->rknn_ctx);
        app_facel_ctx->rknn_ctx = 0;
    }
    if (app_facel_ctx->input_attrs != NULL) {
        free(app_facel_ctx->input_attrs);
        app_facel_ctx->input_attrs = NULL;
    }
    if (app_facel_ctx->output_attrs != NULL) {
        free(app_facel_ctx->output_attrs);
        app_facel_ctx->output_attrs = NULL;
    }
    for (int i = 0; i < app_facel_ctx->io_num.n_input; i++) {
        if (app_facel_ctx->input_mems[i] != NULL) {
            rknn_destroy_mem(app_facel_ctx->rknn_ctx, app_facel_ctx->input_mems[i]);
            free(app_facel_ctx->input_mems[i]);
        }
    }
    for (int i = 0; i < app_facel_ctx->io_num.n_output; i++) {
        if (app_facel_ctx->output_mems[i] != NULL) {
            rknn_destroy_mem(app_facel_ctx->rknn_ctx, app_facel_ctx->output_mems[i]);
            free(app_facel_ctx->output_mems[i]);
        }
    }

    return 0;
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
            input[high] = input[low];
            indices[high] = indices[low];
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

static float deqnt_affine_to_f32(int8_t qnt, int32_t zp, float scale) {
    return ((float)qnt - (float)zp) * scale;
}

int inference_faced_model(rknn_app_context_t *app_ctx, image_buffer_t* src_image, object_detect_result_list *od_results) {
    int ret;

    // Debug: Print source image details
    printf("Source image: width=%d, height=%d, size=%zu, format=%d\n", 
           src_image->width, src_image->height, src_image->size, src_image->format);

    // Fix source image dimensions to match the expected values (640x427)
    if (src_image->width != 640 || src_image->height != 427) {
        printf("Warning: Source image dimensions do not match expected values (640x427). Adjusting...\n");
        src_image->width = 640;
        src_image->height = 427;
        src_image->size = src_image->width * src_image->height * 3;
    }

    // Convert image to RGB if it's in YCbCr format (to fix RGA error)
    if (src_image->format != IMAGE_FORMAT_RGB888) {
        printf("Converting YCbCr to RGB\n");
        unsigned char* rgb_data = (unsigned char*)malloc(src_image->width * src_image->height * 3);
        if (rgb_data == NULL) {
            printf("Failed to allocate memory for RGB conversion\n");
            return -1;
        }
        // YCbCr to RGB conversion (4:2:0 subsampling)
        for (int i = 0; i < src_image->width * src_image->height; i++) {
            int y_idx = i;
            int uv_idx = src_image->width * src_image->height + (i / 4);
            int y = src_image->virt_addr[y_idx];
            int cb = src_image->virt_addr[uv_idx];
            int cr = src_image->virt_addr[src_image->width * src_image->height * 5 / 4 + (i / 4)];
            int r = y + (int)(1.402 * (cr - 128));
            int g = y - (int)(0.344 * (cb - 128)) - (int)(0.714 * (cr - 128));
            int b = y + (int)(1.772 * (cb - 128));
            rgb_data[i * 3 + 0] = (unsigned char)clamp(r, 0, 255);
            rgb_data[i * 3 + 1] = (unsigned char)clamp(g, 0, 255);
            rgb_data[i * 3 + 2] = (unsigned char)clamp(b, 0, 255);
        }
        // Free the original buffer and update src_image
        free(src_image->virt_addr);
        src_image->virt_addr = rgb_data;
        src_image->size = src_image->width * src_image->height * 3;
        src_image->format = IMAGE_FORMAT_RGB888;
    }

    // Debug: Verify source image after conversion
    printf("After conversion - Source image: width=%d, height=%d, size=%zu, format=%d\n", 
           src_image->width, src_image->height, src_image->size, src_image->format);

    // Convert image to the required format (128x128, NHWC)
    image_buffer_t dst_image;
    memset(&dst_image, 0, sizeof(image_buffer_t));

    // Set up the destination image
    dst_image.width = app_ctx->model_width;   // Should be 128
    dst_image.height = app_ctx->model_height; // Should be 128
    dst_image.format = IMAGE_FORMAT_RGB888;
    dst_image.size = dst_image.width * dst_image.height * 3; // 128 * 128 * 3 = 49152
    printf("Allocating dst_image.virt_addr: size=%zu\n", dst_image.size);
    dst_image.virt_addr = (unsigned char*)malloc(dst_image.size);
    if (dst_image.virt_addr == NULL) {
        printf("Failed to allocate memory for dst_image\n");
        return -1;
    }

    // Debug: Print destination image details
    printf("Destination image: width=%d, height=%d, size=%zu, format=%d\n", 
           dst_image.width, dst_image.height, dst_image.size, dst_image.format);

    // Define source and destination rectangles
    image_rect_t src_box = {0, 0, src_image->width, src_image->height};
    image_rect_t dst_box = {0, 0, app_ctx->model_width, app_ctx->model_height};

    ret = convert_image(src_image, &dst_image, &src_box, &dst_box, 0);
    if (ret != 0) {
        printf("convert image fail! ret=%d\n", ret);
        if (dst_image.virt_addr != NULL) {
            free(dst_image.virt_addr);
        }
        return -1;
    }

    // Verify pixel values are in the range [0, 255]
    for (int i = 0; i < dst_image.size; i++) {
        if (dst_image.virt_addr[i] > 255 || dst_image.virt_addr[i] < 0) {
            printf("Pixel value out of range at index %d: %d\n", i, dst_image.virt_addr[i]);
            dst_image.virt_addr[i] = (unsigned char)clamp(dst_image.virt_addr[i], 0, 255);
        }
    }

    // Copy image data to RKNN input memory (direct copy since both are NHWC)
    unsigned char* input_data = (unsigned char*)app_ctx->input_mems[0]->virt_addr;
    memcpy(input_data, dst_image.virt_addr, dst_image.size);

    ret = rknn_run(app_ctx->rknn_ctx, nullptr);
    if (ret < 0) {
        printf("rknn_run fail! ret=%d\n", ret);
        if (dst_image.virt_addr != NULL) {
            free(dst_image.virt_addr);
        }
        return -1;
    }

    uint8_t *locations = (uint8_t *)(app_ctx->output_mems[0]->virt_addr); // norm_tensor1: [1, 896, 16]
    uint8_t *scores = (uint8_t *)(app_ctx->output_mems[1]->virt_addr);   // norm_tensor2: [1, 896, 1]

    const float (*prior_ptr)[4];
    int num_priors = 896;
    prior_ptr = BOX_PRIORS_128;

    int filter_indices[num_priors];
    float props[num_priors];
    uint32_t location_size = app_ctx->output_mems[0]->size / sizeof(uint8_t);
    printf("Allocating loc_fp32: size=%zu\n", location_size * sizeof(float));
    float *loc_fp32 = (float*)malloc(location_size * sizeof(float));
    if (loc_fp32 == NULL) {
        printf("Failed to allocate memory for loc_fp32\n");
        if (dst_image.virt_addr != NULL) {
            free(dst_image.virt_addr);
        }
        return -1;
    }
    memset(loc_fp32, 0, location_size * sizeof(float));
    memset(filter_indices, 0, sizeof(int) * num_priors);
    memset(props, 0, sizeof(float) * num_priors);

    int validCount = 0;
    const float VARIANCES[2] = {0.1, 0.2};

    int loc_zp = app_ctx->output_attrs[0].zp;
    float loc_scale = app_ctx->output_attrs[0].scale;
    int scores_zp = app_ctx->output_attrs[1].zp;
    float scores_scale = app_ctx->output_attrs[1].scale;

    // Decode bounding boxes
    for (int i = 0; i < num_priors; i++) {
        float face_score = deqnt_affine_to_f32(scores[i], scores_zp, scores_scale);
        if (face_score > 0.5) {
            filter_indices[validCount] = i;
            props[validCount] = face_score;
            int offset = i * 16;
            uint8_t *bbox = locations + offset;

            float dx = deqnt_affine_to_f32(bbox[0], loc_zp, loc_scale);
            float dy = deqnt_affine_to_f32(bbox[1], loc_zp, loc_scale);
            float dw = deqnt_affine_to_f32(bbox[2], loc_zp, loc_scale);
            float dh = deqnt_affine_to_f32(bbox[3], loc_zp, loc_scale);

            float box_x = dx * VARIANCES[0] * prior_ptr[i][2] + prior_ptr[i][0];
            float box_y = dy * VARIANCES[0] * prior_ptr[i][3] + prior_ptr[i][1];
            float box_w = expf(dw * VARIANCES[1]) * prior_ptr[i][2];
            float box_h = expf(dh * VARIANCES[1]) * prior_ptr[i][3];

            float xmin = box_x - box_w * 0.5f;
            float ymin = box_y - box_h * 0.5f;
            float xmax = xmin + box_w;
            float ymax = ymin + box_h;

            loc_fp32[i * 4 + 0] = xmin;
            loc_fp32[i * 4 + 1] = ymin;
            loc_fp32[i * 4 + 2] = xmax;
            loc_fp32[i * 4 + 3] = ymax;

            ++validCount;
        }
    }

    quick_sort_indice_inverse(props, 0, validCount - 1, filter_indices);
    nms(validCount, loc_fp32, filter_indices, 0.2, app_ctx->model_width, app_ctx->model_height);

    uint8_t num_face_count = 0;
    for (int i = 0; i < validCount; ++i) {
        if (num_face_count >= 128) {
            printf("Warning: detected more than 128 faces, cannot handle that\n");
            break;
        }
        if (filter_indices[i] == -1 || props[i] < 0.5) {
            continue;
        }

        int n = filter_indices[i];
        float x1 = loc_fp32[n * 4 + 0] * app_ctx->model_width;
        float y1 = loc_fp32[n * 4 + 1] * app_ctx->model_height;
        float x2 = loc_fp32[n * 4 + 2] * app_ctx->model_width;
        float y2 = loc_fp32[n * 4 + 3] * app_ctx->model_height;

        od_results->results[num_face_count].box.left   = clamp(x1, 0, app_ctx->model_width);
        od_results->results[num_face_count].box.top    = clamp(y1, 0, app_ctx->model_height);
        od_results->results[num_face_count].box.right  = clamp(x2, 0, app_ctx->model_width);
        od_results->results[num_face_count].box.bottom = clamp(y2, 0, app_ctx->model_height);
        od_results->results[num_face_count].prop = props[i];
        num_face_count++;
    }

    od_results->count = num_face_count;

    // Free allocated memory
    if (loc_fp32 != NULL) {
        free(loc_fp32);
    }
    if (dst_image.virt_addr != NULL) {
        free(dst_image.virt_addr);
    }

    return 0;
}

int inference_facel_model(rknn_app_context_t *app_ctx, image_buffer_t* face_img, point_t* landmarks) {
    int ret;

    // Debug: Print face image details
    printf("Face image: width=%d, height=%d, size=%zu, format=%d\n", 
           face_img->width, face_img->height, face_img->size, face_img->format);

    // Convert image to RGB if it's in YCbCr format
    if (face_img->format != IMAGE_FORMAT_RGB888) {
        printf("Converting YCbCr to RGB for face image\n");
        unsigned char* rgb_data = (unsigned char*)malloc(face_img->width * face_img->height * 3);
        if (rgb_data == NULL) {
            printf("Failed to allocate memory for RGB conversion\n");
            return -1;
        }
        for (int i = 0; i < face_img->width * face_img->height; i++) {
            int y_idx = i;
            int uv_idx = face_img->width * face_img->height + (i / 4);
            int y = face_img->virt_addr[y_idx];
            int cb = face_img->virt_addr[uv_idx];
            int cr = face_img->virt_addr[face_img->width * face_img->height * 5 / 4 + (i / 4)];
            int r = y + (int)(1.402 * (cr - 128));
            int g = y - (int)(0.344 * (cb - 128)) - (int)(0.714 * (cr - 128));
            int b = y + (int)(1.772 * (cb - 128));
            rgb_data[i * 3 + 0] = (unsigned char)clamp(r, 0, 255);
            rgb_data[i * 3 + 1] = (unsigned char)clamp(g, 0, 255);
            rgb_data[i * 3 + 2] = (unsigned char)clamp(b, 0, 255);
        }
        free(face_img->virt_addr);
        face_img->virt_addr = rgb_data;
        face_img->size = face_img->width * face_img->height * 3;
        face_img->format = IMAGE_FORMAT_RGB888;
    }

    // Debug: Verify face image after conversion
    printf("After conversion - Face image: width=%d, height=%d, size=%zu, format=%d\n", 
           face_img->width, face_img->height, face_img->size, face_img->format);

    // Convert face image to the required format (192x192, NHWC)
    image_buffer_t dst_image;
    memset(&dst_image, 0, sizeof(image_buffer_t));

    // Set up the destination image
    dst_image.width = app_ctx->model_width;   // Should be 192
    dst_image.height = app_ctx->model_height; // Should be 192
    dst_image.format = IMAGE_FORMAT_RGB888;
    dst_image.size = dst_image.width * dst_image.height * 3; // 192 * 192 * 3 = 110592
    printf("Allocating dst_image.virt_addr: size=%zu\n", dst_image.size);
    dst_image.virt_addr = (unsigned char*)malloc(dst_image.size);
    if (dst_image.virt_addr == NULL) {
        printf("Failed to allocate memory for dst_image\n");
        return -1;
    }

    // Debug: Print destination image details
    printf("Destination image: width=%d, height=%d, size=%zu, format=%d\n", 
           dst_image.width, dst_image.height, dst_image.size, dst_image.format);

    // Define source and destination rectangles
    image_rect_t src_box = {0, 0, face_img->width, face_img->height};
    image_rect_t dst_box = {0, 0, app_ctx->model_width, app_ctx->model_height};

    ret = convert_image(face_img, &dst_image, &src_box, &dst_box, 0);
    if (ret != 0) {
        printf("convert image fail! ret=%d\n", ret);
        if (dst_image.virt_addr != NULL) {
            free(dst_image.virt_addr);
        }
        return -1;
    }

    // Verify pixel values are in the range [0, 255]
    for (int i = 0; i < dst_image.size; i++) {
        if (dst_image.virt_addr[i] > 255 || dst_image.virt_addr[i] < 0) {
            printf("Pixel value out of range at index %d: %d\n", i, dst_image.virt_addr[i]);
            dst_image.virt_addr[i] = (unsigned char)clamp(dst_image.virt_addr[i], 0, 255);
        }
    }

    // Copy image data to RKNN input memory (direct copy since both are NHWC)
    unsigned char* input_data = (unsigned char*)app_ctx->input_mems[0]->virt_addr;
    memcpy(input_data, dst_image.virt_addr, dst_image.size);

    ret = rknn_run(app_ctx->rknn_ctx, nullptr);
    if (ret < 0) {
        printf("rknn_run fail! ret=%d\n", ret);
        if (dst_image.virt_addr != NULL) {
            free(dst_image.virt_addr);
        }
        return -1;
    }

    uint8_t *landmarks_data = (uint8_t *)(app_ctx->output_mems[0]->virt_addr); // conv2d_21: [1, 1, 1404, 1]
    uint8_t *confidences = (uint8_t *)(app_ctx->output_mems[1]->virt_addr);    // conv2d_31: [1, 1, 1, 1]

    int landmarks_zp = app_ctx->output_attrs[0].zp;
    float landmarks_scale = app_ctx->output_attrs[0].scale;
    int conf_zp = app_ctx->output_attrs[1].zp;
    float conf_scale = app_ctx->output_attrs[1].scale;

    // Assuming 468 points, each with x, y, z coordinates (1404 / 3 = 468 points)
    for (int i = 0; i < 468; i++) {
        float conf = deqnt_affine_to_f32(confidences[0], conf_zp, conf_scale);
        if (conf < 0.5) continue;

        int offset = i * 3;
        float x = deqnt_affine_to_f32(landmarks_data[offset + 0], landmarks_zp, landmarks_scale);
        float y = deqnt_affine_to_f32(landmarks_data[offset + 1], landmarks_zp, landmarks_scale);

        landmarks[i].x = clamp(x * app_ctx->model_width, 0, app_ctx->model_width);
        landmarks[i].y = clamp(y * app_ctx->model_height, 0, app_ctx->model_height);
    }

    // Free the converted image buffer
    if (dst_image.virt_addr != NULL) {
        free(dst_image.virt_addr);
    }

    return 0;
}

void mapCoordinates(image_buffer_t* input, image_buffer_t* output, int *x, int *y) {
    float scaleX = (float)output->width / (float)input->width;
    float scaleY = (float)output->height / (float)input->height;
    *x = (int)((float)*x / scaleX);
    *y = (int)((float)*y / scaleY);
}

int crop_image(image_buffer_t* src_image, image_buffer_t* dst_image, int x, int y, int width, int height) {
    image_rect_t src_box = {x, y, x + width, y + height};
    image_rect_t dst_box = {0, 0, width, height};
    return convert_image(src_image, dst_image, &src_box, &dst_box, 0);
}

int write_data_to_file(const char *path, const void *data, size_t size) {
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        printf("Failed to open file %s for writing\n", path);
        return -1;
    }
    size_t written = fwrite(data, 1, size, fp);
    fclose(fp);
    if (written != size) {
        printf("Failed to write %zu bytes to file %s, wrote %zu\n", size, path, written);
        return -1;
    }
    return 0;
}