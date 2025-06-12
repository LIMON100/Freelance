// #include "wav2vec2.h"
// #include <math.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// static TokenDictEntry tokenizer_dict[] = {
//             {0, "<pad>"}, {1, "<s>"}, {2, "</s>"}, {3, "<unk>"}, {4, "|"}, {5, "E"}, {6, "T"}, {7, "A"}, 
//             {8, "O"}, {9, "N"}, {10, "I"}, {11, "H"}, {12, "S"}, {13, "R"}, {14, "D"}, {15, "L"}, 
//             {16, "U"}, {17, "M"}, {18, "W"}, {19, "C"}, {20, "F"}, {21, "G"}, {22, "Y"}, {23, "P"}, 
//             {24, "B"}, {25, "V"}, {26, "K"}, {27, "'"}, {28, "X"}, {29, "J"}, {30, "Q"}, {31, "Z"}};

// static void pad_or_trim(const std::vector<float> &array, std::vector<float> &result, int array_shape, int length)
// {
//     if (array_shape > length)
//     {
//         std::copy(array.begin(), array.begin() + length, result.begin());
//     }
//     else
//     {
//         std::copy(array.begin(), array.end(), result.begin());
//         std::fill(result.begin() + array_shape, result.end(), 0.0f);
//     }
// }

// static int argmax(float *array, int size)
// {
//     int max_index = 0;
//     float max_value = array[0];

//     for (int i = 1; i < size; i++)
//     {
//         if (array[i] > max_value)
//         {
//             max_index = i;
//             max_value = array[i];
//         }
//     }

//     return max_index;
// }

// static void compress_sequence(int *sequence, int num_rows, std::vector<int> &compressed_sequence)
// {
//     compressed_sequence.push_back(sequence[0]);

//     for (size_t i = 1; i < num_rows; i++)
//     {
//         if (sequence[i] != sequence[i - 1])
//         {
//             compressed_sequence.push_back(sequence[i]);
//         }
//     }
// }

// static void decode(int *token_ids, int num_rows, std::vector<std::string> &recognized_text)
// {
//     std::vector<int> compressed_token_ids;
//     std::string token;
//     compress_sequence(token_ids, num_rows, compressed_token_ids);
//     for (int token_id : compressed_token_ids)
//     {
//         if (token_id <= 4)
//         {
//             if (token_id == 4)
//             {
//                 recognized_text.push_back(" ");
//             }
//             continue;
//         }

//         token = tokenizer_dict[token_id].token;
//         recognized_text.push_back(token);
//     }
// }

// void audio_preprocess(audio_buffer_t *audio, std::vector<float> &audio_data)
// {
//     std::vector<float> ori_audio_data(audio->data, audio->data + audio->num_frames);
//     pad_or_trim(ori_audio_data, audio_data, audio->num_frames, N_SAMPLES);
// }

// void post_process(float *output, std::vector<std::string> &recognized_text)
// {
//     int num_rows = OUTPUT_SIZE;
//     int num_columns = VOCAB_NUM;

//     int predicted_ids[num_rows];
//     for (int i = 0; i < num_rows; i++)
//     {
//         int maxIndex = argmax(&output[i * num_columns], num_columns);
//         predicted_ids[i] = maxIndex;
//     }

//     decode(predicted_ids, num_rows, recognized_text);
// }




#include "wav2vec2.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

static TokenDictEntry tokenizer_dict[] = {
    {0, "<pad>"}, {1, "<s>"}, {2, "</s>"}, {3, "<unk>"}, {4, "|"},
    {5, "E"}, {6, "T"}, {7, "A"}, {8, "O"}, {9, "N"},
    {10, "I"}, {11, "H"}, {12, "S"}, {13, "R"}, {14, "D"},
    {15, "L"}, {16, "U"}, {17, "M"}, {18, "W"}, {19, "C"},
    {20, "F"}, {21, "G"}, {22, "Y"}, {23, "P"}, {24, "B"},
    {25, "V"}, {26, "K"}, {27, "'"}, {28, "X"}, {29, "J"},
    {30, "Q"}, {31, "Z"}
};

// Command dictionary for single-word commands
static const char *command_dict[] = {
    "STOP", "START", "LEFT", "RIGHT", "UP", "DOWN", "VOLUME", "PLAY", "PAUSE", "NEXT", "PREVIOUS"
};
static const int command_dict_size = 11;

static void pad_or_trim(const std::vector<float> &array, std::vector<float> &result, int array_shape, int length)
{
    if (array_shape > length)
    {
        std::copy(array.begin(), array.begin() + length, result.begin());
    }
    else
    {
        std::copy(array.begin(), array.end(), result.begin());
        std::fill(result.begin() + array_shape, result.end(), 0.0f);
    }
}

static int argmax(float *array, int size, float &max_value)
{
    int max_index = 0;
    max_value = array[0];

    for (int i = 1; i < size; i++)
    {
        if (array[i] > max_value)
        {
            max_index = i;
            max_value = array[i];
        }
    }

    return max_index;
}

static void compress_sequence(int *sequence, int num_rows, std::vector<int> &compressed_sequence)
{
    compressed_sequence.push_back(sequence[0]);

    for (size_t i = 1; i < num_rows; i++)
    {
        if (sequence[i] != sequence[i - 1])
        {
            compressed_sequence.push_back(sequence[i]);
        }
    }
}

static void decode(int *token_ids, int num_rows, std::vector<std::string> &recognized_text)
{
    std::vector<int> compressed_token_ids;
    std::string token;
    compress_sequence(token_ids, num_rows, compressed_token_ids);

    // Debug: Print raw token IDs
    std::cout << "Raw token IDs: ";
    for (int id : compressed_token_ids)
    {
        std::cout << id << " (" << tokenizer_dict[id].token << ") ";
    }
    std::cout << std::endl;

    std::string word;
    for (int token_id : compressed_token_ids)
    {
        if (token_id <= 4)
        {
            if (token_id == 4 && !word.empty())
            {
                // Output the formed word
                recognized_text.push_back(word);
                // Check if the word is a command
                std::string upper_word = word;
                std::transform(upper_word.begin(), upper_word.end(), upper_word.begin(), ::toupper);
                for (int i = 0; i < command_dict_size; ++i)
                {
                    if (upper_word == command_dict[i])
                    {
                        recognized_text.push_back("[" + upper_word + "]");
                        break;
                    }
                }
                word.clear();
            }
            continue;
        }

        token = tokenizer_dict[token_id].token;
        word += token;
    }

    // Check the last word
    if (!word.empty())
    {
        recognized_text.push_back(word);
        std::string upper_word = word;
        std::transform(upper_word.begin(), upper_word.end(), upper_word.begin(), ::toupper);
        for (int i = 0; i < command_dict_size; ++i)
        {
            if (upper_word == command_dict[i])
            {
                recognized_text.push_back("[" + upper_word + "]");
                break;
            }
        }
    }
}

void audio_preprocess(audio_buffer_t *audio, std::vector<float> &audio_data, int chunk_samples)
{
    // Normalize audio chunk
    std::vector<float> chunk_data(chunk_samples, 0.0f);
    int samples_to_copy = std::min(chunk_samples, (int)audio->num_frames);
    float max_amplitude = 0.0f;
    for (int i = 0; i < samples_to_copy; ++i)
    {
        chunk_data[i] = audio->data[i];
        max_amplitude = std::max(max_amplitude, std::abs(chunk_data[i]));
    }

    // Normalize to [-1, 1] if max_amplitude is significant
    if (max_amplitude > 1e-5)
    {
        for (int i = 0; i < samples_to_copy; ++i)
        {
            chunk_data[i] /= max_amplitude;
        }
    }

    // Debug: Print first few samples
    std::cout << "Audio chunk samples (first 5): ";
    for (int i = 0; i < std::min(5, (int)chunk_data.size()); ++i)
    {
        std::cout << chunk_data[i] << " ";
    }
    std::cout << std::endl;

    // Pad or trim to match model input size
    pad_or_trim(chunk_data, audio_data, chunk_samples, N_SAMPLES);
}

void post_process(float *output, std::vector<std::string> &recognized_text)
{
    int num_rows = OUTPUT_SIZE;
    int num_columns = VOCAB_NUM;

    int predicted_ids[num_rows];
    for (int i = 0; i < num_rows; i++)
    {
        float max_value;
        int maxIndex = argmax(&output[i * num_columns], num_columns, max_value);
        predicted_ids[i] = max_value > 0.1f ? maxIndex : 0; // Threshold low-confidence tokens
    }

    decode(predicted_ids, num_rows, recognized_text);
}