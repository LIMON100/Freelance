#include "KalmanFilter.hpp"

KalmanFilter::KalmanFilter() : kf(state_dim, measure_dim, 0, CV_32F), measurement(measure_dim, 1, CV_32F)
{
    // Transition matrix (A)
    // Describes how the state evolves from t-1 to t without control inputs.
    // Here, we assume constant velocity.
    // x = x + vx*dt, y = y + vy*dt, etc. dt is assumed to be 1 frame.
    kf.transitionMatrix = (cv::Mat_<float>(state_dim, state_dim) <<
        1, 0, 0, 0, 1, 0, 0, 0,
        0, 1, 0, 0, 0, 1, 0, 0,
        0, 0, 1, 0, 0, 0, 1, 0,
        0, 0, 0, 1, 0, 0, 0, 1,
        0, 0, 0, 0, 1, 0, 0, 0,
        0, 0, 0, 0, 0, 1, 0, 0,
        0, 0, 0, 0, 0, 0, 1, 0,
        0, 0, 0, 0, 0, 0, 0, 1);

    // Measurement matrix (H)
    // Maps the state vector to the measurement vector.
    // We can only measure position and size, not velocity.
    kf.measurementMatrix = (cv::Mat_<float>(measure_dim, state_dim) <<
        1, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 0, 0, 0, 0, 0, 0,
        0, 0, 1, 0, 0, 0, 0, 0,
        0, 0, 0, 1, 0, 0, 0, 0);

    // Process noise covariance (Q)
    // Uncertainty in the motion model. Higher values mean we trust the model less.
    cv::setIdentity(kf.processNoiseCov, cv::Scalar::all(1e-2));
    kf.processNoiseCov.at<float>(4,4) = 1e-1;
    kf.processNoiseCov.at<float>(5,5) = 1e-1;
    kf.processNoiseCov.at<float>(6,6) = 1e-1;
    kf.processNoiseCov.at<float>(7,7) = 1e-1;

    // Measurement noise covariance (R)
    // Uncertainty in the measurement (from YOLO). Higher values mean we trust the detector less.
    // We will make this adaptive in a later step. For now, it's constant.
    cv::setIdentity(kf.measurementNoiseCov, cv::Scalar::all(1e-1));
}

void KalmanFilter::init(const cv::Rect2f& bbox)
{
    // Initialize state vector from the first detection
    kf.statePost.at<float>(0) = bbox.x + bbox.width / 2;
    kf.statePost.at<float>(1) = bbox.y + bbox.height / 2;
    kf.statePost.at<float>(2) = bbox.width / bbox.height;
    kf.statePost.at<float>(3) = bbox.height;
    // Initialize velocities to zero
    kf.statePost.at<float>(4) = 0;
    kf.statePost.at<float>(5) = 0;
    kf.statePost.at<float>(6) = 0;
    kf.statePost.at<float>(7) = 0;
    
    cv::setIdentity(kf.errorCovPost, cv::Scalar::all(0.1));
}

cv::Rect2f KalmanFilter::predict()
{
    cv::Mat prediction = kf.predict();
    float cx = prediction.at<float>(0);
    float cy = prediction.at<float>(1);
    float aspect_ratio = prediction.at<float>(2);
    float h = prediction.at<float>(3);
    float w = aspect_ratio * h;

    return cv::Rect2f(cx - w / 2, cy - h / 2, w, h);
}

void KalmanFilter::update(const cv::Rect2f& bbox)
{
    measurement.at<float>(0) = bbox.x + bbox.width / 2;
    measurement.at<float>(1) = bbox.y + bbox.height / 2;
    measurement.at<float>(2) = bbox.width / bbox.height;
    measurement.at<float>(3) = bbox.height;

    kf.correct(measurement);
}
