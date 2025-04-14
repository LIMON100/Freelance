// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <math.h>
// #include <vector>
// #include <algorithm> // For std::min, std::max, std::sort
// #include <cmath>     // For expf, roundf

// // Include RKNN API first
// #include "rknn_api.h"

// // Include common utilities
// #include "common.h"      // For get_qnt_type_string etc. & letterbox_t
// #include "file_utils.h"  // For read_data_from_file
// #include "image_utils.h" // For image operations (crop, letterbox) & IMAGE_FORMAT_*

// // Include the header for this specific implementation last
// #include "face_analyzer.h"

// // --- Constants (Copied from previous version, adjust if needed) ---
// #define NMS_THRESHOLD 0.55
// #define FACE_CONF_THRESHOLD 0.5
// #define BOX_SCALE_X 1.5 // Scaling for face crop for face landmark model
// #define BOX_SCALE_Y 1.7 // Scaling for face crop for face landmark model
// #define EYE_CROP_SCALE 1.8 // Scaling factor for eye crop relative to eye landmarks
// #define IRIS_CROP_SCALE 2.8 // Scaling factor for iris crop relative to iris center (if needed, currently uses EYE_CROP_SCALE)

// // --- Anchor Generation Structs and Function (Copied from previous version) ---
// struct AnchorOptions {
//     int input_size_width = 128; int input_size_height = 128;
//     std::vector<int> strides = {8, 16}; int num_layers = 2;
//     std::vector<int> anchors_per_location = {2, 6};
//     float anchor_offset_x = 0.5f; float anchor_offset_y = 0.5f;
//     float fixed_anchor_size = 1.0f;
// };
// struct Anchor { float x_center, y_center, h, w; };

// static std::vector<Anchor> generate_anchors(const AnchorOptions& options) {
//     std::vector<Anchor> anchors;
//     int layer_id = 0;
//     while (layer_id < options.num_layers) {
//         int feature_map_height = ceil((float)options.input_size_height / options.strides[layer_id]);
//         int feature_map_width = ceil((float)options.input_size_width / options.strides[layer_id]);
//         int anchors_per_loc = options.anchors_per_location[layer_id];

//         for (int y = 0; y < feature_map_height; ++y) {
//             for (int x = 0; x < feature_map_width; ++x) {
//                 for (int anchor_id = 0; anchor_id < anchors_per_loc; ++anchor_id) {
//                     float x_center = (x + options.anchor_offset_x) * (1.0f / feature_map_width);
//                     float y_center = (y + options.anchor_offset_y) * (1.0f / feature_map_height);
//                     float w = options.fixed_anchor_size;
//                     float h = options.fixed_anchor_size;
//                     anchors.push_back({x_center, y_center, h, w});
//                 }
//             }
//         }
//         layer_id++;
//     }
//     // Ensure the number matches the model output (e.g., 896)
//     if (anchors.size() != 896) {
//          printf("WARN: Generated anchor count (%zu) != expected (896)!\n", anchors.size());
//          // Depending on the model, this might be an error or expected.
//          // If it's an error, return an empty vector or handle appropriately.
//     }
//     return anchors;
// }
// // --- End Anchor Generation ---


// // --- Utility Functions (Copied/Adapted) ---
// static inline float dequantize_int8_to_float(int8_t val, int32_t zp, float scale) {
//     return ((float)val - (float)zp) * scale;
// }

// static int clamp(int x, int min_val, int max_val) {
//     return std::max(min_val, std::min(x, max_val));
// }

// static float clampf(float x, float min_val, float max_val) {
//      return std::max(min_val, std::min(x, max_val));
// }

// static void dump_tensor_attr(rknn_tensor_attr *attr, const char* model_name) {
//     // Simple version - uncomment more details if needed
//     // printf("%s Idx:%d Name:%s Dims:[%d,%d,%d,%d] Fmt:%s Type:%s Qnt:%s ZP:%d Scale:%.3f\n",
//     //     model_name, attr->index, attr->name,
//     //     attr->n_dims > 0 ? attr->dims[0] : -1,
//     //     attr->n_dims > 1 ? attr->dims[1] : -1,
//     //     attr->n_dims > 2 ? attr->dims[2] : -1,
//     //     attr->n_dims > 3 ? attr->dims[3] : -1,
//     //     get_format_string(attr->fmt), get_type_string(attr->type),
//     //     get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
// }

// // Simple crop (assumes continuous buffer, only works for specific formats)
// // Consider using a more robust version from image_utils if available/needed
// static int crop_image_simple(image_buffer_t *src_img, image_buffer_t *dst_img, box_rect_t crop_box) {
//     if (!src_img || !src_img->virt_addr || !dst_img) return -1;

//     int channels = 0;
//     if (src_img->format == IMAGE_FORMAT_RGB888) channels = 3;
// #ifdef IMAGE_FORMAT_BGR888
//     else if (src_img->format == IMAGE_FORMAT_BGR888) channels = 3;
// #endif
//     else return -1; // Only support RGB/BGR for this simple version

//     int src_w = src_img->width;
//     int src_h = src_img->height;
//     int crop_x = crop_box.left;
//     int crop_y = crop_box.top;
//     int crop_w = crop_box.right - crop_box.left;
//     int crop_h = crop_box.bottom - crop_box.top;

//     // Basic validation
//     if (crop_w <= 0 || crop_h <= 0 || crop_x >= src_w || crop_y >= src_h) {
//         printf("ERROR: Invalid crop ROI [%d, %d, %d, %d] for source image %dx%d\n",
//                crop_box.left, crop_box.top, crop_box.right, crop_box.bottom, src_w, src_h);
//         return -1;
//     }

//     // Adjust ROI to fit within source bounds
//     int x_start = std::max(0, crop_x);
//     int y_start = std::max(0, crop_y);
//     int x_end = std::min(src_w, crop_x + crop_w);
//     int y_end = std::min(src_h, crop_y + crop_h);

//     int valid_crop_w = x_end - x_start;
//     int valid_crop_h = y_end - y_start;

//     if (valid_crop_w <= 0 || valid_crop_h <= 0) {
//          printf("ERROR: Adjusted crop ROI has zero size.\n");
//          return -1;
//     }

//     dst_img->width = crop_w; // Destination retains original requested crop size
//     dst_img->height = crop_h;
//     dst_img->format = src_img->format;
//     dst_img->size = crop_w * crop_h * channels;

//     // Allocate destination buffer if not already allocated (or reallocate if size differs?)
//     // For simplicity, assume caller manages allocation or it's allocated once.
//     // If virt_addr is NULL, allocate it.
//     if (dst_img->virt_addr == NULL) {
//         dst_img->virt_addr = (unsigned char*)malloc(dst_img->size);
//         if (!dst_img->virt_addr) {
//             printf("ERROR: Failed to allocate memory for cropped image (%d bytes)\n", dst_img->size);
//             return -1;
//         }
//     } else if (dst_img->size < (size_t)(crop_w * crop_h * channels)) {
//          printf("ERROR: Provided destination buffer too small for crop.\n");
//          // Or realloc? For now, treat as error.
//          return -1;
//     }


//     // Clear destination buffer (e.g., black background)
//     memset(dst_img->virt_addr, 0, dst_img->size);

//     unsigned char* src_data = src_img->virt_addr;
//     unsigned char* dst_data = dst_img->virt_addr;
//     size_t src_stride = src_img->width_stride ? src_img->width_stride : (size_t)src_w * channels; // Use width_stride if available
//     size_t dst_stride = (size_t)crop_w * channels;

//     // Calculate where the valid source data should be placed in the destination
//     int dst_x_offset = x_start - crop_x; // Offset within the destination buffer
//     int dst_y_offset = y_start - crop_y;

//     // Copy valid region row by row
//     for (int y = 0; y < valid_crop_h; ++y) {
//         unsigned char* src_row_ptr = src_data + (size_t)(y_start + y) * src_stride + (size_t)x_start * channels;
//         unsigned char* dst_row_ptr = dst_data + (size_t)(dst_y_offset + y) * dst_stride + (size_t)dst_x_offset * channels;
//         memcpy(dst_row_ptr, src_row_ptr, (size_t)valid_crop_w * channels);
//     }

//     return 0;
// }


// static float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1, float ymax1) {
//     float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1)); // Width requires no +1 for IoU
//     float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1)); // Height requires no +1 for IoU
//     float intersection = w * h;
//     float area0 = (xmax0 - xmin0) * (ymax0 - ymin0);
//     float area1 = (xmax1 - xmin1) * (ymax1 - ymin1);
//     float union_area = area0 + area1 - intersection;
//     return union_area <= 0.f ? 0.f : (intersection / union_area);
// }

// struct DetectionCandidate {
//     int index;
//     float score;
//     box_rect_t box;
//     // Removed operator< as we sort using lambda now
// };

// static void nms(std::vector<DetectionCandidate>& candidates, float threshold) {
//     if (candidates.empty()) return;
//     // Sort candidates by score in descending order
//     std::sort(candidates.begin(), candidates.end(), [](const DetectionCandidate& a, const DetectionCandidate& b) {
//         return a.score > b.score;
//     });

//     std::vector<bool> removed(candidates.size(), false);
//     std::vector<DetectionCandidate> result_candidates;
//     result_candidates.reserve(candidates.size()); // Reserve space

//     for (size_t i = 0; i < candidates.size(); ++i) {
//         if (removed[i]) continue;
//         result_candidates.push_back(candidates[i]); // Keep this candidate

//         // Suppress overlapping boxes with lower scores
//         for (size_t j = i + 1; j < candidates.size(); ++j) {
//             if (removed[j]) continue;
//             float iou = CalculateOverlap(
//                 (float)candidates[i].box.left, (float)candidates[i].box.top,
//                 (float)candidates[i].box.right, (float)candidates[i].box.bottom,
//                 (float)candidates[j].box.left, (float)candidates[j].box.top,
//                 (float)candidates[j].box.right, (float)candidates[j].box.bottom
//             );
//             if (iou > threshold) {
//                 removed[j] = true;
//             }
//         }
//     }
//     candidates = std::move(result_candidates); // Efficiently replace with results
// }
// // --- End Utility Functions ---


// // --- Post Processing Functions (Modified for Zero-Copy) ---
// static int post_process_face_detection(
//     rknn_tensor_mem* output_mems[],
//     face_analyzer_app_context_t *app_ctx,
//     const letterbox_t *det_letter_box,
//     int src_img_width, int src_img_height,
//     std::vector<DetectionCandidate>& out_faces)
// {
//     AnchorOptions anchor_opts;
//     anchor_opts.input_size_width = app_ctx->model1_width;
//     anchor_opts.input_size_height = app_ctx->model1_height;
//     std::vector<Anchor> anchors = generate_anchors(anchor_opts);

//     if (app_ctx->io_num_detect.n_output < 2) { /* error */ return -1; }

//     rknn_tensor_attr* box_attr   = &app_ctx->output_attrs_detect[0];
//     rknn_tensor_attr* score_attr = &app_ctx->output_attrs_detect[1];

//     // Basic validation
//     if (box_attr->type != RKNN_TENSOR_INT8 || score_attr->type != RKNN_TENSOR_INT8 ||
//         box_attr->dims[1] != score_attr->dims[1]) // Check if anchor count matches
//     {
//         printf("ERROR: Detection output type/dims mismatch.\n");
//         return -1;
//     }

//     int num_anchors = box_attr->dims[1];
//     int box_regressors_per_anchor = box_attr->dims[box_attr->n_dims - 1]; // Usually 16

//     if (anchors.size() != num_anchors) { /* error */ return -1; }

//     int8_t* box_data   = (int8_t*)output_mems[0]->virt_addr;
//     int8_t* score_data = (int8_t*)output_mems[1]->virt_addr;
//     if (!box_data || !score_data) {
//         printf("ERROR: Output memory virtual address is NULL in post_process_face_detection.\n");
//         return -1; // Critical error
//     }

//     float box_scale   = box_attr->scale;
//     int32_t box_zp    = box_attr->zp;
//     float score_scale = score_attr->scale;
//     int32_t score_zp  = score_attr->zp;

//     std::vector<DetectionCandidate> candidates;
//     candidates.reserve(num_anchors / 4);

//     const float detection_size_w = (float)app_ctx->model1_width;
//     const float detection_size_h = (float)app_ctx->model1_height;

//     for (int i = 0; i < num_anchors; ++i) {
//         float logit = dequantize_int8_to_float(score_data[i], score_zp, score_scale);
//         float score = 1.0f / (1.0f + expf(-logit));

//         if (score >= FACE_CONF_THRESHOLD) {
//             int box_offset = i * box_regressors_per_anchor;
//             float x_offset = dequantize_int8_to_float(box_data[box_offset + 0], box_zp, box_scale);
//             float y_offset = dequantize_int8_to_float(box_data[box_offset + 1], box_zp, box_scale);
//             float w_scale  = dequantize_int8_to_float(box_data[box_offset + 2], box_zp, box_scale);
//             float h_scale  = dequantize_int8_to_float(box_data[box_offset + 3], box_zp, box_scale);

//             const Anchor& anchor = anchors[i];
//             float pred_x_center = (x_offset / detection_size_w * anchor.w) + anchor.x_center;
//             float pred_y_center = (y_offset / detection_size_h * anchor.h) + anchor.y_center;
//             float pred_w = (w_scale / detection_size_w) * anchor.w;
//             float pred_h = (h_scale / detection_size_h) * anchor.h;

//             float ymin_norm = pred_y_center - pred_h / 2.0f;
//             float xmin_norm = pred_x_center - pred_w / 2.0f;
//             float ymax_norm = pred_y_center + pred_h / 2.0f;
//             float xmax_norm = pred_x_center + pred_w / 2.0f;

//             ymin_norm = clampf(ymin_norm, 0.0f, 1.0f); xmin_norm = clampf(xmin_norm, 0.0f, 1.0f);
//             ymax_norm = clampf(ymax_norm, 0.0f, 1.0f); xmax_norm = clampf(xmax_norm, 0.0f, 1.0f);

//             float xmin_unscaled = (xmin_norm * detection_size_w - det_letter_box->x_pad) / det_letter_box->scale;
//             float ymin_unscaled = (ymin_norm * detection_size_h - det_letter_box->y_pad) / det_letter_box->scale;
//             float xmax_unscaled = (xmax_norm * detection_size_w - det_letter_box->x_pad) / det_letter_box->scale;
//             float ymax_unscaled = (ymax_norm * detection_size_h - det_letter_box->y_pad) / det_letter_box->scale;

//             int xmin = clamp((int)roundf(xmin_unscaled), 0, src_img_width - 1);
//             int ymin = clamp((int)roundf(ymin_unscaled), 0, src_img_height - 1);
//             int xmax = clamp((int)roundf(xmax_unscaled), 0, src_img_width - 1);
//             int ymax = clamp((int)roundf(ymax_unscaled), 0, src_img_height - 1);

//             if (xmax > xmin && ymax > ymin) {
//                 DetectionCandidate cand; cand.index = i; cand.score = score;
//                 cand.box.left = xmin; cand.box.top = ymin; cand.box.right = xmax; cand.box.bottom = ymax;
//                 candidates.push_back(cand);
//             }
//         }
//     }

//     nms(candidates, NMS_THRESHOLD);
//     out_faces = std::move(candidates);
//     return 0;
// }

// // Updated to read from rknn_tensor_mem* landmark_mem
// static int post_process_face_landmarks(
//     rknn_tensor_mem* landmark_mem,
//     face_analyzer_app_context_t *app_ctx,
//     const box_rect_t& crop_roi,
//     const letterbox_t *lmk_letter_box,
//     point_t out_landmarks[NUM_FACE_LANDMARKS])
// {
//     rknn_tensor_attr* landmark_attr = &app_ctx->output_attrs_landmark[0];
//     if (landmark_attr->type != RKNN_TENSOR_INT8) { /* error */ return -1; }

//     bool valid_dims = false; int num_elements = 0;
//     if (landmark_attr->n_dims == 4 && landmark_attr->dims[0] == 1) { num_elements = landmark_attr->dims[1] * landmark_attr->dims[2] * landmark_attr->dims[3]; valid_dims = (num_elements == NUM_FACE_LANDMARKS * 3); }
//     else if (landmark_attr->n_dims == 2 && landmark_attr->dims[0] == 1) { num_elements = landmark_attr->dims[1]; valid_dims = (num_elements == NUM_FACE_LANDMARKS * 3); }
//     if (!valid_dims) { /* error */ return -1; }

//     int8_t* landmark_data = (int8_t*)landmark_mem->virt_addr;
//     if (!landmark_data) { printf("ERROR: Landmark output virt_addr is NULL.\n"); return -1;}

//     float landmark_scale = landmark_attr->scale;
//     int32_t landmark_zp  = landmark_attr->zp;

//     float crop_roi_w = (float)(crop_roi.right - crop_roi.left);
//     float crop_roi_h = (float)(crop_roi.bottom - crop_roi.top);
//     if (crop_roi_w <= 0 || crop_roi_h <= 0) { /* error */ return -1; }

//     for (int i = 0; i < NUM_FACE_LANDMARKS; ++i) {
//         int offset = i * 3;
//         float x_lmk_model = dequantize_int8_to_float(landmark_data[offset + 0], landmark_zp, landmark_scale);
//         float y_lmk_model = dequantize_int8_to_float(landmark_data[offset + 1], landmark_zp, landmark_scale);

//         float x_unpadded = (x_lmk_model - lmk_letter_box->x_pad);
//         float y_unpadded = (y_lmk_model - lmk_letter_box->y_pad);
//         float x_orig_crop_rel = x_unpadded / lmk_letter_box->scale;
//         float y_orig_crop_rel = y_unpadded / lmk_letter_box->scale;
//         float final_x = x_orig_crop_rel + crop_roi.left;
//         float final_y = y_orig_crop_rel + crop_roi.top;

//         out_landmarks[i].x = (int)roundf(final_x);
//         out_landmarks[i].y = (int)roundf(final_y);
//     }
//     return 0;
// }

// // Updated to read from rknn_tensor_mem* iris_output_mems[]
// static int post_process_iris_landmarks(
//     rknn_tensor_mem* iris_output_mems[],
//     face_analyzer_app_context_t *app_ctx,
//     const box_rect_t& eye_crop_roi,
//     const letterbox_t *iris_letter_box,
//     point_t out_eye_contour[NUM_EYE_CONTOUR_LANDMARKS],
//     point_t out_iris[NUM_IRIS_LANDMARKS])
// {
//      if (app_ctx->io_num_iris.n_output < 2) { /* error */ return -1; }
//      rknn_tensor_attr* eye_contour_attr = &app_ctx->output_attrs_iris[0];
//      rknn_tensor_attr* iris_pts_attr    = &app_ctx->output_attrs_iris[1];

//      bool eye_dims_ok = (eye_contour_attr->n_dims == 4 && eye_contour_attr->n_elems == NUM_EYE_CONTOUR_LANDMARKS * 3) || (eye_contour_attr->n_dims == 2 && eye_contour_attr->n_elems == NUM_EYE_CONTOUR_LANDMARKS * 3);
//      bool iris_dims_ok = (iris_pts_attr->n_dims == 4 && iris_pts_attr->n_elems == NUM_IRIS_LANDMARKS * 3) || (iris_pts_attr->n_dims == 2 && iris_pts_attr->n_elems == NUM_IRIS_LANDMARKS * 3);
//      if (!eye_dims_ok || !iris_dims_ok || eye_contour_attr->type != RKNN_TENSOR_INT8 || iris_pts_attr->type != RKNN_TENSOR_INT8) { /* error */ return -1; }

//      int8_t* eye_data = (int8_t*)iris_output_mems[0]->virt_addr;
//      if (!eye_data) { printf("ERROR: Eye contour output virt_addr is NULL.\n"); return -1; }
//      float eye_scale  = eye_contour_attr->scale;
//      int32_t eye_zp   = eye_contour_attr->zp;

//      int8_t* iris_data = (int8_t*)iris_output_mems[1]->virt_addr;
//       if (!iris_data) { printf("ERROR: Iris points output virt_addr is NULL.\n"); return -1; }
//      float iris_scale = iris_pts_attr->scale;
//      int32_t iris_zp  = iris_pts_attr->zp;

//      float eye_crop_roi_w = (float)(eye_crop_roi.right - eye_crop_roi.left);
//      float eye_crop_roi_h = (float)(eye_crop_roi.bottom - eye_crop_roi.top);
//      if (eye_crop_roi_w <= 0 || eye_crop_roi_h <= 0) { /* error */ return -1; }

//      // Process Eye Contour Landmarks
//      for (int i = 0; i < NUM_EYE_CONTOUR_LANDMARKS; ++i) {
//          int offset = i * 3;
//          float x_eye_model = dequantize_int8_to_float(eye_data[offset + 0], eye_zp, eye_scale);
//          float y_eye_model = dequantize_int8_to_float(eye_data[offset + 1], eye_zp, eye_scale);

//          float x_unpadded = (x_eye_model - iris_letter_box->x_pad);
//          float y_unpadded = (y_eye_model - iris_letter_box->y_pad);
//          float x_orig_eye_rel = x_unpadded / iris_letter_box->scale;
//          float y_orig_eye_rel = y_unpadded / iris_letter_box->scale;
//          float final_x = x_orig_eye_rel + eye_crop_roi.left;
//          float final_y = y_orig_eye_rel + eye_crop_roi.top;
//          out_eye_contour[i].x = (int)roundf(final_x);
//          out_eye_contour[i].y = (int)roundf(final_y);
//      }

//     // Process Iris Center Landmarks
//      for (int i = 0; i < NUM_IRIS_LANDMARKS; ++i) {
//          int offset = i * 3;
//          float x_iris_model = dequantize_int8_to_float(iris_data[offset + 0], iris_zp, iris_scale);
//          float y_iris_model = dequantize_int8_to_float(iris_data[offset + 1], iris_zp, iris_scale);

//          float x_unpadded = (x_iris_model - iris_letter_box->x_pad);
//          float y_unpadded = (y_iris_model - iris_letter_box->y_pad);
//          float x_orig_eye_rel = x_unpadded / iris_letter_box->scale;
//          float y_orig_eye_rel = y_unpadded / iris_letter_box->scale;
//          float final_x = x_orig_eye_rel + eye_crop_roi.left;
//          float final_y = y_orig_eye_rel + eye_crop_roi.top;
//          out_iris[i].x = (int)roundf(final_x);
//          out_iris[i].y = (int)roundf(final_y);
//      }

//     return 0;
// }
// // --- End Post Processing Functions ---

// // // --- Model Initialization and Release ---
// int init_face_analyzer(const char *detection_model_path,
//                        const char* landmark_model_path,
//                        const char* iris_model_path,
//                        face_analyzer_app_context_t *app_ctx) {
//     memset(app_ctx, 0, sizeof(face_analyzer_app_context_t));
//     int ret;
//     int model_len = 0;
//     unsigned char *model_buf = NULL;

//     // --- Init Detect Model ---
//     printf("Init Face Detect Model: %s\n", detection_model_path);
//     model_len = read_data_from_file(detection_model_path, reinterpret_cast<char**>(&model_buf));
//     if (!model_buf) { printf("ERROR: read_data_from_file failed for detect model\n"); return -1; }
//     ret = rknn_init(&app_ctx->rknn_ctx_detect, model_buf, model_len, 0, NULL);
//     free(model_buf); model_buf = NULL;
//     if (ret < 0) { printf("ERROR: rknn_init(detect) failed! ret=%d\n", ret); return -1; }
//     ret = rknn_set_core_mask(app_ctx->rknn_ctx_detect, RKNN_NPU_CORE_0);
//     if(ret < 0) { printf("WARN: rknn_set_core_mask(detect, CORE_0) failed! ret=%d\n", ret); }
//     else { printf("INFO: Face Detection Model assigned to NPU Core 0.\n"); }
//     ret = rknn_query(app_ctx->rknn_ctx_detect, RKNN_QUERY_IN_OUT_NUM, &app_ctx->io_num_detect, sizeof(app_ctx->io_num_detect));
//     if (ret != RKNN_SUCC) { goto cleanup_detect; }
//     app_ctx->input_attrs_detect = (rknn_tensor_attr*)malloc(app_ctx->io_num_detect.n_input * sizeof(rknn_tensor_attr));
//     if (!app_ctx->input_attrs_detect) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_detect; }
//     memset(app_ctx->input_attrs_detect, 0, app_ctx->io_num_detect.n_input * sizeof(rknn_tensor_attr));
//     for (uint32_t i = 0; i < app_ctx->io_num_detect.n_input; i++) { app_ctx->input_attrs_detect[i].index = i; ret = rknn_query(app_ctx->rknn_ctx_detect, RKNN_QUERY_NATIVE_INPUT_ATTR, &app_ctx->input_attrs_detect[i], sizeof(rknn_tensor_attr)); if (ret != RKNN_SUCC) goto cleanup_detect; dump_tensor_attr(&app_ctx->input_attrs_detect[i], "DetectIn(Native)"); }
//     app_ctx->output_attrs_detect = (rknn_tensor_attr*)malloc(app_ctx->io_num_detect.n_output * sizeof(rknn_tensor_attr));
//     if (!app_ctx->output_attrs_detect) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_detect; }
//     memset(app_ctx->output_attrs_detect, 0, app_ctx->io_num_detect.n_output * sizeof(rknn_tensor_attr));
//     for (uint32_t i = 0; i < app_ctx->io_num_detect.n_output; i++) { app_ctx->output_attrs_detect[i].index = i; ret = rknn_query(app_ctx->rknn_ctx_detect, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &app_ctx->output_attrs_detect[i], sizeof(rknn_tensor_attr)); if (ret != RKNN_SUCC) goto cleanup_detect; dump_tensor_attr(&app_ctx->output_attrs_detect[i], "DetectOut(Native)"); }
//     if (app_ctx->io_num_detect.n_input > 0) { app_ctx->input_attrs_detect[0].type = RKNN_TENSOR_UINT8; app_ctx->input_attrs_detect[0].fmt = RKNN_TENSOR_NHWC; app_ctx->input_fmt_detect = RKNN_TENSOR_NHWC; }
//     app_ctx->input_mems_detect = (rknn_tensor_mem**)malloc(sizeof(rknn_tensor_mem*) * app_ctx->io_num_detect.n_input);
//     if(!app_ctx->input_mems_detect){ ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_detect; } memset(app_ctx->input_mems_detect, 0, sizeof(rknn_tensor_mem*) * app_ctx->io_num_detect.n_input);
//     for(uint32_t i = 0; i < app_ctx->io_num_detect.n_input; ++i) { app_ctx->input_mems_detect[i] = rknn_create_mem(app_ctx->rknn_ctx_detect, app_ctx->input_attrs_detect[i].size_with_stride); if(!app_ctx->input_mems_detect[i]) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_detect; } ret = rknn_set_io_mem(app_ctx->rknn_ctx_detect, app_ctx->input_mems_detect[i], &app_ctx->input_attrs_detect[i]); if (ret < 0) goto cleanup_detect; }
//     app_ctx->output_mems_detect = (rknn_tensor_mem**)malloc(sizeof(rknn_tensor_mem*) * app_ctx->io_num_detect.n_output);
//     if(!app_ctx->output_mems_detect){ ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_detect; } memset(app_ctx->output_mems_detect, 0, sizeof(rknn_tensor_mem*) * app_ctx->io_num_detect.n_output);
//     for(uint32_t i = 0; i < app_ctx->io_num_detect.n_output; ++i) { app_ctx->output_mems_detect[i] = rknn_create_mem(app_ctx->rknn_ctx_detect, app_ctx->output_attrs_detect[i].size_with_stride); if(!app_ctx->output_mems_detect[i]) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_detect; } ret = rknn_set_io_mem(app_ctx->rknn_ctx_detect, app_ctx->output_mems_detect[i], &app_ctx->output_attrs_detect[i]); if (ret < 0) goto cleanup_detect; }
//     app_ctx->model1_height = app_ctx->input_attrs_detect[0].dims[1]; app_ctx->model1_width = app_ctx->input_attrs_detect[0].dims[2]; app_ctx->model1_channel = app_ctx->input_attrs_detect[0].dims[3];

//     // --- Init Landmark Model ---
//     printf("Init Face Landmark Model: %s\n", landmark_model_path);
//     model_len = read_data_from_file(landmark_model_path, reinterpret_cast<char**>(&model_buf));
//     if (!model_buf) { ret = RKNN_ERR_MODEL_INVALID; goto cleanup_detect; }
//     ret = rknn_init(&app_ctx->rknn_ctx_landmark, model_buf, model_len, 0, NULL);
//     free(model_buf); model_buf = NULL;
//     if (ret < 0) { goto cleanup_detect; }
//     ret = rknn_set_core_mask(app_ctx->rknn_ctx_landmark, RKNN_NPU_CORE_0);
//     if(ret < 0) { printf("WARN: rknn_set_core_mask(landmark, CORE_0) failed! ret=%d\n", ret); } else { printf("INFO: Face Landmark Model assigned to NPU Core 0.\n"); }
//     ret = rknn_query(app_ctx->rknn_ctx_landmark, RKNN_QUERY_IN_OUT_NUM, &app_ctx->io_num_landmark, sizeof(app_ctx->io_num_landmark));
//     if (ret != RKNN_SUCC) { goto cleanup_landmark; }
//     app_ctx->input_attrs_landmark = (rknn_tensor_attr*)malloc(app_ctx->io_num_landmark.n_input * sizeof(rknn_tensor_attr));
//     if (!app_ctx->input_attrs_landmark) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_landmark; }
//     memset(app_ctx->input_attrs_landmark, 0, app_ctx->io_num_landmark.n_input * sizeof(rknn_tensor_attr));
//     for (uint32_t i = 0; i < app_ctx->io_num_landmark.n_input; i++) { app_ctx->input_attrs_landmark[i].index = i; ret = rknn_query(app_ctx->rknn_ctx_landmark, RKNN_QUERY_NATIVE_INPUT_ATTR, &app_ctx->input_attrs_landmark[i], sizeof(rknn_tensor_attr)); if (ret != RKNN_SUCC) goto cleanup_landmark; dump_tensor_attr(&app_ctx->input_attrs_landmark[i], "LmkIn(Native)"); }
//     app_ctx->output_attrs_landmark = (rknn_tensor_attr*)malloc(app_ctx->io_num_landmark.n_output * sizeof(rknn_tensor_attr));
//     if (!app_ctx->output_attrs_landmark) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_landmark; }
//     memset(app_ctx->output_attrs_landmark, 0, app_ctx->io_num_landmark.n_output * sizeof(rknn_tensor_attr));
//     for (uint32_t i = 0; i < app_ctx->io_num_landmark.n_output; i++) { app_ctx->output_attrs_landmark[i].index = i; ret = rknn_query(app_ctx->rknn_ctx_landmark, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &app_ctx->output_attrs_landmark[i], sizeof(rknn_tensor_attr)); if (ret != RKNN_SUCC) goto cleanup_landmark; dump_tensor_attr(&app_ctx->output_attrs_landmark[i], "LmkOut(Native)"); }
//     if (app_ctx->io_num_landmark.n_input > 0) { app_ctx->input_attrs_landmark[0].type = RKNN_TENSOR_UINT8; app_ctx->input_attrs_landmark[0].fmt = RKNN_TENSOR_NHWC; app_ctx->input_fmt_landmark = RKNN_TENSOR_NHWC; }
//     app_ctx->input_mems_landmark = (rknn_tensor_mem**)malloc(sizeof(rknn_tensor_mem*) * app_ctx->io_num_landmark.n_input);
//     if(!app_ctx->input_mems_landmark){ ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_landmark; } memset(app_ctx->input_mems_landmark, 0, sizeof(rknn_tensor_mem*) * app_ctx->io_num_landmark.n_input);
//     for(uint32_t i = 0; i < app_ctx->io_num_landmark.n_input; ++i) { app_ctx->input_mems_landmark[i] = rknn_create_mem(app_ctx->rknn_ctx_landmark, app_ctx->input_attrs_landmark[i].size_with_stride); if(!app_ctx->input_mems_landmark[i]) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_landmark; } ret = rknn_set_io_mem(app_ctx->rknn_ctx_landmark, app_ctx->input_mems_landmark[i], &app_ctx->input_attrs_landmark[i]); if (ret < 0) goto cleanup_landmark; }
//     app_ctx->output_mems_landmark = (rknn_tensor_mem**)malloc(sizeof(rknn_tensor_mem*) * app_ctx->io_num_landmark.n_output);
//     if(!app_ctx->output_mems_landmark){ ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_landmark; } memset(app_ctx->output_mems_landmark, 0, sizeof(rknn_tensor_mem*) * app_ctx->io_num_landmark.n_output);
//     for(uint32_t i = 0; i < app_ctx->io_num_landmark.n_output; ++i) { app_ctx->output_mems_landmark[i] = rknn_create_mem(app_ctx->rknn_ctx_landmark, app_ctx->output_attrs_landmark[i].size_with_stride); if(!app_ctx->output_mems_landmark[i]) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_landmark; } ret = rknn_set_io_mem(app_ctx->rknn_ctx_landmark, app_ctx->output_mems_landmark[i], &app_ctx->output_attrs_landmark[i]); if (ret < 0) goto cleanup_landmark; }
//     app_ctx->model2_height = app_ctx->input_attrs_landmark[0].dims[1]; app_ctx->model2_width = app_ctx->input_attrs_landmark[0].dims[2]; app_ctx->model2_channel = app_ctx->input_attrs_landmark[0].dims[3];

//     // --- Init Iris Model ---
//     printf("Init Iris Landmark Model: %s\n", iris_model_path);
//     model_len = read_data_from_file(iris_model_path, reinterpret_cast<char**>(&model_buf));
//     if (!model_buf) { ret = RKNN_ERR_MODEL_INVALID; goto cleanup_landmark; }
//     ret = rknn_init(&app_ctx->rknn_ctx_iris, model_buf, model_len, 0, NULL);
//     free(model_buf); model_buf = NULL;
//     if (ret < 0) { goto cleanup_landmark; }
//     ret = rknn_set_core_mask(app_ctx->rknn_ctx_iris, RKNN_NPU_CORE_0);
//     if(ret < 0) { printf("WARN: rknn_set_core_mask(iris, CORE_0) failed! ret=%d\n", ret); } else { printf("INFO: Iris Landmark Model assigned to NPU Core 0.\n"); }
//     ret = rknn_query(app_ctx->rknn_ctx_iris, RKNN_QUERY_IN_OUT_NUM, &app_ctx->io_num_iris, sizeof(app_ctx->io_num_iris));
//     if (ret != RKNN_SUCC || app_ctx->io_num_iris.n_output < 2) { goto cleanup_iris; }
//     app_ctx->input_attrs_iris = (rknn_tensor_attr*)malloc(app_ctx->io_num_iris.n_input * sizeof(rknn_tensor_attr));
//     if (!app_ctx->input_attrs_iris) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_iris; }
//     memset(app_ctx->input_attrs_iris, 0, app_ctx->io_num_iris.n_input * sizeof(rknn_tensor_attr));
//     for (uint32_t i = 0; i < app_ctx->io_num_iris.n_input; i++) { app_ctx->input_attrs_iris[i].index = i; ret = rknn_query(app_ctx->rknn_ctx_iris, RKNN_QUERY_NATIVE_INPUT_ATTR, &app_ctx->input_attrs_iris[i], sizeof(rknn_tensor_attr)); if (ret != RKNN_SUCC) goto cleanup_iris; dump_tensor_attr(&app_ctx->input_attrs_iris[i], "IrisIn(Native)"); }
//     app_ctx->output_attrs_iris = (rknn_tensor_attr*)malloc(app_ctx->io_num_iris.n_output * sizeof(rknn_tensor_attr));
//     if (!app_ctx->output_attrs_iris) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_iris; }
//     memset(app_ctx->output_attrs_iris, 0, app_ctx->io_num_iris.n_output * sizeof(rknn_tensor_attr));
//     for (uint32_t i = 0; i < app_ctx->io_num_iris.n_output; i++) { app_ctx->output_attrs_iris[i].index = i; ret = rknn_query(app_ctx->rknn_ctx_iris, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &app_ctx->output_attrs_iris[i], sizeof(rknn_tensor_attr)); if (ret != RKNN_SUCC) goto cleanup_iris; dump_tensor_attr(&app_ctx->output_attrs_iris[i], "IrisOut(Native)"); }
//     if (app_ctx->io_num_iris.n_input > 0) { app_ctx->input_attrs_iris[0].type = RKNN_TENSOR_UINT8; app_ctx->input_attrs_iris[0].fmt = RKNN_TENSOR_NHWC; app_ctx->input_fmt_iris = RKNN_TENSOR_NHWC; }
//     app_ctx->input_mems_iris = (rknn_tensor_mem**)malloc(sizeof(rknn_tensor_mem*) * app_ctx->io_num_iris.n_input);
//     if(!app_ctx->input_mems_iris){ ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_iris; } memset(app_ctx->input_mems_iris, 0, sizeof(rknn_tensor_mem*) * app_ctx->io_num_iris.n_input);
//     for(uint32_t i = 0; i < app_ctx->io_num_iris.n_input; ++i) { app_ctx->input_mems_iris[i] = rknn_create_mem(app_ctx->rknn_ctx_iris, app_ctx->input_attrs_iris[i].size_with_stride); if(!app_ctx->input_mems_iris[i]) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_iris; } ret = rknn_set_io_mem(app_ctx->rknn_ctx_iris, app_ctx->input_mems_iris[i], &app_ctx->input_attrs_iris[i]); if (ret < 0) goto cleanup_iris; }
//     app_ctx->output_mems_iris = (rknn_tensor_mem**)malloc(sizeof(rknn_tensor_mem*) * app_ctx->io_num_iris.n_output);
//     if(!app_ctx->output_mems_iris){ ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_iris; } memset(app_ctx->output_mems_iris, 0, sizeof(rknn_tensor_mem*) * app_ctx->io_num_iris.n_output);
//     for(uint32_t i = 0; i < app_ctx->io_num_iris.n_output; ++i) { app_ctx->output_mems_iris[i] = rknn_create_mem(app_ctx->rknn_ctx_iris, app_ctx->output_attrs_iris[i].size_with_stride); if(!app_ctx->output_mems_iris[i]) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_iris; } ret = rknn_set_io_mem(app_ctx->rknn_ctx_iris, app_ctx->output_mems_iris[i], &app_ctx->output_attrs_iris[i]); if (ret < 0) goto cleanup_iris; }
//     app_ctx->model3_height = app_ctx->input_attrs_iris[0].dims[1]; app_ctx->model3_width = app_ctx->input_attrs_iris[0].dims[2]; app_ctx->model3_channel = app_ctx->input_attrs_iris[0].dims[3];

//     printf("INFO: Face Analyzer init successful.\n");
//     return 0; // Success

// // --- Cleanup Labels ---
// cleanup_iris:
//     if (app_ctx->rknn_ctx_iris != 0) rknn_destroy(app_ctx->rknn_ctx_iris);
//     if (app_ctx->input_attrs_iris) free(app_ctx->input_attrs_iris);
//     if (app_ctx->output_attrs_iris) free(app_ctx->output_attrs_iris);
//     if(app_ctx->input_mems_iris) { for(uint32_t i=0; i<app_ctx->io_num_iris.n_input; ++i) if(app_ctx->input_mems_iris[i]) rknn_destroy_mem(app_ctx->rknn_ctx_iris, app_ctx->input_mems_iris[i]); free(app_ctx->input_mems_iris); }
//     if(app_ctx->output_mems_iris) { for(uint32_t i=0; i<app_ctx->io_num_iris.n_output; ++i) if(app_ctx->output_mems_iris[i]) rknn_destroy_mem(app_ctx->rknn_ctx_iris, app_ctx->output_mems_iris[i]); free(app_ctx->output_mems_iris); }
//     // Fall through
// cleanup_landmark:
//     if (app_ctx->rknn_ctx_landmark != 0) rknn_destroy(app_ctx->rknn_ctx_landmark);
//     if (app_ctx->input_attrs_landmark) free(app_ctx->input_attrs_landmark);
//     if (app_ctx->output_attrs_landmark) free(app_ctx->output_attrs_landmark);
//     if(app_ctx->input_mems_landmark) { for(uint32_t i=0; i<app_ctx->io_num_landmark.n_input; ++i) if(app_ctx->input_mems_landmark[i]) rknn_destroy_mem(app_ctx->rknn_ctx_landmark, app_ctx->input_mems_landmark[i]); free(app_ctx->input_mems_landmark); }
//     if(app_ctx->output_mems_landmark) { for(uint32_t i=0; i<app_ctx->io_num_landmark.n_output; ++i) if(app_ctx->output_mems_landmark[i]) rknn_destroy_mem(app_ctx->rknn_ctx_landmark, app_ctx->output_mems_landmark[i]); free(app_ctx->output_mems_landmark); }
//     // Fall through
// cleanup_detect:
//     if (app_ctx->rknn_ctx_detect != 0) rknn_destroy(app_ctx->rknn_ctx_detect);
//     if (app_ctx->input_attrs_detect) free(app_ctx->input_attrs_detect);
//     if (app_ctx->output_attrs_detect) free(app_ctx->output_attrs_detect);
//     if(app_ctx->input_mems_detect) { for(uint32_t i=0; i<app_ctx->io_num_detect.n_input; ++i) if(app_ctx->input_mems_detect[i]) rknn_destroy_mem(app_ctx->rknn_ctx_detect, app_ctx->input_mems_detect[i]); free(app_ctx->input_mems_detect); }
//     if(app_ctx->output_mems_detect) { for(uint32_t i=0; i<app_ctx->io_num_detect.n_output; ++i) if(app_ctx->output_mems_detect[i]) rknn_destroy_mem(app_ctx->rknn_ctx_detect, app_ctx->output_mems_detect[i]); free(app_ctx->output_mems_detect); }
//     // Final cleanup
//     memset(app_ctx, 0, sizeof(face_analyzer_app_context_t));
//     printf("ERROR: init_face_analyzer failed during cleanup stage with ret=%d\n", ret);
//     return ret > 0 ? -ret : ret;
// }

// // --- Release Function (Zero-Copy Version) ---
// int release_face_analyzer(face_analyzer_app_context_t *app_ctx) {
//     // Release Iris resources
//     if(app_ctx->input_mems_iris) {
//         for(uint32_t i=0; i<app_ctx->io_num_iris.n_input; ++i) { if(app_ctx->input_mems_iris[i]) { rknn_destroy_mem(app_ctx->rknn_ctx_iris, app_ctx->input_mems_iris[i]); app_ctx->input_mems_iris[i] = NULL; } }
//         free(app_ctx->input_mems_iris); app_ctx->input_mems_iris = NULL;
//     }
//     if(app_ctx->output_mems_iris) {
//         for(uint32_t i=0; i<app_ctx->io_num_iris.n_output; ++i) { if(app_ctx->output_mems_iris[i]) { rknn_destroy_mem(app_ctx->rknn_ctx_iris, app_ctx->output_mems_iris[i]); app_ctx->output_mems_iris[i] = NULL; } }
//         free(app_ctx->output_mems_iris); app_ctx->output_mems_iris = NULL;
//     }
//     if (app_ctx->input_attrs_iris) { free(app_ctx->input_attrs_iris); app_ctx->input_attrs_iris = NULL; }
//     if (app_ctx->output_attrs_iris) { free(app_ctx->output_attrs_iris); app_ctx->output_attrs_iris = NULL; }
//     if (app_ctx->rknn_ctx_iris != 0) { rknn_destroy(app_ctx->rknn_ctx_iris); app_ctx->rknn_ctx_iris = 0; }

//     // Release Landmark resources
//      if(app_ctx->input_mems_landmark) {
//         for(uint32_t i=0; i<app_ctx->io_num_landmark.n_input; ++i) { if(app_ctx->input_mems_landmark[i]) { rknn_destroy_mem(app_ctx->rknn_ctx_landmark, app_ctx->input_mems_landmark[i]); app_ctx->input_mems_landmark[i] = NULL; } }
//         free(app_ctx->input_mems_landmark); app_ctx->input_mems_landmark = NULL;
//     }
//     if(app_ctx->output_mems_landmark) {
//         for(uint32_t i=0; i<app_ctx->io_num_landmark.n_output; ++i) { if(app_ctx->output_mems_landmark[i]) { rknn_destroy_mem(app_ctx->rknn_ctx_landmark, app_ctx->output_mems_landmark[i]); app_ctx->output_mems_landmark[i] = NULL; } }
//         free(app_ctx->output_mems_landmark); app_ctx->output_mems_landmark = NULL;
//     }
//     if (app_ctx->input_attrs_landmark) { free(app_ctx->input_attrs_landmark); app_ctx->input_attrs_landmark = NULL; }
//     if (app_ctx->output_attrs_landmark) { free(app_ctx->output_attrs_landmark); app_ctx->output_attrs_landmark = NULL; }
//     if (app_ctx->rknn_ctx_landmark != 0) { rknn_destroy(app_ctx->rknn_ctx_landmark); app_ctx->rknn_ctx_landmark = 0; }

//     // Release Detection resources
//      if(app_ctx->input_mems_detect) {
//         for(uint32_t i=0; i<app_ctx->io_num_detect.n_input; ++i) { if(app_ctx->input_mems_detect[i]) { rknn_destroy_mem(app_ctx->rknn_ctx_detect, app_ctx->input_mems_detect[i]); app_ctx->input_mems_detect[i] = NULL; } }
//         free(app_ctx->input_mems_detect); app_ctx->input_mems_detect = NULL;
//     }
//     if(app_ctx->output_mems_detect) {
//         for(uint32_t i=0; i<app_ctx->io_num_detect.n_output; ++i) { if(app_ctx->output_mems_detect[i]) { rknn_destroy_mem(app_ctx->rknn_ctx_detect, app_ctx->output_mems_detect[i]); app_ctx->output_mems_detect[i] = NULL; } }
//         free(app_ctx->output_mems_detect); app_ctx->output_mems_detect = NULL;
//     }
//     if (app_ctx->input_attrs_detect) { free(app_ctx->input_attrs_detect); app_ctx->input_attrs_detect = NULL; }
//     if (app_ctx->output_attrs_detect) { free(app_ctx->output_attrs_detect); app_ctx->output_attrs_detect = NULL; }
//     if (app_ctx->rknn_ctx_detect != 0) { rknn_destroy(app_ctx->rknn_ctx_detect); app_ctx->rknn_ctx_detect = 0; }

//     printf("INFO: Face Analyzer released successfully.\n");
//     return 0;
// }


// // --- Main Inference Function (Modified for Zero-Copy) ---
// int inference_face_analyzer(face_analyzer_app_context_t *app_ctx,
//                             image_buffer_t *src_img, // Source image (e.g., from camera)
//                             face_analyzer_result_t *out_result) {
//     int ret = 0;
//     memset(out_result, 0, sizeof(face_analyzer_result_t));
//     letterbox_t det_letter_box;
//     memset(&det_letter_box, 0, sizeof(letterbox_t));
//     int bg_color = 114;

//     // --- Stage 1: Face Detection ---
//     if (!app_ctx->input_mems_detect || !app_ctx->input_mems_detect[0] || !app_ctx->input_mems_detect[0]->virt_addr) {
//         printf("ERROR: Detection input memory not initialized.\n"); return -1;
//     }
//     image_buffer_t detect_input_buf_wrapper;
//     detect_input_buf_wrapper.width = app_ctx->model1_width;
//     detect_input_buf_wrapper.height = app_ctx->model1_height;
//     detect_input_buf_wrapper.format = IMAGE_FORMAT_RGB888; // Must match what letterbox outputs
//     detect_input_buf_wrapper.virt_addr = (unsigned char*)app_ctx->input_mems_detect[0]->virt_addr;
//     detect_input_buf_wrapper.size = app_ctx->input_mems_detect[0]->size;
//     detect_input_buf_wrapper.fd = app_ctx->input_mems_detect[0]->fd;
//     detect_input_buf_wrapper.width_stride = app_ctx->input_attrs_detect[0].w_stride;
//     detect_input_buf_wrapper.height_stride = app_ctx->input_attrs_detect[0].h_stride ? app_ctx->input_attrs_detect[0].h_stride : app_ctx->model1_height;

//     ret = convert_image_with_letterbox(src_img, &detect_input_buf_wrapper, &det_letter_box, bg_color);
//     if (ret < 0) { printf("ERROR: convert_image_with_letterbox (detect) failed! ret=%d\n", ret); return ret; }

//     // Sync input memory to device if rknn_mem_sync is available
// #ifdef RKNN_MEMORY_SYNC_TO_DEVICE
//     ret = rknn_mem_sync(app_ctx->rknn_ctx_detect, app_ctx->input_mems_detect[0], RKNN_MEMORY_SYNC_TO_DEVICE);
//     if (ret < 0) { printf("ERROR: rknn_mem_sync (detect input) failed! ret=%d\n", ret); return ret; }
// #else
//     // Fallback: No explicit sync, rely on driver default behavior
//     printf("INFO: rknn_mem_sync not available, skipping input sync for detection.\n");
// #endif

//     ret = rknn_run(app_ctx->rknn_ctx_detect, nullptr);
//     if (ret < 0) { printf("ERROR: rknn_run (detect) failed! ret=%d\n", ret); return ret; }

//     // Sync output memory from device if rknn_mem_sync is available
// #ifdef RKNN_MEMORY_SYNC_FROM_DEVICE
//     for (uint32_t i = 0; i < app_ctx->io_num_detect.n_output; ++i) {
//         ret = rknn_mem_sync(app_ctx->rknn_ctx_detect, app_ctx->output_mems_detect[i], RKNN_MEMORY_SYNC_FROM_DEVICE);
//         if (ret < 0) { printf("ERROR: rknn_mem_sync (detect output %u) failed! ret=%d\n", i, ret); return ret; }
//     }
// #else
//     // Fallback: No explicit sync, rely on driver default behavior
//     printf("INFO: rknn_mem_sync not available, skipping output sync for detection.\n");
// #endif

//     std::vector<DetectionCandidate> detected_faces;
//     ret = post_process_face_detection(app_ctx->output_mems_detect, app_ctx, &det_letter_box,
//                                       src_img->width, src_img->height, detected_faces);
//     if (ret < 0) { printf("ERROR: post_process_face_detection failed! ret=%d\n", ret); return ret; }

//     int num_faces_to_process = std::min((int)detected_faces.size(), MAX_FACE_RESULTS);
//     out_result->count = num_faces_to_process;
//     if (num_faces_to_process == 0) return 0;

//     // --- Setup Wrappers & Buffers for Loop ---
//     image_buffer_t landmark_input_buf_wrapper; // Wrapper for landmark NPU input
//     landmark_input_buf_wrapper.width = app_ctx->model2_width; 
//     landmark_input_buf_wrapper.height = app_ctx->model2_height; 
//     landmark_input_buf_wrapper.format = IMAGE_FORMAT_RGB888; 
//     landmark_input_buf_wrapper.virt_addr = (unsigned char*)app_ctx->input_mems_landmark[0]->virt_addr; 
//     landmark_input_buf_wrapper.size = app_ctx->input_mems_landmark[0]->size; 
//     landmark_input_buf_wrapper.fd = app_ctx->input_mems_landmark[0]->fd;
//     landmark_input_buf_wrapper.width_stride = app_ctx->input_attrs_landmark[0].w_stride; 
//     landmark_input_buf_wrapper.height_stride = app_ctx->input_attrs_landmark[0].h_stride ? app_ctx->input_attrs_landmark[0].h_stride : app_ctx->model2_height;

//     image_buffer_t iris_input_buf_wrapper; // Wrapper for iris NPU input
//     iris_input_buf_wrapper.width = app_ctx->model3_width; 
//     iris_input_buf_wrapper.height = app_ctx->model3_height; 
//     iris_input_buf_wrapper.format = IMAGE_FORMAT_RGB888; 
//     iris_input_buf_wrapper.virt_addr = (unsigned char*)app_ctx->input_mems_iris[0]->virt_addr; 
//     iris_input_buf_wrapper.size = app_ctx->input_mems_iris[0]->size; 
//     iris_input_buf_wrapper.fd = app_ctx->input_mems_iris[0]->fd;
//     iris_input_buf_wrapper.width_stride = app_ctx->input_attrs_iris[0].w_stride; 
//     iris_input_buf_wrapper.height_stride = app_ctx->input_attrs_iris[0].h_stride ? app_ctx->input_attrs_iris[0].h_stride : app_ctx->model3_height;

//     image_buffer_t cropped_img_cpu; 
//     memset(&cropped_img_cpu, 0, sizeof(image_buffer_t)); // Reusable temp CPU buffer

//     // --- Loop ---
//     for (int i = 0; i < num_faces_to_process; ++i) {
//         out_result->faces[i].box = detected_faces[i].box;
//         out_result->faces[i].score = detected_faces[i].score;
//         out_result->faces[i].face_landmarks_valid = false; 
//         out_result->faces[i].iris_landmarks_right_valid = false;

//         // --- Stage 2: Face Landmarks ---
//         letterbox_t lmk_letter_box; 
//         memset(&lmk_letter_box, 0, sizeof(letterbox_t));
//         box_rect_t face_crop_roi_lmk;
//         int center_x = (detected_faces[i].box.left + detected_faces[i].box.right) / 2; 
//         int center_y = (detected_faces[i].box.top + detected_faces[i].box.bottom) / 2; 
//         int box_w = detected_faces[i].box.right - detected_faces[i].box.left; 
//         int box_h = detected_faces[i].box.bottom - detected_faces[i].box.top; 
//         int crop_w_lmk = (int)(box_w * BOX_SCALE_X); 
//         int crop_h_lmk = (int)(box_h * BOX_SCALE_Y);
//         face_crop_roi_lmk.left = center_x - crop_w_lmk / 2; 
//         face_crop_roi_lmk.top = center_y - crop_h_lmk / 2; 
//         face_crop_roi_lmk.right = face_crop_roi_lmk.left + crop_w_lmk; 
//         face_crop_roi_lmk.bottom = face_crop_roi_lmk.top + crop_h_lmk;

//         cropped_img_cpu.virt_addr = NULL;
//         ret = crop_image_simple(src_img, &cropped_img_cpu, face_crop_roi_lmk);
//         if (ret < 0 || !cropped_img_cpu.virt_addr) { 
//             if(cropped_img_cpu.virt_addr) free(cropped_img_cpu.virt_addr); 
//             continue; 
//         }

//         ret = convert_image_with_letterbox(&cropped_img_cpu, &landmark_input_buf_wrapper, &lmk_letter_box, bg_color);
//         free(cropped_img_cpu.virt_addr); 
//         cropped_img_cpu.virt_addr = NULL;
//         if (ret < 0) { continue; }

//         // Sync landmark input memory to device if rknn_mem_sync is available
// #ifdef RKNN_MEMORY_SYNC_TO_DEVICE
//         ret = rknn_mem_sync(app_ctx->rknn_ctx_landmark, app_ctx->input_mems_landmark[0], RKNN_MEMORY_SYNC_TO_DEVICE);
//         if (ret < 0) { printf("ERROR: rknn_mem_sync (landmark input) failed! ret=%d\n", ret); continue; }
// #else
//         // Fallback: No explicit sync
//         printf("INFO: rknn_mem_sync not available, skipping input sync for landmarks.\n");
// #endif

//         ret = rknn_run(app_ctx->rknn_ctx_landmark, nullptr);
//         if (ret < 0) { continue; }

//         // Sync landmark output memory from device if rknn_mem_sync is available
// #ifdef RKNN_MEMORY_SYNC_FROM_DEVICE
//         ret = rknn_mem_sync(app_ctx->rknn_ctx_landmark, app_ctx->output_mems_landmark[0], RKNN_MEMORY_SYNC_FROM_DEVICE);
//         if (ret < 0) { printf("ERROR: rknn_mem_sync (landmark output) failed! ret=%d\n", ret); continue; }
// #else
//         // Fallback: No explicit sync
//         printf("INFO: rknn_mem_sync not available, skipping output sync for landmarks.\n");
// #endif

//         ret = post_process_face_landmarks(app_ctx->output_mems_landmark[0], app_ctx, face_crop_roi_lmk, &lmk_letter_box, out_result->faces[i].face_landmarks);
//         if (ret == 0) { out_result->faces[i].face_landmarks_valid = true; }
//         else { continue; } // Skip iris if landmarks failed

//         // --- Stage 3: Iris Landmarks ---
//         if (out_result->faces[i].face_landmarks_valid) {
//             letterbox_t iris_letter_box; // Reused for both eyes

//             // *** Define Eye ROI Landmark Indices HERE ***
//             const int LEFT_EYE_ROI_IDX1  = 33;  // Example: Left eye outer corner
//             const int LEFT_EYE_ROI_IDX2  = 133; // Example: Left eye inner corner
//             const int RIGHT_EYE_ROI_IDX1 = 362; // Example: Right eye inner corner
//             const int RIGHT_EYE_ROI_IDX2 = 263; // Example: Right eye outer corner
//             // *******************************************

//             // --- Process Left Eye ---
//             memset(&iris_letter_box, 0, sizeof(letterbox_t));
//             if (LEFT_EYE_ROI_IDX1 >= NUM_FACE_LANDMARKS || LEFT_EYE_ROI_IDX2 >= NUM_FACE_LANDMARKS) {
//                  printf("ERROR: Invalid LEFT eye landmark indices for ROI calculation.\n");
//             } else {
//                 point_t left_p1 = out_result->faces[i].face_landmarks[LEFT_EYE_ROI_IDX1];
//                 point_t left_p2 = out_result->faces[i].face_landmarks[LEFT_EYE_ROI_IDX2];
//                 int left_eye_cx = (left_p1.x + left_p2.x) / 2; 
//                 int left_eye_cy = (left_p1.y + left_p2.y) / 2;
//                 int left_eye_w = abs(left_p1.x - left_p2.x); 
//                 int left_eye_h = abs(left_p1.y - left_p2.y);
//                 int left_eye_size = (int)(std::max({left_eye_w, left_eye_h, 1}) * EYE_CROP_SCALE);
//                 box_rect_t left_eye_crop_roi;
//                 left_eye_crop_roi.left = left_eye_cx - left_eye_size / 2; 
//                 left_eye_crop_roi.top = left_eye_cy - left_eye_size / 2;
//                 left_eye_crop_roi.right = left_eye_crop_roi.left + left_eye_size; 
//                 left_eye_crop_roi.bottom = left_eye_crop_roi.top + left_eye_size;

//                 cropped_img_cpu.virt_addr = NULL;
//                 ret = crop_image_simple(src_img, &cropped_img_cpu, left_eye_crop_roi);
//                 if (ret == 0 && cropped_img_cpu.virt_addr) {
//                     ret = convert_image_with_letterbox(&cropped_img_cpu, &iris_input_buf_wrapper, &iris_letter_box, bg_color);
//                     free(cropped_img_cpu.virt_addr); 
//                     cropped_img_cpu.virt_addr = NULL;
//                     if (ret == 0) {
//                         // Sync iris input memory to device if rknn_mem_sync is available
// #ifdef RKNN_MEMORY_SYNC_TO_DEVICE
//                         ret = rknn_mem_sync(app_ctx->rknn_ctx_iris, app_ctx->input_mems_iris[0], RKNN_MEMORY_SYNC_TO_DEVICE);
//                         if (ret < 0) { printf("ERROR: rknn_mem_sync (iris input left) failed! ret=%d\n", ret); continue; }
// #else
//                         printf("INFO: rknn_mem_sync not available, skipping input sync for iris (left).\n");
// #endif

//                         ret = rknn_run(app_ctx->rknn_ctx_iris, nullptr);
//                         if (ret == 0) {
//                             // Sync iris output memory from device if rknn_mem_sync is available
// #ifdef RKNN_MEMORY_SYNC_FROM_DEVICE
//                             for (uint32_t j = 0; j < app_ctx->io_num_iris.n_output; ++j) {
//                                 ret = rknn_mem_sync(app_ctx->rknn_ctx_iris, app_ctx->output_mems_iris[j], RKNN_MEMORY_SYNC_FROM_DEVICE);
//                                 if (ret < 0) { printf("ERROR: rknn_mem_sync (iris output left %u) failed! ret=%d\n", j, ret); continue; }
//                             }
// #else
//                             printf("INFO: rknn_mem_sync not available, skipping output sync for iris (left).\n");
// #endif
//                             ret = post_process_iris_landmarks(app_ctx->output_mems_iris, app_ctx, left_eye_crop_roi, &iris_letter_box, 
//                                                               out_result->faces[i].eye_landmarks_left, out_result->faces[i].iris_landmarks_left);
//                             if (ret == 0) { 
//                                 out_result->faces[i].eye_landmarks_left_valid = true; 
//                                 out_result->faces[i].iris_landmarks_left_valid = true; 
//                             }
//                         }
//                     }
//                 } else { if (cropped_img_cpu.virt_addr) free(cropped_img_cpu.virt_addr); cropped_img_cpu.virt_addr = NULL;}
//             } // End left eye index check

//             // --- Process Right Eye ---
//             memset(&iris_letter_box, 0, sizeof(letterbox_t));
//             if (RIGHT_EYE_ROI_IDX1 >= NUM_FACE_LANDMARKS || RIGHT_EYE_ROI_IDX2 >= NUM_FACE_LANDMARKS) {
//                  printf("ERROR: Invalid RIGHT eye landmark indices for ROI calculation.\n");
//             } else {
//                 point_t right_p1 = out_result->faces[i].face_landmarks[RIGHT_EYE_ROI_IDX1];
//                 point_t right_p2 = out_result->faces[i].face_landmarks[RIGHT_EYE_ROI_IDX2];
//                 int right_eye_cx = (right_p1.x + right_p2.x) / 2; 
//                 int right_eye_cy = (right_p1.y + right_p2.y) / 2;
//                 int right_eye_w = abs(right_p1.x - right_p2.x); 
//                 int right_eye_h = abs(right_p1.y - right_p2.y);
//                 int right_eye_size = (int)(std::max({right_eye_w, right_eye_h, 1}) * EYE_CROP_SCALE);
//                 box_rect_t right_eye_crop_roi;
//                 right_eye_crop_roi.left = right_eye_cx - right_eye_size / 2; 
//                 right_eye_crop_roi.top = right_eye_cy - right_eye_size / 2;
//                 right_eye_crop_roi.right = right_eye_crop_roi.left + right_eye_size; 
//                 right_eye_crop_roi.bottom = right_eye_crop_roi.top + right_eye_size;

//                 cropped_img_cpu.virt_addr = NULL;
//                 ret = crop_image_simple(src_img, &cropped_img_cpu, right_eye_crop_roi);
//                 if (ret == 0 && cropped_img_cpu.virt_addr) {
//                     ret = convert_image_with_letterbox(&cropped_img_cpu, &iris_input_buf_wrapper, &iris_letter_box, bg_color);
//                     free(cropped_img_cpu.virt_addr); 
//                     cropped_img_cpu.virt_addr = NULL;
//                     if (ret == 0) {
//                         // Sync iris input memory to device if rknn_mem_sync is available
// #ifdef RKNN_MEMORY_SYNC_TO_DEVICE
//                         ret = rknn_mem_sync(app_ctx->rknn_ctx_iris, app_ctx->input_mems_iris[0], RKNN_MEMORY_SYNC_TO_DEVICE);
//                         if (ret < 0) { printf("ERROR: rknn_mem_sync (iris input right) failed! ret=%d\n", ret); continue; }
// #else
//                         printf("INFO: rknn_mem_sync not available, skipping input sync for iris (right).\n");
// #endif

//                         ret = rknn_run(app_ctx->rknn_ctx_iris, nullptr);
//                         if (ret == 0) {
//                             // Sync iris output memory from device if rknn_mem_sync is available
// #ifdef RKNN_MEMORY_SYNC_FROM_DEVICE
//                             for (uint32_t j = 0; j < app_ctx->io_num_iris.n_output; ++j) {
//                                 ret = rknn_mem_sync(app_ctx->rknn_ctx_iris, app_ctx->output_mems_iris[j], RKNN_MEMORY_SYNC_FROM_DEVICE);
//                                 if (ret < 0) { printf("ERROR: rknn_mem_sync (iris output right %u) failed! ret=%d\n", j, ret); continue; }
//                             }
// #else
//                             printf("INFO: rknn_mem_sync not available, skipping output sync for iris (right).\n");
// #endif
//                             ret = post_process_iris_landmarks(app_ctx->output_mems_iris, app_ctx, right_eye_crop_roi, &iris_letter_box, 
//                                                               out_result->faces[i].eye_landmarks_right, out_result->faces[i].iris_landmarks_right);
//                             if (ret == 0) { 
//                                 out_result->faces[i].eye_landmarks_right_valid = true; 
//                                 out_result->faces[i].iris_landmarks_right_valid = true; 
//                             }
//                         }
//                     }
//                 } else { if (cropped_img_cpu.virt_addr) free(cropped_img_cpu.virt_addr); cropped_img_cpu.virt_addr = NULL;}
//             } // End right eye index check

//         } // End if face landmarks valid
//     } // End loop over detected faces

//     // Cleanup any lingering temp buffer
//     if(cropped_img_cpu.virt_addr) free(cropped_img_cpu.virt_addr);

//     return 0; // Success
// }








// update face-detection retinaface part

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <algorithm> // For std::min, std::max, std::sort
#include <cmath>     // For expf, roundf

// Include RKNN API first
#include "rknn_api.h"

// Include common utilities
#include "common.h"      // For get_qnt_type_string etc. & letterbox_t
#include "file_utils.h"  // For read_data_from_file
#include "image_utils.h" // For image operations (crop, letterbox) & IMAGE_FORMAT_*

// Include the header for this specific implementation last
#include "face_analyzer.h"

// *** Include the RetinaFace box priors ***
#include "rknn_box_priors.h"

// --- Constants (FROM RETINAFACE, adjust thresholds as needed) ---
#define RETINA_NMS_THRESHOLD 0.4f
#define RETINA_CONF_THRESHOLD 0.5f // Score threshold for a prior to be considered
#define RETINA_VIS_THRESHOLD 0.4f  // Final score threshold after NMS

// --- Constants (Needed for Landmark/Iris Cropping, previously missing) ---
#define BOX_SCALE_X 1.5f // Scaling for face crop for face landmark model
#define BOX_SCALE_Y 1.7f // Scaling for face crop for face landmark model
#define EYE_CROP_SCALE 1.8f // Scaling factor for eye crop relative to eye landmarks
// #define IRIS_CROP_SCALE 2.8f // Scaling factor for iris crop (if needed, currently uses EYE_CROP_SCALE)


// --- Utility Functions (Keep existing ones like clamp, dump_tensor_attr etc.) ---
static inline float dequantize_int8_to_float(int8_t val, int32_t zp, float scale) {
    return ((float)val - (float)zp) * scale;
}
// *** ADDED Missing Function ***
static inline float dequantize_uint8_to_float(uint8_t val, int32_t zp, float scale) {
    return ((float)val - (float)zp) * scale;
}
static int clamp(int x, int min_val, int max_val) {
    return std::max(min_val, std::min(x, max_val));
}
static float clampf(float x, float min_val, float max_val) {
     return std::max(min_val, std::min(x, max_val));
}
static void dump_tensor_attr(rknn_tensor_attr *attr, const char* model_name) {
    // Simple version - uncomment more details if needed
     printf("%s Idx:%d Name:%s Dims:[%d,%d,%d,%d] Fmt:%s Type:%s Qnt:%s ZP:%d Scale:%.3f Size:%u Stride:%u\n",
        model_name, attr->index, attr->name,
        attr->n_dims > 0 ? attr->dims[0] : -1,
        attr->n_dims > 1 ? attr->dims[1] : -1,
        attr->n_dims > 2 ? attr->dims[2] : -1,
        attr->n_dims > 3 ? attr->dims[3] : -1,
        get_format_string(attr->fmt), get_type_string(attr->type),
        get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale,
        attr->size, attr->w_stride); // Added size/stride
}

// Simple crop (assumes continuous buffer, only works for specific formats)
// Consider using a more robust version from image_utils if available/needed
static int crop_image_simple(image_buffer_t *src_img, image_buffer_t *dst_img, box_rect_t crop_box) {
    if (!src_img || !src_img->virt_addr || !dst_img) return -1;

    int channels = 0;
    if (src_img->format == IMAGE_FORMAT_RGB888) channels = 3;
#ifdef IMAGE_FORMAT_BGR888 // If BGR support is compiled in image_utils/common
    else if (src_img->format == IMAGE_FORMAT_BGR888) channels = 3;
#endif
    else {
        printf("ERROR: crop_image_simple unsupported format %d\n", src_img->format);
        return -1; // Only support RGB/BGR for this simple version
    }


    int src_w = src_img->width;
    int src_h = src_img->height;
    int crop_x = crop_box.left;
    int crop_y = crop_box.top;
    int crop_w = crop_box.right - crop_box.left;
    int crop_h = crop_box.bottom - crop_box.top;

    // Basic validation
    if (crop_w <= 0 || crop_h <= 0) {
        // Don't print error if it's just off-screen, just return error silently
        // printf("WARN: Invalid crop ROI size [%d x %d] for source image %dx%d\n",
        //        crop_w, crop_h, src_w, src_h);
        return -1;
    }

    // Adjust ROI to fit within source bounds
    int x_start = std::max(0, crop_x);
    int y_start = std::max(0, crop_y);
    int x_end = std::min(src_w, crop_x + crop_w);
    int y_end = std::min(src_h, crop_y + crop_h);

    int valid_crop_w = x_end - x_start;
    int valid_crop_h = y_end - y_start;

    if (valid_crop_w <= 0 || valid_crop_h <= 0) {
         // printf("WARN: Adjusted crop ROI has zero size.\n");
         return -1;
    }

    dst_img->width = crop_w; // Destination retains original requested crop size
    dst_img->height = crop_h;
    dst_img->format = src_img->format;
    dst_img->size = crop_w * crop_h * channels;

    // Allocate destination buffer if not already allocated
    if (dst_img->virt_addr == NULL) {
        dst_img->virt_addr = (unsigned char*)malloc(dst_img->size);
        if (!dst_img->virt_addr) {
            printf("ERROR: Failed to allocate memory for cropped image (%d bytes)\n", dst_img->size);
            return -1;
        }
    } else if (dst_img->size < (size_t)(crop_w * crop_h * channels)) {
         printf("ERROR: Provided destination buffer too small for crop.\n");
         // Or realloc? For now, treat as error.
         return -1;
    }


    // Clear destination buffer (e.g., black background)
    memset(dst_img->virt_addr, 0, dst_img->size);

    unsigned char* src_data = src_img->virt_addr;
    unsigned char* dst_data = dst_img->virt_addr;
    // Use width_stride if available, otherwise calculate based on width
    size_t src_stride = src_img->width_stride ? src_img->width_stride : (size_t)src_w * channels;
    size_t dst_stride = (size_t)crop_w * channels;

    // Calculate where the valid source data should be placed in the destination
    int dst_x_offset = x_start - crop_x; // Offset within the destination buffer
    int dst_y_offset = y_start - crop_y;

    // Copy valid region row by row
    for (int y = 0; y < valid_crop_h; ++y) {
        unsigned char* src_row_ptr = src_data + (size_t)(y_start + y) * src_stride + (size_t)x_start * channels;
        unsigned char* dst_row_ptr = dst_data + (size_t)(dst_y_offset + y) * dst_stride + (size_t)dst_x_offset * channels;
        memcpy(dst_row_ptr, src_row_ptr, (size_t)valid_crop_w * channels);
    }

    return 0;
}


// --- Helper Functions from RetinaFace Post-processing (Adapted) ---

// Need a struct to hold intermediate decoded results for RetinaFace
typedef struct {
    box_rect_t norm_box; // Box coordinates normalized to [0, 1] (scaled for NMS)
    float score;
    int index; // Original prior index
} RetinaFaceDecodedCandidate;

// Comparison for sorting by score (descending)
static bool compareRetinaCandidates(const RetinaFaceDecodedCandidate& a, const RetinaFaceDecodedCandidate& b) {
    return a.score > b.score;
}

// NMS function adapted for RetinaFaceDecodedCandidate (using normalized coords)
static float calculate_normalized_overlap(const box_rect_t& box1, const box_rect_t& box2) {
    // Use float division
    float xmin1 = box1.left / 10000.0f; float ymin1 = box1.top / 10000.0f;
    float xmax1 = box1.right / 10000.0f; float ymax1 = box1.bottom / 10000.0f;
    float xmin2 = box2.left / 10000.0f; float ymin2 = box2.top / 10000.0f;
    float xmax2 = box2.right / 10000.0f; float ymax2 = box2.bottom / 10000.0f;

    float inter_xmin = std::max(xmin1, xmin2);
    float inter_ymin = std::max(ymin1, ymin2);
    float inter_xmax = std::min(xmax1, xmax2);
    float inter_ymax = std::min(ymax1, ymax2);

    float inter_w = std::max(0.0f, inter_xmax - inter_xmin);
    float inter_h = std::max(0.0f, inter_ymax - inter_ymin);
    float inter_area = inter_w * inter_h;

    float area1 = (xmax1 - xmin1) * (ymax1 - ymin1);
    float area2 = (xmax2 - xmin2) * (ymax2 - ymin2);
    float union_area = area1 + area2 - inter_area;

    return (union_area <= 1e-6f) ? 0.0f : (inter_area / union_area);
}


static void nms_retina(std::vector<RetinaFaceDecodedCandidate>& candidates, float nms_threshold) {
     if (candidates.empty()) return;
    std::sort(candidates.begin(), candidates.end(), compareRetinaCandidates);
    std::vector<bool> removed(candidates.size(), false);
    std::vector<RetinaFaceDecodedCandidate> result_candidates;
    result_candidates.reserve(candidates.size());

    for (size_t i = 0; i < candidates.size(); ++i) {
        if (removed[i]) continue;
        result_candidates.push_back(candidates[i]);
        for (size_t j = i + 1; j < candidates.size(); ++j) {
            if (removed[j]) continue;
            // Use normalized overlap calculation
            float overlap = calculate_normalized_overlap(candidates[i].norm_box, candidates[j].norm_box);
            if (overlap > nms_threshold) {
                removed[j] = true;
            }
        }
    }
    candidates = std::move(result_candidates);
}
// --- End RetinaFace Helpers ---

// --- Post Processing Functions (Modified for Zero-Copy) ---

// *** REMOVED OLD post_process_face_detection function ***
// static int post_process_face_detection(...) { ... }

static int calculate_letterbox_rects(int src_w, int src_h, int dst_w, int dst_h,
                                     int allow_slight_change,
                                     letterbox_t* out_letterbox, image_rect_t* out_dst_box)
{
    if (!out_letterbox || !out_dst_box || src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0) {
        return -1;
    }

    float scale_w = (float)dst_w / src_w;
    float scale_h = (float)dst_h / src_h;
    float scale = std::min(scale_w, scale_h);

    int resize_w = (int)(src_w * scale);
    int resize_h = (int)(src_h * scale);

    // Optional slight adjustment for alignment (mimics original logic)
    if (allow_slight_change == 1) {
        // RGA usually requires even dimensions, some HW needs 4 or 16 alignment for src/dst?
        // Let's keep it simple for now and just ensure even dims for resize target
        if (resize_w % 2 != 0) resize_w--;
        if (resize_h % 2 != 0) resize_h--;
        if (resize_w <= 0 || resize_h <= 0) return -1; // Invalid size after adjustment
        // Recalculate scale based on adjusted size ONLY if aspect ratio changed significantly?
        // For simplicity, we'll use the original scale for coordinate mapping,
        // but use the adjusted resize_w/h for padding calculation.
        // This matches the likely behavior of the original function more closely.
    }

    int padding_w = dst_w - resize_w;
    int padding_h = dst_h - resize_h;

    // Calculate destination box (where the scaled image will be placed)
    out_dst_box->left = padding_w / 2;
    out_dst_box->top = padding_h / 2;
    // Ensure even offsets if possible/needed by HW
    if (out_dst_box->left % 2 != 0) out_dst_box->left--;
    if (out_dst_box->top % 2 != 0) out_dst_box->top--;
    // Clamp offsets
    out_dst_box->left = std::max(0, out_dst_box->left);
    out_dst_box->top = std::max(0, out_dst_box->top);

    out_dst_box->right = out_dst_box->left + resize_w;
    out_dst_box->bottom = out_dst_box->top + resize_h;
    // Ensure box doesn't exceed destination buffer boundaries
    out_dst_box->right = std::min(dst_w, out_dst_box->right);
    out_dst_box->bottom = std::min(dst_h, out_dst_box->bottom);

    // Store letterbox parameters
    out_letterbox->scale = scale;
    out_letterbox->x_pad = out_dst_box->left;
    out_letterbox->y_pad = out_dst_box->top;

    // printf("Calc Letterbox: src[%dx%d]->dst[%dx%d], resize[%dx%d], scale=%.3f, pad[%d,%d], dst_box[%d,%d,%d,%d]\n",
    //        src_w, src_h, dst_w, dst_h, resize_w, resize_h, scale, padding_w, padding_h,
    //        out_dst_box->left, out_dst_box->top, out_dst_box->right, out_dst_box->bottom);

    return 0;
}

// --- NEW Post Processing for RetinaFace Detection ---
// *** MODIFIED SIGNATURE to accept face_analyzer_result_t* ***
static int post_process_retinaface_detection_only(
    rknn_tensor_mem* output_mems[],
    face_analyzer_app_context_t *app_ctx,
    const letterbox_t *det_letter_box,
    int src_img_width, int src_img_height,
    face_analyzer_result_t* out_result)
{
    out_result->count = 0; // Initialize count

    if (!app_ctx || !output_mems || !out_result) {
        printf("ERROR: NULL pointer passed to post_process_retinaface_detection_only.\n");
        return -1;
    }

    if (app_ctx->io_num_detect.n_output < 3) {
        printf("ERROR: RetinaFace model requires at least 3 outputs (loc, score, landm), got %d\n", app_ctx->io_num_detect.n_output);
        return -1;
    }

    // --- Get attributes and buffer pointers (DECLARE variables needed) ---
    rknn_tensor_attr* loc_attr = &app_ctx->output_attrs_detect[0];
    rknn_tensor_attr* score_attr = &app_ctx->output_attrs_detect[1];
    // rknn_tensor_attr* landm_attr = &app_ctx->output_attrs_detect[2]; // Still not needed for detection only

    void* loc_buf = output_mems[0]->virt_addr;
    void* score_buf = output_mems[1]->virt_addr;
    // void* landm_buf = output_mems[2]->virt_addr; // Still not needed

    if (!loc_buf || !score_buf) {
        printf("ERROR: Output memory virtual address is NULL in post_process_retinaface_detection_only.\n");
        return -1;
    }
    // --- End variable declarations ---


    // --- Get prior boxes (DECLARE variables needed) ---
    const float (*prior_ptr)[4] = nullptr; // Declare prior_ptr
    int num_priors = 0;                   // Declare num_priors
    if (app_ctx->model1_height == 320) {
        num_priors = 4200;
        prior_ptr = BOX_PRIORS_320;
    } else if (app_ctx->model1_height == 640) {
        num_priors = 16800;
        prior_ptr = BOX_PRIORS_640;
    } else {
        printf("ERROR: Unsupported RetinaFace model height: %d\n", app_ctx->model1_height);
        return -1;
    }
    // --- End prior box logic ---

    bool is_quant = app_ctx->is_quant_detect;

    // --- Declare vector for candidates ---
    std::vector<RetinaFaceDecodedCandidate> decoded_candidates;
    decoded_candidates.reserve(num_priors / 10);

    // --- Declare VARIANCES constant ---
    const float VARIANCES[2] = {0.1f, 0.2f};

    // --- Step 1: Filter and Decode ---
    for (int i = 0; i < num_priors; ++i) { // Use declared num_priors
        float face_score;

        // Dequantize score (Use declared score_attr and score_buf)
        if (score_attr->type == RKNN_TENSOR_FLOAT16) {
             face_score = (float)((__fp16*)score_buf)[i * 2 + 1];
        } else if (is_quant) { // Check is_quant only after handling non-quant types
            if (score_attr->type == RKNN_TENSOR_INT8) {
                 int8_t score_q = ((int8_t*)score_buf)[i * 2 + 1];
                 face_score = dequantize_int8_to_float(score_q, score_attr->zp, score_attr->scale);
            } else if (score_attr->type == RKNN_TENSOR_UINT8) {
                 uint8_t score_q = ((uint8_t*)score_buf)[i * 2 + 1];
                 face_score = dequantize_uint8_to_float(score_q, score_attr->zp, score_attr->scale);
            } else {
                 printf("WARN: Unsupported score quantization type: %s. Treating as float.\n", get_type_string(score_attr->type));
                 face_score = ((float*)score_buf)[i * 2 + 1]; // Fallback
            }
        } else { // Non-quantized (FP32)
             face_score = ((float*)score_buf)[i * 2 + 1];
        }


        // Check confidence threshold
        if (face_score >= RETINA_CONF_THRESHOLD) {
            // Declare cand inside the scope where it's used
            RetinaFaceDecodedCandidate cand;
            cand.score = face_score;
            cand.index = i;

            // Decode location (box) (Use declared loc_attr and loc_buf)
            float dx, dy, dw, dh;
            if (loc_attr->type == RKNN_TENSOR_FLOAT16) {
                dx = (float)((__fp16*)loc_buf)[i * 4 + 0];
                dy = (float)((__fp16*)loc_buf)[i * 4 + 1];
                dw = (float)((__fp16*)loc_buf)[i * 4 + 2];
                dh = (float)((__fp16*)loc_buf)[i * 4 + 3];
            } else if (is_quant) {
                 if (loc_attr->type == RKNN_TENSOR_INT8) {
                    dx = dequantize_int8_to_float(((int8_t*)loc_buf)[i * 4 + 0], loc_attr->zp, loc_attr->scale);
                    dy = dequantize_int8_to_float(((int8_t*)loc_buf)[i * 4 + 1], loc_attr->zp, loc_attr->scale);
                    dw = dequantize_int8_to_float(((int8_t*)loc_buf)[i * 4 + 2], loc_attr->zp, loc_attr->scale);
                    dh = dequantize_int8_to_float(((int8_t*)loc_buf)[i * 4 + 3], loc_attr->zp, loc_attr->scale);
                 } else if (loc_attr->type == RKNN_TENSOR_UINT8) {
                    dx = dequantize_uint8_to_float(((uint8_t*)loc_buf)[i * 4 + 0], loc_attr->zp, loc_attr->scale);
                    dy = dequantize_uint8_to_float(((uint8_t*)loc_buf)[i * 4 + 1], loc_attr->zp, loc_attr->scale);
                    dw = dequantize_uint8_to_float(((uint8_t*)loc_buf)[i * 4 + 2], loc_attr->zp, loc_attr->scale);
                    dh = dequantize_uint8_to_float(((uint8_t*)loc_buf)[i * 4 + 3], loc_attr->zp, loc_attr->scale);
                } else {
                    printf("WARN: Unsupported location quantization type: %s. Treating as float.\n", get_type_string(loc_attr->type));
                    dx = ((float*)loc_buf)[i * 4 + 0]; // Fallback
                    dy = ((float*)loc_buf)[i * 4 + 1];
                    dw = ((float*)loc_buf)[i * 4 + 2];
                    dh = ((float*)loc_buf)[i * 4 + 3];
                }
            } else { // Non-quantized (FP32)
                 dx = ((float*)loc_buf)[i * 4 + 0];
                 dy = ((float*)loc_buf)[i * 4 + 1];
                 dw = ((float*)loc_buf)[i * 4 + 2];
                 dh = ((float*)loc_buf)[i * 4 + 3];
            }

            // Apply prior and variance decoding (Use declared prior_ptr and VARIANCES)
            float prior_cx = prior_ptr[i][0];
            float prior_cy = prior_ptr[i][1];
            float prior_w = prior_ptr[i][2];
            float prior_h = prior_ptr[i][3];

            float decoded_cx = dx * VARIANCES[0] * prior_w + prior_cx;
            float decoded_cy = dy * VARIANCES[0] * prior_h + prior_cy;
            float decoded_w = expf(dw * VARIANCES[1]) * prior_w;
            float decoded_h = expf(dh * VARIANCES[1]) * prior_h;

            float xmin_norm = decoded_cx - decoded_w * 0.5f;
            float ymin_norm = decoded_cy - decoded_h * 0.5f;
            float xmax_norm = decoded_cx + decoded_w * 0.5f;
            float ymax_norm = decoded_cy + decoded_h * 0.5f;

            // Store scaled normalized coordinates (Use declared cand)
             cand.norm_box.left   = (int)(clampf(xmin_norm, 0.0f, 1.0f) * 10000.0f);
             cand.norm_box.top    = (int)(clampf(ymin_norm, 0.0f, 1.0f) * 10000.0f);
             cand.norm_box.right  = (int)(clampf(xmax_norm, 0.0f, 1.0f) * 10000.0f);
             cand.norm_box.bottom = (int)(clampf(ymax_norm, 0.0f, 1.0f) * 10000.0f);

            // Use declared decoded_candidates
            decoded_candidates.push_back(cand);
        }
    }

    // --- Step 2: NMS ---
    // Use declared decoded_candidates
    nms_retina(decoded_candidates, RETINA_NMS_THRESHOLD);

    // --- Step 3: Final Coordinate Transformation ---
    float model_in_w = (float)app_ctx->model1_width;
    float model_in_h = (float)app_ctx->model1_height;

    // Use declared decoded_candidates
    for (const auto& cand : decoded_candidates) {
        if (cand.score < RETINA_VIS_THRESHOLD) { continue; }

        // ... (Coordinate transformation logic remains the same) ...
         // Convert back to normalized float coordinates
        float xmin_norm = cand.norm_box.left / 10000.0f;
        float ymin_norm = cand.norm_box.top / 10000.0f;
        float xmax_norm = cand.norm_box.right / 10000.0f;
        float ymax_norm = cand.norm_box.bottom / 10000.0f;

        // Scale to model input size
        float xmin_model = xmin_norm * model_in_w;
        float ymin_model = ymin_norm * model_in_h;
        float xmax_model = xmax_norm * model_in_w;
        float ymax_model = ymax_norm * model_in_h;

        // Adjust for letterbox padding
        float xmin_unpad = (xmin_model - det_letter_box->x_pad) / det_letter_box->scale;
        float ymin_unpad = (ymin_model - det_letter_box->y_pad) / det_letter_box->scale;
        float xmax_unpad = (xmax_model - det_letter_box->x_pad) / det_letter_box->scale;
        float ymax_unpad = (ymax_model - det_letter_box->y_pad) / det_letter_box->scale;


        // Write directly to out_result
        if (out_result->count < MAX_FACE_RESULTS) {
            face_object_t* final_face = &out_result->faces[out_result->count];
            memset(final_face, 0, sizeof(face_object_t));
            final_face->box.left   = clamp((int)roundf(xmin_unpad), 0, src_img_width - 1);
            final_face->box.top    = clamp((int)roundf(ymin_unpad), 0, src_img_height - 1);
            final_face->box.right  = clamp((int)roundf(xmax_unpad), 0, src_img_width - 1);
            final_face->box.bottom = clamp((int)roundf(ymax_unpad), 0, src_img_height - 1);
            final_face->score = cand.score;

            if (final_face->box.right > final_face->box.left && final_face->box.bottom > final_face->box.top) {
                out_result->count++;
            }
        } else {
            printf("WARN: Exceeded MAX_FACE_RESULTS (%d)\n", MAX_FACE_RESULTS);
            break;
        }
    }

    printf("RetinaFace Detection: Found %d faces after NMS.\n", out_result->count);
    return 0;
}


// Post-process face landmarks (Keep original logic, reads from landmark_mem)
static int post_process_face_landmarks(
    rknn_tensor_mem* landmark_mem,
    face_analyzer_app_context_t *app_ctx,
    const box_rect_t& crop_roi,
    const letterbox_t *lmk_letter_box,
    point_t out_landmarks[NUM_FACE_LANDMARKS])
{
    // ... (Keep the existing implementation for post_process_face_landmarks)
     rknn_tensor_attr* landmark_attr = &app_ctx->output_attrs_landmark[0];
    if (landmark_attr->type != RKNN_TENSOR_INT8) { /* error */ return -1; }

    bool valid_dims = false; int num_elements = 0;
    // Check different possible dimension orders (NHWC or NCHW interpretation)
    if (landmark_attr->n_dims == 4 && landmark_attr->dims[0] == 1) { num_elements = landmark_attr->dims[1] * landmark_attr->dims[2] * landmark_attr->dims[3]; valid_dims = (num_elements == NUM_FACE_LANDMARKS * 3); }
    else if (landmark_attr->n_dims == 2 && landmark_attr->dims[0] == 1) { num_elements = landmark_attr->dims[1]; valid_dims = (num_elements == NUM_FACE_LANDMARKS * 3); }
    // Add more checks if other layouts are possible

    if (!valid_dims) { printf("ERROR: Landmark output tensor has unexpected dimensions/elements. Got %d elements.\n", num_elements); dump_tensor_attr(landmark_attr, "LandmarkOut"); return -1; }

    int8_t* landmark_data = (int8_t*)landmark_mem->virt_addr;
    if (!landmark_data) { printf("ERROR: Landmark output virt_addr is NULL.\n"); return -1;}

    float landmark_scale = landmark_attr->scale;
    int32_t landmark_zp  = landmark_attr->zp;

    float crop_roi_w = (float)(crop_roi.right - crop_roi.left);
    float crop_roi_h = (float)(crop_roi.bottom - crop_roi.top);
    if (crop_roi_w <= 0 || crop_roi_h <= 0) { /* error */ return -1; }

    float model_lm_w = (float)app_ctx->model2_width;
    float model_lm_h = (float)app_ctx->model2_height;

    for (int i = 0; i < NUM_FACE_LANDMARKS; ++i) {
        int offset = i * 3;
        // Ensure offset does not go out of bounds
        if (offset + 2 >= num_elements) { printf("ERROR: Landmark index out of bounds.\n"); return -1;}

        float x_lmk_model = dequantize_int8_to_float(landmark_data[offset + 0], landmark_zp, landmark_scale);
        float y_lmk_model = dequantize_int8_to_float(landmark_data[offset + 1], landmark_zp, landmark_scale);

        // Reverse the letterbox transformation applied during landmark preprocessing
        float x_unpadded = (x_lmk_model - lmk_letter_box->x_pad);
        float y_unpadded = (y_lmk_model - lmk_letter_box->y_pad);
        float x_orig_crop_rel = x_unpadded / lmk_letter_box->scale;
        float y_orig_crop_rel = y_unpadded / lmk_letter_box->scale;

        // Add the top-left corner of the crop ROI to get coordinates relative to the original image
        float final_x = x_orig_crop_rel + crop_roi.left;
        float final_y = y_orig_crop_rel + crop_roi.top;

        out_landmarks[i].x = (int)roundf(final_x);
        out_landmarks[i].y = (int)roundf(final_y);
    }
    return 0;
}

// Post-process iris landmarks (Keep original logic, reads from iris_output_mems)
static int post_process_iris_landmarks(
    rknn_tensor_mem* iris_output_mems[],
    face_analyzer_app_context_t *app_ctx,
    const box_rect_t& eye_crop_roi,
    const letterbox_t *iris_letter_box,
    point_t out_eye_contour[NUM_EYE_CONTOUR_LANDMARKS],
    point_t out_iris[NUM_IRIS_LANDMARKS])
{
    // ... (Keep the existing implementation for post_process_iris_landmarks) ...
     if (app_ctx->io_num_iris.n_output < 2) { /* error */ return -1; }
     rknn_tensor_attr* eye_contour_attr = &app_ctx->output_attrs_iris[0];
     rknn_tensor_attr* iris_pts_attr    = &app_ctx->output_attrs_iris[1];

    // Simplified dimension check (total elements)
    bool eye_dims_ok = (eye_contour_attr->n_elems == NUM_EYE_CONTOUR_LANDMARKS * 3);
    bool iris_dims_ok = (iris_pts_attr->n_elems == NUM_IRIS_LANDMARKS * 3);

     if (!eye_dims_ok || !iris_dims_ok || eye_contour_attr->type != RKNN_TENSOR_INT8 || iris_pts_attr->type != RKNN_TENSOR_INT8) {
         printf("ERROR: Iris output tensor mismatch (Eye Elem:%d Expected:%d, Iris Elem:%d Expected:%d, EyeType:%d IrisType:%d)\n",
                eye_contour_attr->n_elems, NUM_EYE_CONTOUR_LANDMARKS * 3,
                iris_pts_attr->n_elems, NUM_IRIS_LANDMARKS * 3,
                eye_contour_attr->type, iris_pts_attr->type);
         dump_tensor_attr(eye_contour_attr, "IrisOut0");
         dump_tensor_attr(iris_pts_attr, "IrisOut1");
         return -1;
     }

     int8_t* eye_data = (int8_t*)iris_output_mems[0]->virt_addr;
     if (!eye_data) { printf("ERROR: Eye contour output virt_addr is NULL.\n"); return -1; }
     float eye_scale  = eye_contour_attr->scale;
     int32_t eye_zp   = eye_contour_attr->zp;

     int8_t* iris_data = (int8_t*)iris_output_mems[1]->virt_addr;
      if (!iris_data) { printf("ERROR: Iris points output virt_addr is NULL.\n"); return -1; }
     float iris_scale = iris_pts_attr->scale;
     int32_t iris_zp  = iris_pts_attr->zp;

     float eye_crop_roi_w = (float)(eye_crop_roi.right - eye_crop_roi.left);
     float eye_crop_roi_h = (float)(eye_crop_roi.bottom - eye_crop_roi.top);
     if (eye_crop_roi_w <= 0 || eye_crop_roi_h <= 0) { /* error */ return -1; }

     float model_iris_w = (float)app_ctx->model3_width;
     float model_iris_h = (float)app_ctx->model3_height;

     // Process Eye Contour Landmarks
     for (int i = 0; i < NUM_EYE_CONTOUR_LANDMARKS; ++i) {
         int offset = i * 3;
         if (offset + 2 >= eye_contour_attr->n_elems) { printf("ERROR: Eye contour index out of bounds.\n"); return -1;}
         float x_eye_model = dequantize_int8_to_float(eye_data[offset + 0], eye_zp, eye_scale);
         float y_eye_model = dequantize_int8_to_float(eye_data[offset + 1], eye_zp, eye_scale);

         float x_unpadded = (x_eye_model - iris_letter_box->x_pad);
         float y_unpadded = (y_eye_model - iris_letter_box->y_pad);
         float x_orig_eye_rel = x_unpadded / iris_letter_box->scale;
         float y_orig_eye_rel = y_unpadded / iris_letter_box->scale;
         float final_x = x_orig_eye_rel + eye_crop_roi.left;
         float final_y = y_orig_eye_rel + eye_crop_roi.top;
         out_eye_contour[i].x = (int)roundf(final_x);
         out_eye_contour[i].y = (int)roundf(final_y);
     }

    // Process Iris Center Landmarks
     for (int i = 0; i < NUM_IRIS_LANDMARKS; ++i) {
         int offset = i * 3;
         if (offset + 2 >= iris_pts_attr->n_elems) { printf("ERROR: Iris point index out of bounds.\n"); return -1;}
         float x_iris_model = dequantize_int8_to_float(iris_data[offset + 0], iris_zp, iris_scale);
         float y_iris_model = dequantize_int8_to_float(iris_data[offset + 1], iris_zp, iris_scale);

         float x_unpadded = (x_iris_model - iris_letter_box->x_pad);
         float y_unpadded = (y_iris_model - iris_letter_box->y_pad);
         float x_orig_eye_rel = x_unpadded / iris_letter_box->scale;
         float y_orig_eye_rel = y_unpadded / iris_letter_box->scale;
         float final_x = x_orig_eye_rel + eye_crop_roi.left;
         float final_y = y_orig_eye_rel + eye_crop_roi.top;
         out_iris[i].x = (int)roundf(final_x);
         out_iris[i].y = (int)roundf(final_y);
     }

    return 0;
}
// --- End Post Processing Functions ---


// --- Model Initialization and Release (Modified for RetinaFace Detection) ---
// *** FIXED TYPO intt -> int ***
int init_face_analyzer(const char *retinaface_model_path, // Renamed parameter
                       const char* landmark_model_path,
                       const char* iris_model_path,
                       face_analyzer_app_context_t *app_ctx) {
    memset(app_ctx, 0, sizeof(face_analyzer_app_context_t));
    int ret;
    int model_len = 0;
    unsigned char *model_buf = NULL;

    // --- Init Detect Model (RetinaFace) ---
    printf("Init Face Detect Model (RetinaFace): %s\n", retinaface_model_path);
    model_len = read_data_from_file(retinaface_model_path, reinterpret_cast<char**>(&model_buf));
    if (!model_buf) { printf("ERROR: read_data_from_file failed for RetinaFace model\n"); return -1; }

    ret = rknn_init(&app_ctx->rknn_ctx_detect, model_buf, model_len, 0, NULL);
    free(model_buf); model_buf = NULL;
    if (ret < 0) { printf("ERROR: rknn_init(RetinaFace) failed! ret=%d\n", ret); return -1; }

    ret = rknn_set_core_mask(app_ctx->rknn_ctx_detect, RKNN_NPU_CORE_0);
    if (ret < 0) { printf("WARN: rknn_set_core_mask(detect, CORE_0) failed! ret=%d\n", ret); }
    else { printf("INFO: RetinaFace Detection Model assigned to NPU Core 0.\n"); }

    ret = rknn_query(app_ctx->rknn_ctx_detect, RKNN_QUERY_IN_OUT_NUM, &app_ctx->io_num_detect, sizeof(app_ctx->io_num_detect));
    if (ret != RKNN_SUCC || app_ctx->io_num_detect.n_output < 3) {
        printf("ERROR: Failed to query IO num or insufficient outputs for RetinaFace (need 3, got %d)\n", app_ctx->io_num_detect.n_output);
        goto cleanup_detect;
    }
    printf("RetinaFace Model IO: Inputs=%d, Outputs=%d\n", app_ctx->io_num_detect.n_input, app_ctx->io_num_detect.n_output);

    // Query Input Attributes (Native)
    app_ctx->input_attrs_detect = (rknn_tensor_attr*)malloc(app_ctx->io_num_detect.n_input * sizeof(rknn_tensor_attr));
    if (!app_ctx->input_attrs_detect) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_detect; }
    memset(app_ctx->input_attrs_detect, 0, app_ctx->io_num_detect.n_input * sizeof(rknn_tensor_attr));
    for (uint32_t i = 0; i < app_ctx->io_num_detect.n_input; i++) {
        app_ctx->input_attrs_detect[i].index = i;
        ret = rknn_query(app_ctx->rknn_ctx_detect, RKNN_QUERY_NATIVE_INPUT_ATTR, &app_ctx->input_attrs_detect[i], sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) { printf("ERROR: Query Detect Input Attr %d failed\n", i); goto cleanup_detect; }
        dump_tensor_attr(&app_ctx->input_attrs_detect[i], "DetectIn(Native)");
    }

    // Query Output Attributes (Native NHWC format often works well for zero-copy access)
    app_ctx->output_attrs_detect = (rknn_tensor_attr*)malloc(app_ctx->io_num_detect.n_output * sizeof(rknn_tensor_attr));
    if (!app_ctx->output_attrs_detect) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_detect; }
    memset(app_ctx->output_attrs_detect, 0, app_ctx->io_num_detect.n_output * sizeof(rknn_tensor_attr));
    for (uint32_t i = 0; i < app_ctx->io_num_detect.n_output; i++) {
        app_ctx->output_attrs_detect[i].index = i;
        ret = rknn_query(app_ctx->rknn_ctx_detect, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &app_ctx->output_attrs_detect[i], sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) { printf("ERROR: Query Detect Output Attr %d failed\n", i); goto cleanup_detect; }
        dump_tensor_attr(&app_ctx->output_attrs_detect[i], "DetectOut(Native)");
    }

    // Set input format and type for preprocessing and memory setting
    if (app_ctx->io_num_detect.n_input > 0) {
        app_ctx->input_attrs_detect[0].type = RKNN_TENSOR_UINT8;
        app_ctx->input_attrs_detect[0].fmt = RKNN_TENSOR_NHWC;
        app_ctx->input_fmt_detect = RKNN_TENSOR_NHWC;
    }

    // Check if the RetinaFace model is quantized
    app_ctx->is_quant_detect = (app_ctx->output_attrs_detect[0].qnt_type != RKNN_TENSOR_QNT_NONE &&
                               (app_ctx->output_attrs_detect[0].type == RKNN_TENSOR_INT8 ||
                                app_ctx->output_attrs_detect[0].type == RKNN_TENSOR_UINT8));
    printf("INFO: RetinaFace model quantization: %s\n", app_ctx->is_quant_detect ? "YES" : "NO");

    // Allocate Zero-Copy Input Memory for Detection
    app_ctx->input_mems_detect = (rknn_tensor_mem**)malloc(sizeof(rknn_tensor_mem*) * app_ctx->io_num_detect.n_input);
    if (!app_ctx->input_mems_detect) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_detect; }
    memset(app_ctx->input_mems_detect, 0, sizeof(rknn_tensor_mem*) * app_ctx->io_num_detect.n_input);
    for (uint32_t i = 0; i < app_ctx->io_num_detect.n_input; ++i) {
        app_ctx->input_mems_detect[i] = rknn_create_mem(app_ctx->rknn_ctx_detect, app_ctx->input_attrs_detect[i].size_with_stride);
        if (!app_ctx->input_mems_detect[i]) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_detect; }
        // Don't set IO mem here, set before each inference run
    }

    // Allocate Zero-Copy Output Memory for Detection (3 outputs)
    app_ctx->output_mems_detect = (rknn_tensor_mem**)malloc(sizeof(rknn_tensor_mem*) * app_ctx->io_num_detect.n_output);
    if (!app_ctx->output_mems_detect) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_detect; }
    memset(app_ctx->output_mems_detect, 0, sizeof(rknn_tensor_mem*) * app_ctx->io_num_detect.n_output);
    for (uint32_t i = 0; i < app_ctx->io_num_detect.n_output; ++i) {
        app_ctx->output_mems_detect[i] = rknn_create_mem(app_ctx->rknn_ctx_detect, app_ctx->output_attrs_detect[i].size_with_stride);
        if (!app_ctx->output_mems_detect[i]) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_detect; }
        ret = rknn_set_io_mem(app_ctx->rknn_ctx_detect, app_ctx->output_mems_detect[i], &app_ctx->output_attrs_detect[i]);
        if (ret < 0) { printf("ERROR: Set Detect Output Mem %d failed\n", i); goto cleanup_detect; }
    }

    // Get RetinaFace input dimensions
    if (app_ctx->input_attrs_detect[0].fmt == RKNN_TENSOR_NCHW) { /* Should be NHWC based on query */ }
    else { // Assume NHWC
        app_ctx->model1_height = app_ctx->input_attrs_detect[0].dims[1];
        app_ctx->model1_width = app_ctx->input_attrs_detect[0].dims[2];
        app_ctx->model1_channel = app_ctx->input_attrs_detect[0].dims[3];
    }
    printf("RetinaFace model input HxWxC: %dx%dx%d\n", app_ctx->model1_height, app_ctx->model1_width, app_ctx->model1_channel);


    // --- Init Landmark Model (Keep Original Logic) ---
    printf("Init Face Landmark Model: %s\n", landmark_model_path);
    model_len = read_data_from_file(landmark_model_path, reinterpret_cast<char**>(&model_buf));
    if (!model_buf) { ret = RKNN_ERR_MODEL_INVALID; goto cleanup_detect; }
    ret = rknn_init(&app_ctx->rknn_ctx_landmark, model_buf, model_len, 0, NULL);
    free(model_buf); model_buf = NULL;
    if (ret < 0) { goto cleanup_detect; }
    ret = rknn_set_core_mask(app_ctx->rknn_ctx_landmark, RKNN_NPU_CORE_0); // Can share core or use another
    if(ret < 0) { printf("WARN: rknn_set_core_mask(landmark, CORE_0) failed! ret=%d\n", ret); } else { printf("INFO: Face Landmark Model assigned to NPU Core 0.\n"); }
    ret = rknn_query(app_ctx->rknn_ctx_landmark, RKNN_QUERY_IN_OUT_NUM, &app_ctx->io_num_landmark, sizeof(app_ctx->io_num_landmark));
    if (ret != RKNN_SUCC) { goto cleanup_landmark; }
    app_ctx->input_attrs_landmark = (rknn_tensor_attr*)malloc(app_ctx->io_num_landmark.n_input * sizeof(rknn_tensor_attr));
    if (!app_ctx->input_attrs_landmark) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_landmark; }
    memset(app_ctx->input_attrs_landmark, 0, app_ctx->io_num_landmark.n_input * sizeof(rknn_tensor_attr));
    for (uint32_t i = 0; i < app_ctx->io_num_landmark.n_input; i++) { app_ctx->input_attrs_landmark[i].index = i; ret = rknn_query(app_ctx->rknn_ctx_landmark, RKNN_QUERY_NATIVE_INPUT_ATTR, &app_ctx->input_attrs_landmark[i], sizeof(rknn_tensor_attr)); if (ret != RKNN_SUCC) goto cleanup_landmark; dump_tensor_attr(&app_ctx->input_attrs_landmark[i], "LmkIn(Native)"); }
    app_ctx->output_attrs_landmark = (rknn_tensor_attr*)malloc(app_ctx->io_num_landmark.n_output * sizeof(rknn_tensor_attr));
    if (!app_ctx->output_attrs_landmark) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_landmark; }
    memset(app_ctx->output_attrs_landmark, 0, app_ctx->io_num_landmark.n_output * sizeof(rknn_tensor_attr));
    for (uint32_t i = 0; i < app_ctx->io_num_landmark.n_output; i++) { app_ctx->output_attrs_landmark[i].index = i; ret = rknn_query(app_ctx->rknn_ctx_landmark, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &app_ctx->output_attrs_landmark[i], sizeof(rknn_tensor_attr)); if (ret != RKNN_SUCC) goto cleanup_landmark; dump_tensor_attr(&app_ctx->output_attrs_landmark[i], "LmkOut(Native)"); }
    if (app_ctx->io_num_landmark.n_input > 0) { app_ctx->input_attrs_landmark[0].type = RKNN_TENSOR_UINT8; app_ctx->input_attrs_landmark[0].fmt = RKNN_TENSOR_NHWC; app_ctx->input_fmt_landmark = RKNN_TENSOR_NHWC; }
    app_ctx->input_mems_landmark = (rknn_tensor_mem**)malloc(sizeof(rknn_tensor_mem*) * app_ctx->io_num_landmark.n_input);
    if(!app_ctx->input_mems_landmark){ ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_landmark; } memset(app_ctx->input_mems_landmark, 0, sizeof(rknn_tensor_mem*) * app_ctx->io_num_landmark.n_input);
    for(uint32_t i = 0; i < app_ctx->io_num_landmark.n_input; ++i) { app_ctx->input_mems_landmark[i] = rknn_create_mem(app_ctx->rknn_ctx_landmark, app_ctx->input_attrs_landmark[i].size_with_stride); if(!app_ctx->input_mems_landmark[i]) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_landmark; } /* Set IO mem later */ }
    app_ctx->output_mems_landmark = (rknn_tensor_mem**)malloc(sizeof(rknn_tensor_mem*) * app_ctx->io_num_landmark.n_output);
    if(!app_ctx->output_mems_landmark){ ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_landmark; } memset(app_ctx->output_mems_landmark, 0, sizeof(rknn_tensor_mem*) * app_ctx->io_num_landmark.n_output);
    for(uint32_t i = 0; i < app_ctx->io_num_landmark.n_output; ++i) { app_ctx->output_mems_landmark[i] = rknn_create_mem(app_ctx->rknn_ctx_landmark, app_ctx->output_attrs_landmark[i].size_with_stride); if(!app_ctx->output_mems_landmark[i]) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_landmark; } ret = rknn_set_io_mem(app_ctx->rknn_ctx_landmark, app_ctx->output_mems_landmark[i], &app_ctx->output_attrs_landmark[i]); if (ret < 0) goto cleanup_landmark; }
    app_ctx->model2_height = app_ctx->input_attrs_landmark[0].dims[1]; app_ctx->model2_width = app_ctx->input_attrs_landmark[0].dims[2]; app_ctx->model2_channel = app_ctx->input_attrs_landmark[0].dims[3];


    // --- Init Iris Model (Keep Original Logic) ---
    printf("Init Iris Landmark Model: %s\n", iris_model_path);
    model_len = read_data_from_file(iris_model_path, reinterpret_cast<char**>(&model_buf));
    if (!model_buf) { ret = RKNN_ERR_MODEL_INVALID; goto cleanup_landmark; }
    ret = rknn_init(&app_ctx->rknn_ctx_iris, model_buf, model_len, 0, NULL);
    free(model_buf); model_buf = NULL;
    if (ret < 0) { goto cleanup_landmark; }
    ret = rknn_set_core_mask(app_ctx->rknn_ctx_iris, RKNN_NPU_CORE_0); // Can share core or use another
    if(ret < 0) { printf("WARN: rknn_set_core_mask(iris, CORE_0) failed! ret=%d\n", ret); } else { printf("INFO: Iris Landmark Model assigned to NPU Core 0.\n"); }
    ret = rknn_query(app_ctx->rknn_ctx_iris, RKNN_QUERY_IN_OUT_NUM, &app_ctx->io_num_iris, sizeof(app_ctx->io_num_iris));
    if (ret != RKNN_SUCC || app_ctx->io_num_iris.n_output < 2) { goto cleanup_iris; }
    app_ctx->input_attrs_iris = (rknn_tensor_attr*)malloc(app_ctx->io_num_iris.n_input * sizeof(rknn_tensor_attr));
    if (!app_ctx->input_attrs_iris) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_iris; }
    memset(app_ctx->input_attrs_iris, 0, app_ctx->io_num_iris.n_input * sizeof(rknn_tensor_attr));
    for (uint32_t i = 0; i < app_ctx->io_num_iris.n_input; i++) { app_ctx->input_attrs_iris[i].index = i; ret = rknn_query(app_ctx->rknn_ctx_iris, RKNN_QUERY_NATIVE_INPUT_ATTR, &app_ctx->input_attrs_iris[i], sizeof(rknn_tensor_attr)); if (ret != RKNN_SUCC) goto cleanup_iris; dump_tensor_attr(&app_ctx->input_attrs_iris[i], "IrisIn(Native)"); }
    app_ctx->output_attrs_iris = (rknn_tensor_attr*)malloc(app_ctx->io_num_iris.n_output * sizeof(rknn_tensor_attr));
    if (!app_ctx->output_attrs_iris) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_iris; }
    memset(app_ctx->output_attrs_iris, 0, app_ctx->io_num_iris.n_output * sizeof(rknn_tensor_attr));
    for (uint32_t i = 0; i < app_ctx->io_num_iris.n_output; i++) { app_ctx->output_attrs_iris[i].index = i; ret = rknn_query(app_ctx->rknn_ctx_iris, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &app_ctx->output_attrs_iris[i], sizeof(rknn_tensor_attr)); if (ret != RKNN_SUCC) goto cleanup_iris; dump_tensor_attr(&app_ctx->output_attrs_iris[i], "IrisOut(Native)"); }
    if (app_ctx->io_num_iris.n_input > 0) { app_ctx->input_attrs_iris[0].type = RKNN_TENSOR_UINT8; app_ctx->input_attrs_iris[0].fmt = RKNN_TENSOR_NHWC; app_ctx->input_fmt_iris = RKNN_TENSOR_NHWC; }
    app_ctx->input_mems_iris = (rknn_tensor_mem**)malloc(sizeof(rknn_tensor_mem*) * app_ctx->io_num_iris.n_input);
    if(!app_ctx->input_mems_iris){ ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_iris; } memset(app_ctx->input_mems_iris, 0, sizeof(rknn_tensor_mem*) * app_ctx->io_num_iris.n_input);
    for(uint32_t i = 0; i < app_ctx->io_num_iris.n_input; ++i) { app_ctx->input_mems_iris[i] = rknn_create_mem(app_ctx->rknn_ctx_iris, app_ctx->input_attrs_iris[i].size_with_stride); if(!app_ctx->input_mems_iris[i]) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_iris; } /* Set IO mem later */ }
    app_ctx->output_mems_iris = (rknn_tensor_mem**)malloc(sizeof(rknn_tensor_mem*) * app_ctx->io_num_iris.n_output);
    if(!app_ctx->output_mems_iris){ ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_iris; } memset(app_ctx->output_mems_iris, 0, sizeof(rknn_tensor_mem*) * app_ctx->io_num_iris.n_output);
    for(uint32_t i = 0; i < app_ctx->io_num_iris.n_output; ++i) { app_ctx->output_mems_iris[i] = rknn_create_mem(app_ctx->rknn_ctx_iris, app_ctx->output_attrs_iris[i].size_with_stride); if(!app_ctx->output_mems_iris[i]) { ret = RKNN_ERR_MALLOC_FAIL; goto cleanup_iris; } ret = rknn_set_io_mem(app_ctx->rknn_ctx_iris, app_ctx->output_mems_iris[i], &app_ctx->output_attrs_iris[i]); if (ret < 0) goto cleanup_iris; }
    app_ctx->model3_height = app_ctx->input_attrs_iris[0].dims[1]; app_ctx->model3_width = app_ctx->input_attrs_iris[0].dims[2]; app_ctx->model3_channel = app_ctx->input_attrs_iris[0].dims[3];

    printf("INFO: Face Analyzer init successful.\n");
    return 0; // Success

// --- Cleanup Labels ---
cleanup_iris:
    if (app_ctx->rknn_ctx_iris != 0) rknn_destroy(app_ctx->rknn_ctx_iris);
    if (app_ctx->input_attrs_iris) free(app_ctx->input_attrs_iris);
    if (app_ctx->output_attrs_iris) free(app_ctx->output_attrs_iris);
    if(app_ctx->input_mems_iris) { for(uint32_t i=0; i<app_ctx->io_num_iris.n_input; ++i) if(app_ctx->input_mems_iris[i]) rknn_destroy_mem(app_ctx->rknn_ctx_iris, app_ctx->input_mems_iris[i]); free(app_ctx->input_mems_iris); }
    if(app_ctx->output_mems_iris) { for(uint32_t i=0; i<app_ctx->io_num_iris.n_output; ++i) if(app_ctx->output_mems_iris[i]) rknn_destroy_mem(app_ctx->rknn_ctx_iris, app_ctx->output_mems_iris[i]); free(app_ctx->output_mems_iris); }
cleanup_landmark:
    if (app_ctx->rknn_ctx_landmark != 0) rknn_destroy(app_ctx->rknn_ctx_landmark);
    if (app_ctx->input_attrs_landmark) free(app_ctx->input_attrs_landmark);
    if (app_ctx->output_attrs_landmark) free(app_ctx->output_attrs_landmark);
    if(app_ctx->input_mems_landmark) { for(uint32_t i=0; i<app_ctx->io_num_landmark.n_input; ++i) if(app_ctx->input_mems_landmark[i]) rknn_destroy_mem(app_ctx->rknn_ctx_landmark, app_ctx->input_mems_landmark[i]); free(app_ctx->input_mems_landmark); }
    if(app_ctx->output_mems_landmark) { for(uint32_t i=0; i<app_ctx->io_num_landmark.n_output; ++i) if(app_ctx->output_mems_landmark[i]) rknn_destroy_mem(app_ctx->rknn_ctx_landmark, app_ctx->output_mems_landmark[i]); free(app_ctx->output_mems_landmark); }
cleanup_detect:
    if (app_ctx->rknn_ctx_detect != 0) rknn_destroy(app_ctx->rknn_ctx_detect);
    if (app_ctx->input_attrs_detect) free(app_ctx->input_attrs_detect);
    if (app_ctx->output_attrs_detect) free(app_ctx->output_attrs_detect);
    // Use correct counts for detect mem cleanup
    if(app_ctx->input_mems_detect) { for(uint32_t i=0; i<app_ctx->io_num_detect.n_input; ++i) if(app_ctx->input_mems_detect[i]) rknn_destroy_mem(app_ctx->rknn_ctx_detect, app_ctx->input_mems_detect[i]); free(app_ctx->input_mems_detect); }
    if(app_ctx->output_mems_detect) { for(uint32_t i=0; i<app_ctx->io_num_detect.n_output; ++i) if(app_ctx->output_mems_detect[i]) rknn_destroy_mem(app_ctx->rknn_ctx_detect, app_ctx->output_mems_detect[i]); free(app_ctx->output_mems_detect); }
    memset(app_ctx, 0, sizeof(face_analyzer_app_context_t));
    printf("ERROR: init_face_analyzer failed during cleanup stage with ret=%d\n", ret);
    return ret > 0 ? -ret : ret;
}

// --- Release Function (Keep original, already correct) ---
int release_face_analyzer(face_analyzer_app_context_t *app_ctx) {
    // ... (Keep the existing implementation for release_face_analyzer) ...
    // Release Iris resources
    if(app_ctx->input_mems_iris) { for(uint32_t i=0; i<app_ctx->io_num_iris.n_input; ++i) { if(app_ctx->input_mems_iris[i]) { rknn_destroy_mem(app_ctx->rknn_ctx_iris, app_ctx->input_mems_iris[i]); app_ctx->input_mems_iris[i] = NULL; } } free(app_ctx->input_mems_iris); app_ctx->input_mems_iris = NULL; }
    if(app_ctx->output_mems_iris) { for(uint32_t i=0; i<app_ctx->io_num_iris.n_output; ++i) { if(app_ctx->output_mems_iris[i]) { rknn_destroy_mem(app_ctx->rknn_ctx_iris, app_ctx->output_mems_iris[i]); app_ctx->output_mems_iris[i] = NULL; } } free(app_ctx->output_mems_iris); app_ctx->output_mems_iris = NULL; }
    if (app_ctx->input_attrs_iris) { free(app_ctx->input_attrs_iris); app_ctx->input_attrs_iris = NULL; }
    if (app_ctx->output_attrs_iris) { free(app_ctx->output_attrs_iris); app_ctx->output_attrs_iris = NULL; }
    if (app_ctx->rknn_ctx_iris != 0) { rknn_destroy(app_ctx->rknn_ctx_iris); app_ctx->rknn_ctx_iris = 0; }

    // Release Landmark resources
     if(app_ctx->input_mems_landmark) { for(uint32_t i=0; i<app_ctx->io_num_landmark.n_input; ++i) { if(app_ctx->input_mems_landmark[i]) { rknn_destroy_mem(app_ctx->rknn_ctx_landmark, app_ctx->input_mems_landmark[i]); app_ctx->input_mems_landmark[i] = NULL; } } free(app_ctx->input_mems_landmark); app_ctx->input_mems_landmark = NULL; }
    if(app_ctx->output_mems_landmark) { for(uint32_t i=0; i<app_ctx->io_num_landmark.n_output; ++i) { if(app_ctx->output_mems_landmark[i]) { rknn_destroy_mem(app_ctx->rknn_ctx_landmark, app_ctx->output_mems_landmark[i]); app_ctx->output_mems_landmark[i] = NULL; } } free(app_ctx->output_mems_landmark); app_ctx->output_mems_landmark = NULL; }
    if (app_ctx->input_attrs_landmark) { free(app_ctx->input_attrs_landmark); app_ctx->input_attrs_landmark = NULL; }
    if (app_ctx->output_attrs_landmark) { free(app_ctx->output_attrs_landmark); app_ctx->output_attrs_landmark = NULL; }
    if (app_ctx->rknn_ctx_landmark != 0) { rknn_destroy(app_ctx->rknn_ctx_landmark); app_ctx->rknn_ctx_landmark = 0; }

    // Release Detection (RetinaFace) resources
     if(app_ctx->input_mems_detect) { for(uint32_t i=0; i < app_ctx->io_num_detect.n_input; ++i) { if(app_ctx->input_mems_detect[i]) { rknn_destroy_mem(app_ctx->rknn_ctx_detect, app_ctx->input_mems_detect[i]); app_ctx->input_mems_detect[i] = NULL; } } free(app_ctx->input_mems_detect); app_ctx->input_mems_detect = NULL; }
    if(app_ctx->output_mems_detect) { for(uint32_t i=0; i < app_ctx->io_num_detect.n_output; ++i) { if(app_ctx->output_mems_detect[i]) { rknn_destroy_mem(app_ctx->rknn_ctx_detect, app_ctx->output_mems_detect[i]); app_ctx->output_mems_detect[i] = NULL; } } free(app_ctx->output_mems_detect); app_ctx->output_mems_detect = NULL; }
    if (app_ctx->input_attrs_detect) { free(app_ctx->input_attrs_detect); app_ctx->input_attrs_detect = NULL; }
    if (app_ctx->output_attrs_detect) { free(app_ctx->output_attrs_detect); app_ctx->output_attrs_detect = NULL; }
    if (app_ctx->rknn_ctx_detect != 0) { rknn_destroy(app_ctx->rknn_ctx_detect); app_ctx->rknn_ctx_detect = 0; }

    printf("INFO: Face Analyzer released successfully.\n");
    return 0;
}


// --- Main Inference Function (Modified for RetinaFace Detection) ---
// File: face_analyzer.cc

int inference_face_analyzer(face_analyzer_app_context_t *app_ctx,
                            image_buffer_t *src_img,
                            face_analyzer_result_t *out_result) {
    int ret = 0;
    memset(out_result, 0, sizeof(face_analyzer_result_t));
    letterbox_t det_letter_box;
    memset(&det_letter_box, 0, sizeof(letterbox_t));
    int bg_color = 114;

    // --- Stage 1: Face Detection (RetinaFace) ---
    if (!app_ctx || !app_ctx->input_mems_detect || !app_ctx->input_mems_detect[0] || !app_ctx->input_mems_detect[0]->virt_addr) {
        printf("ERROR: RetinaFace detection input memory not initialized.\n"); return -1;
    }
    image_buffer_t detect_input_buf_wrapper; /* ... setup wrapper ... */
    detect_input_buf_wrapper.width = app_ctx->model1_width; detect_input_buf_wrapper.height = app_ctx->model1_height; detect_input_buf_wrapper.format = IMAGE_FORMAT_RGB888; detect_input_buf_wrapper.virt_addr = (unsigned char*)app_ctx->input_mems_detect[0]->virt_addr; detect_input_buf_wrapper.size = app_ctx->input_mems_detect[0]->size; detect_input_buf_wrapper.fd = app_ctx->input_mems_detect[0]->fd; detect_input_buf_wrapper.width_stride = app_ctx->input_attrs_detect[0].w_stride; detect_input_buf_wrapper.height_stride = app_ctx->input_attrs_detect[0].h_stride ? app_ctx->input_attrs_detect[0].h_stride : app_ctx->model1_height;

    ret = convert_image_with_letterbox(src_img, &detect_input_buf_wrapper, &det_letter_box, bg_color);
    if (ret < 0) { return ret; }
    ret = rknn_set_io_mem(app_ctx->rknn_ctx_detect, app_ctx->input_mems_detect[0], &app_ctx->input_attrs_detect[0]);
    if (ret < 0) { return ret; }

// *** GUARD START: Detect Input Sync ***
#ifdef ZERO_COPY
    ret = rknn_mem_sync(app_ctx->rknn_ctx_detect, app_ctx->input_mems_detect[0], RKNN_MEMORY_SYNC_TO_DEVICE);
    if (ret < 0) { printf("ERROR: rknn_mem_sync (RetinaFace input) failed! ret=%d\n", ret); return ret; }
#endif
// *** GUARD END ***

    ret = rknn_run(app_ctx->rknn_ctx_detect, nullptr);
    if (ret < 0) { return ret; }

// *** GUARD START: Detect Output Sync ***
#ifdef ZERO_COPY
    for (uint32_t i = 0; i < app_ctx->io_num_detect.n_output; ++i) {
        ret = rknn_mem_sync(app_ctx->rknn_ctx_detect, app_ctx->output_mems_detect[i], RKNN_MEMORY_SYNC_FROM_DEVICE);
        if (ret < 0) { printf("ERROR: rknn_mem_sync (RetinaFace output %u) failed! ret=%d\n", i, ret); return ret; }
    }
#endif
// *** GUARD END ***

    ret = post_process_retinaface_detection_only(app_ctx->output_mems_detect, app_ctx, &det_letter_box, src_img->width, src_img->height, out_result);
    if (ret < 0) { return ret; }
    // --- End Stage 1 ---

    int num_faces_to_process = out_result->count;
    if (num_faces_to_process == 0) return 0;

    // --- Setup Wrappers & Buffers for Landmark/Iris Loop ---
    image_buffer_t landmark_input_buf_wrapper; /* ... setup ... */
    landmark_input_buf_wrapper.width = app_ctx->model2_width; landmark_input_buf_wrapper.height = app_ctx->model2_height; landmark_input_buf_wrapper.format = IMAGE_FORMAT_RGB888; landmark_input_buf_wrapper.virt_addr = (unsigned char*)app_ctx->input_mems_landmark[0]->virt_addr; landmark_input_buf_wrapper.size = app_ctx->input_mems_landmark[0]->size; landmark_input_buf_wrapper.fd = app_ctx->input_mems_landmark[0]->fd; landmark_input_buf_wrapper.width_stride = app_ctx->input_attrs_landmark[0].w_stride; landmark_input_buf_wrapper.height_stride = app_ctx->input_attrs_landmark[0].h_stride ? app_ctx->input_attrs_landmark[0].h_stride : app_ctx->model2_height;
    image_buffer_t iris_input_buf_wrapper; /* ... setup ... */
    iris_input_buf_wrapper.width = app_ctx->model3_width; iris_input_buf_wrapper.height = app_ctx->model3_height; iris_input_buf_wrapper.format = IMAGE_FORMAT_RGB888; iris_input_buf_wrapper.virt_addr = (unsigned char*)app_ctx->input_mems_iris[0]->virt_addr; iris_input_buf_wrapper.size = app_ctx->input_mems_iris[0]->size; iris_input_buf_wrapper.fd = app_ctx->input_mems_iris[0]->fd; iris_input_buf_wrapper.width_stride = app_ctx->input_attrs_iris[0].w_stride; iris_input_buf_wrapper.height_stride = app_ctx->input_attrs_iris[0].h_stride ? app_ctx->input_attrs_iris[0].h_stride : app_ctx->model3_height;

    // --- Loop for Landmarks and Iris ---
    for (int i = 0; i < num_faces_to_process; ++i) {
        face_object_t* current_face_result = &out_result->faces[i];

        // --- Stage 2: Face Landmarks ---
        letterbox_t lmk_letter_box;
        image_rect_t src_rect_lmk;
        image_rect_t dst_rect_lmk;
        box_rect_t face_crop_roi_for_post; // Keep for post-processing

        int center_x = (current_face_result->box.left + current_face_result->box.right) / 2; int center_y = (current_face_result->box.top + current_face_result->box.bottom) / 2; int box_w = current_face_result->box.right - current_face_result->box.left; int box_h = current_face_result->box.bottom - current_face_result->box.top;
        int crop_w_lmk = (int)(box_w * BOX_SCALE_X); int crop_h_lmk = (int)(box_h * BOX_SCALE_Y);
        src_rect_lmk.left = center_x - crop_w_lmk / 2; src_rect_lmk.top = center_y - crop_h_lmk / 2; src_rect_lmk.right = src_rect_lmk.left + crop_w_lmk; src_rect_lmk.bottom = src_rect_lmk.top + crop_h_lmk;
        face_crop_roi_for_post = {src_rect_lmk.left, src_rect_lmk.top, src_rect_lmk.right, src_rect_lmk.bottom}; // Store original crop region

        ret = calculate_letterbox_rects(crop_w_lmk, crop_h_lmk, app_ctx->model2_width, app_ctx->model2_height, 1, &lmk_letter_box, &dst_rect_lmk);
        if (ret < 0) { continue; }

        ret = convert_image(src_img, &landmark_input_buf_wrapper, &src_rect_lmk, &dst_rect_lmk, bg_color);
        if (ret < 0) { /* warning or continue */ }

        ret = rknn_set_io_mem(app_ctx->rknn_ctx_landmark, app_ctx->input_mems_landmark[0], &app_ctx->input_attrs_landmark[0]);
        if (ret < 0) { continue; }

// *** GUARD START: Landmark Input Sync ***
#ifdef ZERO_COPY
        ret = rknn_mem_sync(app_ctx->rknn_ctx_landmark, app_ctx->input_mems_landmark[0], RKNN_MEMORY_SYNC_TO_DEVICE);
        if (ret < 0) { continue; }
#endif
// *** GUARD END ***

        ret = rknn_run(app_ctx->rknn_ctx_landmark, nullptr);
        if (ret < 0) { continue; }

// *** GUARD START: Landmark Output Sync ***
#ifdef ZERO_COPY
        ret = rknn_mem_sync(app_ctx->rknn_ctx_landmark, app_ctx->output_mems_landmark[0], RKNN_MEMORY_SYNC_FROM_DEVICE);
        if (ret < 0) { continue; }
#endif
// *** GUARD END ***

        ret = post_process_face_landmarks(app_ctx->output_mems_landmark[0], app_ctx, face_crop_roi_for_post, &lmk_letter_box, current_face_result->face_landmarks);
        if (ret == 0) { current_face_result->face_landmarks_valid = true; }
        else { continue; }


        // --- Stage 3: Iris Landmarks ---
        if (current_face_result->face_landmarks_valid) {
             // ... (Constants for eye indices) ...
             const int LEFT_EYE_ROI_IDX1 = 33; const int LEFT_EYE_ROI_IDX2 = 133;
             const int RIGHT_EYE_ROI_IDX1 = 362; const int RIGHT_EYE_ROI_IDX2 = 263;

            // --- Process Left Eye ---
            letterbox_t iris_letter_box_left; image_rect_t src_rect_iris_left; image_rect_t dst_rect_iris_left; box_rect_t left_eye_crop_roi_for_post;
            if (LEFT_EYE_ROI_IDX1 < NUM_FACE_LANDMARKS && LEFT_EYE_ROI_IDX2 < NUM_FACE_LANDMARKS) {
                // ... Calculate src_rect_iris_left and left_eye_crop_roi_for_post ...
                 point_t left_p1=current_face_result->face_landmarks[LEFT_EYE_ROI_IDX1]; point_t left_p2=current_face_result->face_landmarks[LEFT_EYE_ROI_IDX2]; int left_eye_cx=(left_p1.x+left_p2.x)/2; int left_eye_cy=(left_p1.y+left_p2.y)/2; int left_eye_w=abs(left_p1.x-left_p2.x); int left_eye_h=abs(left_p1.y-left_p2.y); int left_eye_size=(int)(std::max({left_eye_w,left_eye_h,1}) * EYE_CROP_SCALE); src_rect_iris_left.left=left_eye_cx-left_eye_size/2; src_rect_iris_left.top=left_eye_cy-left_eye_size/2; src_rect_iris_left.right=src_rect_iris_left.left+left_eye_size; src_rect_iris_left.bottom=src_rect_iris_left.top+left_eye_size; left_eye_crop_roi_for_post = {src_rect_iris_left.left, src_rect_iris_left.top, src_rect_iris_left.right, src_rect_iris_left.bottom};

                ret = calculate_letterbox_rects(left_eye_size, left_eye_size, app_ctx->model3_width, app_ctx->model3_height, 1, &iris_letter_box_left, &dst_rect_iris_left);
                if (ret == 0) {
                    ret = convert_image(src_img, &iris_input_buf_wrapper, &src_rect_iris_left, &dst_rect_iris_left, bg_color);
                    if (ret == 0) {
                        ret = rknn_set_io_mem(app_ctx->rknn_ctx_iris, app_ctx->input_mems_iris[0], &app_ctx->input_attrs_iris[0]); if (ret<0) { /* handle error */ }
// *** GUARD START: Iris Input Sync (Left) ***
#ifdef ZERO_COPY
                        ret = rknn_mem_sync(app_ctx->rknn_ctx_iris, app_ctx->input_mems_iris[0], RKNN_MEMORY_SYNC_TO_DEVICE); if (ret<0) { /* handle error */ }
#endif
// *** GUARD END ***
                        ret = rknn_run(app_ctx->rknn_ctx_iris, nullptr);
                        if (ret == 0) {
// *** GUARD START: Iris Output Sync (Left) ***
#ifdef ZERO_COPY
                            for (uint32_t j=0; j<app_ctx->io_num_iris.n_output; ++j) { ret = rknn_mem_sync(app_ctx->rknn_ctx_iris, app_ctx->output_mems_iris[j], RKNN_MEMORY_SYNC_FROM_DEVICE); if(ret<0) break; }
#endif
// *** GUARD END ***
                            if(ret == 0) {
                                ret = post_process_iris_landmarks(app_ctx->output_mems_iris, app_ctx, left_eye_crop_roi_for_post, &iris_letter_box_left, current_face_result->eye_landmarks_left, current_face_result->iris_landmarks_left);
                                if (ret == 0) { current_face_result->eye_landmarks_left_valid = true; current_face_result->iris_landmarks_left_valid = true; }
                            }
                        } else { /* handle error */ }
                    } else { /* handle error */ }
                } else { /* handle error */ }
            }

            // --- Process Right Eye (Similar logic with guards) ---
            letterbox_t iris_letter_box_right; image_rect_t src_rect_iris_right; image_rect_t dst_rect_iris_right; box_rect_t right_eye_crop_roi_for_post;
            if (RIGHT_EYE_ROI_IDX1 < NUM_FACE_LANDMARKS && RIGHT_EYE_ROI_IDX2 < NUM_FACE_LANDMARKS) {
                // ... Calculate src_rect_iris_right and right_eye_crop_roi_for_post ...
                 point_t right_p1=current_face_result->face_landmarks[RIGHT_EYE_ROI_IDX1]; point_t right_p2=current_face_result->face_landmarks[RIGHT_EYE_ROI_IDX2]; int right_eye_cx=(right_p1.x+right_p2.x)/2; int right_eye_cy=(right_p1.y+right_p2.y)/2; int right_eye_w=abs(right_p1.x-right_p2.x); int right_eye_h=abs(right_p1.y-right_p2.y); int right_eye_size=(int)(std::max({right_eye_w,right_eye_h,1}) * EYE_CROP_SCALE); src_rect_iris_right.left=right_eye_cx-right_eye_size/2; src_rect_iris_right.top=right_eye_cy-right_eye_size/2; src_rect_iris_right.right=src_rect_iris_right.left+right_eye_size; src_rect_iris_right.bottom=src_rect_iris_right.top+right_eye_size; right_eye_crop_roi_for_post = {src_rect_iris_right.left, src_rect_iris_right.top, src_rect_iris_right.right, src_rect_iris_right.bottom};

                ret = calculate_letterbox_rects(right_eye_size, right_eye_size, app_ctx->model3_width, app_ctx->model3_height, 1, &iris_letter_box_right, &dst_rect_iris_right);
                if (ret == 0) {
                    ret = convert_image(src_img, &iris_input_buf_wrapper, &src_rect_iris_right, &dst_rect_iris_right, bg_color);
                    if (ret == 0) {
                        ret = rknn_set_io_mem(app_ctx->rknn_ctx_iris, app_ctx->input_mems_iris[0], &app_ctx->input_attrs_iris[0]); if (ret < 0) { /* handle error */ }
// *** GUARD START: Iris Input Sync (Right) ***
#ifdef ZERO_COPY
                        ret = rknn_mem_sync(app_ctx->rknn_ctx_iris, app_ctx->input_mems_iris[0], RKNN_MEMORY_SYNC_TO_DEVICE); if (ret < 0) { /* handle error */ }
#endif
// *** GUARD END ***
                        ret = rknn_run(app_ctx->rknn_ctx_iris, nullptr);
                        if (ret == 0) {
// *** GUARD START: Iris Output Sync (Right) ***
#ifdef ZERO_COPY
                            for (uint32_t j=0; j<app_ctx->io_num_iris.n_output; ++j) { ret = rknn_mem_sync(app_ctx->rknn_ctx_iris, app_ctx->output_mems_iris[j], RKNN_MEMORY_SYNC_FROM_DEVICE); if(ret<0) break; }
#endif
// *** GUARD END ***
                            if(ret == 0) {
                                ret = post_process_iris_landmarks(app_ctx->output_mems_iris, app_ctx, right_eye_crop_roi_for_post, &iris_letter_box_right, current_face_result->eye_landmarks_right, current_face_result->iris_landmarks_right);
                                if (ret == 0) { current_face_result->eye_landmarks_right_valid = true; current_face_result->iris_landmarks_right_valid = true; }
                            }
                        } else { /* handle error */ }
                    } else { /* handle error */ }
                } else { /* handle error */ }
            }
        } // End if face landmarks valid
    } // End loop over detected faces

    return 0; // Success
}