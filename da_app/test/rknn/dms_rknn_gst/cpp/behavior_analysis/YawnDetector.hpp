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
//         double yawnCount;
//         double yawnDuration;
//         double yawnFrequency;
//         int yawnKSS;
//     };

//     // YawnMetrics run(const cv::Mat& frame, const std::vector<cv::Point>& faceLandmarks);
//     YawnMetrics run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height);

// private:
//     // void _draw_annotations(const cv::Mat& frame, cv::Point mouth_top, cv::Point mouth_bottom);
//     int calculateYawnKSS();
//     void resetKSS();
//     // double calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, cv::Size frame_size);
//     double calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height);
//     double calculateYawnFrequency(double current_time);

// private:
//     const int MOUTH_TOP;
//     const int MOUTH_BOTTOM;
//     const int YAWN_THRESHOLD;
//     const double MIN_YAWN_DURATION;
//     const double YAWN_COOLDOWN;
//     std::deque<double> yawn_timestamps;
//     std::vector<std::map<std::string, double>> yawns;
//     bool is_yawn;
//     time_t yawn_start_time;
//     double last_yawn_time;
//     int yawn_kss;
//     double yawn_frequency;
//     double last_yawn_duration;
//     double yawn_duration;
//     double reset_interval;
//     double last_reset_time;
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

    struct YawnMetrics {
        bool isYawning;
        double yawnCount;       // Total yawn count since start
        double yawnFrequency;   // Yawns in the last 60 seconds
        double yawnDuration;
        int yawnKSS;
    };

    YawnMetrics run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height);

private:
    int calculateYawnKSS();
    void resetKSS();
    double calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height);
    double calculateYawnFrequency(double current_time);

private:
    const int MOUTH_TOP;
    const int MOUTH_BOTTOM;
    const int YAWN_THRESHOLD;
    const double MIN_YAWN_DURATION;
    const double YAWN_COOLDOWN;
    std::deque<double> yawn_timestamps;  // For 60-second frequency
    std::vector<std::map<std::string, double>> yawns;
    bool is_yawn;
    time_t yawn_start_time;
    double last_yawn_time;
    int yawn_kss;
    double yawn_frequency;  // Yawns in last 60 seconds
    double last_yawn_duration;
    double yawn_duration;
    double reset_interval;
    double last_reset_time;
    double totalYawnCount;  // New: Total yawns since start
};

#endif // YAWNDETECTOR_HPP