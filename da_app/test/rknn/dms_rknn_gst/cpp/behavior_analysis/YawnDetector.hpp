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