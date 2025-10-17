#include "tracking_pipeline.hpp"
#include <iostream>

int main(int, char**)
{
    // 1. Create the configuration object.
    PipelineConfig config;

    try {
        // 2. Create an instance of the pipeline.
        TrackingPipeline pipeline(config);

        // 3. Run the pipeline.
        pipeline.run();

    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: An exception occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
