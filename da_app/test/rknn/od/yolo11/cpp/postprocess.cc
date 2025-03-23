#include "postprocess.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <functional>

#define OBJ_CLASS_NUM 4

static char *labels[OBJ_CLASS_NUM] = {
    strdup("cigarette"),
    strdup("drinking"),
    strdup("eating"),
    strdup("mobile")
};

int init_post_process() {
    for (int i = 0; i < OBJ_CLASS_NUM; i++) {
        if (labels[i] == nullptr) {
            printf("Failed to allocate memory for label %d\n", i);
            return -1;
        }
    }
    return 0;
}

void deinit_post_process() {
    for (int i = 0; i < OBJ_CLASS_NUM; i++) {
        if (labels[i] != nullptr) {
            free(labels[i]);
            labels[i] = nullptr;
        }
    }
}

const char* coco_cls_to_name(int cls_id) {
    static const char* class_names[OBJ_CLASS_NUM] = {
        "cigarette",
        "drinking",
        "eating",
        "mobile"
    };
    if (cls_id >= 0 && cls_id < OBJ_CLASS_NUM) {
        return class_names[cls_id];
    }
    return "unknown";
}

void nms(int num_boxes, std::vector<float> &box_prob, std::vector<float> &box, std::vector<int> &indices, float nms_threshold) {
    std::vector<std::pair<float, int>> score_index(num_boxes);
    for (int i = 0; i < num_boxes; i++) {
        score_index[i] = std::make_pair(box_prob[i], i);
    }
    std::sort(score_index.begin(), score_index.end(), std::greater<std::pair<float, int>>());

    std::vector<bool> in_box(num_boxes, true);
    for (int i = 0; i < num_boxes; i++) {
        int idx = score_index[i].second;
        if (!in_box[idx]) continue;

        indices.push_back(idx);
        for (int j = i + 1; j < num_boxes; j++) {
            int idx2 = score_index[j].second;
            if (!in_box[idx2]) continue;

            float x1 = box[idx * 4 + 0];
            float y1 = box[idx * 4 + 1];
            float x2 = box[idx * 4 + 2];
            float y2 = box[idx * 4 + 3];
            float x1_2 = box[idx2 * 4 + 0];
            float y1_2 = box[idx2 * 4 + 1];
            float x2_2 = box[idx2 * 4 + 2];
            float y2_2 = box[idx2 * 4 + 3];

            float xx1 = std::max(x1, x1_2);
            float yy1 = std::max(y1, y1_2);
            float xx2 = std::min(x2, x2_2);
            float yy2 = std::min(y2, y2_2);

            float w = std::max(0.0f, xx2 - xx1);
            float h = std::max(0.0f, yy2 - yy1);
            float inter = w * h;
            float area1 = (x2 - x1) * (y2 - y1);
            float area2 = (x2_2 - x1_2) * (y2_2 - y1_2);
            float iou = inter / (area1 + area2 - inter);

            if (iou > nms_threshold) {
                in_box[idx2] = false;
            }
        }
    }
}

static int process_i8(int8_t *box_tensor, int grid_h, int grid_w, int stride, float threshold, int zp, float scale, std::vector<float> &box, std::vector<float> &box_prob, std::vector<int> &classId) {
    int grid_len = 4 + 1 + OBJ_CLASS_NUM; // 9 (4 box + 1 conf + 4 classes)
    int validCount = 0;
    float threshold_i8 = threshold * scale + zp;

    for (int i = 0; i < grid_h * grid_w; i++) {
        int offset = i * grid_len;
        int conf = box_tensor[offset + 4]; // Objectness confidence
        if (conf < threshold_i8) {
            continue;
        }

        // Class scores
        int max_score = -128;
        int max_class_id = -1;
        for (int c = 0; c < OBJ_CLASS_NUM; c++) {
            int score = box_tensor[offset + 5 + c];
            if (score > max_score) {
                max_score = score;
                max_class_id = c;
            }
        }
        if (max_score < threshold_i8) {
            continue;
        }

        // Dequantize confidence and class score
        float conf_deq = (conf - zp) * scale;
        float score_deq = (max_score - zp) * scale;
        float prob = conf_deq * score_deq;
        if (prob < threshold) {
            continue;
        }

        // Box decoding (assuming direct coordinates)
        float x = (box_tensor[offset + 0] - zp) * scale;
        float y = (box_tensor[offset + 1] - zp) * scale;
        float w = (box_tensor[offset + 2] - zp) * scale;
        float h = (box_tensor[offset + 3] - zp) * scale;

        // Convert center-x, center-y, width, height to top-left and bottom-right
        float left = x - w / 2;
        float top = y - h / 2;
        float right = x + w / 2;
        float bottom = y + h / 2;

        box.push_back(left);
        box.push_back(top);
        box.push_back(right);
        box.push_back(bottom);
        box_prob.push_back(prob);
        classId.push_back(max_class_id);
        validCount++;
    }

    return validCount;
}

int post_process(rknn_app_context_t *app_ctx, void *outputs, letterbox_t *letter_box, float conf_threshold, float nms_threshold, object_detect_result_list *od_results) {
    std::vector<float> box;
    std::vector<float> box_prob;
    std::vector<int> classId;

    // Single output tensor [1, 9, 8400]
    int8_t *output = (int8_t *)outputs;
    int grid_h = 1; // Treat as a flat tensor
    int grid_w = 8400; // 8400 anchors
    int stride = 1; // Stride doesn't apply in this format

    // Process the tensor
    int validCount = process_i8(output, grid_h, grid_w, stride, conf_threshold, app_ctx->output_attrs[0].zp, app_ctx->output_attrs[0].scale, box, box_prob, classId);

    // NMS
    std::vector<int> indices;
    nms(validCount, box_prob, box, indices, nms_threshold);

    // Scale boxes and store results
    od_results->count = 0;
    for (int i = 0; i < indices.size(); i++) {
        int idx = indices[i];
        object_detect_result *det_result = &(od_results->results[od_results->count]);
        det_result->cls_id = classId[idx];
        det_result->prop = box_prob[idx];
        det_result->box.left = box[idx * 4 + 0] * letter_box->scale + letter_box->x_pad;
        det_result->box.top = box[idx * 4 + 1] * letter_box->scale + letter_box->y_pad;
        det_result->box.right = box[idx * 4 + 2] * letter_box->scale + letter_box->x_pad;
        det_result->box.bottom = box[idx * 4 + 3] * letter_box->scale + letter_box->y_pad;
        od_results->count++;
    }

    return 0;
}