// #include "tracking_pipeline.hpp"
// #include <iostream>

// int main(int, char**)
// {
//     // 1. Create the configuration object.
//     PipelineConfig config;

//     try {
//         // 2. Create an instance of the pipeline.
//         TrackingPipeline pipeline(config);

//         // 3. Run the pipeline.
//         pipeline.run();

//     } catch (const std::exception& e) {
//         std::cerr << "FATAL ERROR: An exception occurred: " << e.what() << std::endl;
//         return 1;
//     }

//     return 0;
// }



// File: main.cpp
#include "tracking_pipeline.hpp" 
#include <iostream>
#include <exception> 

/*-------------------------------------------
                  Main Function
-------------------------------------------*/
int main(int /*argc*/, char ** /*argv*/) // Marked argc and argv as unused
{
    // Create the configuration object.
    PipelineConfig config;

    try {
        // Create an instance of the pipeline.
        // The constructor initializes GStreamer and the Hailo model.
        TrackingPipeline pipeline(config);

        // Run the pipeline.
        pipeline.run();

    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: An exception occurred: " << e.what() << std::endl;
        // Note: GStreamer cleanup should be handled by TrackingPipeline's destructor
        // even if an exception occurs.
        return 1;
    } catch (...) { // Catch any other unexpected exceptions
        std::cerr << "FATAL ERROR: An unknown exception occurred." << std::endl;
        return 1;
    }

    return 0;
}