#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <algorithm>  // Added for std::sort
#include <numeric>   // Added for std::iota

#include "retinaface.h"
#include "common.h"
#include "file_utils.h"
#include "image_utils.h"
#include "rknn_box_priors.h" // Include header for BOX_PRIORS_320

// Move clamp function to the top
static int clamp(int x, int min, int max) {
    if (x > max) return max;
    if (x < min) return min;
    return x;
}

static int image_crop_resize(image_buffer_t *src, image_buffer_t *dst, int x, int y, int width, int height) {
    // Check for valid input
    if (src == nullptr || dst == nullptr || src->virt_addr == nullptr || dst->virt_addr == nullptr) {
        printf("Error: Invalid input to image_crop_resize\n");
        return -1;
    }

    // Clamp the crop region to be within the source image
    int crop_x = clamp(x, 0, src->width - 1);
    int crop_y = clamp(y, 0, src->height - 1);
    int crop_width = clamp(width, 0, src->width - crop_x);
    int crop_height = clamp(height, 0, src->height - crop_y);

    // Calculate scaling factors
    float scale_x = (float)dst->width / crop_width;
    float scale_y = (float)dst->height / crop_height;

    for (int j = 0; j < dst->height; ++j) {
        for (int i = 0; i < dst->width; ++i) {
            // Calculate corresponding coordinates in the source image
            float src_x = i / scale_x + crop_x;
            float src_y = j / scale_y + crop_y;

            // Bilinear interpolation
            int x1 = (int)src_x;
            int y1 = (int)src_y;
            int x2 = x1 + 1;
            int y2 = y1 + 1;

            // Ensure the coordinates are within the source image bounds
            x1 = clamp(x1, 0, src->width - 1);
            y1 = clamp(y1, 0, src->height - 1);
            x2 = clamp(x2, 0, src->width - 1);
            y2 = clamp(y2, 0, src->height - 1);

            unsigned char *dst_pixel = dst->virt_addr + (j * dst->width + i) * 3;

            if (src->format == IMAGE_FORMAT_RGB888) {
                unsigned char *src_pixel11 = src->virt_addr + (y1 * src->width + x1) * 3;
                unsigned char *src_pixel12 = src->virt_addr + (y1 * src->width + x2) * 3;
                unsigned char *src_pixel21 = src->virt_addr + (y2 * src->width + x1) * 3;
                unsigned char *src_pixel22 = src->virt_addr + (y2 * src->width + x2) * 3;

                dst_pixel[0] = (unsigned char)(
                    (1 - x_ratio) * (1 - y_ratio) * src_pixel11[0] +
                    x_ratio * (1 - y_ratio) * src_pixel12[0] +
                    (1 - x_ratio) * y_ratio * src_pixel21[0] +
                    x_ratio * y_ratio * src_pixel22[0]);
                dst_pixel[1] = (unsigned char)(
                    (1 - x_ratio) * (1 - y_ratio) * src_pixel11[1] +
                    x_ratio * (1 - y_ratio) * src_pixel12[1] +
                    (1 - x_ratio) * y_ratio * src_pixel21[1] +
                    x_ratio * y_ratio * src_pixel22[1]);
                dst_pixel[2] = (unsigned char)(
                    (1 - x_ratio) * (1 - y_ratio) * src_pixel11[2] +
                    x_ratio * (1 - y_ratio) * src_pixel12[2] +
                    (1 - x_ratio) * y_ratio * src_pixel21[2] +
                    x_ratio * y_ratio * src_pixel22[2]);
            } else {
                printf("Error: Unsupported source image format for cropping and resizing\n");
                return -1;
            }
        }
    }

    return 0;
}

#define NMS_THRESHOLD 0.4
#define CONF_THRESHOLD 0.5
#define VIS_THRESHOLD 0.4

//static int clamp(int x, int min, int max) {  // Moved to the top
//    if (x > max) return max;
//    if (x < min) return min;
//    return x;
//}

static void dump_tensor_attr(rknn_tensor_attr *attr) {
    printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
           "zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3],
           attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
           get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

static float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1, float ymax1) {
    float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1) + 1);
    float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1) + 1);
    float i = w * h;
    float u = (xmax0 - xmin0 + 1) * (ymax0 - ymin0 + 1) + (xmax1 - xmin1 + 1) * (ymax1 - ymin1 + 1) - i;
    return u <= 0.f ? 0.f : (i / u);
}

static int nms(std::vector<retinaface_object_t>& objects, float threshold, float width, float height) {
    std::vector<size_t> sorted_indices(objects.size());
    std::iota(sorted_indices.begin(), sorted_indices.end(), 0);

    // Sort by confidence (highest to lowest)
    std::sort(sorted_indices.begin(), sorted_indices.end(),
              [&](size_t i1, size_t i2) { return objects[i1].score > objects[i2].score; });

    std::vector<bool> keep(objects.size(), true);
    for (size_t i = 0; i < objects.size(); ++i) {
        if (!keep[sorted_indices[i]]) continue;

        for (size_t j = i + 1; j < objects.size(); ++j) {
            if (!keep[sorted_indices[j]]) continue;

            float xmin0 = objects[sorted_indices[i]].box.left;
            float ymin0 = objects[sorted_indices[i]].box.top;
            float xmax0 = objects[sorted_indices[i]].box.right;
            float ymax0 = objects[sorted_indices[i]].box.bottom;

            float xmin1 = objects[sorted_indices[j]].box.left;
            float ymin1 = objects[sorted_indices[j]].box.top;
            float xmax1 = objects[sorted_indices[j]].box.right;
            float ymax1 = objects[sorted_indices[j]].box.bottom;

            float iou = CalculateOverlap(xmin0, ymin0, xmax0, ymax0, xmin1, ymin1, xmax1, ymax1);

            if (iou > threshold) {
                keep[sorted_indices[j]] = false;
            }
        }
    }

    // Filter out the boxes that are not kept
    std::vector<retinaface_object_t> filtered_objects;
    for (size_t i = 0; i < objects.size(); ++i) {
        if (keep[i]) {
            filtered_objects.push_back(objects[i]);
        }
    }

    objects = filtered_objects;
    return 0;
}

// Initialize the face detection model
int init_face_detection_model(const char *model_path, rknn_model_context_t *model_ctx) {
    int ret;
    int model_len = 0;
    char *model;
    rknn_context ctx = 0;

    // Load RKNN Model
    model_len = read_data_from_file(model_path, &model);
    if (model == NULL) {
        printf("load_model fail!\n");
        return -1;
    }

    ret = rknn_init(&ctx, model, model_len, 0, NULL);
    free(model);
    if (ret < 0) {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }

    // Get Model Input Output Number
    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC) {
        printf("rknn_query fail! ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

    // Get Model Input Info
    printf("input tensors:\n");
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++) {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs[i]));
    }

    // Get Model Output Info
    printf("output tensors:\n");
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++) {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_attrs[i]));
    }

    // Set to context
    model_ctx->rknn_ctx = ctx;
    model_ctx->io_num = io_num;
    model_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(model_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    model_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(model_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
        printf("model is NCHW input fmt\n");
        model_ctx->model_channel = input_attrs[0].dims[1];
        model_ctx->model_height  = input_attrs[0].dims[2];
        model_ctx->model_width   = input_attrs[0].dims[3];
    } else {
        printf("model is NHWC input fmt\n");
        model_ctx->model_height  = input_attrs[0].dims[1];
        model_ctx->model_width   = input_attrs[0].dims[2];
        model_ctx->model_channel = input_attrs[0].dims[3];
    }
    printf("model input height=%d, width=%d, channel=%d\n",
           model_ctx->model_height, model_ctx->model_width, model_ctx->model_channel);

    return 0;
}

// Initialize the landmark model
int init_landmark_model(const char *model_path, rknn_model_context_t *model_ctx) {
     int ret;
    int model_len = 0;
    char *model;
    rknn_context ctx = 0;

    // Load RKNN Model
    model_len = read_data_from_file(model_path, &model);
    if (model == NULL) {
        printf("load_model fail!\n");
        return -1;
    }

    ret = rknn_init(&ctx, model, model_len, 0, NULL);
    free(model);
    if (ret < 0) {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }

    // Get Model Input Output Number
    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC) {
        printf("rknn_query fail! ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

    // Get Model Input Info
    printf("input tensors:\n");
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++) {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs[i]));
    }

    // Get Model Output Info
    printf("output tensors:\n");
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++) {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_attrs[i]));
    }

    // Set to context
    model_ctx->rknn_ctx = ctx;
    model_ctx->io_num = io_num;
    model_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(model_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    model_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(model_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
        printf("model is NCHW input fmt\n");
        model_ctx->model_channel = input_attrs[0].dims[1];
        model_ctx->model_height  = input_attrs[0].dims[2];
        model_ctx->model_width   = input_attrs[0].dims[3];
    } else {
        printf("model is NHWC input fmt\n");
        model_ctx->model_height  = input_attrs[0].dims[1];
        model_ctx->model_width   = input_attrs[0].dims[2];
        model_ctx->model_channel = input_attrs[0].dims[3];
    }
    printf("model input height=%d, width=%d, channel=%d\n",
           model_ctx->model_height, model_ctx->model_width, model_ctx->model_channel);

    return 0;
}

int release_face_detection_model(rknn_model_context_t *model_ctx) {
    if (model_ctx->input_attrs != NULL) {
        free(model_ctx->input_attrs);
        model_ctx->input_attrs = NULL;
    }
    if (model_ctx->output_attrs != NULL) {
        free(model_ctx->output_attrs);
        model_ctx->output_attrs = NULL;
    }
    if (model_ctx->rknn_ctx != 0) {
        rknn_destroy(model_ctx->rknn_ctx);
        model_ctx->rknn_ctx = 0;
    }
    return 0;
}

int release_landmark_model(rknn_model_context_t *model_ctx) {
    if (model_ctx->input_attrs != NULL) {
        free(model_ctx->input_attrs);
        model_ctx->input_attrs = NULL;
    }
    if (model_ctx->output_attrs != NULL) {
        free(model_ctx->output_attrs);
        model_ctx->output_attrs = NULL;
    }
    if (model_ctx->rknn_ctx != 0) {
        rknn_destroy(model_ctx->rknn_ctx);
        model_ctx->rknn_ctx = 0;
    }
    return 0;
}

static int get_channel_count(image_buffer_t *img) {
    switch (img->format) {
        case IMAGE_FORMAT_RGB888:
            return 3;
        case IMAGE_FORMAT_GRAY8:
            return 1;
        default:
            return 3; // Default to 3 if unknown format
    }
}

// Function to run the face detection model
std::vector<retinaface_object_t> run_face_detection(rknn_model_context_t *face_ctx, image_buffer_t *src_img, float orig_width, float orig_height) {
    std::vector<retinaface_object_t> detected_faces;
    int ret;

    // Prepare input for face detection model
    rknn_input inputs[1];
    memset(inputs, 0, sizeof(inputs));
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].size = src_img->width * src_img->height * get_channel_count(src_img);
    inputs[0].buf = src_img->virt_addr;

    ret = rknn_inputs_set(face_ctx->rknn_ctx, 1, inputs);
    if (ret < 0) {
        printf("rknn_input_set fail! ret=%d\n", ret);
        return detected_faces;
    }

    // Run face detection inference
    ret = rknn_run(face_ctx->rknn_ctx, nullptr);
    if (ret < 0) {
        printf("rknn_run fail! ret=%d\n", ret);
        return detected_faces;
    }

     // Get face detection output
    rknn_output outputs[2]; // Now expecting two outputs
    memset(outputs, 0, sizeof(outputs));
    outputs[0].index = 0;
    outputs[0].want_float = 1;
    outputs[1].index = 1;
    outputs[1].want_float = 1;

    ret = rknn_outputs_get(face_ctx->rknn_ctx, 2, outputs, NULL);
    if (ret < 0) {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        return detected_faces;
    }
    // Process face detections
    float *regressors = (float *)outputs[0].buf;  // bounding box regressions
    float *classificators = (float *)outputs[1].buf;  // face/non-face scores
    int num_boxes = face_ctx->output_attrs[0].dims[1];  // Number of bounding boxes
    float scale_x = orig_width / face_ctx->model_width;
    float scale_y = orig_height / face_ctx->model_height;

    // Assuming the output format is [xmin, ymin, xmax, ymax, confidence] * num_boxes
    for (int i = 0; i < num_boxes; i++) {

        float confidence = classificators[i];  // confidence score for each box
       if (confidence > CONF_THRESHOLD) { // Apply confidence threshold
            retinaface_object_t face;

            // Decode the bounding box
            face.box.left = (int)(regressors[i * 16 + 0] * scale_x);
            face.box.top = (int)(regressors[i * 16 + 1] * scale_y);
            face.box.right = (int)(regressors[i * 16 + 2] * scale_x);
            face.box.bottom = (int)(regressors[i * 16 + 3] * scale_y);
            face.score = confidence;

             // Clamp the bounding box coordinates
            face.box.left = clamp(face.box.left, 0, (int)orig_width - 1);
            face.box.top = clamp(face.box.top, 0, (int)orig_height - 1);
            face.box.right = clamp(face.box.right, face.box.left + 1, (int)orig_width); // Ensure right > left
            face.box.bottom = clamp(face.box.bottom, face.box.top + 1, (int)orig_height);   // Ensure bottom > top
            detected_faces.push_back(face);
        }
    }

     rknn_outputs_release(face_ctx->rknn_ctx, 2, outputs); // Release both output buffers

    return detected_faces;
}
// Function to run the landmark detection model on a cropped face
std::vector<ponit_t> run_landmark_detection(rknn_model_context_t *landmark_ctx, image_buffer_t *face_img) {
    std::vector<ponit_t> landmarks;
    int ret;

    // Prepare input for landmark model
    rknn_input inputs[1];
    memset(inputs, 0, sizeof(inputs));
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;  // Assuming UINT8. Adjust if needed.
    inputs[0].fmt = RKNN_TENSOR_NHWC; // Assuming NHWC.  Adjust if needed.
    inputs[0].size = face_img->width * face_img->height * get_channel_count(face_img);
    inputs[0].buf = face_img->virt_addr;

    ret = rknn_inputs_set(landmark_ctx->rknn_ctx, 1, inputs);
    if (ret < 0) {
        printf("rknn_input_set fail! ret=%d\n", ret);
        return landmarks; // Return empty vector if fails
    }

    // Run landmark inference
    ret = rknn_run(landmark_ctx->rknn_ctx, nullptr);
    if (ret < 0) {
        printf("rknn_run fail! ret=%d\n", ret);
        return landmarks; // Return empty vector if fails
    }

    // Get landmark output
    rknn_output outputs[1];  // Assuming 1 output for landmarks
    memset(outputs, 0, sizeof(outputs));
    outputs[0].index = 0;
    outputs[0].want_float = 1; // Assuming you want the output in float.  Adjust if needed.
    ret = rknn_outputs_get(landmark_ctx->rknn_ctx, 1, outputs, NULL);
    if (ret < 0) {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        return landmarks; // Return empty vector if fails
    }

   float *output_data = (float *)outputs[0].buf;

    // Get the number of landmarks from the output tensor's dimensions
    int num_landmarks = landmark_ctx->output_attrs[0].dims[3] / 2;
     float scale_x = 192 / face_img->width;
    float scale_y = 192 / face_img->height;

    // Process landmarks
    for (int i = 0; i < num_landmarks; ++i) {
        ponit_t landmark;
         // Scaling landmark points to face image dimensions (assuming that output values between 0 and 1, need scaling based on output tensor)
        landmark.x = (int)(output_data[i * 2 + 0] * scale_x);  // Assuming x is at index 0. Adjust if needed.
        landmark.y = (int)(output_data[i * 2 + 1] * scale_y); // Assuming y is at index 1. Adjust if needed.
        landmarks.push_back(landmark);
    }
    rknn_outputs_release(landmark_ctx->rknn_ctx, 1, outputs); // Release the output buffer
    return landmarks;
}

int inference_retinaface_model(rknn_app_context_t *app_ctx, image_buffer_t *src_img, retinaface_result *out_result) {
    int ret;

     // Create resized image buffer for face detection model input
    image_buffer_t face_detect_input;
    memset(&face_detect_input, 0, sizeof(image_buffer_t));
    face_detect_input.width = 320;
    face_detect_input.height = 320;
    face_detect_input.format = src_img->format;  // keep format as the original image
    face_detect_input.size = get_image_size(&face_detect_input);
    face_detect_input.virt_addr = (unsigned char *)malloc(face_detect_input.size);
    if (face_detect_input.virt_addr == NULL) {
        printf("malloc buffer size:%d fail!\n", face_detect_input.size);
        return -1;
    }

    ret = image_crop_resize(src_img, &face_detect_input, 0, 0, src_img->width, src_img->height);

     if (ret < 0) {
        printf("image_crop_resize failed!\n");
        free(face_detect_input.virt_addr);
        return -1;
    }

    // 1. Run Face Detection

    std::vector<retinaface_object_t> detected_faces = run_face_detection(&app_ctx->face_detect_ctx, &face_detect_input,src_img->width, src_img->height);
    nms(detected_faces, NMS_THRESHOLD,src_img->width, src_img->height);
    free(face_detect_input.virt_addr);

    out_result->object = detected_faces;
    out_result->count = detected_faces.size();
    // 2. For each detected face, run Landmark Extraction
    for (size_t i = 0; i < detected_faces.size(); ++i) {
        // Crop the face from the original image
        image_buffer_t face_img;
        memset(&face_img, 0, sizeof(image_buffer_t));
        face_img.width = app_ctx->landmark_ctx.model_width;
        face_img.height = app_ctx->landmark_ctx.model_height;
        face_img.format = IMAGE_FORMAT_RGB888;
        face_img.size = get_image_size(&face_img);
        face_img.virt_addr = (unsigned char *)malloc(face_img.size);

        if (face_img.virt_addr == NULL) {
            printf("malloc buffer size:%d fail!\n");
            continue;
        }

        ret = image_crop_resize(src_img, &face_img, detected_faces[i].box.left, detected_faces[i].box.top,
                               detected_faces[i].box.right - detected_faces[i].box.left, detected_faces[i].box.bottom - detected_faces[i].box.top);

        if (ret < 0) {
            printf("crop_and_resize failed!\n");
            free(face_img.virt_addr);
            continue;
        }

        // Run Landmark Detection
        std::vector<ponit_t> landmarks = run_landmark_detection(&app_ctx->landmark_ctx, &face_img);

        // Assign landmarks to the detected face
        for (int j = 0; j < 5 && j < landmarks.size(); ++j) {
            out_result->object[i].ponit[j] = landmarks[j];
        }

        free(face_img.virt_addr);
    }

    return 0;
}