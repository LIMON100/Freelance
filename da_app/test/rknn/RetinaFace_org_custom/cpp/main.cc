// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// #include "face_analyzer.h" // Updated header
// #include "image_utils.h"
// #include "image_drawing.h"
// #include "file_utils.h"

// //------------------------------------------- // <--- FIXED
// // Main Function
// //------------------------------------------- // <--- FIXED
// int main(int argc, char **argv) {
//     // Update argument check for two model paths
//     if (argc != 4) {
//         printf("Usage: %s <face_detection_model_path> <face_landmark_model_path> <image_path>\n", argv[0]);
//         return -1;
//     }

//     const char *detection_model_path = argv[1];
//     const char *landmark_model_path = argv[2];
//     const char *image_path = argv[3];
//     int ret;
//     rknn_app_context_t rknn_app_ctx; // Context now holds info for both models
//     memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));

//     // Initialize both models
//     ret = init_face_analyzer_models(detection_model_path, landmark_model_path, &rknn_app_ctx);
//     if (ret != 0) {
//         printf("init_face_analyzer_models fail! ret=%d\n", ret);
//         return -1;
//     }

//     image_buffer_t src_image;
//     memset(&src_image, 0, sizeof(image_buffer_t));
//     ret = read_image(image_path, &src_image);
//     if (ret != 0) {
//         printf("read image fail! ret=%d image_path=%s\n", ret, image_path);
//         release_face_analyzer_models(&rknn_app_ctx); // Release models on error
//         return -1;
//     }

//     // Run inference using both models
//     face_analyzer_result_t result; // Use the new result struct
//     ret = inference_face_analyzer_models(&rknn_app_ctx, &src_image, &result);
//     if (ret != 0) {
//         printf("inference_face_analyzer_models fail! ret=%d\n", ret);
//         goto out;
//     }

//     // Process and draw results
//     printf("Detected %d faces.\n", result.count);
//     for (int i = 0; i < result.count; ++i) {
//         face_object_t *face = &result.faces[i];

//         printf("Face %d: box=(%d, %d, %d, %d), score=%.2f, landmarks_valid=%d\n", i,
//                face->box.left, face->box.top, face->box.right, face->box.bottom,
//                face->score, face->landmarks_valid);

//         // Draw bounding box
//         int rx = face->box.left;
//         int ry = face->box.top;
//         int rw = face->box.right - face->box.left;
//         int rh = face->box.bottom - face->box.top;
//         draw_rectangle(&src_image, rx, ry, rw, rh, COLOR_GREEN, 3);

//         // Draw score
//         char score_text[20];
//         snprintf(score_text, 20, "%.2f", face->score);
//         draw_text(&src_image, score_text, rx, ry - 20 > 0 ? ry - 20 : ry, COLOR_RED, 20); // Adjust text position

//         // Draw landmarks if valid
//         if (face->landmarks_valid) {
//             for (int j = 0; j < NUM_LANDMARKS; j++) {
//                 // Draw small circles for each landmark
//                  draw_circle(&src_image, face->landmarks[j].x, face->landmarks[j].y, 1, COLOR_ORANGE, 2);
//             }
//         } else {
//              printf("  Skipping landmark drawing for face %d (invalid).\n", i);
//         }
//     }
//     write_image("result.jpg", &src_image);
//     printf("Result image saved to result.jpg\n");

// out:
//     // Release models
//     ret = release_face_analyzer_models(&rknn_app_ctx);
//     if (ret != 0) {
//         printf("release_face_analyzer_models fail! ret=%d\n", ret);
//     }

//     // Free image buffer
//     if (src_image.virt_addr != NULL) {
//         free(src_image.virt_addr);
//     }

//     return 0;
// }



#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "face_analyzer.h" // Updated header
#include "image_utils.h"
#include "image_drawing.h"
#include "file_utils.h"

//-------------------------------------------
// Main Function
//-------------------------------------------
int main(int argc, char **argv) {
    // Update argument check for THREE model paths
    if (argc != 5) {
        printf("Usage: %s <face_detect_model> <face_lmk_model> <iris_lmk_model> <image_path>\n", argv[0]);
        return -1;
    }

    const char *detection_model_path = argv[1];
    const char *landmark_model_path = argv[2];
    const char *iris_model_path = argv[3]; // Get iris model path
    const char *image_path = argv[4];
    int ret;
    rknn_app_context_t rknn_app_ctx;
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));

    // Initialize all three models
    ret = init_face_analyzer_models(detection_model_path, landmark_model_path, iris_model_path, &rknn_app_ctx);
    if (ret != 0) {
        printf("init_face_analyzer_models fail! ret=%d\n", ret);
        return -1;
    }

    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t));
    ret = read_image(image_path, &src_image);
    if (ret != 0) {
        printf("read image fail! ret=%d image_path=%s\n", ret, image_path);
        release_face_analyzer_models(&rknn_app_ctx);
        return -1;
    }

    // Run inference using all models
    face_analyzer_result_t result;
    ret = inference_face_analyzer_models(&rknn_app_ctx, &src_image, &result);
    if (ret != 0) {
        printf("inference_face_analyzer_models fail! ret=%d\n", ret);
        goto out;
    }

    // Process and draw results
    printf("Detected %d faces.\n", result.count);
    for (int i = 0; i < result.count; ++i) {
        face_object_t *face = &result.faces[i];

        printf("Face %d: box=(%d, %d, %d, %d), score=%.2f, face_lmk=%d, left_eye=%d, left_iris=%d, right_eye=%d, right_iris=%d\n", i,
               face->box.left, face->box.top, face->box.right, face->box.bottom,
               face->score, face->face_landmarks_valid,
               face->eye_landmarks_left_valid, face->iris_landmarks_left_valid,
               face->eye_landmarks_right_valid, face->iris_landmarks_right_valid);

        // Draw bounding box
        int rx = face->box.left; int ry = face->box.top; int rw = face->box.right - face->box.left; int rh = face->box.bottom - face->box.top;
        draw_rectangle(&src_image, rx, ry, rw, rh, COLOR_GREEN, 3);

        // Draw score
        char score_text[20]; snprintf(score_text, 20, "%.2f", face->score);
        draw_text(&src_image, score_text, rx, ry - 20 > 0 ? ry - 20 : ry, COLOR_RED, 20);

        // Draw 468 Face landmarks if valid
        if (face->face_landmarks_valid) {
            for (int j = 0; j < NUM_FACE_LANDMARKS; j++) {
                 draw_circle(&src_image, face->face_landmarks[j].x, face->face_landmarks[j].y, 1, COLOR_ORANGE, 2); // Orange for face mesh
            }
        }

        // Draw Left Eye Contour landmarks if valid (BLUE)
        if (face->eye_landmarks_left_valid) {
            for (int j = 0; j < NUM_EYE_CONTOUR_LANDMARKS; j++) {
                 draw_circle(&src_image, face->eye_landmarks_left[j].x, face->eye_landmarks_left[j].y, 1, COLOR_BLUE, 2);
            }
        }
        // Draw Left Iris landmarks if valid (YELLOW)
        if (face->iris_landmarks_left_valid) {
             for (int j = 0; j < NUM_IRIS_LANDMARKS; j++) {
                 draw_circle(&src_image, face->iris_landmarks_left[j].x, face->iris_landmarks_left[j].y, 2, COLOR_YELLOW, 3); // Slightly larger
            }
        }

        // Draw Right Eye Contour landmarks if valid (BLUE)
         if (face->eye_landmarks_right_valid) {
            for (int j = 0; j < NUM_EYE_CONTOUR_LANDMARKS; j++) {
                 draw_circle(&src_image, face->eye_landmarks_right[j].x, face->eye_landmarks_right[j].y, 1, COLOR_BLUE, 2);
            }
        }
        // Draw Right Iris landmarks if valid (YELLOW)
         if (face->iris_landmarks_right_valid) {
             for (int j = 0; j < NUM_IRIS_LANDMARKS; j++) {
                 draw_circle(&src_image, face->iris_landmarks_right[j].x, face->iris_landmarks_right[j].y, 2, COLOR_YELLOW, 3); // Slightly larger
            }
        }

    } // End face loop

    write_image("result_iris.jpg", &src_image); // Save to different name
    printf("Result image saved to result_iris.jpg\n");

out:
    release_face_analyzer_models(&rknn_app_ctx);
    if (src_image.virt_addr != NULL) { free(src_image.virt_addr); }
    return 0;
}