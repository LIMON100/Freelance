#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include headers for both pipelines
#include "face_analyzer/face_analyzer.h" // Use relative path based on structure
#include "yolo_detector/yolo11.h"       // Use relative path

// Common utilities
#include "image_utils.h"
#include "file_utils.h"
#include "image_drawing.h"

// Define additional colors manually if needed
#ifndef COLOR_MAGENTA
#define COLOR_MAGENTA (0xFF00FF) // R=FF, G=00, B=FF
#endif
#ifndef COLOR_CYAN
#define COLOR_CYAN (0x00FFFF)    // R=00, G=FF, B=FF
#endif
#ifndef COLOR_YELLOW
#define COLOR_YELLOW (0xFFFF00)   // R=FF, G=FF, B=00
#endif
#ifndef COLOR_BLUE // Make sure BLUE is defined
#define COLOR_BLUE (0x0000FF)
#endif
#ifndef COLOR_ORANGE // Make sure ORANGE is defined
#define COLOR_ORANGE (0xFF A5 00) // R=FF, G=A5, B=00
#endif


// Define a top-level context structure
typedef struct app_context_t {
    face_analyzer_app_context_t face_ctx; // Context for face/iris models
    yolo11_app_context_t        yolo_ctx; // Context for YOLO model
} app_context_t;


/*-------------------------------------------
                  Main Function
-------------------------------------------*/
int main(int argc, char **argv)
{
    // Update argument check for FOUR model paths + image
    if (argc != 6)
    {
        printf("Usage: %s <face_detect_model> <face_lmk_model> <iris_lmk_model> <yolo_model> <image_path>\n", argv[0]);
        return -1;
    }

    const char *detection_model_path = argv[1];
    const char *landmark_model_path  = argv[2];
    const char *iris_model_path      = argv[3]; // Get iris model path
    const char *yolo_model_path      = argv[4]; // Get YOLO model path
    const char *image_path           = argv[5];

    int ret;
    app_context_t app_ctx; // Use the combined context
    memset(&app_ctx, 0, sizeof(app_context_t));

    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t));

    // Initialize YOLO postprocessing first (loads class names)
    ret = init_post_process();
    if (ret != 0) {
         printf("Error initializing YOLO postprocess.\n");
         return -1;
    }

    // Initialize Face Analyzer models (Detect, Face Lmk, Iris Lmk)
    ret = init_face_analyzer(detection_model_path, landmark_model_path, iris_model_path, &app_ctx.face_ctx); // Pass face_ctx substruct
    if (ret != 0) {
        printf("init_face_analyzer fail! ret=%d\n", ret);
        goto out;
    }

    // Initialize YOLO model
    ret = init_yolo11(yolo_model_path, &app_ctx.yolo_ctx); // Pass yolo_ctx substruct
    if (ret != 0) {
        printf("init_yolo11 fail! ret=%d model_path=%s\n", ret, yolo_model_path);
        goto out; // Make sure face analyzer is released too
    }

    // Read Source Image
    ret = read_image(image_path, &src_image);
    if (ret != 0) {
        printf("read image fail! ret=%d image_path=%s\n", ret, image_path);
        goto out;
    }
    // Note: DMA handling for RV1106 was removed for simplicity in merging.

    // --- Inference ---
    face_analyzer_result_t face_results; // Results for face pipeline
    object_detect_result_list yolo_results; // Results for YOLO pipeline

    // Run Face Analyzer Inference (Detect + Face Lmk + Iris Lmk)
    ret = inference_face_analyzer(&app_ctx.face_ctx, &src_image, &face_results);
    if (ret != 0) {
        printf("inference_face_analyzer fail! ret=%d\n", ret);
    }

    // Run YOLO Inference
    ret = inference_yolo11(&app_ctx.yolo_ctx, &src_image, &yolo_results);
    if (ret != 0) {
        printf("inference_yolo11 fail! ret=%d\n", ret);
    }

    // --- Draw Results ---

    // Draw Face Analysis Results
    printf("Detected %d faces.\n", face_results.count);
    for (int i = 0; i < face_results.count; ++i) {
        face_object_t *face = &face_results.faces[i];

        // Print status flags
        printf("Face %d: box=(%d, %d, %d, %d), score=%.2f, face_lmk=%d, left_eye=%d, left_iris=%d, right_eye=%d, right_iris=%d\n", i,
               face->box.left, face->box.top, face->box.right, face->box.bottom,
               face->score, face->face_landmarks_valid,
               face->eye_landmarks_left_valid, face->iris_landmarks_left_valid,
               face->eye_landmarks_right_valid, face->iris_landmarks_right_valid);


        // Draw bounding box (Green)
        // int rx = face->box.left; int ry = face->box.top; int rw = face->box.right - face->box.left; int rh = face->box.bottom - face->box.top;
        // draw_rectangle(&src_image, rx, ry, rw, rh, COLOR_GREEN, 3);

        // // Draw score (Red)
        // char score_text[20]; snprintf(score_text, 20, "%.2f", face->score);
        // draw_text(&src_image, score_text, rx, ry - 20 > 0 ? ry - 20 : ry, COLOR_RED, 20);

        // Draw 468 Face landmarks if valid (Orange)
        if (face->face_landmarks_valid) {
            for (int j = 0; j < NUM_FACE_LANDMARKS; j++) {
                 draw_circle(&src_image, face->face_landmarks[j].x, face->face_landmarks[j].y, 1, COLOR_ORANGE, 2);
            }
        }

        // *** START: ADDED DRAWING FOR EYE/IRIS ***
        // Draw Left Eye Contour landmarks if valid (Blue)
        if (face->eye_landmarks_left_valid) {
            for (int j = 0; j < NUM_EYE_CONTOUR_LANDMARKS; j++) {
                 draw_circle(&src_image, face->eye_landmarks_left[j].x, face->eye_landmarks_left[j].y, 1, COLOR_BLUE, 2);
            }
        }
        // Draw Left Iris landmarks if valid (Yellow)
        if (face->iris_landmarks_left_valid) {
             for (int j = 0; j < NUM_IRIS_LANDMARKS; j++) {
                 // Draw iris points slightly larger/thicker
                 draw_circle(&src_image, face->iris_landmarks_left[j].x, face->iris_landmarks_left[j].y, 2, COLOR_YELLOW, 3);
            }
        }

        // Draw Right Eye Contour landmarks if valid (Blue)
         if (face->eye_landmarks_right_valid) {
            for (int j = 0; j < NUM_EYE_CONTOUR_LANDMARKS; j++) {
                 draw_circle(&src_image, face->eye_landmarks_right[j].x, face->eye_landmarks_right[j].y, 1, COLOR_BLUE, 2);
            }
        }
        // Draw Right Iris landmarks if valid (Yellow)
         if (face->iris_landmarks_right_valid) {
             for (int j = 0; j < NUM_IRIS_LANDMARKS; j++) {
                 // Draw iris points slightly larger/thicker
                 draw_circle(&src_image, face->iris_landmarks_right[j].x, face->iris_landmarks_right[j].y, 2, COLOR_YELLOW, 3);
            }
        }
        // *** END: ADDED DRAWING FOR EYE/IRIS ***

    } // End face loop

    // Draw YOLO Detection Results
    // ... (YOLO drawing logic remains the same) ...
     printf("Detected %d objects (YOLO).\n", yolo_results.count);
    for (int i = 0; i < yolo_results.count; i++) {
        object_detect_result *det_result = &(yolo_results.results[i]);
        printf("  YOLO: %s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(det_result->cls_id),
               det_result->box.left, det_result->box.top,
               det_result->box.right, det_result->box.bottom,
               det_result->prop);
        int x1 = det_result->box.left; int y1 = det_result->box.top;
        int x2 = det_result->box.right; int y2 = det_result->box.bottom;
        draw_rectangle(&src_image, x1, y1, x2 - x1, y2 - y1, COLOR_MAGENTA, 3);
        char text[256];
        sprintf(text, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
        draw_text(&src_image, text, x1, y1 - 40 > 0 ? y1 - 40 : y1, COLOR_CYAN, 15);
    }

    // Save Final Image
    write_image("out_combined.jpg", &src_image); // Save to a consistent name
    printf("Combined result image saved to out_combined.jpg\n");

out:
    // Release all resources
    release_face_analyzer(&app_ctx.face_ctx);
    release_yolo11(&app_ctx.yolo_ctx);
    deinit_post_process(); // Deinit YOLO postprocessing

    if (src_image.virt_addr != NULL) {
        // Handle potential DMA buffer from YOLO if RV1106/3 was enabled and needs specific free
        #if defined(RV1106_1103)
            // Add appropriate dma_buf_free call here if used
        #else
            free(src_image.virt_addr);
        #endif
    }

    return 0;
}