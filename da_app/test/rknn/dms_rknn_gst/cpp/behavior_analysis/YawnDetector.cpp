// #include "YawnDetector.hpp"
// #include <iostream>
// #include <opencv2/imgproc.hpp>
// #include <chrono>
// #include <cmath>
// #include <numeric>
// #include <algorithm> // For std::count_if

// YawnDetector::YawnDetector() :
//     MOUTH_TOP(13),
//     MOUTH_BOTTOM(14),
//     YAWN_THRESHOLD(50.0),
//     MIN_YAWN_DURATION(0.3),   // Minimum duration to be considered a yawn at all
//     YAWN_COOLDOWN(1.0),
//     KSS_TIME_WINDOW_SECONDS(300.0), // 5 minutes for KSS calculation
//     is_yawning_now(false),
//     yawn_start_time(0),
//     last_valid_yawn_end_time(0.0),
//     yawn_kss(0), // Initialize KSS to 0 (or 1 if preferred baseline)
//     last_yawn_duration(0.0),
//     processing_detected_yawn(false),
//     frames_below_threshold(0),
//     FRAMES_HYSTERESIS(3)
// {
//     // No reset time needed now, history deque manages the window
// }

// // ++ Rewritten KSS Calculation ++
// int YawnDetector::calculateYawnKSS(double current_time_seconds) {
//     // 1. Filter history for the last 5 minutes
//     double window_start_time = current_time_seconds - KSS_TIME_WINDOW_SECONDS;
//     // Remove yawns older than the window from the front
//     while (!completed_yawns_history.empty() && completed_yawns_history.front().timestamp < window_start_time) {
//         completed_yawns_history.pop_front();
//     }

//     // 2. Count yawns meeting different duration criteria within the window
//     int count_ge_2_sec = 0;
//     int count_ge_3_sec = 0;
//     int count_ge_4_sec = 0;

//     for (const auto& yawn : completed_yawns_history) {
//         // No need to check timestamp again, deque only contains yawns within the window now
//         if (yawn.duration >= 4.0) {
//             count_ge_4_sec++;
//             count_ge_3_sec++;
//             count_ge_2_sec++;
//         } else if (yawn.duration >= 3.0) {
//             count_ge_3_sec++;
//             count_ge_2_sec++;
//         } else if (yawn.duration >= 2.0) {
//             count_ge_2_sec++;
//         }
//         // Yawns < 2 seconds (but >= MIN_YAWN_DURATION) are ignored for KSS calculation based on rules
//     }

//     // 3. Apply KSS rules (highest matching rule takes precedence)
//     int calculated_kss = 0; // Default to 0 KSS points for yawn component


//     if (count_ge_4_sec >= 5) {
//         calculated_kss = 7; // KSS 7-9 range
//     } else if (count_ge_3_sec >= 3) {
//         calculated_kss = 4; // KSS 4-6 range
//     } else if (count_ge_2_sec >= 3) {
//         calculated_kss = 1; // KSS 1-3 range
//     }


//     // ADDED: Store counts for getters
//     // NEW UI
//     this->count_ge_2_sec_last_calc = count_ge_2_sec;
//     this->count_ge_3_sec_last_calc = count_ge_3_sec;
//     this->count_ge_4_sec_last_calc = count_ge_4_sec;
//     // END ADDED

//     this->yawn_kss = calculated_kss; // Store the calculated KSS
//     return this->yawn_kss;
// }


// double YawnDetector::calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height) {
//     // Frame width/height might not be needed if distance is absolute pixels
//     return std::hypot((double)landmark1.x - landmark2.x, (double)landmark1.y - landmark2.y);
// }

// // Removed calculateYawnFrequency function


// YawnDetector::YawnMetrics YawnDetector::run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height) {
//     YawnMetrics metrics = {};
//     auto current_time_point = std::chrono::system_clock::now();
//     double current_time_seconds = std::chrono::duration<double>(current_time_point.time_since_epoch()).count();
//     auto yawn_start_time_point = std::chrono::system_clock::from_time_t(yawn_start_time); // For duration calc

//     // Reset logic removed - handled by KSS window

//     if (faceLandmarks.size() > MOUTH_BOTTOM) {
//         cv::Point mouth_top = faceLandmarks[MOUTH_TOP];
//         cv::Point mouth_bottom = faceLandmarks[MOUTH_BOTTOM];
//         double mouth_distance = calculatePixelDistance(mouth_top, mouth_bottom, frame_width, frame_height);

//         // --- Yawn Detection State Machine (logic remains similar) ---
//         if (mouth_distance > YAWN_THRESHOLD) {
//             frames_below_threshold = 0;
//             if (!is_yawning_now) {
//                 is_yawning_now = true;
//                 yawn_start_time = std::chrono::system_clock::to_time_t(current_time_point);
//                 processing_detected_yawn = false; // Reset flag for potential new yawn
//                 last_yawn_duration = 0.0; // Reset duration display
//             }
//         } else { // Mouth distance is below threshold
//             if (is_yawning_now) { // Was yawning in the previous frame(s)
//                 frames_below_threshold++;
//                 if (frames_below_threshold >= FRAMES_HYSTERESIS) { // Mouth closed long enough
//                     std::chrono::duration<double> yawn_duration_chrono = current_time_point - yawn_start_time_point;
//                     double current_yawn_duration = yawn_duration_chrono.count();
//                     last_yawn_duration = current_yawn_duration; // Store for display metric

//                     double time_since_last_valid_yawn = current_time_seconds - last_valid_yawn_end_time;

//                     // Check if this completed yawn is valid to be *recorded*
//                     if (current_yawn_duration >= MIN_YAWN_DURATION &&
//                         !processing_detected_yawn &&
//                         time_since_last_valid_yawn > YAWN_COOLDOWN)
//                     {
//                         // *** Record the valid yawn event ***
//                         completed_yawns_history.push_back({current_time_seconds, current_yawn_duration});
//                         last_valid_yawn_end_time = current_time_seconds;  // Update time of last recorded valid yawn
//                         processing_detected_yawn = true; // Mark this specific potential yawn as processed
//                     } else if (current_yawn_duration < MIN_YAWN_DURATION) {
//                         last_yawn_duration = 0.0; // Don't display duration if too short
//                     }

//                     // End the yawn state regardless of validity for recording
//                     is_yawning_now = false;
//                     yawn_start_time = 0;
//                     // processing_detected_yawn remains true until a new yawn starts
//                 }
//             } else {
//                  // Mouth is closed, and wasn't yawning previously. Reset hysteresis counter.
//                  frames_below_threshold = 0;
//                  // processing_detected_yawn flag remains unchanged here.
//             }
//         }

//         // Update KSS based on history every frame
//         metrics.yawnKSS = calculateYawnKSS(current_time_seconds);

//     } else { // Not enough landmarks
//         if (is_yawning_now) { // If a yawn was in progress, end it abruptly
//             is_yawning_now = false;
//             yawn_start_time = 0;
//             last_yawn_duration = 0.0;
//             processing_detected_yawn = false;
//             frames_below_threshold = 0;
//         }
//         // Calculate KSS based on existing history even if face is lost temporarily
//         metrics.yawnKSS = calculateYawnKSS(current_time_seconds);
//     }

//     // Fill metrics structure
//     metrics.isYawning = is_yawning_now;             // Current mouth open state
//     metrics.yawnDuration = last_yawn_duration;      // Duration of the last completed yawn (valid or not)
//     // Calculate the frequency metric based on the same logic as KSS (count >=2s in 5min)
//     metrics.yawnFrequency_5min = 0; // Calculate this based on history if needed for display
//     for(const auto& yawn : completed_yawns_history) {
//         if (yawn.duration >= 2.0) {
//              metrics.yawnFrequency_5min++;
//         }
//     }
//     // metrics.yawnCount remains removed unless you re-add the resettable counter

//     return metrics;
// }




// DYNAMIC CALIBRATION
// File: behavior_analysis/YawnDetector.cpp
#include "YawnDetector.hpp"
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <chrono>
#include <cmath>
#include <numeric>     // For std::accumulate
#include <algorithm>   // For std::sort, std::count_if
#include <vector>      // Needed for sorting in calculate_median

// +++ ADDED: Helper function definition HERE +++
// Helper to calculate median from a deque
double calculate_median(std::deque<double>& data) { // Takes non-const ref as it needs sorting copy
    if (data.empty()) {
        return 0.0; // Return 0 or some default for empty data
    }
    // Copy deque to vector for sorting
    std::vector<double> vec(data.begin(), data.end());
    std::sort(vec.begin(), vec.end());
    size_t n = vec.size();
    if (n % 2 == 0) {
        // Even number of elements: average the middle two
        return (vec[n / 2 - 1] + vec[n / 2]) / 2.0;
    } else {
        // Odd number of elements: return the middle element
        return vec[n / 2];
    }
}
// ++++++++++++++++++++++++++++++++++++++++++++++++

YawnDetector::YawnDetector() :
    MOUTH_TOP(13),
    MOUTH_BOTTOM(14),
    // YAWN_THRESHOLD(50.0), // Removed const initialization here
    MIN_YAWN_DURATION(0.3),
    YAWN_COOLDOWN(1.0),
    KSS_TIME_WINDOW_SECONDS(300.0),
    is_yawning_now(false),
    yawn_start_time(0),
    last_valid_yawn_end_time(0.0),
    yawn_kss(0),
    last_yawn_duration(0.0),
    processing_detected_yawn(false),
    frames_below_threshold(0),
    FRAMES_HYSTERESIS(3),
    personalized_yawn_threshold(50.0) // Initialize with default
{
    // completed_yawns_history initialized implicitly
}

void YawnDetector::setPersonalizedYawnThreshold(double threshold) {
    this->personalized_yawn_threshold = threshold;
    printf("INFO: Personalized Yawn threshold set: %.2f\n", this->personalized_yawn_threshold);
}

int YawnDetector::calculateYawnKSS(double current_time_seconds) {
    // 1. Filter history for the last 5 minutes
    double window_start_time = current_time_seconds - KSS_TIME_WINDOW_SECONDS;
    while (!completed_yawns_history.empty() && completed_yawns_history.front().timestamp < window_start_time) {
        completed_yawns_history.pop_front();
    }

    // 2. Count yawns meeting different duration criteria
    int count_ge_2_sec = 0;
    int count_ge_3_sec = 0;
    int count_ge_4_sec = 0;
    for (const auto& yawn : completed_yawns_history) {
        if (yawn.duration >= 4.0) { count_ge_4_sec++; count_ge_3_sec++; count_ge_2_sec++; }
        else if (yawn.duration >= 3.0) { count_ge_3_sec++; count_ge_2_sec++; }
        else if (yawn.duration >= 2.0) { count_ge_2_sec++; }
    }

    // 3. Apply KSS rules
    int calculated_kss = 0;
    if (count_ge_4_sec >= 5) { calculated_kss = 7; }
    else if (count_ge_3_sec >= 3) { calculated_kss = 4; }
    else if (count_ge_2_sec >= 3) { calculated_kss = 1; }

    // Store counts for getters
    this->count_ge_2_sec_last_calc = count_ge_2_sec;
    this->count_ge_3_sec_last_calc = count_ge_3_sec;
    this->count_ge_4_sec_last_calc = count_ge_4_sec;

    this->yawn_kss = calculated_kss;
    return this->yawn_kss;
}

double YawnDetector::calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height) {
    return std::hypot((double)landmark1.x - landmark2.x, (double)landmark1.y - landmark2.y);
}


YawnDetector::YawnMetrics YawnDetector::run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height) {
    YawnMetrics metrics = {};
    auto current_time_point = std::chrono::system_clock::now();
    double current_time_seconds = std::chrono::duration<double>(current_time_point.time_since_epoch()).count();
    auto yawn_start_time_point = std::chrono::system_clock::from_time_t(yawn_start_time);

    if (faceLandmarks.size() > MOUTH_BOTTOM) {
        cv::Point mouth_top = faceLandmarks[MOUTH_TOP];
        cv::Point mouth_bottom = faceLandmarks[MOUTH_BOTTOM];
        double mouth_distance = calculatePixelDistance(mouth_top, mouth_bottom, frame_width, frame_height);

        if (mouth_distance > this->personalized_yawn_threshold) { // Use personalized threshold
            frames_below_threshold = 0;
            if (!is_yawning_now) {
                is_yawning_now = true;
                yawn_start_time = std::chrono::system_clock::to_time_t(current_time_point);
                processing_detected_yawn = false;
                last_yawn_duration = 0.0;
            }
        } else { // Mouth distance is below threshold
            if (is_yawning_now) {
                frames_below_threshold++;
                if (frames_below_threshold >= FRAMES_HYSTERESIS) {
                    std::chrono::duration<double> yawn_duration_chrono = current_time_point - yawn_start_time_point;
                    double current_yawn_duration = yawn_duration_chrono.count();
                    last_yawn_duration = current_yawn_duration;

                    double time_since_last_valid_yawn = current_time_seconds - last_valid_yawn_end_time;

                    if (current_yawn_duration >= MIN_YAWN_DURATION &&
                        !processing_detected_yawn &&
                        time_since_last_valid_yawn > YAWN_COOLDOWN)
                    {
                        completed_yawns_history.push_back({current_time_seconds, current_yawn_duration});
                        last_valid_yawn_end_time = current_time_seconds;
                        processing_detected_yawn = true;
                    } else if (current_yawn_duration < MIN_YAWN_DURATION) {
                        last_yawn_duration = 0.0;
                    }
                    is_yawning_now = false;
                    yawn_start_time = 0;
                }
            } else {
                 frames_below_threshold = 0;
            }
        }
        metrics.yawnKSS = calculateYawnKSS(current_time_seconds);
    } else { // Not enough landmarks
        if (is_yawning_now) { /* Reset state */ is_yawning_now = false; yawn_start_time = 0; last_yawn_duration = 0.0; processing_detected_yawn = false; frames_below_threshold = 0; }
        metrics.yawnKSS = calculateYawnKSS(current_time_seconds); // Calculate based on history
    }

    // Fill metrics structure
    metrics.isYawning = is_yawning_now;
    metrics.yawnDuration = last_yawn_duration;
    metrics.yawnFrequency_5min = 0; // Recalculate based on updated history
    for(const auto& yawn : completed_yawns_history) { if (yawn.duration >= 2.0) { metrics.yawnFrequency_5min++; } }

    return metrics;
}