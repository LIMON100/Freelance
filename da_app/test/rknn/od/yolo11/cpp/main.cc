#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yolo11.h"
#include "image_utils.h"
#include "postprocess.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <rknn_model_path> <image_path>\n", argv[0]);
        return -1;
    }

    char *model_path = argv[1];
    char *image_path = argv[2];
    rknn_app_context_t rknn_app_ctx;
    object_detect_result_list od_results;
    image_buffer_t src_image;

    // Initialize model
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));
    int ret = init_yolo11_model(model_path, &rknn_app_ctx);
    if (ret != 0) {
        printf("init_yolo11_model fail! ret=%d\n", ret);
        return -1;
    }

    // Load image
    memset(&src_image, 0, sizeof(image_buffer_t));
    ret = read_image(image_path, &src_image);
    if (ret != 0) {
        printf("read image fail! ret=%d\n", ret);
        return -1;
    }

    // Initialize post-process
    ret = init_post_process();
    if (ret != 0) {
        printf("init_post_process fail! ret=%d\n", ret);
        goto cleanup;
    }

    // Inference
    memset(&od_results, 0, sizeof(object_detect_result_list));
    ret = inference_yolo11_model(&rknn_app_ctx, &src_image, &od_results);
    if (ret != 0) {
        printf("inference_yolo11_model fail! ret=%d\n", ret);
        goto cleanup;
    }

    // Print detection results
    printf("Detected %d objects\n", od_results.count);
    for (int i = 0; i < od_results.count; i++) {
        object_detect_result *det = &od_results.results[i];
        printf("%s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(det->cls_id),
               det->box.left, det->box.top, det->box.right, det->box.bottom, det->prop);
    }

    // Save the original image (without drawing)
    write_image("out.png", &src_image);

cleanup:
    deinit_post_process();
    release_yolo11_model(&rknn_app_ctx);
    if (src_image.virt_addr != nullptr) {
        free(src_image.virt_addr);
    }
    return ret;
}