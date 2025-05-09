#ifndef HEADPOSETRACKER_H
#define HEADPOSETRACKER_H

#include <vector>
#include <deque>
#include <ctime>
#include <opencv2/core.hpp>
#include <Eigen/Dense>
#include <string>
#include <sstream>
#include <iomanip>

namespace my {
class HeadPoseTracker {
public:
    HeadPoseTracker();
    ~HeadPoseTracker() = default;

    enum LandmarkIndex {
        LEFT_EYE = 33,
        RIGHT_EYE = 263,
        NOSE_TIP = 1,
        MOUTH_CENTER = 13,
        FOREHEAD = 10,
        MOUTH_LEFT = 61,
        MOUTH_RIGHT = 291
    };

    enum class HeadDirection {
        NEUTRAL,
        LEFT,
        RIGHT,
        UP,
        DOWN,
        LEFT_UP,
        LEFT_DOWN,
        RIGHT_UP,
        RIGHT_DOWN
    };

    struct HeadPoseData {
        float yaw;
        float pitch;
        float roll;
        time_t timestamp;
    };

    struct HeadTurnEvent {
        float angle;
        time_t timestamp;
    };

    struct HeadPoseResults {
        std::vector<std::vector<std::string>> rows;
        bool reference_set;
    };

    HeadPoseResults run(const std::vector<cv::Point>& faceLandmarks);
    bool isCalibrated() const { return calibration_state.reference_set; }


    // FOR NEW UI
    int getHeadTurnCount15() const { return count_15_last_calc; }
    int getHeadTurnCount30() const { return count_30_last_calc; }
    int getHeadTurnCount45() const { return count_45_last_calc; }

private:
    bool isNearPerfectAxes(const Eigen::Matrix3f& computedAxes, float tolerance = 0.2);
    Eigen::Matrix3f calculateAxes(const std::vector<cv::Point>& faceLandmarks);
    void autoCalibrate(const std::vector<cv::Point>& faceLandmarks);
    std::tuple<float, float, float> rotationMatrixToEulerAngles(const Eigen::Matrix3f& R);
    int calculateHeadMovementKSS(int timeWindow = 300, int minDuration = 3); // Updated timeWindow to 300 seconds (5 minutes)
    HeadDirection calculateHeadDirection(float yaw, float pitch);

    float roll = 0.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float perclos = 0.0f;
    std::vector<std::vector<std::string>> rows;

    float SMOOTHING_FACTOR = 0.2f;
    float MIN_SCALE = 0.05f;
    float MAX_SCALE = 0.2f;

    const float YAW_THRESHOLD = 10.0f;
    const float PITCH_THRESHOLD = 5.0f;

    struct CalibrationState {
        bool reference_set = false;
        Eigen::Vector3f reference_origin = Eigen::Vector3f::Zero();
        Eigen::Matrix3f reference_rotation = Eigen::Matrix3f::Identity();
        Eigen::Matrix3f rotation = Eigen::Matrix3f::Identity();
        float scale = 0.1f;
    } calibration_state;

    int head_movement_kss = 0;
    std::deque<HeadPoseData> head_movement_history;
    std::deque<HeadTurnEvent> head_turn_events; // New: Store head turn events

    float clamp(float value, float min, float max) {
        return std::max(min, std::min(max, value));
    }

    // ADDED: Member variables to store last calculated counts
    // FOR NEW UI
    int count_15_last_calc = 0;
    int count_30_last_calc = 0;
    int count_45_last_calc = 0;
};

} // namespace my

#endif // HEADPOSETRACKER_H