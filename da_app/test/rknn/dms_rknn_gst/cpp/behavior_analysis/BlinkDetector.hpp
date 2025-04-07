#ifndef BLINKDETECTOR_HPP
#define BLINKDETECTOR_HPP

#include <opencv2/opencv.hpp>
#include <vector>
#include <chrono>
#include <numeric>
#include <iostream>
#include <deque>

namespace my {

class BlinkDetector {
public:
    BlinkDetector() :
        wasLeftEyeClosed(false),
        blink_counter(0), // Cumulative blink count since start
        blinks_in_window(0), // Blinks in the current 1-minute window
        perclos_start_time(0),
        eye_closed_time(0),
        LEFT_EYE_POINTS({ 33, 160, 158, 133, 153, 144 }),
        RIGHT_EYE_POINTS({ 362, 385, 387, 263, 380, 373 }),
        threshold(0.25),
        blink_rate(0.0), // Initialize blink rate
        last_blink_time(0.0) // Initialize last blink time
    {
        // Initialize perclos_start_time to the current time when the object is created
        perclos_start_time = std::chrono::duration_cast<std::chrono::seconds>(
                                 std::chrono::system_clock::now().time_since_epoch()).count();
    };

    void run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height);
    float calculateEAR(const std::vector<cv::Point>& landmarks, const std::vector<int>& eye_points, int w, int h);
    bool isEyeClosed(float ear, float threshold = 0.2);

    // Accessor functions:
    bool isLeftEyeClosed() const;
    bool isRightEyeClosed() const;
    float getLeftEARValue() const;
    float getRightEARValue() const;
    int getBlinkCount() const { return blink_counter; } // Cumulative count
    int getBlinksInWindow() const { return blinks_in_window; } // Blinks in the last minute window
    double getPerclos() const { return !perclos_durations.empty() ? perclos_durations.back() : 0.0; }
    double getLastBlinkDuration() const { return !blink_durations.empty() ? blink_durations.back() : 0.0; }
    double getBlinkRate() const { return blink_rate; }

private:
    double blink_rate;
    double last_blink_time;

    bool wasLeftEyeClosed;
    std::chrono::high_resolution_clock::time_point leftEyeClosedStart;
    bool isLeftEyeClosedFlag;
    bool isRightEyeClosedFlag;
    float leftEAR;
    float rightEAR;
    int blink_counter;      // Cumulative total blinks
    int blinks_in_window;   // Blinks counted within the current 60s window
    double perclos_start_time; // Start time of the current 60s PERCLOS window
    double eye_closed_time;    // Time eye has been closed in the *current* state (not used for PERCLOS calc directly now)
    const std::vector<int> LEFT_EYE_POINTS;
    const std::vector<int> RIGHT_EYE_POINTS;
    const float threshold;

    std::vector<double> blink_durations; // Durations of blinks completed *within* the current 60s window
    std::vector<double> perclos_durations; // History of calculated PERCLOS values
    std::deque<float> leftEARHistory;
    std::deque<float> rightEARHistory;
    float smoothEAR(std::deque<float>& earHistory, float newEAR);
};

} // namespace my

#endif