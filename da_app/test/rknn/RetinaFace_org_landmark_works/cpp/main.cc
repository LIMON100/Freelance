#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // Include for types if needed by headers

#include "image_utils.h"
#include "image_drawing.h"
#include "retinaface.h" // Your corrected header

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("Usage: %s <face_model_path> <landmark_model_path> <image_path>\n", argv[0]);
        return -1;
    }

    const char *face_model_path = argv[1];
    const char *landmark_model_path = argv[2];
    const char *image_path = argv[3];
    int ret;

    // Initialize RKNN context for BOTH models
    rknn_app_context_t rknn_app_ctx;
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));
    ret = init_retinaface_model(face_model_path, landmark_model_path, &rknn_app_ctx);
    if (ret < 0) {
        printf("init_retinaface_model fail! ret=%d\n", ret);
        return -1;
    }

    // Load image
    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t));
    ret = read_image(image_path, &src_image);
    if (ret < 0) {
        printf("read_image fail! ret=%d image_path=%s\n", ret, image_path);
        release_retinaface_model(&rknn_app_ctx);
        return -1;
    }

    // Run combined inference pipeline
    retinaface_result result;
    memset(&result, 0, sizeof(retinaface_result));
    // V-- Use the correct function name declared in the header --V
    ret = inference_retinaface_pipeline(&rknn_app_ctx, &src_image, &result);
    // ^-- Use the correct function name declared in the header --^
    if (ret < 0) {
        printf("inference_retinaface_pipeline fail! ret=%d\n", ret); // Use correct function name in message
        if (src_image.virt_addr != NULL) free(src_image.virt_addr);
        release_retinaface_model(&rknn_app_ctx);
        return -1;
    }

    printf("Inference successful. Detected %d faces.\n", result.count);

    // Draw bounding boxes and landmarks
    for (int i = 0; i < result.count; i++) {
        // Draw box
        if (result.object[i].box.right > result.object[i].box.left && result.object[i].box.bottom > result.object[i].box.top) {
            draw_rectangle(&src_image, result.object[i].box.left, result.object[i].box.top,
                           result.object[i].box.right - result.object[i].box.left,
                           result.object[i].box.bottom - result.object[i].box.top,
                           COLOR_RED, 2);
        } else {
             printf("Warning: Invalid bounding box for face %d\n", i);
        }

        // Draw landmarks
        for (int j = 0; j < 468; j++) {
            if (result.object[i].landmarks[j].x >= 0 && result.object[i].landmarks[j].x < src_image.width &&
                result.object[i].landmarks[j].y >= 0 && result.object[i].landmarks[j].y < src_image.height)
            {
                draw_circle(&src_image, result.object[i].landmarks[j].x, result.object[i].landmarks[j].y, 1, COLOR_ORANGE, 2);
            }
        }
         printf("Processed landmarks for face %d\n", i);
    }

    // Save output image
    ret = write_image("output.jpg", &src_image);
    if (ret < 0) {
        printf("write_image fail! ret=%d\n", ret);
    } else {
        printf("Output image saved to output.jpg\n");
    }

    // Clean up
    if (src_image.virt_addr != NULL) {
        free(src_image.virt_addr);
        src_image.virt_addr = NULL;
    }
    release_retinaface_model(&rknn_app_ctx);
    printf("Resources released.\n");
    return 0;
}