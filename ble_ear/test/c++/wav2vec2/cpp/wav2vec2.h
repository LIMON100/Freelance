// #ifndef _RKNN_DEMO_WAV2VEC2_H_
// #define _RKNN_DEMO_WAV2VEC2_H_

// #include "rknn_api.h"
// #include "audio_utils.h"
// #include <iostream>
// #include <vector>
// #include <string>
// #include "process.h"

// typedef struct
// {
//     rknn_context rknn_ctx;
//     rknn_input_output_num io_num;
//     rknn_tensor_attr *input_attrs;
//     rknn_tensor_attr *output_attrs;
// } rknn_app_context_t;

// int init_wav2vec2_model(const char *model_path, rknn_app_context_t *app_ctx);
// int release_wav2vec2_model(rknn_app_context_t *app_ctx);
// int inference_wav2vec2_model(rknn_app_context_t *app_ctx, std::vector<float> audio_data, std::vector<std::string> &recognized_text);

// #endif //_RKNN_DEMO_WAV2VEC2_H_



#ifndef RKNN_DEMO_WAV2VEC2_H
#define RKNN_DEMO_WAV2VEC2_H

#include "rknn_api.h"
#include "audio_utils.h"
#include <iostream>
#include <vector>
#include <string>
#include "process.h"

typedef struct {
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr *input_attrs;
    rknn_tensor_attr *output_attrs;
} rknn_app_context_t;

int init_wav2vec2_model(const char *model_path, rknn_app_context_t *app_ctx);
int release_wav2vec2_model(rknn_app_context_t *app_ctx);
int inference_wav2vec2_model(rknn_app_context_t *app_ctx, std::vector<float> audio_data, std::vector<std::string> &recognized_text);

#endif //RKNN_DEMO_WAV2VEC2_H