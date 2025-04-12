// #include "YawnDetector.hpp"
// #include <iostream>
// #include <opencv2/imgproc.hpp>
// #include <chrono>
// #include <cmath>     // Include for std::hypot, std::max
// #include <numeric>   // Include for std::accumulate

// YawnDetector::YawnDetector() :
//     MOUTH_TOP(13),
//     MOUTH_BOTTOM(14),
//     YAWN_THRESHOLD(50.0),
//     MIN_YAWN_DURATION(0.3),
//     YAWN_COOLDOWN(1.0),
//     RESET_INTERVAL_SECONDS(300.0),
//     is_yawning_now(false),
//     yawn_start_time(0),
//     last_yawn_time(0.0),
//     yawn_kss(0), // Initialize KSS
//     yawn_frequency(0.0),
//     last_yawn_duration(0.0),
//     totalYawnCount(0), // Initialize total count
//     processing_detected_yawn(false),
//     frames_below_threshold(0),
//     FRAMES_HYSTERESIS(3),
//     last_reset_time_seconds(0.0) // <<<--- Will be initialized properly below
// {
//     // Initialize last_reset_time properly using current time
//     last_reset_time_seconds = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
//     // printf("YawnDetector Initialized. Initial Reset Time: %.2f\n", last_reset_time_seconds);
// }

// int YawnDetector::calculateYawnKSS() {
//     // KSS based on frequency in the last 60 seconds
//     double current_freq = std::max(0.0, yawn_frequency);
//     if (current_freq < 3) yawn_kss = 1;
//     else if (current_freq < 5) yawn_kss = 4;
//     else if (current_freq < 7) yawn_kss = 7;
//     else yawn_kss = 9; // >= 7 yawns/min is high
//     return yawn_kss;
// }

// double YawnDetector::calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height) {
//     return std::hypot((double)landmark1.x - landmark2.x, (double)landmark1.y - landmark2.y);
// }

// double YawnDetector::calculateYawnFrequency(double current_time_seconds) {
//     // Calculate frequency based on *completed* yawns in the last 60 seconds
//     while (!yawn_timestamps.empty() && current_time_seconds - yawn_timestamps.front() > 60.0) {
//         yawn_timestamps.pop_front(); // Remove timestamps older than 60 seconds
//     }
//     yawn_frequency = static_cast<double>(yawn_timestamps.size()); // Update frequency based on remaining timestamps
//     return yawn_frequency;
// }

// YawnDetector::YawnMetrics YawnDetector::run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height) {
//     YawnMetrics metrics = {};
//     auto current_time_point = std::chrono::system_clock::now();
//     double current_time_seconds = std::chrono::duration<double>(current_time_point.time_since_epoch()).count();
//     auto yawn_start_time_point = std::chrono::system_clock::from_time_t(yawn_start_time); // For duration calc


//     // ---<<< ADDED: Yawn Count Reset Logic >>>---
//     double elapsed_since_last_reset = current_time_seconds - last_reset_time_seconds;
//     if (elapsed_since_last_reset >= RESET_INTERVAL_SECONDS) {
//         // printf("INFO: Resetting total yawn count (%.1f seconds elapsed >= %.1f). Old count: %.0f\n",
//         //        elapsed_since_last_reset, RESET_INTERVAL_SECONDS, totalYawnCount);
//         totalYawnCount = 0; // Reset the count
//         last_reset_time_seconds = current_time_seconds; // Update the last reset time
//     }
//     // ---<<< END Reset Logic >>>---


//     if (faceLandmarks.size() > MOUTH_BOTTOM) {
//         cv::Point mouth_top = faceLandmarks[MOUTH_TOP];
//         cv::Point mouth_bottom = faceLandmarks[MOUTH_BOTTOM];
//         double mouth_distance = calculatePixelDistance(mouth_top, mouth_bottom, frame_width, frame_height);

//         // --- Yawn Detection State Machine with Hysteresis ---
//         if (mouth_distance > YAWN_THRESHOLD) {
//             frames_below_threshold = 0;
//             if (!is_yawning_now) {
//                 is_yawning_now = true;
//                 yawn_start_time = std::chrono::system_clock::to_time_t(current_time_point);
//                 processing_detected_yawn = false;
//                 last_yawn_duration = 0.0;
//             }
//         } else {
//             if (is_yawning_now) {
//                 frames_below_threshold++;
//                 if (frames_below_threshold >= FRAMES_HYSTERESIS) {
//                     std::chrono::duration<double> yawn_duration_chrono = current_time_point - yawn_start_time_point;
//                     last_yawn_duration = yawn_duration_chrono.count();
//                     double time_since_last_recorded_yawn = current_time_seconds - last_yawn_time;

//                     if (last_yawn_duration >= MIN_YAWN_DURATION && !processing_detected_yawn && time_since_last_recorded_yawn > YAWN_COOLDOWN) {
//                         // *** Record the valid yawn ***
//                         totalYawnCount++;                       // Increment the (potentially reset) total count
//                         yawn_timestamps.push_back(current_time_seconds); // Add timestamp for frequency calculation
//                         last_yawn_time = current_time_seconds;  // Update time of *last recorded* yawn
//                         processing_detected_yawn = true;        // Mark this specific yawn instance as processed
//                     } else if (last_yawn_duration < MIN_YAWN_DURATION) {
//                         last_yawn_duration = 0.0; // Reset duration display if too short
//                     }
//                     // End the yawn state
//                     is_yawning_now = false;
//                     yawn_start_time = 0;
//                 }
//             }
//         }
//         // Update yawn frequency based on completed yawns in the window
//         yawn_frequency = calculateYawnFrequency(current_time_seconds);

//     } else {
//         // Not enough landmarks, reset yawn state if active
//         if (is_yawning_now) {
//             is_yawning_now = false;
//             yawn_start_time = 0;
//             last_yawn_duration = 0.0;
//             processing_detected_yawn = false;
//             frames_below_threshold = 0;
//         }
//         // Keep existing frequency and count as is
//     }

//     // Fill metrics structure
//     metrics.isYawning = is_yawning_now;           // Current mouth open state
//     metrics.yawnCount = totalYawnCount;           // Total count since last reset
//     metrics.yawnFrequency = yawn_frequency;       // Frequency in last 60s
//     metrics.yawnDuration = last_yawn_duration;    // Duration of the last completed valid yawn
//     metrics.yawnKSS = calculateYawnKSS();         // Calculate KSS based on current 60s frequency

//     return metrics;
// }


#include "YawnDetector.hpp"
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <chrono>
#include <cmath>
#include <numeric>
#include <algorithm> // For std::count_if

YawnDetector::YawnDetector() :
    MOUTH_TOP(13),
    MOUTH_BOTTOM(14),
    YAWN_THRESHOLD(50.0),
    MIN_YAWN_DURATION(0.3),   // Minimum duration to be considered a yawn at all
    YAWN_COOLDOWN(1.0),
    KSS_TIME_WINDOW_SECONDS(300.0), // 5 minutes for KSS calculation
    is_yawning_now(false),
    yawn_start_time(0),
    last_valid_yawn_end_time(0.0),
    yawn_kss(0), // Initialize KSS to 0 (or 1 if preferred baseline)
    last_yawn_duration(0.0),
    processing_detected_yawn(false),
    frames_below_threshold(0),
    FRAMES_HYSTERESIS(3)
{
    // No reset time needed now, history deque manages the window
}

// ++ Rewritten KSS Calculation ++
int YawnDetector::calculateYawnKSS(double current_time_seconds) {
    // 1. Filter history for the last 5 minutes
    double window_start_time = current_time_seconds - KSS_TIME_WINDOW_SECONDS;
    // Remove yawns older than the window from the front
    while (!completed_yawns_history.empty() && completed_yawns_history.front().timestamp < window_start_time) {
        completed_yawns_history.pop_front();
    }

    // 2. Count yawns meeting different duration criteria within the window
    int count_ge_2_sec = 0;
    int count_ge_3_sec = 0;
    int count_ge_4_sec = 0;

    for (const auto& yawn : completed_yawns_history) {
        // No need to check timestamp again, deque only contains yawns within the window now
        if (yawn.duration >= 4.0) {
            count_ge_4_sec++;
            count_ge_3_sec++;
            count_ge_2_sec++;
        } else if (yawn.duration >= 3.0) {
            count_ge_3_sec++;
            count_ge_2_sec++;
        } else if (yawn.duration >= 2.0) {
            count_ge_2_sec++;
        }
        // Yawns < 2 seconds (but >= MIN_YAWN_DURATION) are ignored for KSS calculation based on rules
    }

    // 3. Apply KSS rules (highest matching rule takes precedence)
    int calculated_kss = 0; // Default to 0 KSS points for yawn component

    if (count_ge_4_sec >= 5) {
        calculated_kss = 7; // KSS 7-9 range
    } else if (count_ge_3_sec >= 3) {
        calculated_kss = 4; // KSS 4-6 range
    } else if (count_ge_2_sec >= 3) {
        calculated_kss = 1; // KSS 1-3 range
    }

    // --- Debug Print (Optional) ---
    // printf("Yawn KSS Check: Time=%.1f, WindowStart=%.1f, YawnsInWindow=%zu, Cnt>=2s=%d, Cnt>=3s=%d, Cnt>=4s=%d -> KSS=%d\n",
    //        current_time_seconds, window_start_time, completed_yawns_history.size(),
    //        count_ge_2_sec, count_ge_3_sec, count_ge_4_sec, calculated_kss);
    // --------

    this->yawn_kss = calculated_kss; // Store the calculated KSS
    return this->yawn_kss;
}


double YawnDetector::calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height) {
    // Frame width/height might not be needed if distance is absolute pixels
    return std::hypot((double)landmark1.x - landmark2.x, (double)landmark1.y - landmark2.y);
}

// Removed calculateYawnFrequency function


YawnDetector::YawnMetrics YawnDetector::run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height) {
    YawnMetrics metrics = {};
    auto current_time_point = std::chrono::system_clock::now();
    double current_time_seconds = std::chrono::duration<double>(current_time_point.time_since_epoch()).count();
    auto yawn_start_time_point = std::chrono::system_clock::from_time_t(yawn_start_time); // For duration calc

    // Reset logic removed - handled by KSS window

    if (faceLandmarks.size() > MOUTH_BOTTOM) {
        cv::Point mouth_top = faceLandmarks[MOUTH_TOP];
        cv::Point mouth_bottom = faceLandmarks[MOUTH_BOTTOM];
        double mouth_distance = calculatePixelDistance(mouth_top, mouth_bottom, frame_width, frame_height);

        // --- Yawn Detection State Machine (logic remains similar) ---
        if (mouth_distance > YAWN_THRESHOLD) {
            frames_below_threshold = 0;
            if (!is_yawning_now) {
                is_yawning_now = true;
                yawn_start_time = std::chrono::system_clock::to_time_t(current_time_point);
                processing_detected_yawn = false; // Reset flag for potential new yawn
                last_yawn_duration = 0.0; // Reset duration display
            }
        } else { // Mouth distance is below threshold
            if (is_yawning_now) { // Was yawning in the previous frame(s)
                frames_below_threshold++;
                if (frames_below_threshold >= FRAMES_HYSTERESIS) { // Mouth closed long enough
                    std::chrono::duration<double> yawn_duration_chrono = current_time_point - yawn_start_time_point;
                    double current_yawn_duration = yawn_duration_chrono.count();
                    last_yawn_duration = current_yawn_duration; // Store for display metric

                    double time_since_last_valid_yawn = current_time_seconds - last_valid_yawn_end_time;

                    // Check if this completed yawn is valid to be *recorded*
                    if (current_yawn_duration >= MIN_YAWN_DURATION &&
                        !processing_detected_yawn &&
                        time_since_last_valid_yawn > YAWN_COOLDOWN)
                    {
                        // *** Record the valid yawn event ***
                        completed_yawns_history.push_back({current_time_seconds, current_yawn_duration});
                        last_valid_yawn_end_time = current_time_seconds;  // Update time of last recorded valid yawn
                        processing_detected_yawn = true; // Mark this specific potential yawn as processed
                    } else if (current_yawn_duration < MIN_YAWN_DURATION) {
                        last_yawn_duration = 0.0; // Don't display duration if too short
                    }

                    // End the yawn state regardless of validity for recording
                    is_yawning_now = false;
                    yawn_start_time = 0;
                    // processing_detected_yawn remains true until a new yawn starts
                }
            } else {
                 // Mouth is closed, and wasn't yawning previously. Reset hysteresis counter.
                 frames_below_threshold = 0;
                 // processing_detected_yawn flag remains unchanged here.
            }
        }

        // Update KSS based on history every frame
        metrics.yawnKSS = calculateYawnKSS(current_time_seconds);

    } else { // Not enough landmarks
        if (is_yawning_now) { // If a yawn was in progress, end it abruptly
            is_yawning_now = false;
            yawn_start_time = 0;
            last_yawn_duration = 0.0;
            processing_detected_yawn = false;
            frames_below_threshold = 0;
        }
        // Calculate KSS based on existing history even if face is lost temporarily
        metrics.yawnKSS = calculateYawnKSS(current_time_seconds);
    }

    // Fill metrics structure
    metrics.isYawning = is_yawning_now;             // Current mouth open state
    metrics.yawnDuration = last_yawn_duration;      // Duration of the last completed yawn (valid or not)
    // Calculate the frequency metric based on the same logic as KSS (count >=2s in 5min)
    metrics.yawnFrequency_5min = 0; // Calculate this based on history if needed for display
    for(const auto& yawn : completed_yawns_history) {
        if (yawn.duration >= 2.0) {
             metrics.yawnFrequency_5min++;
        }
    }
    // metrics.yawnCount remains removed unless you re-add the resettable counter

    return metrics;
}