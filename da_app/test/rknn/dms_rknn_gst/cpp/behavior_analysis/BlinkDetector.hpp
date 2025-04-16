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
        wereEyesClosedPreviously(false),
        blink_counter(0),
        blinks_in_window(0),
        perclos_start_time(0),
        LEFT_EYE_POINTS({ 33, 160, 158, 133, 153, 144 }),
        RIGHT_EYE_POINTS({ 362, 385, 387, 263, 380, 373 }),
        // ++ Define separate thresholds ++
        left_ear_threshold(0.20),  // Default/observed threshold for left eye
        right_ear_threshold(0.40), // Observed threshold for right eye (adjust based on testing!)
        blink_rate(0.0),
        last_blink_time(0.0)
    {
        perclos_start_time = std::chrono::duration_cast<std::chrono::seconds>(
                                 std::chrono::system_clock::now().time_since_epoch()).count();
    };

    void run(const std::vector<cv::Point>& faceLandmarks, int frame_width, int frame_height);
    float calculateEAR(const std::vector<cv::Point>& landmarks, const std::vector<int>& eye_points, int w, int h);
    // isEyeClosed function can remain the same conceptually, but we'll call it with different thresholds
    bool isEyeClosed(float ear, float threshold); // Keep signature general

    // Accessor functions:
    bool isLeftEyeClosed() const;
    bool isRightEyeClosed() const;
    float getLeftEARValue() const;
    float getRightEARValue() const;
    int getBlinkCount() const { return blink_counter; }
    int getBlinksInWindow() const { return blinks_in_window; }
    double getPerclos() const { return !perclos_durations.empty() ? perclos_durations.back() : 0.0; }
    double getLastBlinkDuration() const { return !blink_durations.empty() ? blink_durations.back() : 0.0; }
    double getBlinkRate() const { return blink_rate; }

private:
    double blink_rate;
    double last_blink_time;

    bool wereEyesClosedPreviously;
    std::chrono::high_resolution_clock::time_point eyesClosedStartTime;

    bool isLeftEyeClosedFlag;
    bool isRightEyeClosedFlag;

    float leftEAR;
    float rightEAR;

    int blink_counter;
    int blinks_in_window;
    double perclos_start_time;

    // ++ Separate Threshold Constants ++
    const float left_ear_threshold;
    const float right_ear_threshold;

    const std::vector<int> LEFT_EYE_POINTS;
    const std::vector<int> RIGHT_EYE_POINTS;


    std::vector<double> blink_durations;
    std::vector<double> perclos_durations;
    std::deque<float> leftEARHistory;
    std::deque<float> rightEARHistory;
    float smoothEAR(std::deque<float>& earHistory, float newEAR);
};

} // namespace my

#endif