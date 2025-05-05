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

//     // ++ New struct to store yawn details ++
//     struct YawnEvent {
//         double timestamp; // Time the yawn ENDED (in seconds since epoch)
//         double duration;  // Duration of the yawn (in seconds)
//     };

//     struct YawnMetrics {
//         bool isYawning;       // Is the mouth currently open in a yawn state?
//         // double yawnCount;    // Removed, KSS is now based on event history
//         double yawnFrequency_5min; // Calculated count meeting >=2s duration in last 5min (for KSS logic)
//         double yawnDuration;    // Duration of the last completed valid yawn (for display)
//         int yawnKSS;
//     };

//     YawnMetrics run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height);


//     // ADDED: Getters for yawn counts used in KSS calculation
//     // NEW FOR UI
//     int getYawnCount2s() const { return count_ge_2_sec_last_calc; }
//     int getYawnCount3s() const { return count_ge_3_sec_last_calc; }
//     int getYawnCount4s() const { return count_ge_4_sec_last_calc; }

// private:
//     int calculateYawnKSS(double current_time_seconds); // Needs current time
//     double calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height);
//     // Removed calculateYawnFrequency

// private:
//     // Constants
//     const int MOUTH_TOP;
//     const int MOUTH_BOTTOM;
//     const double YAWN_THRESHOLD;
//     const double MIN_YAWN_DURATION; // Still needed to validate *any* yawn
//     const double YAWN_COOLDOWN;
//     const double KSS_TIME_WINDOW_SECONDS; // Window for KSS calculation (300s = 5 min)

//     // ++ Data Storage: Stores events of completed yawns ++
//     std::deque<YawnEvent> completed_yawns_history;

//     // State Variables
//     bool is_yawning_now;
//     time_t yawn_start_time; // Keep using time_t for start time recording
//     double last_valid_yawn_end_time; // Time the last *valid* yawn ended
//     int yawn_kss;
//     // double yawn_frequency; // Removed
//     double last_yawn_duration; // Keep for metrics struct
//     bool processing_detected_yawn;

//     // Hysteresis Variables
//     int frames_below_threshold;
//     const int FRAMES_HYSTERESIS;

//     // NEW FOR UI
//     int count_ge_2_sec_last_calc = 0;
//     int count_ge_3_sec_last_calc = 0;
//     int count_ge_4_sec_last_calc = 0;
// };

// #endif // YAWNDETECTOR_HPP









// Dynamic CALIBRATION
// File: behavior_analysis/YawnDetector.hpp
#ifndef YAWNDETECTOR_HPP
#define YAWNDETECTOR_HPP

// ... (includes remain the same) ...
#include <iostream>
#include <vector>
#include <map>
#include <deque>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <numeric>   // For std::accumulate
#include <algorithm> // For std::sort

double calculate_median(std::deque<double>& data);

class YawnDetector {
public:
    YawnDetector(); // Default threshold in constructor

    // ... (YawnEvent, YawnMetrics structs remain the same) ...
     struct YawnEvent { double timestamp; double duration; }; struct YawnMetrics { bool isYawning; double yawnFrequency_5min; double yawnDuration; int yawnKSS; };

    YawnMetrics run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height);

    // +++ ADDED: Setter for personalized threshold +++
    void setPersonalizedYawnThreshold(double threshold);

    // Getters for KSS counts remain
    int getYawnCount2s() const { return count_ge_2_sec_last_calc; }
    int getYawnCount3s() const { return count_ge_3_sec_last_calc; }
    int getYawnCount4s() const { return count_ge_4_sec_last_calc; }


private:
    int calculateYawnKSS(double current_time_seconds);
    double calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height);

    // Constants
    const int MOUTH_TOP;
    const int MOUTH_BOTTOM;
    // const double YAWN_THRESHOLD; // REMOVED const
    const double MIN_YAWN_DURATION;
    const double YAWN_COOLDOWN;
    const double KSS_TIME_WINDOW_SECONDS;

    // ++ Use non-const member for threshold ++
    double personalized_yawn_threshold; // Default set in constructor

    std::deque<YawnEvent> completed_yawns_history;

    // State Variables
    bool is_yawning_now;
    time_t yawn_start_time;
    double last_valid_yawn_end_time;
    int yawn_kss;
    double last_yawn_duration;
    bool processing_detected_yawn;
    int count_ge_2_sec_last_calc = 0;
    int count_ge_3_sec_last_calc = 0;
    int count_ge_4_sec_last_calc = 0;

    // Hysteresis Variables
    int frames_below_threshold;
    const int FRAMES_HYSTERESIS;
};

#endif // YAWNDETECTOR_HPP