#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <string>
#include <iostream>
#include <thread>
#include <atomic>

static GstElement *pipeline, *input_selector;
static std::atomic<bool> running(true);

void switch_input(GstElement *selector, const std::string &pad_name) 
{
    GstPad *pad = gst_element_get_static_pad(selector, pad_name.c_str());
    if (pad) 
    {
        g_object_set(selector, "active-pad", pad, nullptr);
        std::cout << "Switched to: " << pad_name << std::endl;
        gst_object_unref(pad);
    } 
    else 
    {
        std::cerr << "Failed to get pad: " << pad_name << std::endl;
    }
}

void handle_input() 
{
    char input;
    while (running) 
    {
        std::cin >> input;
        if (input >= '0' && input <= '3') 
        {
            switch_input(input_selector, "sink_" + std::string(1, input));
        } 
        else if (input == 'q') 
        {
            std::cout << "Exiting..." << std::endl;
            running = false;
            break;
        } 
        else 
        {
            std::cout << "Unknown Key input." << std::endl;
        }
    }
}

int main(int argc, char *argv[]) 
{
    GstBus *bus;
    GstMessage *msg;
    GError *error = NULL;

    gst_init(&argc, &argv); // Initialize GStreamer

    pipeline = gst_parse_launch(
        "input-selector name=selector ! videoconvert ! autovideosink "
        "v4l2src device=/dev/video0 ! queue ! selector. "
        "v4l2src device=/dev/video1 ! queue ! selector. "
        "v4l2src device=/dev/video2 ! queue ! selector. "
        "v4l2src device=/dev/video3 ! queue ! selector.", &error);

    if (!pipeline || error) 
    {
        g_printerr("Failed to create pipeline: %s\n", error->message);
        g_clear_error(&error);
        return -1;
    }

    // Get input-selector element
    input_selector = gst_bin_get_by_name(GST_BIN(pipeline), "selector");
    if (!input_selector) 
    {
        g_printerr("Failed to find input-selector\n");
        gst_object_unref(pipeline);
        return -1;
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    std::cout << "Pipeline is playing\n";
    std::cout << "Enter 0~3 to switch inputs or 'q' to exit.\n";

    switch_input(input_selector, "sink_0"); // Set initial input

    std::thread input_thread(handle_input); // Start a separate thread for input handling

    bus = gst_element_get_bus(pipeline); // Process bus messages

    while (running) 
    {
        msg = gst_bus_timed_pop_filtered(bus, GST_SECOND, 
            static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
        if (msg != NULL) 
        {
            gst_message_unref(msg);
            running = false;
        }
    }

    // Render Cleanup
    input_thread.join();
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}