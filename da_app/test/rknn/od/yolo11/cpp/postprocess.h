#ifndef _POSTPROCESS_H
#define _POSTPROCESS_H

#include "yolo11.h"
#include "image_utils.h"
#include <vector>

#define BOX_THRESH 0.25
#define NMS_THRESH 0.45

int init_post_process();
void deinit_post_process();
const char *coco_cls_to_name(int cls_id);
int post_process(rknn_app_context_t *app_ctx, void *outputs, letterbox_t *letter_box, float conf_threshold, float nms_threshold, object_detect_result_list *od_results);
static int process_i8(int8_t *box_tensor, int grid_h, int grid_w, int stride, float threshold, int zp, float scale, std::vector<float> &box, std::vector<float> &box_prob, std::vector<int> &classId);

#endif