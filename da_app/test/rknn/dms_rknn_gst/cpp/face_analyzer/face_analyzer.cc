#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <algorithm>

// Include RKNN API first
#include "rknn_api.h"

// Include common utilities
#include "common.h"
#include "file_utils.h"
#include "image_utils.h"

// Include the header for this specific implementation last
#include "face_analyzer.h"


#define NMS_THRESHOLD 0.45
#define FACE_CONF_THRESHOLD 0.5
#define BOX_SCALE_X 1.5 // Scaling for face crop for face landmark model
#define BOX_SCALE_Y 1.7 // Scaling for face crop for face landmark model
#define EYE_CROP_SCALE 1.8 // Scaling factor for eye crop relative to eye landmarks
#define IRIS_CROP_SCALE 2.8 // Scaling factor for iris crop relative to iris center (if needed, currently uses EYE_CROP_SCALE)


// --- Anchor Generation Structs and Function ---
struct AnchorOptions {
    int input_size_width = 128; int input_size_height = 128;
    std::vector<int> strides = {8, 16}; int num_layers = 2;
    std::vector<int> anchors_per_location = {2, 6};
    float anchor_offset_x = 0.5f; float anchor_offset_y = 0.5f;
    float fixed_anchor_size = 1.0f;
};
struct Anchor { float x_center, y_center, h, w; };

static std::vector<Anchor> generate_anchors(const AnchorOptions& options) {
    std::vector<Anchor> anchors;
    int layer_id = 0;
    while (layer_id < options.num_layers) {
        int feature_map_height = ceil((float)options.input_size_height / options.strides[layer_id]);
        int feature_map_width = ceil((float)options.input_size_width / options.strides[layer_id]);
        int anchors_per_loc = options.anchors_per_location[layer_id];

        for (int y = 0; y < feature_map_height; ++y) {
            for (int x = 0; x < feature_map_width; ++x) {
                for (int anchor_id = 0; anchor_id < anchors_per_loc; ++anchor_id) {
                    float x_center = (x + options.anchor_offset_x) * (1.0f / feature_map_width);
                    float y_center = (y + options.anchor_offset_y) * (1.0f / feature_map_height);
                    float w = options.fixed_anchor_size;
                    float h = options.fixed_anchor_size;
                    anchors.push_back({x_center, y_center, h, w});
                }
            }
        }
        layer_id++;
    }
    printf("Generated %zu anchors.\n", anchors.size());
    // Ensure the number matches the model output (e.g., 896)
    if (anchors.size() != 896) {
         printf("WARN: Generated anchor count (%zu) != expected (896)!\n", anchors.size());
    }
    return anchors;
}
// --- End Anchor Generation ---

// --- Utility Functions ---
static inline float dequantize_int8_to_float(int8_t val, int32_t zp, float scale) { return ((float)val - (float)zp) * scale; }
static int clamp(int x, int min, int max) { if (x > max) return max; if (x < min) return min; return x; }
static float clampf(float x, float min, float max) { if (x > max) return max; if (x < min) return min; return x; }

static void dump_tensor_attr(rknn_tensor_attr *attr, const char* model_name) {
    const char* fmt_str = get_format_string(attr->fmt);
    const char* type_str = get_type_string(attr->type);
    const char* qnt_type_str = get_qnt_type_string(attr->qnt_type);

    printf("%s Tensor: index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s(%d), type=%s(%d), qnt_type=%s(%d), zp=%d, scale=%f\n",
           model_name, attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3],
           attr->n_elems, attr->size,
           fmt_str ? fmt_str : "UNK", (int)attr->fmt,
           type_str ? type_str : "UNK", (int)attr->type,
           qnt_type_str ? qnt_type_str : "UNK", (int)attr->qnt_type,
           attr->zp, attr->scale);
}

static int crop_image(image_buffer_t *src_img, image_buffer_t *dst_img, box_rect_t crop_box) {
    if (!src_img || !src_img->virt_addr || !dst_img) { return -1; }
    bool format_supported = false; int channels = 0;
    if (src_img->format == IMAGE_FORMAT_RGB888) {
        format_supported = true; channels = 3;
    }
// Ensure preprocessor directives are on separate lines
#ifdef IMAGE_FORMAT_BGR888
    else if (src_img->format == IMAGE_FORMAT_BGR888) {
        format_supported = true; channels = 3;
    }
#endif // IMAGE_FORMAT_BGR888
    else if (src_img->format == IMAGE_FORMAT_GRAY8) {
        format_supported = true; channels = 1;
    }
    if (!format_supported) { return -1; }
    int src_w = src_img->width; int src_h = src_img->height; int crop_x = crop_box.left; int crop_y = crop_box.top; int crop_w = crop_box.right - crop_box.left; int crop_h = crop_box.bottom - crop_box.top; if (crop_w <= 0 || crop_h <= 0) { return -1; }
    dst_img->width = crop_w; dst_img->height = crop_h; dst_img->format = src_img->format; dst_img->size = crop_w * crop_h * channels; if (dst_img->virt_addr == NULL) { dst_img->virt_addr = (unsigned char*)malloc(dst_img->size); if (!dst_img->virt_addr) { return -1; } } memset(dst_img->virt_addr, 0, dst_img->size);
    unsigned char* src_data = src_img->virt_addr; unsigned char* dst_data = dst_img->virt_addr; size_t src_stride = src_w * channels; size_t dst_stride = crop_w * channels;
    for (int y = 0; y < crop_h; ++y) { int src_y = crop_y + y; if (src_y < 0 || src_y >= src_h) { continue; } unsigned char* src_row_ptr = src_data + (size_t)src_y * src_stride; unsigned char* dst_row_ptr = dst_data + (size_t)y * dst_stride; for (int x = 0; x < crop_w; ++x) { int src_x = crop_x + x; if (src_x >= 0 && src_x < src_w) { memcpy(dst_row_ptr + x * channels, src_row_ptr + src_x * channels, channels); } } } return 0;
}

static float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1, float ymax1) { float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1)); float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1)); float i = w * h; float u = (xmax0 - xmin0) * (ymax0 - ymin0) + (xmax1 - xmin1) * (ymax1 - ymin1) - i; return u <= 0.f ? 0.f : (i / u); }
struct DetectionCandidate { int index; float score; box_rect_t box; bool operator<(const DetectionCandidate& other) const { return score < other.score; } };

static void nms(std::vector<DetectionCandidate>& candidates, float threshold) {
    if (candidates.empty()) return;
    std::sort(candidates.begin(), candidates.end(), [](const DetectionCandidate& a, const DetectionCandidate& b) { return a.score > b.score; });
    std::vector<bool> removed(candidates.size(), false); std::vector<DetectionCandidate> result_candidates;
    for (size_t i = 0; i < candidates.size(); ++i) { if (removed[i]) continue; result_candidates.push_back(candidates[i]); for (size_t j = i + 1; j < candidates.size(); ++j) { if (removed[j]) continue; float iou = CalculateOverlap((float)candidates[i].box.left, (float)candidates[i].box.top, (float)candidates[i].box.right, (float)candidates[i].box.bottom, (float)candidates[j].box.left, (float)candidates[j].box.top, (float)candidates[j].box.right, (float)candidates[j].box.bottom); if (iou > threshold) removed[j] = true; } }
    candidates = result_candidates;
}
// --- End Utility Functions ---

// --- Post Processing Functions ---
static int post_process_face_detection(rknn_output outputs[], face_analyzer_app_context_t *app_ctx, const letterbox_t *det_letter_box, int src_img_width, int src_img_height, std::vector<DetectionCandidate>& out_faces) {
    AnchorOptions anchor_opts; anchor_opts.input_size_width = app_ctx->model1_width; anchor_opts.input_size_height = app_ctx->model1_height; std::vector<Anchor> anchors = generate_anchors(anchor_opts);
    if (app_ctx->io_num_detect.n_output < 2) { return -1; } rknn_tensor_attr* box_attr = &app_ctx->output_attrs_detect[0]; rknn_tensor_attr* score_attr = &app_ctx->output_attrs_detect[1]; if (box_attr->n_dims < 2 || score_attr->n_dims < 2 || box_attr->dims[1] != score_attr->dims[1] || box_attr->dims[box_attr->n_dims - 1] < 4 || score_attr->dims[score_attr->n_dims - 1] != 1) { return -1; } int num_anchors = box_attr->dims[1]; int box_regressors_per_anchor = box_attr->dims[box_attr->n_dims - 1]; if (anchors.size() != num_anchors) { return -1; }
    int8_t* box_data = (int8_t*)outputs[0].buf; int8_t* score_data = (int8_t*)outputs[1].buf; float box_scale = box_attr->scale; int32_t box_zp = box_attr->zp; float score_scale = score_attr->scale; int32_t score_zp = score_attr->zp; std::vector<DetectionCandidate> candidates; candidates.reserve(num_anchors); const float scale_factor = (float)app_ctx->model1_width;
    for (int i = 0; i < num_anchors; ++i) { float logit = dequantize_int8_to_float(score_data[i], score_zp, score_scale); float score = 1.0f / (1.0f + expf(-logit)); if (score >= FACE_CONF_THRESHOLD) { int box_offset = i * box_regressors_per_anchor; float x_offset = dequantize_int8_to_float(box_data[box_offset + 0], box_zp, box_scale); float y_offset = dequantize_int8_to_float(box_data[box_offset + 1], box_zp, box_scale); float w_scale = dequantize_int8_to_float(box_data[box_offset + 2], box_zp, box_scale); float h_scale = dequantize_int8_to_float(box_data[box_offset + 3], box_zp, box_scale); const Anchor& anchor = anchors[i]; float anchor_x_center = anchor.x_center; float anchor_y_center = anchor.y_center; float anchor_w = anchor.w; float anchor_h = anchor.h; float pred_x_center = (x_offset / scale_factor * anchor_w) + anchor_x_center; float pred_y_center = (y_offset / scale_factor * anchor_h) + anchor_y_center; float pred_w = (w_scale / scale_factor) * anchor_w; float pred_h = (h_scale / scale_factor) * anchor_h; float ymin_norm = pred_y_center - pred_h / 2.0f; float xmin_norm = pred_x_center - pred_w / 2.0f; float ymax_norm = pred_y_center + pred_h / 2.0f; float xmax_norm = pred_x_center + pred_w / 2.0f; ymin_norm = clampf(ymin_norm, 0.0f, 1.0f); xmin_norm = clampf(xmin_norm, 0.0f, 1.0f); ymax_norm = clampf(ymax_norm, 0.0f, 1.0f); xmax_norm = clampf(xmax_norm, 0.0f, 1.0f); float model_input_w = (float)app_ctx->model1_width; float model_input_h = (float)app_ctx->model1_height; float xmin_unscaled = (xmin_norm * model_input_w - det_letter_box->x_pad) / det_letter_box->scale; float ymin_unscaled = (ymin_norm * model_input_h - det_letter_box->y_pad) / det_letter_box->scale; float xmax_unscaled = (xmax_norm * model_input_w - det_letter_box->x_pad) / det_letter_box->scale; float ymax_unscaled = (ymax_norm * model_input_h - det_letter_box->y_pad) / det_letter_box->scale; int xmin = clamp((int)roundf(xmin_unscaled), 0, src_img_width - 1); int ymin = clamp((int)roundf(ymin_unscaled), 0, src_img_height - 1); int xmax = clamp((int)roundf(xmax_unscaled), 0, src_img_width - 1); int ymax = clamp((int)roundf(ymax_unscaled), 0, src_img_height - 1); if (xmax > xmin && ymax > ymin) { DetectionCandidate cand; cand.index = i; cand.score = score; cand.box.left = xmin; cand.box.top = ymin; cand.box.right = xmax; cand.box.bottom = ymax; candidates.push_back(cand); } } }
    printf("Initial face candidates: %zu\n", candidates.size()); nms(candidates, NMS_THRESHOLD); printf("Face candidates after NMS: %zu\n", candidates.size()); out_faces = candidates; return 0;
}

static int post_process_face_landmarks(rknn_output landmark_output, face_analyzer_app_context_t *app_ctx, const box_rect_t& crop_roi, const letterbox_t *lmk_letter_box, point_t out_landmarks[NUM_FACE_LANDMARKS]) {
    rknn_tensor_attr* landmark_attr = &app_ctx->output_attrs_landmark[0]; if (!((landmark_attr->n_dims == 4 && landmark_attr->dims[0] == 1 && landmark_attr->dims[1] == 1 && landmark_attr->dims[2] == 1 && landmark_attr->dims[3] == NUM_FACE_LANDMARKS * 3) || (landmark_attr->n_dims == 2 && landmark_attr->dims[0] == 1 && landmark_attr->dims[1] == NUM_FACE_LANDMARKS * 3)) ) { return -1; } int num_coords = landmark_attr->dims[landmark_attr->n_dims - 1]; if (num_coords != NUM_FACE_LANDMARKS * 3) { return -1; }
    int8_t* landmark_data = (int8_t*)landmark_output.buf; float landmark_scale_zp = landmark_attr->scale; int32_t landmark_zp = landmark_attr->zp; float model_input_w = (float)app_ctx->model2_width; float model_input_h = (float)app_ctx->model2_height; float crop_roi_w = (float)(crop_roi.right - crop_roi.left); float crop_roi_h = (float)(crop_roi.bottom - crop_roi.top); if (crop_roi_w <= 0 || crop_roi_h <= 0) { return -1; }
    for (int i = 0; i < NUM_FACE_LANDMARKS; ++i) { int offset = i * 3; float x_lmk_quant = dequantize_int8_to_float(landmark_data[offset + 0], landmark_zp, landmark_scale_zp); float y_lmk_quant = dequantize_int8_to_float(landmark_data[offset + 1], landmark_zp, landmark_scale_zp); float x_scaled_crop = x_lmk_quant - lmk_letter_box->x_pad; float y_scaled_crop = y_lmk_quant - lmk_letter_box->y_pad; float x_orig_crop_rel = x_scaled_crop / lmk_letter_box->scale; float y_orig_crop_rel = y_scaled_crop / lmk_letter_box->scale; float final_x = x_orig_crop_rel + crop_roi.left; float final_y = y_orig_crop_rel + crop_roi.top; out_landmarks[i].x = (int)(final_x + 0.5f); out_landmarks[i].y = (int)(final_y + 0.5f); } return 0;
}

static int post_process_iris_landmarks(rknn_output iris_outputs[], face_analyzer_app_context_t *app_ctx, const box_rect_t& eye_crop_roi, const letterbox_t *iris_letter_box, point_t out_eye_contour[NUM_EYE_CONTOUR_LANDMARKS], point_t out_iris[NUM_IRIS_LANDMARKS]) {
    if (app_ctx->io_num_iris.n_output < 2) { return -1; } rknn_tensor_attr* eye_contour_attr = &app_ctx->output_attrs_iris[0]; rknn_tensor_attr* iris_pts_attr = &app_ctx->output_attrs_iris[1];
    if (!(eye_contour_attr->n_dims == 2 && eye_contour_attr->dims[0] == 1 && eye_contour_attr->dims[1] == NUM_EYE_CONTOUR_LANDMARKS * 3) && !(eye_contour_attr->n_dims == 4 && eye_contour_attr->dims[0]==1 && eye_contour_attr->dims[1]==1 && eye_contour_attr->dims[2]==1 && eye_contour_attr->dims[3] == NUM_EYE_CONTOUR_LANDMARKS * 3)) { return -1; } if (!(iris_pts_attr->n_dims == 2 && iris_pts_attr->dims[0] == 1 && iris_pts_attr->dims[1] == NUM_IRIS_LANDMARKS * 3) && !(iris_pts_attr->n_dims == 4 && iris_pts_attr->dims[0]==1 && iris_pts_attr->dims[1]==1 && iris_pts_attr->dims[2]==1 && iris_pts_attr->dims[3] == NUM_IRIS_LANDMARKS * 3)) { return -1; }
    int8_t* eye_data = (int8_t*)iris_outputs[0].buf; float eye_scale = eye_contour_attr->scale; int32_t eye_zp = eye_contour_attr->zp; float eye_crop_roi_w = (float)(eye_crop_roi.right - eye_crop_roi.left); float eye_crop_roi_h = (float)(eye_crop_roi.bottom - eye_crop_roi.top); if (eye_crop_roi_w <= 0 || eye_crop_roi_h <= 0) { return -1; }
    for (int i = 0; i < NUM_EYE_CONTOUR_LANDMARKS; ++i) { int offset = i * 3; float x_quant = dequantize_int8_to_float(eye_data[offset + 0], eye_zp, eye_scale); float y_quant = dequantize_int8_to_float(eye_data[offset + 1], eye_zp, eye_scale); float x_scaled = x_quant - iris_letter_box->x_pad; float y_scaled = y_quant - iris_letter_box->y_pad; float x_orig_rel = x_scaled / iris_letter_box->scale; float y_orig_rel = y_scaled / iris_letter_box->scale; float final_x = x_orig_rel + eye_crop_roi.left; float final_y = y_orig_rel + eye_crop_roi.top; out_eye_contour[i].x = (int)(final_x + 0.5f); out_eye_contour[i].y = (int)(final_y + 0.5f); }
    int8_t* iris_data = (int8_t*)iris_outputs[1].buf; float iris_scale = iris_pts_attr->scale; int32_t iris_zp = iris_pts_attr->zp;
    for (int i = 0; i < NUM_IRIS_LANDMARKS; ++i) { int offset = i * 3; float x_quant = dequantize_int8_to_float(iris_data[offset + 0], iris_zp, iris_scale); float y_quant = dequantize_int8_to_float(iris_data[offset + 1], iris_zp, iris_scale); float x_scaled = x_quant - iris_letter_box->x_pad; float y_scaled = y_quant - iris_letter_box->y_pad; float x_orig_rel = x_scaled / iris_letter_box->scale; float y_orig_rel = y_scaled / iris_letter_box->scale; float final_x = x_orig_rel + eye_crop_roi.left; float final_y = y_orig_rel + eye_crop_roi.top; out_iris[i].x = (int)(final_x + 0.5f); out_iris[i].y = (int)(final_y + 0.5f); } return 0;
}
// --- End Post Processing Functions ---

// --- Model Initialization and Release ---
int init_face_analyzer(const char *detection_model_path, const char* landmark_model_path, const char* iris_model_path, face_analyzer_app_context_t *app_ctx) {
    // --- Init Detect Model ---
    memset(app_ctx, 0, sizeof(face_analyzer_app_context_t)); int ret; int model_len = 0; unsigned char *model = NULL; printf("Loading face detection model: %s\n", detection_model_path); model_len = read_data_from_file(detection_model_path, reinterpret_cast<char**>(&model)); if (model == NULL) return -1; ret = rknn_init(&app_ctx->rknn_ctx_detect, model, model_len, 0, NULL); free(model); model = NULL; if (ret < 0) return -1; ret = rknn_query(app_ctx->rknn_ctx_detect, RKNN_QUERY_IN_OUT_NUM, &app_ctx->io_num_detect, sizeof(app_ctx->io_num_detect)); if (ret != RKNN_SUCC) goto cleanup_detect; printf("Detection Model: inputs=%d, outputs=%d\n", app_ctx->io_num_detect.n_input, app_ctx->io_num_detect.n_output); printf("Detection Model Input Tensors:\n"); app_ctx->input_attrs_detect = (rknn_tensor_attr*)malloc(app_ctx->io_num_detect.n_input * sizeof(rknn_tensor_attr)); memset(app_ctx->input_attrs_detect, 0, app_ctx->io_num_detect.n_input * sizeof(rknn_tensor_attr)); for (int i = 0; i < app_ctx->io_num_detect.n_input; i++) { app_ctx->input_attrs_detect[i].index = i; ret = rknn_query(app_ctx->rknn_ctx_detect, RKNN_QUERY_INPUT_ATTR, &app_ctx->input_attrs_detect[i], sizeof(rknn_tensor_attr)); if (ret != RKNN_SUCC) goto cleanup_detect; dump_tensor_attr(&app_ctx->input_attrs_detect[i], "DetectIn"); } printf("Detection Model Output Tensors:\n"); app_ctx->output_attrs_detect = (rknn_tensor_attr*)malloc(app_ctx->io_num_detect.n_output * sizeof(rknn_tensor_attr)); memset(app_ctx->output_attrs_detect, 0, app_ctx->io_num_detect.n_output * sizeof(rknn_tensor_attr)); for (int i = 0; i < app_ctx->io_num_detect.n_output; i++) { app_ctx->output_attrs_detect[i].index = i; ret = rknn_query(app_ctx->rknn_ctx_detect, RKNN_QUERY_OUTPUT_ATTR, &app_ctx->output_attrs_detect[i], sizeof(rknn_tensor_attr)); if (ret != RKNN_SUCC) goto cleanup_detect; dump_tensor_attr(&app_ctx->output_attrs_detect[i], "DetectOut"); } if (app_ctx->input_attrs_detect[0].fmt == RKNN_TENSOR_NCHW) { app_ctx->model1_channel = app_ctx->input_attrs_detect[0].dims[1]; app_ctx->model1_height = app_ctx->input_attrs_detect[0].dims[2]; app_ctx->model1_width = app_ctx->input_attrs_detect[0].dims[3]; } else { app_ctx->model1_height = app_ctx->input_attrs_detect[0].dims[1]; app_ctx->model1_width = app_ctx->input_attrs_detect[0].dims[2]; app_ctx->model1_channel = app_ctx->input_attrs_detect[0].dims[3]; } app_ctx->input_fmt_detect = app_ctx->input_attrs_detect[0].fmt; printf("Detection model input: H=%d, W=%d, C=%d\n", app_ctx->model1_height, app_ctx->model1_width, app_ctx->model1_channel);
    // --- Init Landmark Model ---
    printf("Loading face landmark model: %s\n", landmark_model_path); model_len = read_data_from_file(landmark_model_path, reinterpret_cast<char**>(&model)); if (model == NULL) goto cleanup_detect; ret = rknn_init(&app_ctx->rknn_ctx_landmark, model, model_len, 0, NULL); free(model); model = NULL; if (ret < 0) goto cleanup_detect; ret = rknn_query(app_ctx->rknn_ctx_landmark, RKNN_QUERY_IN_OUT_NUM, &app_ctx->io_num_landmark, sizeof(app_ctx->io_num_landmark)); if (ret != RKNN_SUCC) goto cleanup_landmark; printf("Landmark Model: inputs=%d, outputs=%d\n", app_ctx->io_num_landmark.n_input, app_ctx->io_num_landmark.n_output); printf("Landmark Model Input Tensors:\n"); app_ctx->input_attrs_landmark = (rknn_tensor_attr*)malloc(app_ctx->io_num_landmark.n_input * sizeof(rknn_tensor_attr)); memset(app_ctx->input_attrs_landmark, 0, app_ctx->io_num_landmark.n_input * sizeof(rknn_tensor_attr)); for (int i = 0; i < app_ctx->io_num_landmark.n_input; i++) { app_ctx->input_attrs_landmark[i].index = i; ret = rknn_query(app_ctx->rknn_ctx_landmark, RKNN_QUERY_INPUT_ATTR, &app_ctx->input_attrs_landmark[i], sizeof(rknn_tensor_attr)); if (ret != RKNN_SUCC) goto cleanup_landmark; dump_tensor_attr(&app_ctx->input_attrs_landmark[i], "LandmarkIn"); } printf("Landmark Model Output Tensors:\n"); app_ctx->output_attrs_landmark = (rknn_tensor_attr*)malloc(app_ctx->io_num_landmark.n_output * sizeof(rknn_tensor_attr)); memset(app_ctx->output_attrs_landmark, 0, app_ctx->io_num_landmark.n_output * sizeof(rknn_tensor_attr)); for (int i = 0; i < app_ctx->io_num_landmark.n_output; i++) { app_ctx->output_attrs_landmark[i].index = i; ret = rknn_query(app_ctx->rknn_ctx_landmark, RKNN_QUERY_OUTPUT_ATTR, &app_ctx->output_attrs_landmark[i], sizeof(rknn_tensor_attr)); if (ret != RKNN_SUCC) goto cleanup_landmark; dump_tensor_attr(&app_ctx->output_attrs_landmark[i], "LandmarkOut"); } if (app_ctx->input_attrs_landmark[0].fmt == RKNN_TENSOR_NCHW) { app_ctx->model2_channel = app_ctx->input_attrs_landmark[0].dims[1]; app_ctx->model2_height = app_ctx->input_attrs_landmark[0].dims[2]; app_ctx->model2_width = app_ctx->input_attrs_landmark[0].dims[3]; } else { app_ctx->model2_height = app_ctx->input_attrs_landmark[0].dims[1]; app_ctx->model2_width = app_ctx->input_attrs_landmark[0].dims[2]; app_ctx->model2_channel = app_ctx->input_attrs_landmark[0].dims[3]; } app_ctx->input_fmt_landmark = app_ctx->input_attrs_landmark[0].fmt; printf("Landmark model input: H=%d, W=%d, C=%d\n", app_ctx->model2_height, app_ctx->model2_width, app_ctx->model2_channel);
    // --- Init Iris Model ---
    printf("Loading iris landmark model: %s\n", iris_model_path); model_len = read_data_from_file(iris_model_path, reinterpret_cast<char**>(&model)); if (model == NULL) goto cleanup_landmark; ret = rknn_init(&app_ctx->rknn_ctx_iris, model, model_len, 0, NULL); free(model); model = NULL; if (ret < 0) goto cleanup_landmark; ret = rknn_query(app_ctx->rknn_ctx_iris, RKNN_QUERY_IN_OUT_NUM, &app_ctx->io_num_iris, sizeof(app_ctx->io_num_iris)); if (ret != RKNN_SUCC) goto cleanup_iris; printf("Iris Model: inputs=%d, outputs=%d\n", app_ctx->io_num_iris.n_input, app_ctx->io_num_iris.n_output); if (app_ctx->io_num_iris.n_output != 2) goto cleanup_iris; printf("Iris Model Input Tensors:\n"); app_ctx->input_attrs_iris = (rknn_tensor_attr*)malloc(app_ctx->io_num_iris.n_input * sizeof(rknn_tensor_attr)); memset(app_ctx->input_attrs_iris, 0, app_ctx->io_num_iris.n_input * sizeof(rknn_tensor_attr)); for (int i = 0; i < app_ctx->io_num_iris.n_input; i++) { app_ctx->input_attrs_iris[i].index = i; ret = rknn_query(app_ctx->rknn_ctx_iris, RKNN_QUERY_INPUT_ATTR, &app_ctx->input_attrs_iris[i], sizeof(rknn_tensor_attr)); if (ret != RKNN_SUCC) goto cleanup_iris; dump_tensor_attr(&app_ctx->input_attrs_iris[i], "IrisIn"); } printf("Iris Model Output Tensors:\n"); app_ctx->output_attrs_iris = (rknn_tensor_attr*)malloc(app_ctx->io_num_iris.n_output * sizeof(rknn_tensor_attr)); memset(app_ctx->output_attrs_iris, 0, app_ctx->io_num_iris.n_output * sizeof(rknn_tensor_attr)); for (int i = 0; i < app_ctx->io_num_iris.n_output; i++) { app_ctx->output_attrs_iris[i].index = i; ret = rknn_query(app_ctx->rknn_ctx_iris, RKNN_QUERY_OUTPUT_ATTR, &app_ctx->output_attrs_iris[i], sizeof(rknn_tensor_attr)); if (ret != RKNN_SUCC) goto cleanup_iris; dump_tensor_attr(&app_ctx->output_attrs_iris[i], "IrisOut"); } if (app_ctx->input_attrs_iris[0].fmt == RKNN_TENSOR_NCHW) { app_ctx->model3_channel = app_ctx->input_attrs_iris[0].dims[1]; app_ctx->model3_height = app_ctx->input_attrs_iris[0].dims[2]; app_ctx->model3_width = app_ctx->input_attrs_iris[0].dims[3]; } else { app_ctx->model3_height = app_ctx->input_attrs_iris[0].dims[1]; app_ctx->model3_width = app_ctx->input_attrs_iris[0].dims[2]; app_ctx->model3_channel = app_ctx->input_attrs_iris[0].dims[3]; } app_ctx->input_fmt_iris = app_ctx->input_attrs_iris[0].fmt; if (app_ctx->model3_width != 64 || app_ctx->model3_height != 64 || app_ctx->model3_channel != 3) { printf("WARN: Iris model input dimensions (%dx%dx%d) do not match expected (64x64x3)!\n", app_ctx->model3_height, app_ctx->model3_width, app_ctx->model3_channel); } printf("Iris model input: H=%d, W=%d, C=%d\n", app_ctx->model3_height, app_ctx->model3_width, app_ctx->model3_channel);
    return 0; // Success
cleanup_iris: if (app_ctx->rknn_ctx_iris != 0) rknn_destroy(app_ctx->rknn_ctx_iris); if (app_ctx->input_attrs_iris) free(app_ctx->input_attrs_iris); if (app_ctx->output_attrs_iris) free(app_ctx->output_attrs_iris);
cleanup_landmark: if (app_ctx->rknn_ctx_landmark != 0) rknn_destroy(app_ctx->rknn_ctx_landmark); if (app_ctx->input_attrs_landmark) free(app_ctx->input_attrs_landmark); if (app_ctx->output_attrs_landmark) free(app_ctx->output_attrs_landmark);
cleanup_detect: if (app_ctx->rknn_ctx_detect != 0) rknn_destroy(app_ctx->rknn_ctx_detect); if (app_ctx->input_attrs_detect) free(app_ctx->input_attrs_detect); if (app_ctx->output_attrs_detect) free(app_ctx->output_attrs_detect); memset(app_ctx, 0, sizeof(face_analyzer_app_context_t)); return -1;
}

int release_face_analyzer(face_analyzer_app_context_t *app_ctx) {
    if (app_ctx->input_attrs_detect != NULL) { free(app_ctx->input_attrs_detect); app_ctx->input_attrs_detect = NULL; } if (app_ctx->output_attrs_detect != NULL) { free(app_ctx->output_attrs_detect); app_ctx->output_attrs_detect = NULL; } if (app_ctx->rknn_ctx_detect != 0) { rknn_destroy(app_ctx->rknn_ctx_detect); app_ctx->rknn_ctx_detect = 0; }
    if (app_ctx->input_attrs_landmark != NULL) { free(app_ctx->input_attrs_landmark); app_ctx->input_attrs_landmark = NULL; } if (app_ctx->output_attrs_landmark != NULL) { free(app_ctx->output_attrs_landmark); app_ctx->output_attrs_landmark = NULL; } if (app_ctx->rknn_ctx_landmark != 0) { rknn_destroy(app_ctx->rknn_ctx_landmark); app_ctx->rknn_ctx_landmark = 0; }
    if (app_ctx->input_attrs_iris != NULL) { free(app_ctx->input_attrs_iris); app_ctx->input_attrs_iris = NULL; } if (app_ctx->output_attrs_iris != NULL) { free(app_ctx->output_attrs_iris); app_ctx->output_attrs_iris = NULL; } if (app_ctx->rknn_ctx_iris != 0) { rknn_destroy(app_ctx->rknn_ctx_iris); app_ctx->rknn_ctx_iris = 0; }
    return 0;
}

// --- Main Inference Function ---
int inference_face_analyzer(face_analyzer_app_context_t *app_ctx, image_buffer_t *src_img, face_analyzer_result_t *out_result) {
    int ret = 0; memset(out_result, 0, sizeof(face_analyzer_result_t));
    // === Stage 1: Face Detection ===
    image_buffer_t detect_img; letterbox_t det_letter_box; rknn_input detect_inputs[1]; rknn_output detect_outputs[app_ctx->io_num_detect.n_output]; memset(&detect_img, 0, sizeof(image_buffer_t)); memset(detect_inputs, 0, sizeof(detect_inputs)); memset(detect_outputs, 0, app_ctx->io_num_detect.n_output * sizeof(rknn_output)); memset(&det_letter_box, 0, sizeof(letterbox_t)); int bg_color = 114;
    // --- CPU Preprocessing for Detection ---
    printf("Performing CPU preprocessing for detection input...\n"); detect_img.width = app_ctx->model1_width; detect_img.height = app_ctx->model1_height; detect_img.format = IMAGE_FORMAT_RGB888; detect_img.size = get_image_size(&detect_img); detect_img.virt_addr = (unsigned char *)malloc(detect_img.size); if (detect_img.virt_addr == NULL) { return -1; } float scale = std::min((float)detect_img.width / src_img->width, (float)detect_img.height / src_img->height); int scaled_w = (int)(src_img->width * scale); int scaled_h = (int)(src_img->height * scale); int dx = (detect_img.width - scaled_w) / 2; int dy = (detect_img.height - scaled_h) / 2; printf("CPU Preprocessing Calculated: dx=%d, dy=%d\n", dx, dy); det_letter_box.scale = scale; det_letter_box.x_pad = (float)dx; det_letter_box.y_pad = (float)dy; unsigned char bg_r = bg_color, bg_g = bg_color, bg_b = bg_color; unsigned char* p_detect_buf = detect_img.virt_addr; size_t detect_img_area = detect_img.width * detect_img.height; for(size_t i = 0; i < detect_img_area; ++i) { *p_detect_buf++ = bg_r; *p_detect_buf++ = bg_g; *p_detect_buf++ = bg_b; } image_buffer_t resized_src; memset(&resized_src, 0, sizeof(image_buffer_t)); resized_src.width = scaled_w; resized_src.height = scaled_h; resized_src.format = IMAGE_FORMAT_RGB888; resized_src.size = get_image_size(&resized_src); resized_src.virt_addr = (unsigned char*)malloc(resized_src.size); if(resized_src.virt_addr == NULL) { free(detect_img.virt_addr); return -1; } image_buffer_t src_rgb; memset(&src_rgb, 0, sizeof(image_buffer_t)); image_buffer_t* src_to_resize = src_img;
#ifdef IMAGE_FORMAT_BGR888
     if (src_img->format == IMAGE_FORMAT_BGR888) { printf("Converting source BGR to RGB for CPU resize...\n"); src_rgb.width = src_img->width; src_rgb.height = src_img->height; src_rgb.format = IMAGE_FORMAT_RGB888; src_rgb.size = get_image_size(&src_rgb); src_rgb.virt_addr = (unsigned char*)malloc(src_rgb.size); if (src_rgb.virt_addr == NULL) { free(resized_src.virt_addr); free(detect_img.virt_addr); return -1; } for(size_t i=0; i < src_img->size; i+=3) { src_rgb.virt_addr[i+0] = src_img->virt_addr[i+2]; src_rgb.virt_addr[i+1] = src_img->virt_addr[i+1]; src_rgb.virt_addr[i+2] = src_img->virt_addr[i+0]; } src_to_resize = &src_rgb; } else
#endif
     if (src_img->format != IMAGE_FORMAT_RGB888) { if (src_rgb.virt_addr) free(src_rgb.virt_addr); free(resized_src.virt_addr); free(detect_img.virt_addr); return -1; } printf("Resizing (Nearest Neighbor CPU)...\n"); for (int y = 0; y < scaled_h; ++y) { int src_y = (int)((float)y / scale); src_y = clamp(src_y, 0, src_to_resize->height - 1); for (int x = 0; x < scaled_w; ++x) { int src_x = (int)((float)x / scale); src_x = clamp(src_x, 0, src_to_resize->width - 1); int src_idx = (src_y * src_to_resize->width + src_x) * 3; int dst_idx = (y * scaled_w + x) * 3; memcpy(&resized_src.virt_addr[dst_idx], &src_to_resize->virt_addr[src_idx], 3); } } if (src_rgb.virt_addr) free(src_rgb.virt_addr); printf("Pasting resized image onto background (CPU)...\n"); for (int y = 0; y < scaled_h; ++y) { if (y + dy >= detect_img.height) continue; unsigned char* dst_row = detect_img.virt_addr + (y + dy) * detect_img.width * 3; unsigned char* src_row = resized_src.virt_addr + y * scaled_w * 3; memcpy(dst_row + dx * 3, src_row, scaled_w * 3); } free(resized_src.virt_addr); printf("CPU preprocessing done.\n");
    // --- END CPU Preprocessing ---
    // ... (Set inputs, run detection, get outputs) ...
    detect_inputs[0].index = 0; detect_inputs[0].type = RKNN_TENSOR_UINT8; if (app_ctx->input_fmt_detect == RKNN_TENSOR_NCHW) { detect_inputs[0].fmt = RKNN_TENSOR_NCHW; detect_inputs[0].size = app_ctx->model1_width * app_ctx->model1_height * app_ctx->model1_channel; } else { detect_inputs[0].fmt = RKNN_TENSOR_NHWC; detect_inputs[0].size = app_ctx->model1_width * app_ctx->model1_height * app_ctx->model1_channel; } detect_inputs[0].buf = detect_img.virt_addr; ret = rknn_inputs_set(app_ctx->rknn_ctx_detect, app_ctx->io_num_detect.n_input, detect_inputs); if (ret < 0) { free(detect_img.virt_addr); return -1; } ret = rknn_run(app_ctx->rknn_ctx_detect, nullptr); if (ret < 0) { free(detect_img.virt_addr); return -1; } for (int i = 0; i < app_ctx->io_num_detect.n_output; i++) { detect_outputs[i].index = i; detect_outputs[i].want_float = 0; } ret = rknn_outputs_get(app_ctx->rknn_ctx_detect, app_ctx->io_num_detect.n_output, detect_outputs, NULL); if (ret < 0) { rknn_outputs_release(app_ctx->rknn_ctx_detect, app_ctx->io_num_detect.n_output, detect_outputs); free(detect_img.virt_addr); return -1; }
    // Post-process Detection
    std::vector<DetectionCandidate> detected_faces; ret = post_process_face_detection(detect_outputs, app_ctx, &det_letter_box, src_img->width, src_img->height, detected_faces); rknn_outputs_release(app_ctx->rknn_ctx_detect, app_ctx->io_num_detect.n_output, detect_outputs); free(detect_img.virt_addr); if (ret < 0) { return -1; } int num_faces_to_process = std::min((int)detected_faces.size(), MAX_FACE_RESULTS); out_result->count = num_faces_to_process; if (num_faces_to_process == 0) { return 0; }

    // === Allocate buffers for loops ===
    image_buffer_t landmark_img; memset(&landmark_img, 0, sizeof(image_buffer_t)); landmark_img.width = app_ctx->model2_width; landmark_img.height = app_ctx->model2_height; landmark_img.format = IMAGE_FORMAT_RGB888; landmark_img.size = get_image_size(&landmark_img); landmark_img.virt_addr = (unsigned char *)malloc(landmark_img.size); if (landmark_img.virt_addr == NULL) { return -1; }
    image_buffer_t iris_input_img; memset(&iris_input_img, 0, sizeof(image_buffer_t)); iris_input_img.width = app_ctx->model3_width; iris_input_img.height = app_ctx->model3_height; iris_input_img.format = IMAGE_FORMAT_RGB888; iris_input_img.size = get_image_size(&iris_input_img); iris_input_img.virt_addr = (unsigned char *)malloc(iris_input_img.size); if (iris_input_img.virt_addr == NULL) { free(landmark_img.virt_addr); return -1; }

    // === Loop through detected faces ===
    for (int i = 0; i < num_faces_to_process; ++i) {
        const box_rect_t& face_box = detected_faces[i].box; out_result->faces[i].box = face_box; out_result->faces[i].score = detected_faces[i].score; out_result->faces[i].face_landmarks_valid = false; out_result->faces[i].eye_landmarks_left_valid = false; out_result->faces[i].iris_landmarks_left_valid = false; out_result->faces[i].eye_landmarks_right_valid = false; out_result->faces[i].iris_landmarks_right_valid = false;
        // --- Stage 2: Face Landmarks ---
        letterbox_t lmk_letter_box; rknn_input landmark_inputs[1]; rknn_output landmark_outputs[app_ctx->io_num_landmark.n_output]; memset(landmark_inputs, 0, sizeof(landmark_inputs)); memset(landmark_outputs, 0, app_ctx->io_num_landmark.n_output * sizeof(rknn_output)); memset(&lmk_letter_box, 0, sizeof(letterbox_t));
        int center_x = (face_box.left + face_box.right) / 2; int center_y = (face_box.top + face_box.bottom) / 2; int box_w = face_box.right - face_box.left; int box_h = face_box.bottom - face_box.top; int crop_w = (int)(box_w * BOX_SCALE_X); int crop_h = (int)(box_h * BOX_SCALE_Y); box_rect_t face_crop_roi; face_crop_roi.left = center_x - crop_w / 2; face_crop_roi.top = center_y - crop_h / 2; face_crop_roi.right = face_crop_roi.left + crop_w; face_crop_roi.bottom = face_crop_roi.top + crop_h;
        image_buffer_t cropped_face_img; memset(&cropped_face_img, 0, sizeof(image_buffer_t)); cropped_face_img.virt_addr = NULL; ret = crop_image(src_img, &cropped_face_img, face_crop_roi); if (ret < 0) { if (cropped_face_img.virt_addr) free(cropped_face_img.virt_addr); continue; }
        ret = convert_image_with_letterbox(&cropped_face_img, &landmark_img, &lmk_letter_box, bg_color); if (cropped_face_img.virt_addr) { free(cropped_face_img.virt_addr); cropped_face_img.virt_addr = NULL; } if (ret < 0) { continue; }
        landmark_inputs[0].index = 0; landmark_inputs[0].type = RKNN_TENSOR_UINT8; if (app_ctx->input_fmt_landmark == RKNN_TENSOR_NCHW) { landmark_inputs[0].fmt = RKNN_TENSOR_NCHW; landmark_inputs[0].size = app_ctx->model2_width * app_ctx->model2_height * app_ctx->model2_channel; } else { landmark_inputs[0].fmt = RKNN_TENSOR_NHWC; landmark_inputs[0].size = app_ctx->model2_width * app_ctx->model2_height * app_ctx->model2_channel; } landmark_inputs[0].buf = landmark_img.virt_addr;
        ret = rknn_inputs_set(app_ctx->rknn_ctx_landmark, app_ctx->io_num_landmark.n_input, landmark_inputs); if (ret < 0) { continue; } ret = rknn_run(app_ctx->rknn_ctx_landmark, nullptr); if (ret < 0) { continue; } for (int j = 0; j < app_ctx->io_num_landmark.n_output; j++) { landmark_outputs[j].index = j; landmark_outputs[j].want_float = 0; } ret = rknn_outputs_get(app_ctx->rknn_ctx_landmark, app_ctx->io_num_landmark.n_output, landmark_outputs, NULL); if (ret < 0) { rknn_outputs_release(app_ctx->rknn_ctx_landmark, app_ctx->io_num_landmark.n_output, landmark_outputs); continue; }
        ret = post_process_face_landmarks(landmark_outputs[0], app_ctx, face_crop_roi, &lmk_letter_box, out_result->faces[i].face_landmarks); rknn_outputs_release(app_ctx->rknn_ctx_landmark, app_ctx->io_num_landmark.n_output, landmark_outputs); if (ret == 0) { out_result->faces[i].face_landmarks_valid = true; printf("Face landmarks processed successfully for face %d.\n", i); } else { printf("WARN: post_process_face_landmarks failed face %d, ret=%d\n", i, ret); continue; } // Skip iris if face lmk failed

        // --- Stage 3: Iris Landmarks ---
        if (out_result->faces[i].face_landmarks_valid) {
            rknn_input iris_inputs[1]; rknn_output iris_outputs[app_ctx->io_num_iris.n_output]; memset(iris_inputs, 0, sizeof(iris_inputs)); memset(iris_outputs, 0, app_ctx->io_num_iris.n_output * sizeof(rknn_output));
            const int LEFT_EYE_ROI_IDX1 = 33; const int LEFT_EYE_ROI_IDX2 = 133; const int RIGHT_EYE_ROI_IDX1 = 362; const int RIGHT_EYE_ROI_IDX2 = 263;
            // Process Left Eye
            point_t left_p1 = out_result->faces[i].face_landmarks[LEFT_EYE_ROI_IDX1]; point_t left_p2 = out_result->faces[i].face_landmarks[LEFT_EYE_ROI_IDX2]; int left_eye_cx = (left_p1.x + left_p2.x) / 2; int left_eye_cy = (left_p1.y + left_p2.y) / 2; int left_eye_w = abs(left_p1.x - left_p2.x); int left_eye_h = abs(left_p1.y - left_p2.y); int left_eye_size = (int)(std::max(left_eye_w, left_eye_h) * EYE_CROP_SCALE); box_rect_t left_eye_crop_roi; left_eye_crop_roi.left = left_eye_cx - left_eye_size / 2; left_eye_crop_roi.top = left_eye_cy - left_eye_size / 2; left_eye_crop_roi.right = left_eye_crop_roi.left + left_eye_size; left_eye_crop_roi.bottom = left_eye_crop_roi.top + left_eye_size; image_buffer_t cropped_left_eye; memset(&cropped_left_eye, 0, sizeof(image_buffer_t)); cropped_left_eye.virt_addr = NULL; ret = crop_image(src_img, &cropped_left_eye, left_eye_crop_roi);
            if (ret == 0) { letterbox_t left_iris_letter_box; memset(&left_iris_letter_box, 0, sizeof(letterbox_t)); ret = convert_image_with_letterbox(&cropped_left_eye, &iris_input_img, &left_iris_letter_box, bg_color); if (ret == 0) { iris_inputs[0].index = 0; iris_inputs[0].type = RKNN_TENSOR_UINT8; if(app_ctx->input_fmt_iris == RKNN_TENSOR_NCHW) {/*NCHW*/} else {iris_inputs[0].fmt = RKNN_TENSOR_NHWC; iris_inputs[0].size = app_ctx->model3_width*app_ctx->model3_height*app_ctx->model3_channel;} iris_inputs[0].buf = iris_input_img.virt_addr; ret = rknn_inputs_set(app_ctx->rknn_ctx_iris, app_ctx->io_num_iris.n_input, iris_inputs); if (ret == 0) ret = rknn_run(app_ctx->rknn_ctx_iris, nullptr); if (ret == 0) { for (int j=0; j<app_ctx->io_num_iris.n_output; ++j) {iris_outputs[j].index=j; iris_outputs[j].want_float=0;} ret = rknn_outputs_get(app_ctx->rknn_ctx_iris, app_ctx->io_num_iris.n_output, iris_outputs, NULL); } if (ret == 0) { ret = post_process_iris_landmarks(iris_outputs, app_ctx, left_eye_crop_roi, &left_iris_letter_box, out_result->faces[i].eye_landmarks_left, out_result->faces[i].iris_landmarks_left); if (ret == 0) { out_result->faces[i].eye_landmarks_left_valid = true; out_result->faces[i].iris_landmarks_left_valid = true; printf("Left eye/iris landmarks processed successfully for face %d.\n", i); } else { /*WARN*/} rknn_outputs_release(app_ctx->rknn_ctx_iris, app_ctx->io_num_iris.n_output, iris_outputs); } else { /*WARN, release*/ rknn_outputs_release(app_ctx->rknn_ctx_iris, app_ctx->io_num_iris.n_output, iris_outputs); } } else { /*WARN*/ } if (cropped_left_eye.virt_addr) free(cropped_left_eye.virt_addr); } else { /*WARN*/ if (cropped_left_eye.virt_addr) free(cropped_left_eye.virt_addr); }
            // Process Right Eye
            point_t right_p1 = out_result->faces[i].face_landmarks[RIGHT_EYE_ROI_IDX1]; point_t right_p2 = out_result->faces[i].face_landmarks[RIGHT_EYE_ROI_IDX2]; int right_eye_cx = (right_p1.x + right_p2.x) / 2; int right_eye_cy = (right_p1.y + right_p2.y) / 2; int right_eye_w = abs(right_p1.x - right_p2.x); int right_eye_h = abs(right_p1.y - right_p2.y); int right_eye_size = (int)(std::max(right_eye_w, right_eye_h) * EYE_CROP_SCALE); box_rect_t right_eye_crop_roi; right_eye_crop_roi.left = right_eye_cx - right_eye_size / 2; right_eye_crop_roi.top = right_eye_cy - right_eye_size / 2; right_eye_crop_roi.right = right_eye_crop_roi.left + right_eye_size; right_eye_crop_roi.bottom = right_eye_crop_roi.top + right_eye_size; image_buffer_t cropped_right_eye; memset(&cropped_right_eye, 0, sizeof(image_buffer_t)); cropped_right_eye.virt_addr = NULL; ret = crop_image(src_img, &cropped_right_eye, right_eye_crop_roi);
             if (ret == 0) { letterbox_t right_iris_letter_box; memset(&right_iris_letter_box, 0, sizeof(letterbox_t)); ret = convert_image_with_letterbox(&cropped_right_eye, &iris_input_img, &right_iris_letter_box, bg_color); if (ret == 0) { iris_inputs[0].index = 0; iris_inputs[0].type = RKNN_TENSOR_UINT8; if(app_ctx->input_fmt_iris == RKNN_TENSOR_NCHW) {/*NCHW*/} else {iris_inputs[0].fmt = RKNN_TENSOR_NHWC; iris_inputs[0].size = app_ctx->model3_width*app_ctx->model3_height*app_ctx->model3_channel;} iris_inputs[0].buf = iris_input_img.virt_addr; ret = rknn_inputs_set(app_ctx->rknn_ctx_iris, app_ctx->io_num_iris.n_input, iris_inputs); if (ret == 0) ret = rknn_run(app_ctx->rknn_ctx_iris, nullptr); if (ret == 0) { for (int j=0; j<app_ctx->io_num_iris.n_output; ++j) {iris_outputs[j].index=j; iris_outputs[j].want_float=0;} ret = rknn_outputs_get(app_ctx->rknn_ctx_iris, app_ctx->io_num_iris.n_output, iris_outputs, NULL); } if (ret == 0) { ret = post_process_iris_landmarks(iris_outputs, app_ctx, right_eye_crop_roi, &right_iris_letter_box, out_result->faces[i].eye_landmarks_right, out_result->faces[i].iris_landmarks_right); if (ret == 0) { out_result->faces[i].eye_landmarks_right_valid = true; out_result->faces[i].iris_landmarks_right_valid = true; printf("Right eye/iris landmarks processed successfully for face %d.\n", i); } else { /*WARN*/ } rknn_outputs_release(app_ctx->rknn_ctx_iris, app_ctx->io_num_iris.n_output, iris_outputs); } else { /*WARN, release*/ rknn_outputs_release(app_ctx->rknn_ctx_iris, app_ctx->io_num_iris.n_output, iris_outputs); } } else { /*WARN*/ } if (cropped_right_eye.virt_addr) free(cropped_right_eye.virt_addr); } else { /*WARN*/ if (cropped_right_eye.virt_addr) free(cropped_right_eye.virt_addr); }
        } // End if face landmarks valid
    } // End loop over detected faces

    // === Free buffers allocated outside the loop ===
    if (landmark_img.virt_addr != NULL) { free(landmark_img.virt_addr); }
    if (iris_input_img.virt_addr != NULL) { free(iris_input_img.virt_addr); }

    return 0; // Success
}