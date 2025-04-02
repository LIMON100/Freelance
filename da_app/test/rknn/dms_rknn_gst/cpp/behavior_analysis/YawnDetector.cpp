#include "YawnDetector.hpp"
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <chrono> 

YawnDetector::YawnDetector() :
    MOUTH_TOP(13),
    MOUTH_BOTTOM(14),
    YAWN_THRESHOLD(25),
    MIN_YAWN_DURATION(0.3),
    YAWN_COOLDOWN(0.1),
    is_yawn(false),
    yawn_start_time(0),
    last_yawn_time(0),
    yawn_kss(0),
    yawn_frequency(0),
    last_yawn_duration(0), // Add this line 
    reset_interval(60),
    last_reset_time(0)
{
    yawns.push_back(std::map<std::string, double>{{"start", 0}, {"end", 0}, {"duration", 0}});
    last_reset_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

// void YawnDetector::_draw_annotations(const cv::Mat& frame, cv::Point mouth_top, cv::Point mouth_bottom) {
//     cv::Mat frameCopy = frame.clone();
//     int h = frameCopy.rows;
//     int w = frameCopy.cols;
//     int x1 = mouth_top.x;
//     int y1 = mouth_top.y;
//     int x2 = mouth_bottom.x;
//     int y2 = mouth_bottom.y;

//     // Draw mouth line
//     cv::line(frameCopy, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 255, 0), 2);
//     cv::circle(frameCopy, cv::Point(x1, y1), 3, cv::Scalar(0, 0, 255), -1);
//     cv::circle(frameCopy, cv::Point(x2, y2), 3, cv::Scalar(0, 0, 255), -1);

//     if (yawn_start_time != 0) {
//         cv::putText(frameCopy, "Yawning", cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);

//         double progress = std::min((std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - yawn_start_time) / MIN_YAWN_DURATION, 1.0);
//         cv::rectangle(frameCopy, cv::Point(10, 50), cv::Point(150, 70), cv::Scalar(0, 0, 0), -1);
//         cv::rectangle(frameCopy, cv::Point(10, 50), cv::Point(50 + static_cast<int>(100 * progress), 70), cv::Scalar(0, 255, 255), -1);
//     }
// }

int YawnDetector::calculateYawnKSS() {
    if (yawn_frequency < 3) {
        yawn_kss = 1;
    } else if (yawn_frequency < 3 && yawn_frequency < 5) {
        yawn_kss = 4;
    } else if (yawn_frequency >= 5 && yawn_frequency < 7) {
        yawn_kss = 7;
    } else if (last_yawn_duration > 2 || yawn_frequency > 7) {
        yawn_kss = 9;
    }
    return yawn_kss;
}

void YawnDetector::resetKSS() {
    double current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    if (current_time - last_reset_time >= reset_interval) {
        yawns.clear();
        yawns.push_back(std::map<std::string, double>{{"start", 0}, {"end", 0}, {"duration", 0}});
        yawn_kss = 1;
        last_reset_time = current_time;
    }
}

// double YawnDetector::calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, cv::Size frame_size) {
double YawnDetector::calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height) {
    double x1 = landmark1.x;
    double y1 = landmark1.y;
    double x2 = landmark2.x;
    double y2 = landmark2.y;
    return std::hypot(x2 - x1, y2 - y1);
}

double YawnDetector::calculateYawnFrequency(double current_time) {
    while (!yawn_timestamps.empty() && current_time - yawn_timestamps.front() > 60) {
        yawn_timestamps.pop_front();
    }
    yawn_frequency = yawn_timestamps.size();
    return yawn_frequency;
}

// YawnDetector::YawnMetrics YawnDetector::run(const cv::Mat& frame, const std::vector<cv::Point>& faceLandmarks) {
YawnDetector::YawnMetrics YawnDetector::run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height){
    YawnMetrics metrics;
    auto current_time = std::chrono::system_clock::now(); // Store current time

    if (faceLandmarks.size() > 10){
        cv::Point mouth_top = faceLandmarks[MOUTH_TOP];
        cv::Point mouth_bottom = faceLandmarks[MOUTH_BOTTOM];

        // Calculate mouth openness
        // double mouth_distance = calculatePixelDistance(mouth_top, mouth_bottom, frame.size());
        double mouth_distance = calculatePixelDistance(mouth_top, mouth_bottom, frame_width, frame_height);

        // Yawn detection logic
        if (mouth_distance > YAWN_THRESHOLD) {
            if (yawn_start_time == 0) {
                yawn_start_time = std::chrono::system_clock::to_time_t(current_time); // Store start time in system_clock::time_point
                is_yawn = true;
            }
        } else {
            if (yawn_start_time != 0) {
                auto end_time = current_time;
                std::chrono::duration<double> yawn_duration = end_time - std::chrono::system_clock::from_time_t(yawn_start_time);
                last_yawn_duration = yawn_duration.count();

                if (last_yawn_duration >= MIN_YAWN_DURATION) {

                    if ((std::chrono::system_clock::to_time_t(current_time) - last_yawn_time) > YAWN_COOLDOWN) {
                        
                        std::map<std::string, double> yawn_data;
                        yawn_data["start"] = yawn_start_time;
                        yawn_data["end"] = std::chrono::system_clock::to_time_t(current_time);;
                        yawn_data["duration"] = last_yawn_duration;
                        yawns.push_back(yawn_data);

                        yawn_timestamps.push_back(std::chrono::system_clock::to_time_t(current_time));
                        last_yawn_time = std::chrono::system_clock::to_time_t(current_time);
                    }
                }

                yawn_start_time = 0;
                is_yawn = false;
            }
        }
        // Draw landmarks and info
        // _draw_annotations(frame, mouth_top, mouth_bottom);
        yawn_frequency = calculateYawnFrequency(std::chrono::system_clock::to_time_t(current_time));

    }
    int yawn_kss_ = calculateYawnKSS();
    metrics.isYawning = is_yawn;
    metrics.yawnCount = yawn_frequency;
    metrics.yawnDuration = last_yawn_duration; 
    metrics.yawnFrequency = yawn_frequency;
    metrics.yawnKSS = yawn_kss_;
    return metrics;
}