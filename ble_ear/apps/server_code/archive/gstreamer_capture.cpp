#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <ostream>
#include <sys/stat.h>

static GstElement* pipeline_ = nullptr;
static GstElement* appsrc_ = nullptr;
static GstElement* appsink_ = nullptr;

void setupPipeline(const char* output_file) {
    gst_init(nullptr, nullptr);
    const std::string dir = "/userdata/test_cpp/dms_gst";
    struct stat st = {0};
    if (stat(dir.c_str(), &st) == -1) {
        if (mkdir(dir.c_str(), 0700) == -1) {
            std::cerr << "Error: Cannot create directory " << dir << ": " << strerror(errno) << std::endl;
            return;
        }
        std::cout << "INFO: Created directory " << dir << std::endl;
    } else if (access(dir.c_str(), W_OK) != 0) {
        std::cerr << "Error: Directory " << dir << " is not writable." << std::endl;
        return;
    }

    const std::string pipeline_str = "appsrc name=source ! queue ! videoconvert ! video/x-raw,format=NV12 ! mpph265enc rc-mode=cbr bps=4000000 gop=30 qp-min=10 qp-max=51 ! h265parse ! matroskamux ! filesink location=" + std::string(output_file);
    std::cout << "Saving Pipeline: " << pipeline_str << "\n";
    GError* error = nullptr;
    pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
    if (!pipeline_ || error) {
        std::cerr << "Failed to create saving pipeline: " << (error ? error->message : "Unknown error") << "\n";
        if (error) g_error_free(error);
        return;
    }
    appsrc_ = gst_bin_get_by_name(GST_BIN(pipeline_), "source");
    if (!appsrc_) {
        std::cerr << "Failed to get appsrc\n";
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
        return;
    }

    // Use GST_APP_STREAM_TYPE_SEEKABLE or GST_APP_STREAM_TYPE_STREAM based on GStreamer version
    g_object_set(G_OBJECT(appsrc_),
                 "stream-type", 0, // Default to 0 (GST_APP_STREAM_TYPE_STREAM is deprecated in some versions)
                 "format", GST_FORMAT_TIME,
                 "is-live", FALSE,
                 "do-timestamp", TRUE,
                 NULL);

    if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set saving pipeline to playing state\n";
        gst_object_unref(appsrc_);
        gst_object_unref(pipeline_);
        appsrc_ = nullptr;
        pipeline_ = nullptr;
    } else {
        std::cout << "INFO: Saving pipeline created and set to PLAYING.\n";
    }
}

void pushFrameToPipeline(unsigned char* data, int size, int width, int height, GstClockTime duration) {
    if (!appsrc_) return;
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        std::cerr << "Failed map buffer" << std::endl;
        gst_buffer_unref(buffer);
        return;
    }
    memcpy(map.data, data, size);
    gst_buffer_unmap(buffer, &map);

    GST_BUFFER_DURATION(buffer) = duration;

    GstFlowReturn ret;
    g_signal_emit_by_name(appsrc_, "push-buffer", buffer, &ret);
    if (ret != GST_FLOW_OK) {
        std::cerr << "Failed push buffer, ret=" << gst_flow_get_name(ret) << std::endl;
    }
    gst_buffer_unref(buffer);
}

int main(int argc, char **argv) {
    gst_init(nullptr, nullptr);

    const char* video_source = "v4l2src device=/dev/video1 ! queue ! videoconvert ! video/x-raw,format=BGR,width=1920,height=1080,framerate=30/1 ! appsink name=sink";
    const char* output_file = "/userdata/test_cpp/dms_gst/captured_video.mkv";

    GError* error = nullptr;
    GstElement* input_pipeline = gst_parse_launch(video_source, &error);
    if (!input_pipeline || error) {
        std::cerr << "Failed to create input pipeline: " << (error ? error->message : "Unknown error") << "\n";
        if (error) g_error_free(error);
        return -1;
    }

    appsink_ = gst_bin_get_by_name(GST_BIN(input_pipeline), "sink");
    if (!appsink_) {
        std::cerr << "Failed to get appsink\n";
        gst_object_unref(input_pipeline);
        return -1;
    }

    if (gst_element_set_state(input_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set input pipeline to playing state\n";
        gst_object_unref(appsink_);
        gst_object_unref(input_pipeline);
        return -1;
    }

    setupPipeline(output_file);

    GstSample* sample;
    GstVideoInfo video_info;
    GstBuffer* gst_buffer;
    GstMapInfo map_info;
    bool saving_pipeline_caps_set = false;

    while (true) {
        sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink_));
        if (!sample) {
            std::cerr << "End of stream or error pulling sample." << std::endl;
            break;
        }
        gst_buffer = gst_sample_get_buffer(sample);
        GstCaps* caps = gst_sample_get_caps(sample);
        if (!gst_buffer || !caps || !gst_video_info_from_caps(&video_info, caps) || !gst_buffer_map(gst_buffer, &map_info, GST_MAP_READ)) {
            std::cerr << "Failed to get valid buffer/caps/map" << std::endl;
            if (gst_buffer && map_info.memory) gst_buffer_unmap(gst_buffer, &map_info);
            gst_sample_unref(sample);
            continue;
        }

        if (pipeline_ && appsrc_ && !saving_pipeline_caps_set) {
            GstCaps* save_caps = gst_caps_new_simple("video/x-raw",
                                                    "format", G_TYPE_STRING, "BGR",
                                                    "width", G_TYPE_INT, GST_VIDEO_INFO_WIDTH(&video_info),
                                                    "height", G_TYPE_INT, GST_VIDEO_INFO_HEIGHT(&video_info),
                                                    "framerate", GST_TYPE_FRACTION, GST_VIDEO_INFO_FPS_N(&video_info), GST_VIDEO_INFO_FPS_D(&video_info),
                                                    NULL);
            if (save_caps) {
                printf("INFO: Setting saving pipeline caps to: %s\n", gst_caps_to_string(save_caps));
                g_object_set(G_OBJECT(appsrc_), "caps", save_caps, NULL);
                gst_caps_unref(save_caps);
                saving_pipeline_caps_set = true;
            } else {
                std::cerr << "ERROR: Failed to create caps for saving pipeline!\n";
            }
        }

        if (pipeline_ && appsrc_ && saving_pipeline_caps_set) {
            GstClockTime duration = gst_util_uint64_scale_int(GST_SECOND, video_info.fps_d, video_info.fps_n);
            pushFrameToPipeline(map_info.data, map_info.size, GST_VIDEO_INFO_WIDTH(&video_info), GST_VIDEO_INFO_HEIGHT(&video_info), duration);
        }

        gst_buffer_unmap(gst_buffer, &map_info);
        gst_sample_unref(sample);
    }

    if (input_pipeline) {
        gst_element_set_state(input_pipeline, GST_STATE_NULL);
        gst_object_unref(appsink_);
        gst_object_unref(input_pipeline);
        printf("INFO: Input pipeline released.\n");
    }
    if (pipeline_) {
        gst_element_send_event(pipeline_, gst_event_new_eos());
        GstBus* bus = gst_element_get_bus(pipeline_);
        gst_bus_poll(bus, GST_MESSAGE_EOS, GST_CLOCK_TIME_NONE);
        gst_object_unref(bus);
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(appsrc_);
        gst_object_unref(pipeline_);
        printf("INFO: Saving pipeline released.\n");
    }
    gst_deinit();
    printf("Exiting main\n");
    return 0;
}