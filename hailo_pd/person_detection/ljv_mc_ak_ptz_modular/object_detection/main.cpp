#include "tracking_pipeline.hpp"
#include <iostream>

int main(int, char**)
{
    // 1. Create the configuration object.
    // All user settings are now in one place in config.hpp.
    PipelineConfig config;

    try {
        // 2. Create an instance of the pipeline.
        // The constructor handles all initialization (opening video, loading model, etc.).
        TrackingPipeline pipeline(config);

        // 3. Run the pipeline.
        // This blocks until the video is finished or the user quits.
        pipeline.run();

    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: An exception occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
