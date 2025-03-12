#ifndef BLINKDETECTOR_HPP
#define BLINKDETECTOR_HPP

#include <opencv2/opencv.hpp>
#include <vector>
#include <chrono>
#include <numeric>
#include <iostream>
#include <deque>

namespace my {

class IrisLandmark;

class BlinkDetector {
public:
    BlinkDetector() :
        wasLeftEyeClosed(false),
        blink_counter(0),
        perclos_start_time(0),
        eye_closed_time(0),
        LEFT_EYE_POINTS({ 33, 160, 158, 133, 153, 144 }),
        RIGHT_EYE_POINTS({ 362, 385, 387, 263, 380, 373 }),
        threshold(0.25)
    {};
    void run(const cv::Mat& frame, const my::IrisLandmark& irisLandmarker);
    float calculateEAR(const std::vector<cv::Point>& landmarks, const std::vector<int>& eye_points, int w, int h);
    bool isEyeClosed(float ear, float threshold = 0.2);

    // Accessor functions:
    bool isLeftEyeClosed() const;
    bool isRightEyeClosed() const;
    float getLeftEARValue() const;
    float getRightEARValue() const;
    int getBlinkCount() const { return blink_counter; }
    double getPerclos() const { return !perclos_durations.empty() ? perclos_durations.back() : 0.0; } // Return the latest value
    double getLastBlinkDuration() const { return !blink_durations.empty() ? blink_durations.back() : 0.0; }
    double getBlinkRate() const { return blink_rate; } //Blink Rate - It is the new things

private:
    // New variables
    double blink_rate = 0.0; //Blink Rate - It is the new things
    double last_blink_time = 0.0;

    bool wasLeftEyeClosed;
    std::chrono::high_resolution_clock::time_point leftEyeClosedStart;
    bool isLeftEyeClosedFlag;
    bool isRightEyeClosedFlag;
    float leftEAR;
    float rightEAR;
    int blink_counter;
    double perclos_start_time;
    double eye_closed_time;
    const std::vector<int> LEFT_EYE_POINTS;
    const std::vector<int> RIGHT_EYE_POINTS;
    const float threshold;

    std::vector<double> blink_durations;
    std::vector<double> perclos_durations;
    std::deque<float> leftEARHistory;
    std::deque<float> rightEARHistory;
    float smoothEAR(std::deque<float>& earHistory, float newEAR);
};

} // namespace my

#endif