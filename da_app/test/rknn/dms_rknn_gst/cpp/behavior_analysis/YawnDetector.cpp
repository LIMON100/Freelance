// #include "YawnDetector.hpp"
// #include <iostream>
// #include <opencv2/imgproc.hpp>
// #include <chrono> 

// YawnDetector::YawnDetector() :
//     MOUTH_TOP(13),
//     MOUTH_BOTTOM(14),
//     YAWN_THRESHOLD(45),
//     MIN_YAWN_DURATION(0.3),
//     YAWN_COOLDOWN(0.1),
//     is_yawn(false),
//     yawn_start_time(0),
//     last_yawn_time(0),
//     yawn_kss(0),
//     yawn_frequency(0),
//     last_yawn_duration(0),
//     reset_interval(60),
//     last_reset_time(0),
//     totalYawnCount(0)  // Initialize total yawn count
// {
//     yawns.push_back(std::map<std::string, double>{{"start", 0}, {"end", 0}, {"duration", 0}});
//     last_reset_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
// }

// int YawnDetector::calculateYawnKSS() {
//     if (yawn_frequency < 3) {
//         yawn_kss = 1;
//     } else if (yawn_frequency < 5) {  // Fixed range overlap
//         yawn_kss = 4;
//     } else if (yawn_frequency < 7) {
//         yawn_kss = 7;
//     } else if (last_yawn_duration > 2 || yawn_frequency > 7) {
//         yawn_kss = 9;
//     }
//     return yawn_kss;
// }

// void YawnDetector::resetKSS() {
//     double current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
//     if (current_time - last_reset_time >= reset_interval) {
//         yawns.clear();
//         yawns.push_back(std::map<std::string, double>{{"start", 0}, {"end", 0}, {"duration", 0}});
//         yawn_kss = 1;
//         last_reset_time = current_time;
//         // Note: totalYawnCount is NOT reset here to keep it cumulative
//     }
// }

// double YawnDetector::calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height) {
//     double x1 = landmark1.x;
//     double y1 = landmark1.y;
//     double x2 = landmark2.x;
//     double y2 = landmark2.y;
//     return std::hypot(x2 - x1, y2 - y1);
// }

// double YawnDetector::calculateYawnFrequency(double current_time) {
//     while (!yawn_timestamps.empty() && current_time - yawn_timestamps.front() > 60) {
//         yawn_timestamps.pop_front();
//     }
//     yawn_frequency = yawn_timestamps.size();
//     return yawn_frequency;
// }

// // YawnDetector::YawnMetrics YawnDetector::run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height) {
// //     YawnMetrics metrics;
// //     auto current_time = std::chrono::system_clock::now();

// //     if (faceLandmarks.size() > 14) {  // Ensure enough landmarks
// //         cv::Point mouth_top = faceLandmarks[MOUTH_TOP];
// //         cv::Point mouth_bottom = faceLandmarks[MOUTH_BOTTOM];

// //         double mouth_distance = calculatePixelDistance(mouth_top, mouth_bottom, frame_width, frame_height);
// //         std::cout << "DEBUG YawnDetector: mouth_distance =............................................... " << mouth_distance << std::endl;
// //         if (mouth_distance > YAWN_THRESHOLD) {
// //             if (yawn_start_time == 0) {
// //                 yawn_start_time = std::chrono::system_clock::to_time_t(current_time);
// //                 is_yawn = true;
// //             }
// //         } else {
// //             if (yawn_start_time != 0) {
// //                 auto end_time = current_time;
// //                 std::chrono::duration<double> yawn_duration = end_time - std::chrono::system_clock::from_time_t(yawn_start_time);
// //                 last_yawn_duration = yawn_duration.count();

// //                 if (last_yawn_duration >= MIN_YAWN_DURATION) {
// //                     if ((std::chrono::system_clock::to_time_t(current_time) - last_yawn_time) > YAWN_COOLDOWN) {
// //                         std::map<std::string, double> yawn_data;
// //                         yawn_data["start"] = yawn_start_time;
// //                         yawn_data["end"] = std::chrono::system_clock::to_time_t(current_time);
// //                         yawn_data["duration"] = last_yawn_duration;
// //                         yawns.push_back(yawn_data);
// //                         yawn_timestamps.push_back(std::chrono::system_clock::to_time_t(current_time));
// //                         last_yawn_time = std::chrono::system_clock::to_time_t(current_time);
// //                         totalYawnCount++;  // Increment total yawn count
// //                     }
// //                 }
// //                 yawn_start_time = 0;
// //                 is_yawn = false;
// //             }
// //         }
// //         yawn_frequency = calculateYawnFrequency(std::chrono::system_clock::to_time_t(current_time));
// //     }

// //     int yawn_kss_ = calculateYawnKSS();
// //     metrics.isYawning = is_yawn;
// //     metrics.yawnCount = totalYawnCount;  // Total yawns since start
// //     metrics.yawnFrequency = yawn_frequency;  // Yawns in last 60 seconds
// //     metrics.yawnDuration = last_yawn_duration;
// //     metrics.yawnKSS = yawn_kss_;
// //     return metrics;
// // }

// YawnDetector::YawnMetrics YawnDetector::run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height) {
//     YawnMetrics metrics;
//     auto current_time = std::chrono::system_clock::now();
//     double current_time_sec = std::chrono::system_clock::to_time_t(current_time);

//     if (faceLandmarks.size() > 14) {  // Ensure enough landmarks
//         cv::Point mouth_top = faceLandmarks[MOUTH_TOP];
//         cv::Point mouth_bottom = faceLandmarks[MOUTH_BOTTOM];

//         double mouth_distance = calculatePixelDistance(mouth_top, mouth_bottom, frame_width, frame_height);

//         if (mouth_distance > YAWN_THRESHOLD) {
//             if (yawn_start_time == 0 && (current_time_sec - last_yawn_time) > YAWN_COOLDOWN) {
//                 // Start of a new yawn, but only if we're past the cooldown
//                 yawn_start_time = current_time_sec;
//                 is_yawn = true;
//             }
//             // Do not count here; wait until yawn ends
//         } else {
//             if (yawn_start_time != 0) {
//                 // End of a yawn
//                 auto end_time = current_time;
//                 std::chrono::duration<double> yawn_duration = end_time - std::chrono::system_clock::from_time_t(yawn_start_time);
//                 last_yawn_duration = yawn_duration.count();

//                 if (last_yawn_duration >= MIN_YAWN_DURATION) {
//                     // Valid yawn detected; check cooldown
//                     if ((current_time_sec - last_yawn_time) > YAWN_COOLDOWN) {
//                         // Record the yawn as a single event
//                         std::map<std::string, double> yawn_data;
//                         yawn_data["start"] = yawn_start_time;
//                         yawn_data["end"] = current_time_sec;
//                         yawn_data["duration"] = last_yawn_duration;
//                         yawns.push_back(yawn_data);
//                         yawn_timestamps.push_back(current_time_sec);  // Add timestamp only once per yawn
//                         last_yawn_time = current_time_sec;
//                         totalYawnCount++;  // Increment only once per valid yawn
//                     }
//                 }
//                 // Reset yawn state
//                 yawn_start_time = 0;
//                 is_yawn = false;
//             }
//         }

//         // Update yawn frequency (yawns in last 60 seconds)
//         yawn_frequency = calculateYawnFrequency(current_time_sec);
//     }

//     // Reset KSS if needed
//     resetKSS();

//     // Calculate KSS based on updated frequency and duration
//     int yawn_kss_ = calculateYawnKSS();
//     metrics.isYawning = is_yawn;
//     metrics.yawnCount = totalYawnCount;  // Total yawns since start
//     metrics.yawnFrequency = yawn_frequency;  // Yawns in last 60 seconds
//     metrics.yawnDuration = last_yawn_duration;
//     metrics.yawnKSS = yawn_kss_;

//     return metrics;
// }




#include "YawnDetector.hpp"
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <chrono>
#include <cmath>     // Include for std::hypot, std::max
#include <numeric>   // Include for std::accumulate

YawnDetector::YawnDetector() :
    MOUTH_TOP(13),
    MOUTH_BOTTOM(14),
    YAWN_THRESHOLD(45.0), // Make double for consistency
    MIN_YAWN_DURATION(0.3),
    YAWN_COOLDOWN(1.0),   // Increase cooldown slightly? (e.g., 1 second)
    is_yawning_now(false),
    yawn_start_time(0),
    last_yawn_time(0.0), // Use double for time in seconds
    yawn_kss(0),
    yawn_frequency(0.0),
    last_yawn_duration(0.0),
    reset_interval(60.0),
    last_reset_time(0.0),
    totalYawnCount(0),
    processing_detected_yawn(false),
    // New Hysteresis Variable:
    frames_below_threshold(0),
    FRAMES_HYSTERESIS(3) // Require 3 consecutive frames below threshold to confirm closure
{
    last_reset_time = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
}

int YawnDetector::calculateYawnKSS() {
    double current_freq = std::max(0.0, yawn_frequency);
    if (current_freq < 3) yawn_kss = 1;
    else if (current_freq < 5) yawn_kss = 4;
    else if (current_freq < 7) yawn_kss = 7;
    else yawn_kss = 9;
    return yawn_kss;
}

void YawnDetector::resetKSS() {
    // Not implemented fully based on previous logic, placeholder
}

double YawnDetector::calculatePixelDistance(cv::Point landmark1, cv::Point landmark2, int frame_width, int frame_height) {
    // No change needed here
    return std::hypot((double)landmark1.x - landmark2.x, (double)landmark1.y - landmark2.y);
}

double YawnDetector::calculateYawnFrequency(double current_time_seconds) {
    while (!yawn_timestamps.empty() && current_time_seconds - yawn_timestamps.front() > 60.0) {
        yawn_timestamps.pop_front();
    }
    yawn_frequency = static_cast<double>(yawn_timestamps.size());
    return yawn_frequency;
}

YawnDetector::YawnMetrics YawnDetector::run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height) {
    YawnMetrics metrics = {};
    auto current_time_point = std::chrono::system_clock::now();
    // Use double for time comparisons consistently
    double current_time_seconds = std::chrono::duration<double>(current_time_point.time_since_epoch()).count();
    // Convert start time for duration calculation if needed
    auto yawn_start_time_point = std::chrono::system_clock::from_time_t(yawn_start_time);

    if (faceLandmarks.size() > MOUTH_BOTTOM) {
        cv::Point mouth_top = faceLandmarks[MOUTH_TOP];
        cv::Point mouth_bottom = faceLandmarks[MOUTH_BOTTOM];
        double mouth_distance = calculatePixelDistance(mouth_top, mouth_bottom, frame_width, frame_height);

        // --- Yawn Detection State Machine with Hysteresis ---
        if (mouth_distance > YAWN_THRESHOLD) {
            // Mouth is open wide
             frames_below_threshold = 0; // Reset counter when mouth opens wide again
            if (!is_yawning_now) {
                // Transition: Closed -> Open
                is_yawning_now = true;
                yawn_start_time = std::chrono::system_clock::to_time_t(current_time_point); // Record start time_t
                 processing_detected_yawn = false; // Ready to process the *end* of this yawn later
                last_yawn_duration = 0.0; // Reset duration for the new yawn
                // printf("DEBUG: Yawn Start Detected. Time: %.2f\n", current_time_seconds);
            }
             // else: Still Open state, do nothing

        } else {
            // Mouth is not open wide (<= threshold)
            if (is_yawning_now) {
                // Potential Transition: Open -> Closed (or still closing)
                frames_below_threshold++; // Increment frames counter while below threshold

                // printf("DEBUG: Mouth Below Threshold. Frames Below: %d\n", frames_below_threshold);


                // Check if mouth has been consistently below threshold long enough
                if (frames_below_threshold >= FRAMES_HYSTERESIS) {
                     // Confirmed end of the yawn state
                     std::chrono::duration<double> yawn_duration_chrono = current_time_point - yawn_start_time_point;
                     last_yawn_duration = yawn_duration_chrono.count();

                     // Check if the yawn was valid (duration and cooldown) AND not already processed
                     double time_since_last_recorded_yawn = current_time_seconds - last_yawn_time;

                     // Print values for debugging
                    //  printf("DEBUG: Yawn End Check. Duration=%.2fs (Min=%.1f), Processed=%s, Cooldown=%.2fs (Min=%.1f)\n",
                    //         last_yawn_duration, MIN_YAWN_DURATION, processing_detected_yawn ? "TRUE" : "FALSE",
                    //         time_since_last_recorded_yawn, YAWN_COOLDOWN);

                     if (last_yawn_duration >= MIN_YAWN_DURATION && !processing_detected_yawn && time_since_last_recorded_yawn > YAWN_COOLDOWN) {
                         // *** Record the valid yawn ***
                        //  printf("DEBUG: *** RECORDING YAWN *** Timestamp=%.2f, Freq Deque Size BEFORE add: %zu\n",
                        //         current_time_seconds, yawn_timestamps.size());
                         totalYawnCount++;
                         yawn_timestamps.push_back(current_time_seconds);
                         last_yawn_time = current_time_seconds; // Update time of *last recorded* yawn
                         processing_detected_yawn = true; // Mark this yawn event as processed
                     } else if (last_yawn_duration < MIN_YAWN_DURATION) {
                         // Reset duration if it wasn't long enough
                         last_yawn_duration = 0.0;
                     }
                     // else: Already processed or cooldown not met

                     // Regardless of processing, end the yawn state as mouth is confirmed closed
                     is_yawning_now = false;
                     yawn_start_time = 0;
                     // frames_below_threshold is reset naturally when mouth opens again

                }
                // else: Mouth is below threshold, but not for long enough yet - still closing. Do nothing else.

            }
            // else: Mouth is closed and we weren't yawning. Do nothing.
        }

        yawn_frequency = calculateYawnFrequency(current_time_seconds);
        // printf("DEBUG: Calculated Freq=%.1f (Deque Size=%zu)\n", yawn_frequency, yawn_timestamps.size());


    } else {
        // Not enough landmarks, reset state
        if (is_yawning_now) {
            is_yawning_now = false;
            yawn_start_time = 0;
            last_yawn_duration = 0.0;
            processing_detected_yawn = false;
            frames_below_threshold = 0;
        }
    }

    metrics.isYawning = is_yawning_now;
    metrics.yawnCount = totalYawnCount;
    metrics.yawnFrequency = yawn_frequency;
    metrics.yawnDuration = last_yawn_duration;
    metrics.yawnKSS = calculateYawnKSS(); // Calculate based on latest frequency

    return metrics;
}