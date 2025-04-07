#include "BlinkDetector.hpp"
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <numeric> 
#include <chrono> 

float my::BlinkDetector::smoothEAR(std::deque<float>& earHistory, float newEAR) {
    earHistory.push_back(newEAR);
    if (earHistory.size() > 5) { // Keep a history of 5 frames (adjust as needed)
        earHistory.pop_front();
    }
    if (earHistory.empty()) { // Avoid division by zero
        return newEAR;
    }
    float sum = std::accumulate(earHistory.begin(), earHistory.end(), 0.0f);
    return sum / earHistory.size();
}

void my::BlinkDetector::run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height){
    int h = frame_height;
    int w = frame_width;

    if (faceLandmarks.empty()) {
        isLeftEyeClosedFlag = false;
        isRightEyeClosedFlag = false;
        return;
    }

    leftEAR = calculateEAR(faceLandmarks, LEFT_EYE_POINTS, w, h);
    rightEAR = calculateEAR(faceLandmarks, RIGHT_EYE_POINTS, w, h);

    // Smooth EAR values
    leftEAR = smoothEAR(leftEARHistory, leftEAR);
    rightEAR = smoothEAR(rightEARHistory, rightEAR);

    bool isLeftEyeClosedNow = isEyeClosed(leftEAR);
    bool isRightEyeClosedNow = isEyeClosed(rightEAR); // Use rightEAR for right eye status if needed, but logic follows left

    // --- Blink Detection Logic (using left eye as primary trigger) ---
    if (isLeftEyeClosedNow) {
        // Eye is currently closed
        if (!wasLeftEyeClosed) {
            // Transition: Open -> Closed. Start timer.
            leftEyeClosedStart = std::chrono::high_resolution_clock::now();
            wasLeftEyeClosed = true;
            // eye_closed_time = 0; // Reset the duration tracking for this specific closure period
        }

    } else {
        // Eye is currently open
        if (wasLeftEyeClosed) {
            // Transition: Closed -> Open. A blink just ended.
            auto now = std::chrono::high_resolution_clock::now();
            double blinkDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - leftEyeClosedStart).count() / 1000.0;

            // Increment counters and store duration for PERCLOS calculation
            blink_counter++;      // Increment cumulative counter
            blinks_in_window++;   // Increment counter for the current 1-minute window
            blink_durations.push_back(blinkDuration); // Store duration for PERCLOS

            // Calculate Blink Rate
            double current_time_sec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            if (last_blink_time != 0) {
                double time_since_last_blink = current_time_sec - last_blink_time;
                if (time_since_last_blink > 0) { // Avoid division by zero
                   blink_rate = 60.0 / time_since_last_blink; // Blinks per minute
                } else {
                   blink_rate = 0.0; // Or some indicator of very fast blinks
                }
            }
            last_blink_time = current_time_sec; // Update time of the last completed blink
            wasLeftEyeClosed = false; // Update state
        }
    }

    // --- PERCLOS Calculation Window (Now 60 seconds) ---
    double current_time_sec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (current_time_sec - perclos_start_time >= 60.0) { // Check if 60 seconds have passed
        // Calculate PERCLOS for the completed window
        double total_blink_duration_in_window = std::accumulate(blink_durations.begin(), blink_durations.end(), 0.0);
        double perclos = (total_blink_duration_in_window / 60.0) * 100.0; // Use 60.0 for window duration

        // Store the calculated PERCLOS value (optional, keeps history)
        perclos_durations.push_back(perclos);
    
        perclos_start_time = current_time_sec; // Mark the start of the new window
        blink_durations.clear();             // Clear the durations recorded in the *last* window
        blinks_in_window = 0;                // Reset the blink count for the *new* window
        eye_closed_time = 0;
    }

    // Update flags for external access
    isLeftEyeClosedFlag = isLeftEyeClosedNow;
    isRightEyeClosedFlag = isRightEyeClosedNow;
}

float my::BlinkDetector::calculateEAR(const std::vector<cv::Point>& landmarks, const std::vector<int>& eye_points, int w, int h) {
    // Ensure landmarks vector has enough points before accessing
    if (landmarks.size() <= *std::max_element(eye_points.begin(), eye_points.end())) {
         std::cerr << "Error in calculateEAR: Not enough landmarks provided (" << landmarks.size()
                   << ") for indices in eye_points (max required: "
                   << *std::max_element(eye_points.begin(), eye_points.end()) << ")." << std::endl;
        return 1.0f; // Return a high EAR value to indicate "open" or error
    }

    try {
        // Access landmarks safely using the indices from eye_points
        cv::Point p1 = landmarks[eye_points[0]];
        cv::Point p2 = landmarks[eye_points[1]];
        cv::Point p3 = landmarks[eye_points[2]];
        cv::Point p4 = landmarks[eye_points[3]];
        cv::Point p5 = landmarks[eye_points[4]];
        cv::Point p6 = landmarks[eye_points[5]];

        // Calculate distances using cv::norm
        double vertical_1 = cv::norm(p2 - p6);
        double vertical_2 = cv::norm(p3 - p5);
        double horizontal = cv::norm(p1 - p4);

        // Avoid division by zero or near-zero horizontal distance
        if (horizontal < 1e-6) {
            return 1.0f; // Return high EAR to indicate likely error or closed eye in a strange state
        }

        // Calculate EAR
        return static_cast<float>((vertical_1 + vertical_2) / (2.0 * horizontal));
    }
    catch (const std::out_of_range& oor) {
        // Catch potential out_of_range errors if index check somehow fails (shouldn't with check above)
        std::cerr << "Exception in calculateEAR (out_of_range): " << oor.what() << std::endl;
        return 1.0f; // Indicate error/open
    }
    catch (const std::exception& e) {
        // Catch any other standard exceptions
        std::cerr << "Exception in calculateEAR: " << e.what() << std::endl;
        return 1.0f; // Indicate error/open
    }
}

// Function to check if eye is closed
bool my::BlinkDetector::isEyeClosed(float ear, float threshold) {
    return ear < threshold;
}

// Accessor functions remain the same
bool my::BlinkDetector::isLeftEyeClosed() const{
    return isLeftEyeClosedFlag;
}
bool my::BlinkDetector::isRightEyeClosed() const {
    return isRightEyeClosedFlag;
}
float my::BlinkDetector::getLeftEARValue() const{
    return leftEAR;
}
float my::BlinkDetector::getRightEARValue() const{
    return rightEAR;
}