#ifndef KALMAN_FILTER_HPP
#define KALMAN_FILTER_HPP

#include <opencv2/video/tracking.hpp>

// Define the state vector as [center_x, center_y, aspect_ratio, height, vx, vy, va, vh]
// This is an 8-dimensional state.
const int state_dim = 8;
const int measure_dim = 4; // We measure [center_x, center_y, aspect_ratio, height]

class KalmanFilter
{
public:
    KalmanFilter();
    void init(const cv::Rect2f& bbox);
    cv::Rect2f predict();
    void update(const cv::Rect2f& bbox);

private:
    cv::KalmanFilter kf;
    cv::Mat measurement;
};

#endif // KALMAN_FILTER_HPP
