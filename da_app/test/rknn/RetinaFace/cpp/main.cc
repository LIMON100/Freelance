// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include "faced_facel.h"
// #include "image_drawing.h"
// #include "image_utils.h"


// int main(int argc, char **argv) {
//     if (argc != 4) {
//         printf("%s <faced_model_path> <facel_model_path> <image_path>\n", argv[0]);
//         return -1;
//     }

//     const char *faced_model_path = argv[1];
//     const char *facel_model_path = argv[2];
//     const char *image_path = argv[3];

//     int ret;
//     rknn_app_context_t app_faced_ctx;
//     rknn_app_context_t app_facel_ctx;
//     object_detect_result_list od_results;

//     memset(&app_faced_ctx, 0, sizeof(rknn_app_context_t));
//     memset(&app_facel_ctx, 0, sizeof(rknn_app_context_t));
//     memset(&od_results, 0, sizeof(object_detect_result_list));

//     // Initialize models
//     ret = init_faced_facel_model(faced_model_path, facel_model_path, &app_faced_ctx, &app_facel_ctx);
//     if (ret != 0) {
//         printf("init_faced_facel_model fail! ret=%d\n", ret);
//         return -1;
//     }

//     // Load image
//     printf("Attempting to load image: %s\n", image_path);
//     image_buffer_t src_image;
//     memset(&src_image, 0, sizeof(image_buffer_t));
//     ret = read_image(image_path, &src_image);
//     if (ret != 0) {
//         printf("read image fail! ret=%d image_path=%s\n", ret, image_path);
//         return -1;
//     }
//     printf("Image loaded successfully: %dx%d\n", src_image.width, src_image.height);

//     // Run face detection
//     ret = inference_faced_model(&app_faced_ctx, &src_image, &od_results);
//     if (ret != 0) {
//         printf("inference_faced_model fail! ret=%d\n", ret);
//         goto cleanup;
//     }

//     // Process each detected face with faceL.rknn
//     for (int i = 0; i < od_results.count; i++) {
//         object_detect_result *det_result = &(od_results.results[i]);

//         // Map coordinates back to original image
//         image_buffer_t temp_image;
//         temp_image.width = app_faced_ctx.model_width;
//         temp_image.height = app_faced_ctx.model_height;
//         mapCoordinates(&src_image, &temp_image, &det_result->box.left, &det_result->box.top);
//         mapCoordinates(&src_image, &temp_image, &det_result->box.right, &det_result->box.bottom);

//         // Crop face using convert_image
//         image_buffer_t face_img;
//         memset(&face_img, 0, sizeof(image_buffer_t));
//         image_rect_t src_box = {
//             det_result->box.left,
//             det_result->box.top,
//             det_result->box.right,
//             det_result->box.bottom
//         };
//         // Set dst_box to match the cropped size (no resizing)
//         int crop_width = det_result->box.right - det_result->box.left;
//         int crop_height = det_result->box.bottom - det_result->box.top;
//         image_rect_t dst_box = {0, 0, crop_width, crop_height};

//         ret = convert_image(&src_image, &face_img, &src_box, &dst_box, 2); // Use 2 for RGB-to-RGB
//         if (ret != 0) {
//             printf("convert image (crop) fail! ret=%d\n", ret);
//             if (face_img.virt_addr != NULL) {
//                 free(face_img.virt_addr);
//             }
//             continue;
//         }

//         // Run landmark extraction
//         ret = inference_facel_model(&app_facel_ctx, &face_img, det_result->points);
//         if (ret != 0) {
//             printf("inference_facel_model fail! ret=%d\n", ret);
//             if (face_img.virt_addr != NULL) {
//                 free(face_img.virt_addr);
//             }
//             continue;
//         }

//         // Map landmarks back to original image coordinates
//         float scaleX = (float)face_img.width / (float)app_facel_ctx.model_width;
//         float scaleY = (float)face_img.height / (float)app_facel_ctx.model_height;
//         for (int j = 0; j < 468; j++) {
//             det_result->points[j].x = (int)(det_result->points[j].x * scaleX) + det_result->box.left;
//             det_result->points[j].y = (int)(det_result->points[j].y * scaleY) + det_result->box.top;
//         }

//         // Free the cropped face image
//         if (face_img.virt_addr != NULL) {
//             free(face_img.virt_addr);
//         }
//     }

//     // Draw results on the image
//     for (int i = 0; i < od_results.count; i++) {
//         object_detect_result *det_result = &(od_results.results[i]);
//         image_rect_t box = det_result->box;
//         draw_rectangle(&src_image, box.left, box.top, box.right - box.left, box.bottom - box.top, 0x00FF00, 2); // Green box
//         for (int j = 0; j < 468; j++) {
//             draw_circle(&src_image, det_result->points[j].x, det_result->points[j].y, 2, 0xFF0000, -1); // Red points
//         }
//     }

//     // Write output image
//     // write_image("output.jpg", &src_image);
//     // if (ret != 0) {
//     //     printf("write image fail! ret=%d\n", ret);
//     //     goto cleanup;
//     // }

// cleanup:
//     // Free the source image
//     if (src_image.virt_addr != NULL) {
//         free(src_image.virt_addr);
//     }

//     // Cleanup
//     release_faced_facel_model(&app_faced_ctx, &app_facel_ctx);
//     return 0;
// }


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "faced_facel.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("%s <faced_model_path> <facel_model_path> <image_path>\n", argv[0]);
        return -1;
    }

    const char *faced_model_path = argv[1];
    const char *facel_model_path = argv[2];
    const char *image_path = argv[3];

    int ret;
    rknn_app_context_t app_faced_ctx;
    rknn_app_context_t app_facel_ctx;
    object_detect_result_list od_results;

    memset(&app_faced_ctx, 0, sizeof(rknn_app_context_t));
    memset(&app_facel_ctx, 0, sizeof(rknn_app_context_t));
    memset(&od_results, 0, sizeof(object_detect_result_list));

    // Initialize models
    ret = init_faced_facel_model(faced_model_path, facel_model_path, &app_faced_ctx, &app_facel_ctx);
    if (ret != 0) {
        printf("init_faced_facel_model fail! ret=%d\n", ret);
        return -1;
    }

    // Load image
    printf("Attempting to load image: %s\n", image_path);
    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t));
    ret = read_image(image_path, &src_image);
    if (ret != 0) {
        printf("read image fail! ret=%d image_path=%s\n", ret, image_path);
        return -1;
    }
    printf("Image loaded successfully: %dx%d\n", src_image.width, src_image.height);

    // Run face detection
    ret = inference_faced_model(&app_faced_ctx, &src_image, &od_results);
    if (ret != 0) {
        printf("inference_faced_model fail! ret=%d\n", ret);
        goto cleanup;
    }

    // Process each detected face with faceL.rknn
    for (int i = 0; i < od_results.count; i++) {
        object_detect_result *det_result = &(od_results.results[i]);

        // Map coordinates back to original image
        image_buffer_t temp_image;
        temp_image.width = app_faced_ctx.model_width;
        temp_image.height = app_faced_ctx.model_height;
        mapCoordinates(&src_image, &temp_image, &det_result->box.left, &det_result->box.top);
        mapCoordinates(&src_image, &temp_image, &det_result->box.right, &det_result->box.bottom);

        // Crop face
        image_buffer_t face_img;
        memset(&face_img, 0, sizeof(image_buffer_t));
        ret = crop_image(&src_image, &face_img, det_result->box.left, det_result->box.top,
                         det_result->box.right - det_result->box.left,
                         det_result->box.bottom - det_result->box.top);
        if (ret != 0) {
            printf("crop image fail! ret=%d\n", ret);
            if (face_img.virt_addr != NULL) {
                free(face_img.virt_addr);
            }
            continue;
        }

        // Run landmark extraction
        ret = inference_facel_model(&app_facel_ctx, &face_img, det_result->points);
        if (ret != 0) {
            printf("inference_facel_model fail! ret=%d\n", ret);
            if (face_img.virt_addr != NULL) {
                free(face_img.virt_addr);
            }
            continue;
        }

        // Map landmarks back to original image coordinates
        float scaleX = (float)face_img.width / (float)app_facel_ctx.model_width;
        float scaleY = (float)face_img.height / (float)app_facel_ctx.model_height;
        for (int j = 0; j < 468; j++) {
            det_result->points[j].x = (int)(det_result->points[j].x * scaleX) + det_result->box.left;
            det_result->points[j].y = (int)(det_result->points[j].y * scaleY) + det_result->box.top;
        }

        // Free the cropped face image
        if (face_img.virt_addr != NULL) {
            free(face_img.virt_addr);
        }
    }

    // Draw results on the image
    for (int i = 0; i < od_results.count; i++) {
        object_detect_result *det_result = &(od_results.results[i]);

        // Draw bounding box
        draw_rectangle(&src_image, det_result->box.left, det_result->box.top,
                       det_result->box.right, det_result->box.bottom, 0x00FF00, 2); // Green color (RGB: 0, 255, 0)

        // Draw landmarks
        for (int j = 0; j < 468; j++) {
            draw_circle(&src_image, det_result->points[j].x, det_result->points[j].y, 1, 0xFF0000, -1); // Red color (RGB: 255, 0, 0)
        }

        // Print detection results
        printf("Face %d: Box=[%d, %d, %d, %d], Prop=%.2f\n", i,
               det_result->box.left, det_result->box.top,
               det_result->box.right, det_result->box.bottom,
               det_result->prop); // Fixed: Use det_result->prop instead of det_result->box.prop
        for (int j = 0; j < 468; j++) {
            printf("  Landmark %d: (%d, %d)\n", j,
                   det_result->points[j].x, det_result->points[j].y);
        }
    }

    // Save the output image
    ret = write_image("output.jpg", &src_image);
    if (ret != 0) {
        printf("write image fail! ret=%d\n", ret);
    } else {
        printf("Output image saved as output.jpg\n");
    }

cleanup:
    // Free the source image
    if (src_image.virt_addr != NULL) {
        free(src_image.virt_addr);
    }

    // Cleanup
    release_faced_facel_model(&app_faced_ctx, &app_facel_ctx);
    return 0;
}