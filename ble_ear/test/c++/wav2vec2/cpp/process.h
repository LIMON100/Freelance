// #ifndef _RKNN_WAV2VEC2_DEMO_PROCESS_H_
// #define _RKNN_WAV2VEC2_DEMO_PROCESS_H_

// #include "rknn_api.h"

// #define VOCAB_NUM 32
// #define SAMPLE_RATE 16000
// #define CHUNK_LENGTH 20
// #define OUTPUT_SIZE CHUNK_LENGTH * 50 - 1
// #define N_SAMPLES CHUNK_LENGTH *SAMPLE_RATE // AUDIO_LENGTH

// typedef struct
// {
//     int index;
//     char *token;
// } TokenDictEntry;

// void audio_preprocess(audio_buffer_t *audio, std::vector<float> &audio_data);
// void post_process(float *scores, std::vector<std::string> &recognized_text);

// #endif //_RKNN_WAV2VEC2_DEMO_PROCESS_H_





#ifndef RKNN_WAV2VEC2_DEMO_PROCESS_H
#define RKNN_WAV2VEC2_DEMO_PROCESS_H

#include "rknn_api.h"
#include "audio_utils.h"
#include <vector>
#include <string>

#define VOCAB_NUM 32
#define SAMPLE_RATE 16000
#define CHUNK_LENGTH 1 // Process 1-second chunks
#define MODEL_INPUT_LENGTH 20 // Model expects 20 seconds
#define OUTPUT_SIZE MODEL_INPUT_LENGTH * 50 // Adjusted for model output
#define N_SAMPLES MODEL_INPUT_LENGTH * SAMPLE_RATE // 20 seconds of audio (320,000 samples)

typedef struct
{
    int index;
    char *token;
} TokenDictEntry;

void audio_preprocess(audio_buffer_t *audio, std::vector<float> &audio_data, int chunk_samples);
void post_process(float *scores, std::vector<std::string> &recognized_text);

#endif //RKNN_WAV2VEC2_DEMO_PROCESS_H