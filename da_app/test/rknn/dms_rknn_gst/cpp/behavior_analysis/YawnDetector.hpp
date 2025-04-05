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
//         bool isYawning;
//         double yawnCount;       // Total yawn count since start
//         double yawnFrequency;   // Yawns in the last 60 seconds
//         double yawnDuration;
//         int yawnKSS;
//     };

//     YawnMetrics run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height);

// private:
//     int calculateYawnKSS();
//     void resetKSS();
//     double calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height);
//     double calculateYawnFrequency(double current_time);

// private:
//     const int MOUTH_TOP;
//     const int MOUTH_BOTTOM;
//     const int YAWN_THRESHOLD;
//     const double MIN_YAWN_DURATION;
//     const double YAWN_COOLDOWN;
//     std::deque<double> yawn_timestamps;  // For 60-second frequency
//     std::vector<std::map<std::string, double>> yawns;
//     bool is_yawn;
//     time_t yawn_start_time;
//     double last_yawn_time;
//     int yawn_kss;
//     double yawn_frequency;  // Yawns in last 60 seconds
//     double last_yawn_duration;
//     double yawn_duration;
//     double reset_interval;
//     double last_reset_time;
//     double totalYawnCount;  // New: Total yawns since start
// };

// #endif // YAWNDETECTOR_HPP

#ifndef YAWNDETECTOR_HPP
#define YAWNDETECTOR_HPP

#include <iostream>
#include <vector>
#include <map>
#include <deque>
#include <chrono>
#include <opencv2/opencv.hpp> // Assuming cv::Point is used internally or needed

class YawnDetector {
public:
    YawnDetector();

    struct YawnMetrics {
        bool isYawning;       // Is the mouth currently open in a yawn state?
        double yawnCount;       // Total valid yawns detected since start
        double yawnFrequency;   // Valid yawns completed in the last 60 seconds
        double yawnDuration;    // Duration of the last completed valid yawn
        int yawnKSS;
    };

    YawnMetrics run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height);

private:
    int calculateYawnKSS();
    void resetKSS();
    double calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height);
    double calculateYawnFrequency(double current_time_seconds); // Parameter type changed

private:
    // Constants
    const int MOUTH_TOP;
    const int MOUTH_BOTTOM;
    const double YAWN_THRESHOLD;    // Use double
    const double MIN_YAWN_DURATION;
    const double YAWN_COOLDOWN;

    // Data Storage
    std::deque<double> yawn_timestamps;  // Stores timestamps (in seconds) of *completed* valid yawns
    double totalYawnCount;          // Cumulative count of valid yawns

    // State Variables
    bool is_yawning_now;          // True if mouth is currently open > threshold
    time_t yawn_start_time;       // Use time_t for system_clock::to_time_t
    double last_yawn_time;        // Use double for time in seconds
    int yawn_kss;
    double yawn_frequency;
    double last_yawn_duration;
    bool processing_detected_yawn; // Flag to ensure a single yawn is counted only once

    // Periodic Reset Variables (If needed, currently only resets KSS which is recalculated)
    double reset_interval;
    double last_reset_time;       // Use double for time in seconds

    // Hysteresis Variables
    int frames_below_threshold;   // Counter for consecutive frames below threshold
    const int FRAMES_HYSTERESIS;  // Number of frames required below threshold to confirm closure
};

#endif // YAWNDETECTOR_HPP