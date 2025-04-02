#include "BlinkDetector.hpp"
#include <iostream>
#include <opencv2/imgproc.hpp> 
// #include "IrisLandmark.hpp"

float my::BlinkDetector::smoothEAR(std::deque<float>& earHistory, float newEAR) {
    earHistory.push_back(newEAR);
    if (earHistory.size() > 5) { // Keep a history of 5 frames (adjust as needed)
        earHistory.pop_front();
    }
    float sum = std::accumulate(earHistory.begin(), earHistory.end(), 0.0f);
    return sum / earHistory.size();
}

// void my::BlinkDetector::run(const cv::Mat& frame, const my::IrisLandmark& irisLandmarker) {
void my::BlinkDetector::run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height){
    // int h = frame.rows;
    // int w = frame.cols;

    // std::vector<cv::Point> faceLandmarks = irisLandmarker.getAllFaceLandmarks();

    int h = frame_height;
    int w = frame_width;

    if (faceLandmarks.empty()) {
        isLeftEyeClosedFlag = false;
        isRightEyeClosedFlag = false;
        return;
    }

    leftEAR = calculateEAR(faceLandmarks, LEFT_EYE_POINTS, w, h);
    rightEAR = calculateEAR(faceLandmarks, RIGHT_EYE_POINTS, w, h);

    leftEAR = smoothEAR(leftEARHistory, leftEAR);
    rightEAR = smoothEAR(rightEARHistory, rightEAR);

    bool isLeftEyeClosedNow = isEyeClosed(leftEAR);
    bool isRightEyeClosedNow = isEyeClosed(rightEAR);

    if (isLeftEyeClosedNow) {
        if (!wasLeftEyeClosed) {
            leftEyeClosedStart = std::chrono::high_resolution_clock::now();
        }
        auto now = std::chrono::high_resolution_clock::now();
        eye_closed_time += std::chrono::duration_cast<std::chrono::milliseconds>(now - leftEyeClosedStart).count() / 1000.0;
        wasLeftEyeClosed = true;
    } else {
        if (wasLeftEyeClosed) {
            auto now = std::chrono::high_resolution_clock::now();
            double blinkDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - leftEyeClosedStart).count() / 1000.0;
            blink_counter++;
            blink_durations.push_back(blinkDuration);

            // Calculate Blink Rate
            double current_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            if (last_blink_time != 0) {
                double time_since_last_blink = current_time - last_blink_time;
                blink_rate = 60.0 / time_since_last_blink; // Blinks per minute
            }
            last_blink_time = current_time;
        }

        wasLeftEyeClosed = false;
    }

    double current_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (current_time - perclos_start_time >= 20) {
        double perclos = (std::accumulate(blink_durations.begin(), blink_durations.end(), 0.0) / 60) * 100;
        if (perclos != 0.0) {
            perclos_durations.push_back(perclos);
        }
        eye_closed_time = 0;
        perclos_start_time = current_time;
        blink_durations.clear();
    }

    isLeftEyeClosedFlag = isLeftEyeClosedNow;
    isRightEyeClosedFlag = isRightEyeClosedNow;
}

float my::BlinkDetector::calculateEAR(const std::vector<cv::Point>& landmarks, const std::vector<int>& eye_points, int w, int h) {
    try {
        cv::Point p1 = landmarks[eye_points[0]];
        cv::Point p2 = landmarks[eye_points[1]];
        cv::Point p3 = landmarks[eye_points[2]];
        cv::Point p4 = landmarks[eye_points[3]];
        cv::Point p5 = landmarks[eye_points[4]];
        cv::Point p6 = landmarks[eye_points[5]];

        double vertical_1 = cv::norm(p2 - p6);
        double vertical_2 = cv::norm(p3 - p5);
        double horizontal = cv::norm(p1 - p4);

        return (vertical_1 + vertical_2) / (2.0 * horizontal);
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in calculateEAR: " << e.what() << std::endl;
        return 0.0;
    }
}
// Function to check if eye is closed
bool my::BlinkDetector::isEyeClosed(float ear, float threshold) {
    return ear < threshold;
}

//Functions to set the values
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