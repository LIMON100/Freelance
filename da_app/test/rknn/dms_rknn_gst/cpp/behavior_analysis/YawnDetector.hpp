// #ifndef YAWNDETECTOR_HPP
// #define YAWNDETECTOR_HPP

// #include <iostream>
// #include <vector>
// #include <map>
// #include <deque>
// #include <chrono>
// #include <opencv2/opencv.hpp> 

// class YawnDetector {
// public:
//     YawnDetector();

//     struct YawnMetrics {
//         bool isYawning;       // Is the mouth currently open in a yawn state?
//         double yawnCount;       // Total valid yawns detected SINCE THE LAST RESET
//         double yawnFrequency;   // Valid yawns completed in the last 60 seconds
//         double yawnDuration;    // Duration of the last completed valid yawn
//         int yawnKSS;
//     };

//     YawnMetrics run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height);

// private:
//     int calculateYawnKSS();
//     // void resetKSS(); // Keep if needed elsewhere, but count reset is separate now
//     double calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height);
//     double calculateYawnFrequency(double current_time_seconds); // Parameter type changed

// private:
//     // Constants
//     const int MOUTH_TOP;
//     const int MOUTH_BOTTOM;
//     const double YAWN_THRESHOLD;  
//     const double MIN_YAWN_DURATION;
//     const double YAWN_COOLDOWN;
//     const double RESET_INTERVAL_SECONDS; 

//     // Data Storage
//     std::deque<double> yawn_timestamps;  // Stores timestamps (in seconds) of *completed* valid yawns (for frequency)
//     double totalYawnCount;          // Cumulative count of valid yawns SINCE LAST RESET

//     // State Variables
//     bool is_yawning_now;          
//     time_t yawn_start_time;       // Use time_t for system_clock::to_time_t
//     double last_yawn_time;        // Use double for time in seconds (of last *recorded* yawn)
//     int yawn_kss;
//     double yawn_frequency;        // Frequency in the last 60s
//     double last_yawn_duration;    // Duration of last completed yawn
//     bool processing_detected_yawn; // Flag to ensure a single yawn is counted only once

//     // Reset Variables
//     double last_reset_time_seconds;   

//     // Hysteresis Variables
//     int frames_below_threshold;   
//     const int FRAMES_HYSTERESIS;  
// };

// #endif // YAWNDETECTOR_HPP





#ifndef YAWNDETECTOR_HPP
#define YAWNDETECTOR_HPP

#include <iostream>
#include <vector>
#include <map>
#include <deque>
#include <chrono>
#include <opencv2/opencv.hpp>

class YawnDetector {
public:
    YawnDetector();

    // ++ New struct to store yawn details ++
    struct YawnEvent {
        double timestamp; // Time the yawn ENDED (in seconds since epoch)
        double duration;  // Duration of the yawn (in seconds)
    };

    struct YawnMetrics {
        bool isYawning;       // Is the mouth currently open in a yawn state?
        // double yawnCount;    // Removed, KSS is now based on event history
        double yawnFrequency_5min; // Calculated count meeting >=2s duration in last 5min (for KSS logic)
        double yawnDuration;    // Duration of the last completed valid yawn (for display)
        int yawnKSS;
    };

    YawnMetrics run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height);

private:
    int calculateYawnKSS(double current_time_seconds); // Needs current time
    double calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height);
    // Removed calculateYawnFrequency

private:
    // Constants
    const int MOUTH_TOP;
    const int MOUTH_BOTTOM;
    const double YAWN_THRESHOLD;
    const double MIN_YAWN_DURATION; // Still needed to validate *any* yawn
    const double YAWN_COOLDOWN;
    const double KSS_TIME_WINDOW_SECONDS; // Window for KSS calculation (300s = 5 min)

    // ++ Data Storage: Stores events of completed yawns ++
    std::deque<YawnEvent> completed_yawns_history;

    // State Variables
    bool is_yawning_now;
    time_t yawn_start_time; // Keep using time_t for start time recording
    double last_valid_yawn_end_time; // Time the last *valid* yawn ended
    int yawn_kss;
    // double yawn_frequency; // Removed
    double last_yawn_duration; // Keep for metrics struct
    bool processing_detected_yawn;

    // Hysteresis Variables
    int frames_below_threshold;
    const int FRAMES_HYSTERESIS;
};

#endif // YAWNDETECTOR_HPP