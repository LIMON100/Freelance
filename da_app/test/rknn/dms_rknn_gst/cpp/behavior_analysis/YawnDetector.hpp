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

    struct YawnMetrics {
        bool isYawning;       // Is the mouth currently open in a yawn state?
        double yawnCount;       // Total valid yawns detected SINCE THE LAST RESET
        double yawnFrequency;   // Valid yawns completed in the last 60 seconds
        double yawnDuration;    // Duration of the last completed valid yawn
        int yawnKSS;
    };

    YawnMetrics run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height);

private:
    int calculateYawnKSS();
    // void resetKSS(); // Keep if needed elsewhere, but count reset is separate now
    double calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height);
    double calculateYawnFrequency(double current_time_seconds); // Parameter type changed

private:
    // Constants
    const int MOUTH_TOP;
    const int MOUTH_BOTTOM;
    const double YAWN_THRESHOLD;  
    const double MIN_YAWN_DURATION;
    const double YAWN_COOLDOWN;
    const double RESET_INTERVAL_SECONDS; 

    // Data Storage
    std::deque<double> yawn_timestamps;  // Stores timestamps (in seconds) of *completed* valid yawns (for frequency)
    double totalYawnCount;          // Cumulative count of valid yawns SINCE LAST RESET

    // State Variables
    bool is_yawning_now;          
    time_t yawn_start_time;       // Use time_t for system_clock::to_time_t
    double last_yawn_time;        // Use double for time in seconds (of last *recorded* yawn)
    int yawn_kss;
    double yawn_frequency;        // Frequency in the last 60s
    double last_yawn_duration;    // Duration of last completed yawn
    bool processing_detected_yawn; // Flag to ensure a single yawn is counted only once

    // Reset Variables
    double last_reset_time_seconds;   

    // Hysteresis Variables
    int frames_below_threshold;   
    const int FRAMES_HYSTERESIS;  
};

#endif // YAWNDETECTOR_HPP









// // YawnDetector.hpp
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

//     // ... (YawnMetrics struct as before) ...
//     struct YawnMetrics {
//         bool isYawning;
//         double yawnCount;
//         double yawnFrequency;
//         double yawnDuration;
//         int yawnKSS;
//     };


//     YawnMetrics run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height);

//     // --- New Calibration Method ---
//     void calibrate(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height);
//     bool isCalibrated() const { return is_calibrated; }
//     // --- End New Calibration Method ---


// private:
//     // ... (other private methods as before) ...
//      int calculateYawnKSS();
//      double calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height);
//      double calculateYawnFrequency(double current_time_seconds);

// private:
//     // Constants
//     const int MOUTH_TOP;
//     const int MOUTH_BOTTOM;
//     // Make threshold non-const so it can be adapted
//     double YAWN_THRESHOLD;
//     // Threshold factors/offsets (examples, tune these)
//     const double YAWN_THRESHOLD_FACTOR = 1.8; // e.g., Yawn if opening > 1.8 * baseline
//     const double YAWN_THRESHOLD_OFFSET = 15.0; // e.g., Yawn if opening > baseline + 15 pixels
//     const double MIN_YAWN_THRESHOLD = 35.0; // Don't let threshold go below this absolute minimum
//     // Other constants
//     const double MIN_YAWN_DURATION;
//     const double YAWN_COOLDOWN;
//     const double RESET_INTERVAL_SECONDS;

//     // --- New Calibration Members ---
//     std::deque<double> calibration_mouth_distances;
//     int calibration_samples_collected;
//     const int required_calibration_samples;
//     double baseline_mouth_opening;
//     bool is_calibrated;
//     // --- End New Calibration Members ---


//     // ... (other private members as before) ...
//     std::deque<double> yawn_timestamps;
//     double totalYawnCount;
//     bool is_yawning_now;
//     time_t yawn_start_time;
//     double last_yawn_time;
//     int yawn_kss;
//     double yawn_frequency;
//     double last_yawn_duration;
//     bool processing_detected_yawn;
//     double last_reset_time_seconds;
//     int frames_below_threshold;
//     const int FRAMES_HYSTERESIS;
// };

// #endif // YAWNDETECTOR_HPP