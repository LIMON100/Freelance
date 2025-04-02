#ifndef HEADPOSETRACKER_H
#define HEADPOSETRACKER_H

#include <vector>
#include <deque>
#include <ctime> 
#include <opencv2/core.hpp>
// #include "IrisLandmark.hpp"
#include <Eigen/Dense>

namespace my {

    class HeadPoseTracker {
    public:
        HeadPoseTracker();
        ~HeadPoseTracker() = default;

        struct HeadPoseData {
            float yaw;
            float pitch;
            float roll;
            time_t timestamp;
        };

        struct HeadPoseResults {
            std::vector<std::vector<std::string>> rows;
            bool reference_set;
        };

        HeadPoseResults run(const std::vector<cv::Point>& faceLandmarks);

    //Add Getter for the Calibration Status (Public)
    bool isCalibrated() const { return calibration_state.reference_set; }

    private:
        bool isNearPerfectAxes(const Eigen::Matrix3f& computedAxes, float tolerance = 0.2);
        Eigen::Matrix3f calculateAxes(const std::vector<cv::Point>& faceLandmarks);
        void autoCalibrate(const std::vector<cv::Point>& faceLandmarks);
        std::tuple<float, float, float> rotationMatrixToEulerAngles(const Eigen::Matrix3f& R);
        int calculateHeadMovementKSS(int timeWindow = 600, int minDuration = 3); // in seconds

        // Landmark indices 
        enum LandmarkIndex {
            LEFT_EYE = 33,
            RIGHT_EYE = 263,
            NOSE_TIP = 1,
            MOUTH_CENTER = 13,
            FOREHEAD = 10
        };

        float roll = 0.0f;
        float yaw = 0.0f;
        float pitch = 0.0f;
        float perclos = 0.0f; // or whatever type perclos is
        std::vector<std::vector<std::string>> rows;

        // Calibration parameters 
        float SMOOTHING_FACTOR = 0.2f;
        float MIN_SCALE = 0.05f;
        float MAX_SCALE = 0.2f;

        // Calibration state 
        struct CalibrationState {
            bool reference_set = false;
            Eigen::Vector3f reference_origin = Eigen::Vector3f::Zero();
            Eigen::Matrix3f reference_rotation = Eigen::Matrix3f::Identity();
            Eigen::Matrix3f rotation = Eigen::Matrix3f::Identity();
            float scale;
        } calibration_state;

        // Head movement KSS parameters 
        int head_movement_kss = 0;
        std::deque<HeadPoseData> head_movement_history;
    };

} // namespace my

#endif // HEADPOSETRACKER_H