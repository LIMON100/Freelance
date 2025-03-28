#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> // For fmax, fmin, expf

#include "retinaface.h"
#include "common.h"       // Assuming defines image_buffer_t, letterbox_t, get_format_string, get_type_string, get_qnt_type_string, IMAGE_FORMAT_RGB888, get_image_size
#include "file_utils.h"   // Assuming defines read_data_from_file
#include "image_utils.h"  // Assuming defines convert_image_with_letterbox
#include "rknn_box_priors.h" // Assuming defines BOX_PRIORS_320, BOX_PRIORS_640

#define NMS_THRESHOLD 0.4
#define CONF_THRESHOLD 0.5
#define VIS_THRESHOLD 0.4

//-------------------------------------------------------------------
// STATIC HELPER FUNCTION DEFINITIONS
//-------------------------------------------------------------------

// Dump tensor attributes (for debugging during init)
static void dump_tensor_attr(rknn_tensor_attr *attr) {
    if (attr->fmt == RKNN_TENSOR_NCHW) {
         printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, zp=%d, scale=%f\n",
               attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3],
               attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
               get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
    } else { // Assuming NHWC otherwise
         printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, zp=%d, scale=%f\n",
               attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3], // Assuming order N, H, W, C for print
               attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
               get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
    }
}

// Clamp integer value
static int clamp(int x, int min, int max) {
    if (x > max) return max;
    if (x < min) return min;
    return x;
}

// Calculate Intersection over Union (IoU)
static float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1, float ymax1) {
    float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1) + 1);
    float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1) + 1);
    float i = w * h;
    float u = (xmax0 - xmin0 + 1) * (ymax0 - ymin0 + 1) + (xmax1 - xmin1 + 1) * (ymax1 - ymin1 + 1) - i;
    return u <= 0.f ? 0.f : (i / u);
}

// Non-Maximum Suppression (NMS)
static int nms(int validCount, float *outputLocations, int order[], float threshold, int width, int height) {
    for (int i = 0; i < validCount; ++i) {
        if (order[i] == -1) continue;
        int n = order[i];
        for (int j = i + 1; j < validCount; ++j) {
            int m = order[j];
            if (m == -1) continue;
            // Coordinates are assumed normalized [0,1] before NMS, scale by width/height
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
                order[j] = -1; // Suppress box j
            }
        }
    }
    return 0;
}

// Quick sort for sorting boxes by confidence score (descending)
static int quick_sort_indice_inverse(float *input, int left, int right, int *indices) {
     float key;
     int key_index;
     int low = left;
     int high = right;
     if (left < right) {
         key_index = indices[left];
         key = input[left];
         while (low < high) {
             while (low < high && input[high] <= key) high--; // Find element smaller than key from right
             input[low] = input[high]; // Move smaller element to left
             indices[low] = indices[high];
             while (low < high && input[low] >= key) low++; // Find element larger than key from left
             input[high] = input[low]; // Move larger element to right
             indices[high] = indices[low];
         }
         input[low] = key; // Place key in its sorted position
         indices[low] = key_index;
         quick_sort_indice_inverse(input, left, low - 1, indices); // Sort left sub-array
         quick_sort_indice_inverse(input, low + 1, right, indices); // Sort right sub-array
     }
     return low;
}

// Filter face detection results based on confidence and decode boxes/landmarks
static int filterValidResult(float *scores, float *loc, float *landms, const float boxPriors[][4], int model_in_h, int model_in_w,
                             int filter_indice[], float *props, float threshold, const int num_results) {
    int validCount = 0;
    const float VARIANCES[2] = {0.1, 0.2}; // Variances for box decoding (specific to RetinaFace)
    for (int i = 0; i < num_results; ++i) {
        // Assuming scores tensor has 2 channels: [background_score, face_score]
        float face_score = scores[i * 2 + 1];
        if (face_score > threshold) {
            filter_indice[validCount] = i; // Store index of valid box
            props[validCount] = face_score; // Store confidence score

            // Decode box coordinates relative to priors and variances
            float cx_prior = boxPriors[i][0]; // Prior box center x
            float cy_prior = boxPriors[i][1]; // Prior box center y
            float w_prior  = boxPriors[i][2]; // Prior box width
            float h_prior  = boxPriors[i][3]; // Prior box height

            float cx_delta = loc[i * 4 + 0]; // Predicted offset x
            float cy_delta = loc[i * 4 + 1]; // Predicted offset y
            float w_delta  = loc[i * 4 + 2]; // Predicted scale w
            float h_delta  = loc[i * 4 + 3]; // Predicted scale h

            // Decode center coordinates
            float cx = cx_delta * VARIANCES[0] * w_prior + cx_prior;
            float cy = cy_delta * VARIANCES[0] * h_prior + cy_prior;
            // Decode width and height
            float w = (float)expf(w_delta * VARIANCES[1]) * w_prior;
            float h = (float)expf(h_delta * VARIANCES[1]) * h_prior;

            // Convert center, width, height to xmin, ymin, xmax, ymax
            float xmin = cx - w * 0.5f;
            float ymin = cy - h * 0.5f;
            float xmax = cx + w * 0.5f;
            float ymax = cy + h * 0.5f;

            // Store decoded coordinates (normalized [0,1]) back into loc array for NMS
            loc[i * 4 + 0] = xmin;
            loc[i * 4 + 1] = ymin;
            loc[i * 4 + 2] = xmax;
            loc[i * 4 + 3] = ymax;

            // Decode original 5 landmarks if your face model outputs them (landms is not NULL)
            if (landms != nullptr) {
                for (int j = 0; j < 5; ++j) { // Assuming 5 landmarks in original model if present
                    float lm_x_delta = landms[i * 10 + 2 * j];
                    float lm_y_delta = landms[i * 10 + 2 * j + 1];
                    // Decode landmark coordinates relative to prior center and size
                    landms[i * 10 + 2 * j]     = lm_x_delta * VARIANCES[0] * w_prior + cx_prior; // Store decoded x
                    landms[i * 10 + 2 * j + 1] = lm_y_delta * VARIANCES[0] * h_prior + cy_prior; // Store decoded y
                }
            }
            ++validCount; // Increment count of valid boxes
        }
    }
    return validCount;
}

// Function to crop a region from src_img and resize it into dst_img
// Handles basic padding if the crop region goes out of bounds.
static int crop_and_resize_image(image_buffer_t *src_img, image_buffer_t *dst_img, int x, int y, int width, int height) {
    if (src_img->format != IMAGE_FORMAT_RGB888 || dst_img->format != IMAGE_FORMAT_RGB888) {
        printf("Error: Unsupported image format for crop_and_resize_image\n"); return -1;
    }
     if (width <= 0 || height <= 0) {
        printf("Error: Invalid crop dimensions (%dx%d)\n", width, height); return -1;
    }
    unsigned char *src_data = src_img->virt_addr;
    unsigned char *dst_data = dst_img->virt_addr;

    memset(dst_data, 0, dst_img->size); // Pad destination with black initially

    float scale_x = (float)dst_img->width / width;   // Scaling factor width
    float scale_y = (float)dst_img->height / height; // Scaling factor height

    for (int j = 0; j < dst_img->height; j++) { // Iterate through destination pixels (y)
        for (int i = 0; i < dst_img->width; i++) { // Iterate through destination pixels (x)
            // Find corresponding pixel in the source ROI (before adding offset)
            int src_x_in_roi = (int)(i / scale_x);
            int src_y_in_roi = (int)(j / scale_y);

            // Calculate corresponding pixel in the original source image
            int src_x_orig = src_x_in_roi + x; // x is roi.left
            int src_y_orig = src_y_in_roi + y; // y is roi.top

            // Check if the calculated source pixel is within the bounds of the source image
            if (src_x_orig >= 0 && src_x_orig < src_img->width &&
                src_y_orig >= 0 && src_y_orig < src_img->height)
            {
                // Calculate memory offsets
                int src_offset = (src_y_orig * src_img->width + src_x_orig) * 3; // Offset in source data (RGB)
                int dst_offset = (j * dst_img->width + i) * 3;                   // Offset in destination data (RGB)

                // Copy pixel data (RGB)
                dst_data[dst_offset + 0] = src_data[src_offset + 0]; // R
                dst_data[dst_offset + 1] = src_data[src_offset + 1]; // G
                dst_data[dst_offset + 2] = src_data[src_offset + 2]; // B
            }
            // Else: the pixel remains black (padding) because of the initial memset
        }
    }
    return 0; // Success
}


//-------------------------------------------------------------------
// INTERNAL POST-PROCESSING FUNCTION for Face Detection
//-------------------------------------------------------------------
static int post_process_retinaface(rknn_app_context_t *app_ctx, image_buffer_t *src_img, rknn_output outputs[], retinaface_result *result, letterbox_t *letter_box) {
    // Extract pointers to raw output data
    float *location = (float *)outputs[0].buf; // Box regressions
    float *scores = (float *)outputs[1].buf;   // Confidence scores
    float *landms = nullptr;                   // Optional: 5-point landmarks from face detector
    if (app_ctx->io_num.n_output > 2) { // Check if the third output exists
         landms = (float *)outputs[2].buf;
    }

    // Select appropriate prior boxes based on face detector input size
    const float(*prior_ptr)[4];
    int num_priors = 0;
    if (app_ctx->model_height == 320) { // Example size
        num_priors = 4200; prior_ptr = BOX_PRIORS_320;
    } else if (app_ctx->model_height == 640) { // Example size
        num_priors = 16800; prior_ptr = BOX_PRIORS_640;
    }
    // Add other supported sizes (e.g., 128, 192) with corresponding priors if needed
    else {
        printf("ERROR: Unsupported model input size (%dx%d) for face detection priors.\n", app_ctx->model_height, app_ctx->model_width);
        return -1;
    }

    // Allocate temporary arrays for filtering and sorting
    // Use dynamic allocation if num_priors is very large to avoid stack overflow
    int *filter_indices = (int*)malloc(num_priors * sizeof(int));
    float *props = (float*)malloc(num_priors * sizeof(float));
    if (!filter_indices || !props) {
        printf("Error: Failed to allocate memory for post-processing arrays.\n");
        if(filter_indices) free(filter_indices);
        if(props) free(props);
        return -1;
    }

    memset(filter_indices, 0, sizeof(int) * num_priors);
    memset(props, 0, sizeof(float) * num_priors);

    // Filter boxes based on confidence threshold and decode coordinates
    int validCount = filterValidResult(scores, location, landms, prior_ptr, app_ctx->model_height, app_ctx->model_width,
                                       filter_indices, props, CONF_THRESHOLD, num_priors);

    if (validCount <= 0) {
        printf("No faces detected with confidence > %.2f\n", CONF_THRESHOLD);
        free(filter_indices);
        free(props);
        result->count = 0;
        return 0; // Not an error, just no detections
    }

    // Sort valid boxes by confidence score (descending)
    quick_sort_indice_inverse(props, 0, validCount - 1, filter_indices);

    // Perform Non-Maximum Suppression (NMS)
    // Using model width/height because 'location' contains normalized coords at this point
    nms(validCount, location, filter_indices, NMS_THRESHOLD, app_ctx->model_width, app_ctx->model_height);

    // Process remaining boxes after NMS
    int last_count = 0;
    result->count = 0; // Initialize result count
    for (int i = 0; i < validCount; ++i) {
        if (last_count >= 128) { // Limit max detections
            printf("Warning: Detected more than 128 faces, limiting results.\n");
            break;
        }
        // Check if box was suppressed by NMS or below visibility threshold
        if (filter_indices[i] == -1 || props[i] < VIS_THRESHOLD) {
            continue;
        }

        int n = filter_indices[i]; // Original index of the valid box

        // Get normalized coordinates [0, 1] after decoding
        float xmin_norm = location[n * 4 + 0];
        float ymin_norm = location[n * 4 + 1];
        float xmax_norm = location[n * 4 + 2];
        float ymax_norm = location[n * 4 + 3];

        // Apply inverse letterbox transformation to get coordinates in original image space
        int xmin_let = (int)(xmin_norm * app_ctx->model_width);
        int ymin_let = (int)(ymin_norm * app_ctx->model_height);
        int xmax_let = (int)(xmax_norm * app_ctx->model_width);
        int ymax_let = (int)(ymax_norm * app_ctx->model_height);

        int xmin_orig = (int)((xmin_let - letter_box->x_pad) / letter_box->scale);
        int ymin_orig = (int)((ymin_let - letter_box->y_pad) / letter_box->scale);
        int xmax_orig = (int)((xmax_let - letter_box->x_pad) / letter_box->scale);
        int ymax_orig = (int)((ymax_let - letter_box->y_pad) / letter_box->scale);

        // Store final box coordinates and score, clamping to image boundaries
        result->object[last_count].box.left   = clamp(xmin_orig, 0, src_img->width - 1);
        result->object[last_count].box.top    = clamp(ymin_orig, 0, src_img->height - 1);
        result->object[last_count].box.right  = clamp(xmax_orig, 0, src_img->width - 1);
        result->object[last_count].box.bottom = clamp(ymax_orig, 0, src_img->height - 1);
        result->object[last_count].score      = props[i];
        result->object[last_count].cls        = 0; // Assuming class 0 is face

        // Clear landmark points initially (they will be filled by the landmark model later)
        memset(result->object[last_count].landmarks, 0, sizeof(result->object[last_count].landmarks));

        last_count++;
    }
    result->count = last_count; // Set the final count of detected faces

    // Free temporary arrays
    free(filter_indices);
    free(props);

    return 0; // Success
}

//-------------------------------------------------------------------
// INTERNAL LANDMARK INFERENCE FUNCTION (Called within pipeline)
//-------------------------------------------------------------------
static int inference_landmark_model(rknn_app_context_t *app_ctx, image_buffer_t *src_img, retinaface_result *out_result, int face_index) {
    int ret = 0;
    image_buffer_t face_img_buffer;
    rknn_input inputs[1];
    rknn_output *outputs = NULL;
    unsigned char *face_virt_addr = NULL;

    float *landmark_output = NULL;
    size_t expected_elements_xy = 468 * 2;
    size_t expected_elements_xyz = 468 * 3;
    size_t actual_elements = 0;

    memset(&face_img_buffer, 0, sizeof(image_buffer_t));
    memset(inputs, 0, sizeof(inputs));

    // 1. Get the ROI (using the face box directly from face detection results)
    box_rect_t face_roi = out_result->object[face_index].box;
    int roi_x = face_roi.left;
    int roi_y = face_roi.top;
    int roi_w = face_roi.right - face_roi.left;
    int roi_h = face_roi.bottom - face_roi.top;

    // Basic check for valid ROI dimensions
    if (roi_w <= 0 || roi_h <= 0) {
        printf("Warning: Invalid face ROI size (%dx%d) for face %d. Skipping landmark inference.\n", roi_w, roi_h, face_index);
        memset(out_result->object[face_index].landmarks, 0, sizeof(out_result->object[face_index].landmarks)); // Clear landmarks for this face
        return 0; // Not an error, just skip this face
    }

    // 2. Prepare buffer for the cropped & resized face image (input to landmark model)
    face_img_buffer.width = app_ctx->landmark_model_width;   // Use landmark model's input width
    face_img_buffer.height = app_ctx->landmark_model_height; // Use landmark model's input height
    face_img_buffer.format = IMAGE_FORMAT_RGB888;
    face_img_buffer.size = get_image_size(&face_img_buffer);
    face_virt_addr = (unsigned char *)malloc(face_img_buffer.size); // Allocate memory
    face_img_buffer.virt_addr = face_virt_addr; // Assign pointer to struct

    if (face_virt_addr == NULL) {
        printf("malloc buffer size for landmark input:%d fail!\n", face_img_buffer.size); ret = -1; goto cleanup;
    }

    // 3. Crop the original source image using the face ROI and resize it to the landmark model's input size
    ret = crop_and_resize_image(src_img, &face_img_buffer, roi_x, roi_y, roi_w, roi_h);
    if (ret < 0) { printf("crop_and_resize_image fail! ret=%d\n", ret); goto cleanup; }

    // 4. Set Input Data for the Landmark Model
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8; // Input type is uint8 for quantized models
    inputs[0].fmt = app_ctx->landmark_input_attrs[0].fmt; // Get format (NHWC/NCHW) from queried attributes
    inputs[0].size = face_img_buffer.size; // Size of the preprocessed buffer
    inputs[0].buf = face_img_buffer.virt_addr; // Pointer to the preprocessed data

    // Set input using the landmark context
    ret = rknn_inputs_set(app_ctx->landmark_ctx, 1, inputs);
    if (ret < 0) { printf("rknn_inputs_set (landmark) fail! ret=%d\n", ret); goto cleanup; }

    // 5. Run Landmark Model Inference
    ret = rknn_run(app_ctx->landmark_ctx, nullptr); // Use landmark context
    if (ret < 0) { printf("rknn_run (landmark) fail! ret=%d\n", ret); goto cleanup; }

    // 6. Get Landmark Model Output
    // Allocate memory for the output structures
    outputs = (rknn_output *)malloc(sizeof(rknn_output) * app_ctx->landmark_io_num.n_output);
    if (outputs == NULL) { printf("Failed to allocate memory for landmark outputs\n"); ret = -1; goto cleanup; }
    memset(outputs, 0, sizeof(rknn_output) * app_ctx->landmark_io_num.n_output); // Zero out memory

    // Request outputs as float for easier post-processing
    for (int i = 0; i < app_ctx->landmark_io_num.n_output; i++) {
        outputs[i].index = i;
        outputs[i].want_float = 1;
    }
    // Get outputs using the landmark context
    ret = rknn_outputs_get(app_ctx->landmark_ctx, app_ctx->landmark_io_num.n_output, outputs, NULL);
    if (ret < 0) {
        printf("rknn_outputs_get (landmark) fail! ret=%d\n", ret);
         if (outputs != NULL) { // Attempt to release outputs even if get failed
             rknn_outputs_release(app_ctx->landmark_ctx, app_ctx->landmark_io_num.n_output, outputs);
             free(outputs); outputs = NULL;
         }
        goto cleanup; // Jump to general cleanup
    }

    // 7. Post-Process Landmarks
    if (app_ctx->landmark_io_num.n_output < 1) {
        printf("Error: Landmark model has no outputs defined.\n"); ret = -1; goto release_outputs_and_cleanup;
    }
    // Assuming landmarks are in the first output tensor
    actual_elements = app_ctx->landmark_output_attrs[0].n_elems;

    // Validate output size
    if (actual_elements < expected_elements_xy) {
        printf("Error: Landmark output tensor size (%zu) smaller than expected (%zu).\n", actual_elements, expected_elements_xy); ret = -1; goto release_outputs_and_cleanup;
    }
    if (actual_elements != expected_elements_xy && actual_elements != expected_elements_xyz) {
        printf("Warning: Landmark output size (%zu) not matching expected xy(%zu) or xyz(%zu).\n", actual_elements, expected_elements_xy, expected_elements_xyz);
    }

    landmark_output = (float *)outputs[0].buf; // Get pointer to float output data

    // Iterate through all 468 landmarks
    for (int j = 0; j < 468; j++) {
        // Check bounds before accessing raw output data
        if ((j * 2 + 1) >= actual_elements) {
             printf("Error: Accessing landmark index %d beyond output tensor bounds (%zu).\n", j, actual_elements); break;
        }
        // Get raw x, y coordinates (relative to landmark model input, e.g., 192x192)
        float raw_x = landmark_output[j * 2];
        float raw_y = landmark_output[j * 2 + 1];

        // --- Scale and Offset ---
        // Scale coordinate from landmark input size back to the size of the face ROI
        // Then add the ROI's top-left corner offset to get coordinates in original image space
        int landmark_x = (int)((raw_x / app_ctx->landmark_model_width) * roi_w + roi_x);
        int landmark_y = (int)((raw_y / app_ctx->landmark_model_height) * roi_h + roi_y);

        // Store the final landmark coordinate in the results structure, clamping to image bounds
        out_result->object[face_index].landmarks[j].x = clamp(landmark_x, 0, src_img->width - 1);
        out_result->object[face_index].landmarks[j].y = clamp(landmark_y, 0, src_img->height - 1);
    }

// Label to jump to for releasing outputs before general cleanup
release_outputs_and_cleanup:
    if (outputs != NULL) {
        rknn_outputs_release(app_ctx->landmark_ctx, app_ctx->landmark_io_num.n_output, outputs);
        free(outputs); outputs = NULL; // Free the outputs array itself
    }
// General cleanup label
cleanup:
    if (face_virt_addr != NULL) { free(face_virt_addr); } // Free the cropped image buffer
    return ret; // Return success (0) or error code (<0)
}

//-------------------------------------------------------------------
// PUBLIC INITIALIZATION FUNCTION
//-------------------------------------------------------------------
int init_retinaface_model(const char *face_model_path, const char *landmark_model_path, rknn_app_context_t *app_ctx) {
    int ret;
    int model_len = 0;
    char *model = nullptr;
    rknn_context face_ctx = 0;
    rknn_context landmark_ctx = 0;

    // --- Initialize Face Detection Model ---
    printf("Loading face detection model: %s\n", face_model_path);
    model_len = read_data_from_file(face_model_path, &model);
    if (model == NULL) { printf("load face model fail!\n"); return -1; }
    ret = rknn_init(&face_ctx, model, model_len, 0, NULL);
    free(model); model = nullptr;
    if (ret < 0) { printf("rknn_init face model fail! ret=%d\n", ret); return -1; }

    ret = rknn_query(face_ctx, RKNN_QUERY_IN_OUT_NUM, &app_ctx->io_num, sizeof(app_ctx->io_num));
    if (ret != RKNN_SUCC) { printf("rknn_query face io num fail! ret=%d\n", ret); goto cleanup_face; }
    printf("Face model input num: %d, output num: %d\n", app_ctx->io_num.n_input, app_ctx->io_num.n_output);

    app_ctx->input_attrs = (rknn_tensor_attr *)malloc(app_ctx->io_num.n_input * sizeof(rknn_tensor_attr));
    memset(app_ctx->input_attrs, 0, app_ctx->io_num.n_input * sizeof(rknn_tensor_attr));
    printf("Face Input tensors:\n");
    for (int i = 0; i < app_ctx->io_num.n_input; i++) {
        app_ctx->input_attrs[i].index = i;
        ret = rknn_query(face_ctx, RKNN_QUERY_INPUT_ATTR, &(app_ctx->input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) { printf("rknn_query face input attr %d fail! ret=%d\n", i, ret); goto cleanup_face; }
        dump_tensor_attr(&(app_ctx->input_attrs[i]));
    }

    app_ctx->output_attrs = (rknn_tensor_attr *)malloc(app_ctx->io_num.n_output * sizeof(rknn_tensor_attr));
    memset(app_ctx->output_attrs, 0, app_ctx->io_num.n_output * sizeof(rknn_tensor_attr));
     printf("Face Output tensors:\n");
    for (int i = 0; i < app_ctx->io_num.n_output; i++) {
        app_ctx->output_attrs[i].index = i;
        ret = rknn_query(face_ctx, RKNN_QUERY_OUTPUT_ATTR, &(app_ctx->output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) { printf("rknn_query face output attr %d fail! ret=%d\n", i, ret); goto cleanup_face; }
        dump_tensor_attr(&(app_ctx->output_attrs[i]));
    }

    app_ctx->rknn_ctx = face_ctx;
    if (app_ctx->io_num.n_input > 0) {
         if (app_ctx->input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
            app_ctx->model_channel = app_ctx->input_attrs[0].dims[1]; app_ctx->model_height = app_ctx->input_attrs[0].dims[2]; app_ctx->model_width = app_ctx->input_attrs[0].dims[3];
         } else {
            app_ctx->model_height = app_ctx->input_attrs[0].dims[1]; app_ctx->model_width = app_ctx->input_attrs[0].dims[2]; app_ctx->model_channel = app_ctx->input_attrs[0].dims[3];
         }
         printf("Face model input: H=%d, W=%d, C=%d\n", app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);
    }

    // --- Initialize Landmark Model ---
    printf("Loading landmark model: %s\n", landmark_model_path);
    model_len = read_data_from_file(landmark_model_path, &model);
    if (model == NULL) { printf("load landmark model fail!\n"); goto cleanup_face; }
    ret = rknn_init(&landmark_ctx, model, model_len, 0, NULL);
    free(model); model = nullptr;
    if (ret < 0) { printf("rknn_init landmark model fail! ret=%d\n", ret); goto cleanup_face; }

    ret = rknn_query(landmark_ctx, RKNN_QUERY_IN_OUT_NUM, &app_ctx->landmark_io_num, sizeof(app_ctx->landmark_io_num));
    if (ret != RKNN_SUCC) { printf("rknn_query landmark io num fail! ret=%d\n", ret); goto cleanup_landmark; }
    printf("Landmark model input num: %d, output num: %d\n", app_ctx->landmark_io_num.n_input, app_ctx->landmark_io_num.n_output);

    app_ctx->landmark_input_attrs = (rknn_tensor_attr *)malloc(app_ctx->landmark_io_num.n_input * sizeof(rknn_tensor_attr));
    memset(app_ctx->landmark_input_attrs, 0, app_ctx->landmark_io_num.n_input * sizeof(rknn_tensor_attr));
    printf("Landmark Input tensors:\n");
    for (int i = 0; i < app_ctx->landmark_io_num.n_input; i++) {
        app_ctx->landmark_input_attrs[i].index = i;
        ret = rknn_query(landmark_ctx, RKNN_QUERY_INPUT_ATTR, &(app_ctx->landmark_input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) { printf("rknn_query landmark input attr %d fail! ret=%d\n", i, ret); goto cleanup_landmark; }
        dump_tensor_attr(&(app_ctx->landmark_input_attrs[i]));
    }

    app_ctx->landmark_output_attrs = (rknn_tensor_attr *)malloc(app_ctx->landmark_io_num.n_output * sizeof(rknn_tensor_attr));
    memset(app_ctx->landmark_output_attrs, 0, app_ctx->landmark_io_num.n_output * sizeof(rknn_tensor_attr));
    printf("Landmark Output tensors:\n");
    for (int i = 0; i < app_ctx->landmark_io_num.n_output; i++) {
        app_ctx->landmark_output_attrs[i].index = i;
        ret = rknn_query(landmark_ctx, RKNN_QUERY_OUTPUT_ATTR, &(app_ctx->landmark_output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) { printf("rknn_query landmark output attr %d fail! ret=%d\n", i, ret); goto cleanup_landmark; }
        dump_tensor_attr(&(app_ctx->landmark_output_attrs[i]));
    }

    app_ctx->landmark_ctx = landmark_ctx;
     if (app_ctx->landmark_io_num.n_input > 0) {
        if (app_ctx->landmark_input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
            app_ctx->landmark_model_channel = app_ctx->landmark_input_attrs[0].dims[1]; app_ctx->landmark_model_height = app_ctx->landmark_input_attrs[0].dims[2]; app_ctx->landmark_model_width = app_ctx->landmark_input_attrs[0].dims[3];
        } else {
            app_ctx->landmark_model_height = app_ctx->landmark_input_attrs[0].dims[1]; app_ctx->landmark_model_width = app_ctx->landmark_input_attrs[0].dims[2]; app_ctx->landmark_model_channel = app_ctx->landmark_input_attrs[0].dims[3];
        }
        printf("Landmark model input: H=%d, W=%d, C=%d\n", app_ctx->landmark_model_height, app_ctx->landmark_model_width, app_ctx->landmark_model_channel);
     }

    return 0; // Success

cleanup_landmark:
    if (landmark_ctx != 0) rknn_destroy(landmark_ctx);
    if (app_ctx->landmark_input_attrs) { free(app_ctx->landmark_input_attrs); app_ctx->landmark_input_attrs = nullptr; }
    if (app_ctx->landmark_output_attrs) { free(app_ctx->landmark_output_attrs); app_ctx->landmark_output_attrs = nullptr; }
cleanup_face:
    if (face_ctx != 0) rknn_destroy(face_ctx);
    if (app_ctx->input_attrs) { free(app_ctx->input_attrs); app_ctx->input_attrs = nullptr; }
    if (app_ctx->output_attrs) { free(app_ctx->output_attrs); app_ctx->output_attrs = nullptr; }
    app_ctx->rknn_ctx = 0; app_ctx->landmark_ctx = 0; // Reset stored contexts on failure
    return ret;
}

//-------------------------------------------------------------------
// PUBLIC RELEASE FUNCTION
//-------------------------------------------------------------------
int release_retinaface_model(rknn_app_context_t *app_ctx) {
    // Release face detection resources
    if (app_ctx->input_attrs != NULL) { free(app_ctx->input_attrs); app_ctx->input_attrs = NULL; }
    if (app_ctx->output_attrs != NULL) { free(app_ctx->output_attrs); app_ctx->output_attrs = NULL; }
    if (app_ctx->rknn_ctx != 0) { rknn_destroy(app_ctx->rknn_ctx); app_ctx->rknn_ctx = 0; }

    // Release landmark resources
    if (app_ctx->landmark_input_attrs != NULL) { free(app_ctx->landmark_input_attrs); app_ctx->landmark_input_attrs = NULL; }
    if (app_ctx->landmark_output_attrs != NULL) { free(app_ctx->landmark_output_attrs); app_ctx->landmark_output_attrs = NULL; }
    if (app_ctx->landmark_ctx != 0) { rknn_destroy(app_ctx->landmark_ctx); app_ctx->landmark_ctx = 0; }

    printf("RKNN models released.\n");
    return 0;
}

//-------------------------------------------------------------------
// PUBLIC MAIN INFERENCE PIPELINE FUNCTION
//-------------------------------------------------------------------
int inference_retinaface_pipeline(rknn_app_context_t *app_ctx, image_buffer_t *src_img, retinaface_result *out_result) {
    int ret;
    image_buffer_t face_det_img;
    letterbox_t letter_box;
    rknn_input face_inputs[1];
    rknn_output *face_outputs = NULL;

    // Initialize structs
    memset(&face_det_img, 0, sizeof(image_buffer_t));
    memset(face_inputs, 0, sizeof(face_inputs));
    memset(&letter_box, 0, sizeof(letterbox_t));
    memset(out_result, 0, sizeof(retinaface_result));

    // --- Phase 1: Face Detection ---

    // 1. Pre-process for Face Detection Model (Letterbox)
    int bg_color = 114;
    face_det_img.width = app_ctx->model_width;
    face_det_img.height = app_ctx->model_height;
    face_det_img.format = IMAGE_FORMAT_RGB888;
    face_det_img.size = get_image_size(&face_det_img);
    face_det_img.virt_addr = (unsigned char *)malloc(face_det_img.size);
    if (face_det_img.virt_addr == NULL) { printf("malloc face det buffer fail!\n"); return -1; }

    ret = convert_image_with_letterbox(src_img, &face_det_img, &letter_box, bg_color);
    if (ret < 0) { printf("convert_image_with_letterbox fail! ret=%d\n", ret); goto cleanup_face_det_img; }

    // 2. Set Input for Face Detection
    face_inputs[0].index = 0;
    face_inputs[0].type = RKNN_TENSOR_UINT8;
    face_inputs[0].fmt = app_ctx->input_attrs[0].fmt;
    face_inputs[0].size = face_det_img.size;
    face_inputs[0].buf = face_det_img.virt_addr;

    ret = rknn_inputs_set(app_ctx->rknn_ctx, 1, face_inputs);
    if (ret < 0) { printf("rknn_inputs_set (face) fail! ret=%d\n", ret); goto cleanup_face_det_img; }

    // 3. Run Face Detection
    printf("Running Face Detection...\n");
    ret = rknn_run(app_ctx->rknn_ctx, nullptr);
    if (ret < 0) { printf("rknn_run (face) fail! ret=%d\n", ret); goto cleanup_face_det_img; }

    // 4. Get Face Detection Output
    face_outputs = (rknn_output *)malloc(sizeof(rknn_output) * app_ctx->io_num.n_output);
     if (face_outputs == NULL) { printf("Failed to allocate memory for face outputs\n"); ret = -1; goto cleanup_face_det_img; }
    memset(face_outputs, 0, sizeof(rknn_output) * app_ctx->io_num.n_output);
    for (int i = 0; i < app_ctx->io_num.n_output; i++) {
        face_outputs[i].index = i;
        face_outputs[i].want_float = 1; // Request float for post-processing
    }
    ret = rknn_outputs_get(app_ctx->rknn_ctx, app_ctx->io_num.n_output, face_outputs, NULL);
    if (ret < 0) {
        printf("rknn_outputs_get (face) fail! ret=%d\n", ret);
        if (face_outputs != NULL) {
            // Don't release here, release happens after post-processing or in cleanup
            free(face_outputs); face_outputs = NULL;
        }
        goto cleanup_face_det_img;
    }

    // 5. Post-process Face Detection Results
    ret = post_process_retinaface(app_ctx, src_img, face_outputs, out_result, &letter_box);
    rknn_outputs_release(app_ctx->rknn_ctx, app_ctx->io_num.n_output, face_outputs); // Release outputs after use
    free(face_outputs); face_outputs = NULL; // Free the container array
    if (ret < 0) { printf("post_process_retinaface fail! ret=%d\n", ret); goto cleanup_face_det_img; }

    // Free face detection input buffer now
    free(face_det_img.virt_addr);
    face_det_img.virt_addr = NULL;


    // --- Phase 2: Landmark Detection (Loop) ---
    if (out_result->count > 0) {
        printf("Detected %d faces. Running landmark detection...\n", out_result->count);
        for (int i = 0; i < out_result->count; ++i) {
            //printf("Processing landmarks for face %d...\n", i); // Optional: verbose logging
            int landmark_ret = inference_landmark_model(app_ctx, src_img, out_result, i);
            if (landmark_ret < 0) {
                printf("inference_landmark_model failed for face %d, ret=%d\n", i, landmark_ret);
                // Clear landmarks for this face if inference failed
                 memset(out_result->object[i].landmarks, 0, sizeof(out_result->object[i].landmarks));
            }
        }
        printf("Landmark detection finished.\n");
    } else {
        printf("No faces detected to process landmarks.\n");
    }


    return 0; // Overall success

// Cleanup label for phase 1 resources
cleanup_face_det_img:
    if (face_det_img.virt_addr != NULL) { free(face_det_img.virt_addr); }
    if (face_outputs != NULL) {
        // Note: Don't release rknn_outputs here if rknn_outputs_get failed,
        // as the state might be undefined. Just free the container.
        free(face_outputs);
    }
    return ret; // Return error code
}