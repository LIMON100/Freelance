// #include "BlinkDetector.hpp"
// #include <iostream>
// #include <opencv2/imgproc.hpp>
// #include <numeric>
// #include <chrono>

// float my::BlinkDetector::smoothEAR(std::deque<float>& earHistory, float newEAR) {
//     // (Implementation is the same)
//     earHistory.push_back(newEAR);
//     if (earHistory.size() > 5) { earHistory.pop_front(); }
//     if (earHistory.empty()) { return newEAR; }
//     float sum = std::accumulate(earHistory.begin(), earHistory.end(), 0.0f);
//     return sum / earHistory.size();
// }

// void my::BlinkDetector::run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height){
//     int h = frame_height;
//     int w = frame_width;

//     if (faceLandmarks.empty()) {
//         isLeftEyeClosedFlag = false;
//         isRightEyeClosedFlag = false;
//         wereEyesClosedPreviously = false;
//         return;
//     }

//     leftEAR = calculateEAR(faceLandmarks, LEFT_EYE_POINTS, w, h);
//     rightEAR = calculateEAR(faceLandmarks, RIGHT_EYE_POINTS, w, h);

//     leftEAR = smoothEAR(leftEARHistory, leftEAR);
//     rightEAR = smoothEAR(rightEARHistory, rightEAR);

//     // *** MODIFIED: Use separate thresholds for determining closure state ***
//     isLeftEyeClosedFlag = isEyeClosed(leftEAR, left_ear_threshold);
//     isRightEyeClosedFlag = isEyeClosed(rightEAR, right_ear_threshold);

//     // Determine combined eye closure state using OR logic
//     // bool areEyesClosedNow = isLeftEyeClosedFlag || isRightEyeClosedFlag;
//     bool areEyesClosedNow = isLeftEyeClosedFlag && isRightEyeClosedFlag;

//     // --- Blink Detection Logic (using combined state - remains the same) ---
//     if (areEyesClosedNow) {
//         if (!wereEyesClosedPreviously) {
//             eyesClosedStartTime = std::chrono::high_resolution_clock::now();
//             wereEyesClosedPreviously = true;
//         }
//     } else {
//         if (wereEyesClosedPreviously) {
//             auto now = std::chrono::high_resolution_clock::now();
//             double closureDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - eyesClosedStartTime).count() / 1000.0;
//             blink_counter++;
//             blinks_in_window++;
//             blink_durations.push_back(closureDuration);
//             // Calculate Blink Rate (remains the same logic)
//             double current_time_sec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//             if (last_blink_time != 0) {
//                 double time_since_last_blink = current_time_sec - last_blink_time;
//                 if (time_since_last_blink > 0) blink_rate = 60.0 / time_since_last_blink; else blink_rate = 0.0;
//             }
//             last_blink_time = current_time_sec;
//             wereEyesClosedPreviously = false;
//         }
//     }

//     // --- PERCLOS Calculation Window (remains the same logic) ---
//     double current_time_sec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//     if (current_time_sec - perclos_start_time >= 60.0) {
//         double total_closure_duration_in_window = std::accumulate(blink_durations.begin(), blink_durations.end(), 0.0);
//         double perclos = (total_closure_duration_in_window / 60.0) * 100.0;
//         perclos_durations.push_back(perclos);
//         perclos_start_time = current_time_sec;
//         blink_durations.clear();
//         blinks_in_window = 0;
//     }
//     // isLeftEyeClosedFlag and isRightEyeClosedFlag were updated earlier
// }

// // calculateEAR remains the same
// float my::BlinkDetector::calculateEAR(const std::vector<cv::Point>& landmarks, const std::vector<int>& eye_points, int w, int h) {
//     // (Implementation is the same)
//     if (landmarks.size() <= *std::max_element(eye_points.begin(), eye_points.end())) { std::cerr << "Error in calculateEAR: Not enough landmarks (" << landmarks.size() << ") for max index " << *std::max_element(eye_points.begin(), eye_points.end()) << std::endl; return 1.0f; } try { cv::Point p1=landmarks[eye_points[0]], p2=landmarks[eye_points[1]], p3=landmarks[eye_points[2]], p4=landmarks[eye_points[3]], p5=landmarks[eye_points[4]], p6=landmarks[eye_points[5]]; double v1=cv::norm(p2-p6), v2=cv::norm(p3-p5), horz=cv::norm(p1-p4); if(horz<1e-6) return 1.0f; return static_cast<float>((v1+v2)/(2.0*horz)); } catch (const std::exception& e) { std::cerr << "Exception in calculateEAR: " << e.what() << std::endl; return 1.0f; }
// }

// // isEyeClosed implementation remains the same (takes threshold as argument)
// bool my::BlinkDetector::isEyeClosed(float ear, float threshold) {
//     return ear < threshold;
// }

// // Accessor functions remain the same
// bool my::BlinkDetector::isLeftEyeClosed() const{ return isLeftEyeClosedFlag; }
// bool my::BlinkDetector::isRightEyeClosed() const { return isRightEyeClosedFlag; }
// float my::BlinkDetector::getLeftEARValue() const{ return leftEAR; }
// float my::BlinkDetector::getRightEARValue() const{ return rightEAR; }












// DYNAMIC CALIBRATION

// File: behavior_analysis/BlinkDetector.cpp
#include "BlinkDetector.hpp"
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <numeric>
#include <chrono>

// --- smoothEAR implementation remains the same ---
float my::BlinkDetector::smoothEAR(std::deque<float>& earHistory, float newEAR) {
    earHistory.push_back(newEAR);
    if (earHistory.size() > 5) { earHistory.pop_front(); }
    if (earHistory.empty()) { return newEAR; }
    float sum = std::accumulate(earHistory.begin(), earHistory.end(), 0.0f);
    return sum / earHistory.size();
}

// +++ ADDED: Implementation of the setter +++
void my::BlinkDetector::setPersonalizedEarThresholds(float left_thresh, float right_thresh) {
    this->personalized_left_ear_threshold = left_thresh;
    this->personalized_right_ear_threshold = right_thresh;
    printf("INFO: Personalized EAR thresholds set: L=%.3f, R=%.3f\n",
           this->personalized_left_ear_threshold, this->personalized_right_ear_threshold);
}
// +++++++++++++++++++++++++++++++++++++++++++++

void my::BlinkDetector::run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height){
    int h = frame_height;
    int w = frame_width;

    if (faceLandmarks.empty()) {
        isLeftEyeClosedFlag = false;
        isRightEyeClosedFlag = false;
        wereEyesClosedPreviously = false;
        return;
    }

    leftEAR = calculateEAR(faceLandmarks, LEFT_EYE_POINTS, w, h);
    rightEAR = calculateEAR(faceLandmarks, RIGHT_EYE_POINTS, w, h);

    leftEAR = smoothEAR(leftEARHistory, leftEAR);
    rightEAR = smoothEAR(rightEARHistory, rightEAR);

    // *** MODIFIED: Use personalized member thresholds ***
    isLeftEyeClosedFlag = isEyeClosed(leftEAR, this->personalized_left_ear_threshold);
    isRightEyeClosedFlag = isEyeClosed(rightEAR, this->personalized_right_ear_threshold);

    // Determine combined eye closure state using AND logic (both must be closed for PERCLOS/blink count)
    bool areEyesClosedNow = isLeftEyeClosedFlag && isRightEyeClosedFlag; // Changed from OR

    // --- Blink Detection Logic (using combined state - remains the same) ---
    if (areEyesClosedNow) {
        if (!wereEyesClosedPreviously) {
            eyesClosedStartTime = std::chrono::high_resolution_clock::now();
            wereEyesClosedPreviously = true;
        }
    } else {
        if (wereEyesClosedPreviously) {
            auto now = std::chrono::high_resolution_clock::now();
            double closureDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - eyesClosedStartTime).count() / 1000.0;
            // Only count as a blink if duration is reasonable (e.g., > 0.05s and < 1.0s) - TUNE THIS
            if (closureDuration > 0.05 && closureDuration < 1.0) {
                blink_counter++;
                blinks_in_window++;
                blink_durations.push_back(closureDuration); // Store duration for PERCLOS

                // Calculate Blink Rate (remains the same logic)
                double current_time_sec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                if (last_blink_time != 0) {
                    double time_since_last_blink = current_time_sec - last_blink_time;
                    if (time_since_last_blink > 0) blink_rate = 60.0 / time_since_last_blink; else blink_rate = 0.0;
                }
                last_blink_time = current_time_sec;
            }
            // Reset regardless of whether it was counted as a valid blink duration
            wereEyesClosedPreviously = false;
        }
    }

    // --- PERCLOS Calculation Window (remains the same logic) ---
    double current_time_sec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (current_time_sec - perclos_start_time >= 60.0) {
        double total_closure_duration_in_window = 0;
        // If eyes are currently closed, add the time elapsed since they closed
        if (wereEyesClosedPreviously) {
            total_closure_duration_in_window += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - eyesClosedStartTime).count() / 1000.0;
        }
        // Add durations of completed blinks within the window
        total_closure_duration_in_window += std::accumulate(blink_durations.begin(), blink_durations.end(), 0.0);

        double perclos = (total_closure_duration_in_window / 60.0) * 100.0;
        perclos = std::min(perclos, 100.0); // Cap PERCLOS at 100%
        perclos_durations.push_back(perclos);

        // Reset for next window
        perclos_start_time = current_time_sec;
        blink_durations.clear();
        blinks_in_window = 0;
        // If eyes were closed when window reset, restart the timer for the current closure
        if (wereEyesClosedPreviously) {
            eyesClosedStartTime = std::chrono::high_resolution_clock::now();
        }
    }
}

// --- calculateEAR remains the same ---
float my::BlinkDetector::calculateEAR(const std::vector<cv::Point>& landmarks, const std::vector<int>& eye_points, int w, int h) {
    if (landmarks.empty()) return 1.0f; int max_idx = 0; for(int idx : eye_points) { if (idx > max_idx) max_idx = idx; } if (max_idx >= landmarks.size()) { return 1.0f; } try { cv::Point p1=landmarks.at(eye_points.at(0)); cv::Point p2=landmarks.at(eye_points.at(1)); cv::Point p3=landmarks.at(eye_points.at(2)); cv::Point p4=landmarks.at(eye_points.at(3)); cv::Point p5=landmarks.at(eye_points.at(4)); cv::Point p6=landmarks.at(eye_points.at(5)); double v1=cv::norm(p2-p6); double v2=cv::norm(p3-p5); double h=cv::norm(p1-p4); if(h<1e-6) return 1.0f; return static_cast<float>((v1+v2)/(2.0*h)); } catch(const std::out_of_range& oor) { std::cerr << "Out of Range error in calculateEAR: " << oor.what() << std::endl; return 1.0f; } catch(...) { std::cerr << "Unknown exception in calculateEAR" << std::endl; return 1.0f; }
}

// --- isEyeClosed implementation remains the same ---
bool my::BlinkDetector::isEyeClosed(float ear, float threshold) {
    return ear < threshold;
}

// --- Accessor functions remain the same ---
bool my::BlinkDetector::isLeftEyeClosed() const{ return isLeftEyeClosedFlag; }
bool my::BlinkDetector::isRightEyeClosed() const { return isRightEyeClosedFlag; }
float my::BlinkDetector::getLeftEARValue() const{ return leftEAR; }
float my::BlinkDetector::getRightEARValue() const{ return rightEAR; }