// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <math.h>
// #include "wav2vec2.h"
// #include "file_utils.h"
// #include "audio_utils.h"
// #include <vector>
// #include "process.h"

// static void dump_tensor_attr(rknn_tensor_attr *attr)
// {
//     char dims_str[100];
//     char temp_str[100];
//     memset(dims_str, 0, sizeof(dims_str));
//     for (int i = 0; i < attr->n_dims; i++)
//     {
//         strcpy(temp_str, dims_str);
//         if (i == attr->n_dims - 1)
//         {
//             sprintf(dims_str, "%s%d", temp_str, attr->dims[i]);
//         }
//         else
//         {
//             sprintf(dims_str, "%s%d, ", temp_str, attr->dims[i]);
//         }
//     }

//     printf("  index=%d, name=%s, n_dims=%d, dims=[%s], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, zp=%d, scale=%f\n",
//            attr->index, attr->name, attr->n_dims, dims_str, attr->n_elems, attr->size, get_format_string(attr->fmt),
//            get_type_string(attr->type), get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
// }

// int init_wav2vec2_model(const char *model_path, rknn_app_context_t *app_ctx)
// {
//     int ret;
//     int model_len = 0;
//     char *model;
//     rknn_context ctx = 0;

//     // Load RKNN Model
//     ret = rknn_init(&ctx, (char *)model_path, model_len, 0, NULL);
//     free(model);
//     if (ret < 0)
//     {
//         printf("rknn_init fail! ret=%d\n", ret);
//         return -1;
//     }

//     // Get Model Input Output Number
//     rknn_input_output_num io_num;
//     ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
//     if (ret != RKNN_SUCC)
//     {
//         printf("rknn_query fail! ret=%d\n", ret);
//         return -1;
//     }
//     printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

//     // Get Model Input Info
//     printf("input tensors:\n");
//     rknn_tensor_attr input_attrs[io_num.n_input];
//     memset(input_attrs, 0, sizeof(input_attrs));
//     for (int i = 0; i < io_num.n_input; i++)
//     {
//         input_attrs[i].index = i;
//         ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
//         if (ret != RKNN_SUCC)
//         {
//             printf("rknn_query fail! ret=%d\n", ret);
//             return -1;
//         }
//         dump_tensor_attr(&(input_attrs[i]));
//     }

//     // Get Model Output Info
//     printf("output tensors:\n");
//     rknn_tensor_attr output_attrs[io_num.n_output];
//     memset(output_attrs, 0, sizeof(output_attrs));
//     for (int i = 0; i < io_num.n_output; i++)
//     {
//         output_attrs[i].index = i;
//         ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
//         if (ret != RKNN_SUCC)
//         {
//             printf("rknn_query fail! ret=%d\n", ret);
//             return -1;
//         }
//         dump_tensor_attr(&(output_attrs[i]));
//     }

//     // Set to context
//     app_ctx->rknn_ctx = ctx;
//     app_ctx->io_num = io_num;
//     app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
//     memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
//     app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
//     memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

//     return 0;
// }

// int release_wav2vec2_model(rknn_app_context_t *app_ctx)
// {
//     if (app_ctx->input_attrs != NULL)
//     {
//         free(app_ctx->input_attrs);
//         app_ctx->input_attrs = NULL;
//     }
//     if (app_ctx->output_attrs != NULL)
//     {
//         free(app_ctx->output_attrs);
//         app_ctx->output_attrs = NULL;
//     }
//     if (app_ctx->rknn_ctx != 0)
//     {
//         rknn_destroy(app_ctx->rknn_ctx);
//         app_ctx->rknn_ctx = 0;
//     }
//     return 0;
// }

// int inference_wav2vec2_model(rknn_app_context_t *app_ctx, std::vector<float> audio_data, std::vector<std::string> &recognized_text)
// {
//     int ret;

//     recognized_text.clear();

//     rknn_input inputs[1];
//     rknn_output outputs[1];

//     memset(inputs, 0, sizeof(inputs));
//     memset(outputs, 0, sizeof(outputs));

//     // Set Input Data
//     inputs[0].index = 0;
//     inputs[0].type = RKNN_TENSOR_FLOAT32;
//     inputs[0].size = N_SAMPLES * sizeof(float);
//     inputs[0].buf = (float *)malloc(inputs[0].size);
//     memcpy(inputs[0].buf, audio_data.data(), inputs[0].size);

//     ret = rknn_inputs_set(app_ctx->rknn_ctx, 1, inputs);
//     if (ret < 0)
//     {
//         printf("rknn_input_set fail! ret=%d\n", ret);
//         goto out;
//     }

//     // Run
//     ret = rknn_run(app_ctx->rknn_ctx, nullptr);
//     if (ret < 0)
//     {
//         printf("rknn_run fail! ret=%d\n", ret);
//         goto out;
//     }

//     // Get Output
//     outputs[0].want_float = 1;
//     ret = rknn_outputs_get(app_ctx->rknn_ctx, 1, outputs, NULL);
//     if (ret < 0)
//     {
//         printf("rknn_outputs_get fail! ret=%d\n", ret);
//         goto out;
//     }

//     // post process
//     post_process((float *)outputs[0].buf, recognized_text);

// out:

//     // Remeber to release rknn output
//     rknn_outputs_release(app_ctx->rknn_ctx, 1, outputs);
//     if (inputs[0].buf != NULL)
//     {
//         free(inputs[0].buf);
//     }

//     return ret;
// }











// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <math.h>
// #include "wav2vec2.h"
// #include "file_utils.h"
// #include "audio_utils.h"
// #include <vector>
// #include "process.h"

// static void dump_tensor_attr(rknn_tensor_attr *attr)
// {
//     char dims_str[100];
//     char temp_str[100];
//     memset(dims_str, 0, sizeof(dims_str));
//     for (int i = 0; i < attr->n_dims; i++)
//     {
//         strcpy(temp_str, dims_str);
//         if (i == attr->n_dims - 1)
//         {
//             sprintf(dims_str, "%s%d", temp_str, attr->dims[i]);
//         }
//         else
//         {
//             sprintf(dims_str, "%s%d, ", temp_str, attr->dims[i]);
//         }
//     }

//     printf("  index=%d, name=%s, n_dims=%d, dims=[%s], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, zp=%d, scale=%f\n",
//            attr->index, attr->name, attr->n_dims, dims_str, attr->n_elems, attr->size, get_format_string(attr->fmt),
//            get_type_string(attr->type), get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
// }

// int init_wav2vec2_model(const char *model_path, rknn_app_context_t *app_ctx)
// {
//     int ret;
//     int model_len = 0;
//     char *model;
//     rknn_context ctx = 0;

//     // Load RKNN Model
//     model_len = read_data_from_file(model_path, &model);
//     if (model_len <= 0)
//     {
//         printf("read model file fail!\n");
//         return -1;
//     }

//     ret = rknn_init(&ctx, model, model_len, 0, NULL);
//     free(model);
//     if (ret < 0)
//     {
//         printf("rknn_init fail! ret=%d\n", ret);
//         return -1;
//     }

//     // Get Model Input Output Number
//     rknn_input_output_num io_num;
//     ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
//     if (ret != RKNN_SUCC)
//     {
//         printf("rknn_query fail! ret=%d\n", ret);
//         return -1;
//     }
//     printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

//     // Get Model Input Info
//     printf("input tensors:\n");
//     rknn_tensor_attr input_attrs[io_num.n_input];
//     memset(input_attrs, 0, sizeof(input_attrs));
//     for (int i = 0; i < io_num.n_input; i++)
//     {
//         input_attrs[i].index = i;
//         ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
//         if (ret != RKNN_SUCC)
//         {
//             printf("rknn_query fail! ret=%d\n", ret);
//             return -1;
//         }
//         dump_tensor_attr(&(input_attrs[i]));
//     }pp_ctx->rknn_ctx = ctx;
//     app_ctx->io_num = io_num;
//     app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
//     memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
//     app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
//     memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

//     return 0;
// }

// int release_wav2vec2_model(rknn_app_context_t *app_ctx)
// {
//     if (app_ctx->input_attrs != NULL)
//     {
//         free(app_ctx->input_attrs);
//         app_ctx->input_attrs = NULL;
//     }
//     if (app_ctx->output_attrs != NULL)
//     {
//         free(app_ctx->output_attrs);
//         app_ctx->output_attrs = NULL;
//     }
//     if (app_ctx->rknn_ctx != 0)
//     {
//         rknn_destroy(app_ctx->rknn_ctx);
//         app_ctx->rknn_ctx = 0;
//     }
//     return 0;
// }

// int inference_wav2vec2_model(rknn_app_context_t *app_ctx, std::vector<float> audio_data, std::vector<std::string> &recognized_text)
// {
//     int ret;

//     recognized_text.clear();

//     rknn_input inputs[1];
//     rknn_output outputs[1];

//     memset(inputs, 0, sizeof(inputs));
//     memset(outputs, 0, sizeof(outputs));

//     // Set Input Data
//     inputs[0].index = 0;
//     inputs[0].type = RKNN_TENSOR_FLOAT32; // Use FP32 for compatibility
//     inputs[0].size = N_SAMPLES * sizeof(float);
//     inputs[0].buf = (float *)malloc(inputs[0].size);
//     memcpy(inputs[0].buf, audio_data.data(), inputs[0].size);

//     ret = rknn_inputs_set(app_ctx->rknn_ctx, 1, inputs);
//     if (ret < 0)
//     {
//         printf("rknn_input_set fail! ret=%d\n", ret);
//         goto out;
//     }

//     // Run
//     ret = rknn_run(app_ctx->rknn_ctx, nullptr);
//     if (ret < 0)
//     {
//         printf("rknn_run fail! ret=%d\n", ret);
//         goto out;
//     }

//     // Get Output
//     outputs[0].want_float = 1;
//     ret = rknn_outputs_get(app_ctx->rknn_ctx, 1, outputs, NULL);
//     if (ret < 0)
//     {
//         printf("rknn_outputs_get fail! ret=%d\n", ret);
//         goto out;
//     }

//     // Post process
//     post_process((float *)outputs[0].buf, recognized_text);

// out:
//     // Release rknn output
//     rknn_outputs_release(app_ctx->rknn_ctx, 1, outputs);
//     if (inputs[0].buf != NULL)
//     {
//         free(inputs[0].buf);
//     }

//     return ret;
// }

//     // Get Model Output Info
//     printf("output tensors:\n");
//     rknn_tensor_attr output_attrs[io_num.n_output];
//     memset(output_attrs, 0, sizeof(output_attrs));
//     for (int i = 0; i < io_num.n_output; i++)
//     {
//         output_attrs[i].index = i;
//         ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
//         if (ret != RKNN_SUCC)
//         {
//             printf("rknn_query fail! ret=%d\n", ret);
//             return -1;
//         }
//         dump_tensor_attr(&(output_attrs[i]));
//     }

//     // Set to context
//     app_ctx->rknn_ctx = ctx;
//     app_ctx->io_num = io_num;
//     app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
//     memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
//     app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
//     memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

//     return 0;
// }

// int release_wav2vec2_model(rknn_app_context_t *app_ctx)
// {
//     if (app_ctx->input_attrs != NULL)
//     {
//         free(app_ctx->input_attrs);
//         app_ctx->input_attrs = NULL;
//     }
//     if (app_ctx->output_attrs != NULL)
//     {
//         free(app_ctx->output_attrs);
//         app_ctx->output_attrs = NULL;
//     }
//     if (app_ctx->rknn_ctx != 0)
//     {
//         rknn_destroy(app_ctx->rknn_ctx);
//         app_ctx->rknn_ctx = 0;
//     }
//     return 0;
// }

// int inference_wav2vec2_model(rknn_app_context_t *app_ctx, std::vector<float> audio_data, std::vector<std::string> &recognized_text)
// {
//     int ret;

//     recognized_text.clear();

//     rknn_input inputs[1];
//     rknn_output outputs[1];

//     memset(inputs, 0, sizeof(inputs));
//     memset(outputs, 0, sizeof(outputs));

//     // Set Input Data
//     inputs[0].index = 0;
//     inputs[0].type = RKNN_TENSOR_FLOAT32; // Use FP32 for compatibility
//     inputs[0].size = N_SAMPLES * sizeof(float);
//     inputs[0].buf = (float *)malloc(inputs[0].size);
//     memcpy(inputs[0].buf, audio_data.data(), inputs[0].size);

//     ret = rknn_inputs_set(app_ctx->rknn_ctx, 1, inputs);
//     if (ret < 0)
//     {
//         printf("rknn_input_set fail! ret=%d\n", ret);
//         goto out;
//     }

//     // Run
//     ret = rknn_run(app_ctx->rknn_ctx, nullptr);
//     if (ret < 0)
//     {
//         printf("rknn_run fail! ret=%d\n", ret);
//         goto out;
//     }

//     // Get Output
//     outputs[0].want_float = 1;
//     ret = rknn_outputs_get(app_ctx->rknn_ctx, 1, outputs, NULL);
//     if (ret < 0)
//     {
//         printf("rknn_outputs_get fail! ret=%d\n", ret);
//         goto out;
//     }

//     // Post process
//     post_process((float *)outputs[0].buf, recognized_text);

// out:
//     // Release rknn output
//     rknn_outputs_release(app_ctx->rknn_ctx, 1, outputs);
//     if (inputs[0].buf != NULL)
//     {
//         free(inputs[0].buf);
//     }

//     return ret;
// }



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "wav2vec2.h"
#include "file_utils.h"
#include "audio_utils.h"
#include <vector>
#include "process.h"

static void dump_tensor_attr(rknn_tensor_attr *attr)
{
    char dims_str[100];
    char temp_str[100];
    memset(dims_str, 0, sizeof(dims_str));
    for (int i = 0; i < attr->n_dims; i++)
    {
        strcpy(temp_str, dims_str);
        if (i == attr->n_dims - 1)
        {
            sprintf(dims_str, "%s%d", temp_str, attr->dims[i]);
        }
        else
        {
            sprintf(dims_str, "%s%d, ", temp_str, attr->dims[i]);
        }
    }

    printf("  index=%d, name=%s, n_dims=%d, dims=[%s], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, dims_str, attr->n_elems, attr->size, get_format_string(attr->fmt),
           get_type_string(attr->type), get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

int init_wav2vec2_model(const char *model_path, rknn_app_context_t *app_ctx)
{
    int ret;
    int model_len = 0;
    char *model;
    rknn_context ctx = 0;

    // Load RKNN Model
    model_len = read_data_from_file(model_path, &model);
    if (model_len <= 0)
    {
        printf("read model file fail!\n");
        return -1;
    }

    ret = rknn_init(&ctx, model, model_len, 0, NULL);
    free(model);
    if (ret < 0)
    {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }

    // Get Model Input Output Number
    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC)
    {
        printf("rknn_query fail! ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

    // Get Model Input Info
    printf("input tensors:\n");
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++)
    {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs[i]));
    }

    // Get Model Output Info
    printf("output tensors:\n");
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++)
    {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_attrs[i]));
    }

    // Set to context
    app_ctx->rknn_ctx = ctx;
    app_ctx->io_num = io_num;
    app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

    return 0;
}

int release_wav2vec2_model(rknn_app_context_t *app_ctx)
{
    if (app_ctx->input_attrs != NULL)
    {
        free(app_ctx->input_attrs);
        app_ctx->input_attrs = NULL;
    }
    if (app_ctx->output_attrs != NULL)
    {
        free(app_ctx->output_attrs);
        app_ctx->output_attrs = NULL;
    }
    if (app_ctx->rknn_ctx != 0)
    {
        rknn_destroy(app_ctx->rknn_ctx);
        app_ctx->rknn_ctx = 0;
    }
    return 0;
}

int inference_wav2vec2_model(rknn_app_context_t *app_ctx, std::vector<float> audio_data, std::vector<std::string> &recognized_text)
{
    int ret;

    recognized_text.clear();

    rknn_input inputs[1];
    rknn_output outputs[1];

    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));

    // Set Input Data
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_FLOAT32; // Use FP32 for compatibility
    inputs[0].size = N_SAMPLES * sizeof(float);
    inputs[0].buf = (float *)malloc(inputs[0].size);
    memcpy(inputs[0].buf, audio_data.data(), inputs[0].size);

    ret = rknn_inputs_set(app_ctx->rknn_ctx, 1, inputs);
    if (ret < 0)
    {
        printf("rknn_input_set fail! ret=%d\n", ret);
        goto out;
    }

    // Run
    ret = rknn_run(app_ctx->rknn_ctx, nullptr);
    if (ret < 0)
    {
        printf("rknn_run fail! ret=%d\n", ret);
        goto out;
    }

    // Get Output
    outputs[0].want_float = 1;
    ret = rknn_outputs_get(app_ctx->rknn_ctx, 1, outputs, NULL);
    if (ret < 0)
    {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        goto out;
    }

    // Post process
    post_process((float *)outputs[0].buf, recognized_text);

out:
    // Release rknn output
    rknn_outputs_release(app_ctx->rknn_ctx, 1, outputs);
    if (inputs[0].buf != NULL)
    {
        free(inputs[0].buf);
    }

    return ret;
}