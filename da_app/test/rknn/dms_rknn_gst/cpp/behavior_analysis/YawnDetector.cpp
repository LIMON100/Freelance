#include "YawnDetector.hpp"
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <chrono>
#include <cmath>     // Include for std::hypot, std::max
#include <numeric>   // Include for std::accumulate

YawnDetector::YawnDetector() :
    MOUTH_TOP(13),
    MOUTH_BOTTOM(14),
    YAWN_THRESHOLD(45.0),
    MIN_YAWN_DURATION(0.3),
    YAWN_COOLDOWN(1.0),
    RESET_INTERVAL_SECONDS(300.0), // <<<--- ADDED: 5 minutes * 60 seconds/minute
    is_yawning_now(false),
    yawn_start_time(0),
    last_yawn_time(0.0),
    yawn_kss(0), // Initialize KSS
    yawn_frequency(0.0),
    last_yawn_duration(0.0),
    totalYawnCount(0), // Initialize total count
    processing_detected_yawn(false),
    frames_below_threshold(0),
    FRAMES_HYSTERESIS(3),
    last_reset_time_seconds(0.0) // <<<--- Will be initialized properly below
{
    // Initialize last_reset_time properly using current time
    last_reset_time_seconds = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
    // printf("YawnDetector Initialized. Initial Reset Time: %.2f\n", last_reset_time_seconds);
}

int YawnDetector::calculateYawnKSS() {
    // KSS based on frequency in the last 60 seconds
    double current_freq = std::max(0.0, yawn_frequency);
    if (current_freq < 3) yawn_kss = 1;
    else if (current_freq < 5) yawn_kss = 4;
    else if (current_freq < 7) yawn_kss = 7;
    else yawn_kss = 9; // >= 7 yawns/min is high
    return yawn_kss;
}

double YawnDetector::calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height) {
    return std::hypot((double)landmark1.x - landmark2.x, (double)landmark1.y - landmark2.y);
}

double YawnDetector::calculateYawnFrequency(double current_time_seconds) {
    // Calculate frequency based on *completed* yawns in the last 60 seconds
    while (!yawn_timestamps.empty() && current_time_seconds - yawn_timestamps.front() > 60.0) {
        yawn_timestamps.pop_front(); // Remove timestamps older than 60 seconds
    }
    yawn_frequency = static_cast<double>(yawn_timestamps.size()); // Update frequency based on remaining timestamps
    return yawn_frequency;
}

YawnDetector::YawnMetrics YawnDetector::run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height) {
    YawnMetrics metrics = {};
    auto current_time_point = std::chrono::system_clock::now();
    double current_time_seconds = std::chrono::duration<double>(current_time_point.time_since_epoch()).count();
    auto yawn_start_time_point = std::chrono::system_clock::from_time_t(yawn_start_time); // For duration calc


    // ---<<< ADDED: Yawn Count Reset Logic >>>---
    double elapsed_since_last_reset = current_time_seconds - last_reset_time_seconds;
    if (elapsed_since_last_reset >= RESET_INTERVAL_SECONDS) {
        // printf("INFO: Resetting total yawn count (%.1f seconds elapsed >= %.1f). Old count: %.0f\n",
        //        elapsed_since_last_reset, RESET_INTERVAL_SECONDS, totalYawnCount);
        totalYawnCount = 0; // Reset the count
        last_reset_time_seconds = current_time_seconds; // Update the last reset time
    }
    // ---<<< END Reset Logic >>>---


    if (faceLandmarks.size() > MOUTH_BOTTOM) {
        cv::Point mouth_top = faceLandmarks[MOUTH_TOP];
        cv::Point mouth_bottom = faceLandmarks[MOUTH_BOTTOM];
        double mouth_distance = calculatePixelDistance(mouth_top, mouth_bottom, frame_width, frame_height);

        // --- Yawn Detection State Machine with Hysteresis ---
        if (mouth_distance > YAWN_THRESHOLD) {
            frames_below_threshold = 0;
            if (!is_yawning_now) {
                is_yawning_now = true;
                yawn_start_time = std::chrono::system_clock::to_time_t(current_time_point);
                processing_detected_yawn = false;
                last_yawn_duration = 0.0;
            }
        } else {
            if (is_yawning_now) {
                frames_below_threshold++;
                if (frames_below_threshold >= FRAMES_HYSTERESIS) {
                    std::chrono::duration<double> yawn_duration_chrono = current_time_point - yawn_start_time_point;
                    last_yawn_duration = yawn_duration_chrono.count();
                    double time_since_last_recorded_yawn = current_time_seconds - last_yawn_time;

                    if (last_yawn_duration >= MIN_YAWN_DURATION && !processing_detected_yawn && time_since_last_recorded_yawn > YAWN_COOLDOWN) {
                        // *** Record the valid yawn ***
                        totalYawnCount++;                       // Increment the (potentially reset) total count
                        yawn_timestamps.push_back(current_time_seconds); // Add timestamp for frequency calculation
                        last_yawn_time = current_time_seconds;  // Update time of *last recorded* yawn
                        processing_detected_yawn = true;        // Mark this specific yawn instance as processed
                    } else if (last_yawn_duration < MIN_YAWN_DURATION) {
                        last_yawn_duration = 0.0; // Reset duration display if too short
                    }
                    // End the yawn state
                    is_yawning_now = false;
                    yawn_start_time = 0;
                }
            }
        }
        // Update yawn frequency based on completed yawns in the window
        yawn_frequency = calculateYawnFrequency(current_time_seconds);

    } else {
        // Not enough landmarks, reset yawn state if active
        if (is_yawning_now) {
            is_yawning_now = false;
            yawn_start_time = 0;
            last_yawn_duration = 0.0;
            processing_detected_yawn = false;
            frames_below_threshold = 0;
        }
        // Keep existing frequency and count as is
    }

    // Fill metrics structure
    metrics.isYawning = is_yawning_now;           // Current mouth open state
    metrics.yawnCount = totalYawnCount;           // Total count since last reset
    metrics.yawnFrequency = yawn_frequency;       // Frequency in last 60s
    metrics.yawnDuration = last_yawn_duration;    // Duration of the last completed valid yawn
    metrics.yawnKSS = calculateYawnKSS();         // Calculate KSS based on current 60s frequency

    return metrics;
}