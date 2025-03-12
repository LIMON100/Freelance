#include "HeadPoseTracker.hpp"
#include <opencv2/core/core.hpp> 
#include <cmath> 
#include <iomanip>
#include <sstream> 
#include <algorithm> 
#include <iostream>  
using namespace Eigen;
using namespace std;

namespace my {

HeadPoseTracker::HeadPoseTracker() : head_movement_history(30)
{
}
  template <typename T>
        const T& clamp(const T& val, const T& minVal, const T& maxVal) {
            return std::min(std::max(val, minVal), maxVal);
        }
         constexpr double PI = 3.14159265358979323846;

        double degrees(double radians) {
            return radians * 180.0 / PI;
        }
bool HeadPoseTracker::isNearPerfectAxes(const Eigen::Matrix3f& computedAxes, float tolerance) {
    Eigen::Matrix3f expectedAxes;
    expectedAxes << 1, 0, 0,  // X-axis
                    0, 0, 1,  // Z-axis
                    0, 1, 0;  // Y-axis

    // Compute the absolute difference between computed and expected axes
    Eigen::Matrix3f difference = (computedAxes.cwiseAbs()).cwiseAbs() - (expectedAxes.cwiseAbs()).cwiseAbs();
    // Check if all differences are within the tolerance
    return (difference.array() < tolerance).all();
}

Eigen::Matrix3f HeadPoseTracker::calculateAxes(const std::vector<cv::Point>& faceLandmarks) {
    Vector3f leftEye(faceLandmarks[LEFT_EYE].x, faceLandmarks[LEFT_EYE].y, 0);
    Vector3f rightEye(faceLandmarks[RIGHT_EYE].x, faceLandmarks[RIGHT_EYE].y, 0);
    Vector3f forehead(faceLandmarks[FOREHEAD].x, faceLandmarks[FOREHEAD].y, 0);
    Vector3f mouth(faceLandmarks[MOUTH_CENTER].x, faceLandmarks[MOUTH_CENTER].y, 0);

    Vector3f z_axis = forehead - mouth;
    z_axis.normalize();

    Vector3f x_axis = rightEye - leftEye;
    x_axis.normalize();

    Vector3f y_axis = z_axis.cross(x_axis);  // Recalculate y-axis
    y_axis.normalize();

    //Ensure axis orthonormal: Recalculate x axis and z axis again
    x_axis = y_axis.cross(z_axis);
    x_axis.normalize();
    z_axis = x_axis.cross(y_axis);
    z_axis.normalize();

    Eigen::Matrix3f axes;
    axes.col(0) = x_axis;
    axes.col(1) = y_axis;
    axes.col(2) = z_axis;

    return axes;
}

void HeadPoseTracker::autoCalibrate(const std::vector<cv::Point>& faceLandmarks) {
    if (!calibration_state.reference_set) {
        calibration_state.reference_origin = Vector3f(faceLandmarks[NOSE_TIP].x, faceLandmarks[NOSE_TIP].y,0);
        calibration_state.reference_rotation = calculateAxes(faceLandmarks);
        if (isNearPerfectAxes(calibration_state.reference_rotation)) {
            calibration_state.reference_set = true;
        }
        return;
    }

    Eigen::Matrix3f currentRot = calculateAxes(faceLandmarks);
    Eigen::Matrix3f R_rel = currentRot * calibration_state.reference_rotation.transpose();

    Eigen::Matrix3f R_temp = (1 - SMOOTHING_FACTOR) * calibration_state.rotation + SMOOTHING_FACTOR * R_rel;
    Eigen::JacobiSVD<Eigen::Matrix3f> svd(R_temp, Eigen::ComputeFullU | Eigen::ComputeFullV);
    calibration_state.rotation = svd.matrixU() * svd.matrixV().transpose();

    float eye_dist = cv::norm(cv::Mat(faceLandmarks[RIGHT_EYE]), cv::Mat(faceLandmarks[LEFT_EYE]));
    calibration_state.scale =  clamp(eye_dist * 1.5f, MIN_SCALE, MAX_SCALE);
}

std::tuple<float, float, float> HeadPoseTracker::rotationMatrixToEulerAngles(const Eigen::Matrix3f& R) {
    float sy = std::sqrt(R(0, 0) * R(0, 0) + R(1, 0) * R(1, 0));
    bool singular = sy < 1e-6;

    float yaw, pitch, roll;
    if (!singular) {
        yaw = std::atan2(R(1, 0), R(0, 0));
        pitch = std::atan2(-R(2, 0), sy);
        roll = std::atan2(R(2, 1), R(2, 2));
    } else {
        yaw = std::atan2(-R(1, 2), R(1, 1));
        pitch = std::atan2(-R(2, 0), sy);
        roll = 0;
    }
    return std::make_tuple( degrees(yaw),  degrees(pitch), degrees(roll));
}

int HeadPoseTracker::calculateHeadMovementKSS(int timeWindow, int minDuration) {
    // Define thresholds for magnitude (configurable)
    int low_threshold = 30;
    int mid_threshold = 40;
    int high_threshold = 60;

    if (head_movement_history.empty()) {
        head_movement_kss = 1;
        return head_movement_kss;
    }

    // Filter history to only include recent data
    time_t current_time = time(0);
    std::deque<HeadPoseData> recent_history;
    for (const auto& data : head_movement_history) {
        if (current_time - data.timestamp <= timeWindow) {
            recent_history.push_back(data);
        }
    }

    if (recent_history.empty()) {
        head_movement_kss = 1;  // No recent movement implies alertness
        return head_movement_kss;
    }

    // Sort recent history by timestamp (oldest to newest)
    std::sort(recent_history.begin(), recent_history.end(), [](const HeadPoseData& a, const HeadPoseData& b) {
        return a.timestamp < b.timestamp;
    });

    // Track sustained head positions
    std::vector<HeadPoseData> sustained_positions;
    HeadPoseData prev_data;
    time_t prev_timestamp = 0;
    bool prev_data_set = false;

    for (const auto& data : recent_history) {
        time_t timestamp = data.timestamp;
        float yaw = data.yaw, pitch = data.pitch, roll = data.roll;

        // Check if the head is turned to the right (example condition)
        if (yaw > 30) {  // Adjust this condition based on your definition of "right"
            if (prev_data_set && std::abs(yaw - prev_data.yaw) < 10) {  // Small change in yaw
                time_t duration = timestamp - prev_timestamp;
                if (duration >= minDuration) {
                    sustained_positions.push_back(data);
                }
            } else {
                prev_data = data;
                prev_timestamp = timestamp;
                prev_data_set = true;
            }
        } else {
            prev_data_set = false;
        }
    }

    // Calculate the magnitude of the head movement vector for each entry
    std::vector<float> magnitudes;
    for (const auto& data : recent_history) {
        magnitudes.push_back(std::sqrt(data.yaw * data.yaw + data.pitch * data.pitch + data.roll * data.roll));
    }

    // Use the maximum magnitude for initial scoring
    float max_magnitude = magnitudes.empty() ? 0 : *std::max_element(magnitudes.begin(), magnitudes.end());

    // Check if there are sustained positions (head turned to the right for at least 3 seconds)
    if (!sustained_positions.empty()) {
        head_movement_kss = 9;  // High KSS score due to sustained head position
    } else if (max_magnitude < low_threshold) {
        head_movement_kss = 1;
    } else if (max_magnitude < mid_threshold) {
        head_movement_kss = 4;
    } else if (max_magnitude < high_threshold) {
        head_movement_kss = 7;
    } else {
        head_movement_kss = 9;
    }

    return head_movement_kss;
}

HeadPoseTracker::HeadPoseResults HeadPoseTracker::run(const std::vector<cv::Point>& faceLandmarks) {
    HeadPoseResults results;
    results.reference_set = calibration_state.reference_set;
    if (faceLandmarks.empty()) {
        std::vector<std::vector<std::string>> rows = {
            {"Yaw", "N/A"},
            {"Pitch", "N/A"},
            {"Roll", "N/A"},
            {"Head KSS", "N/A"}
        };
        results.rows = rows;
        return results;
    }

    auto landmarks = faceLandmarks;

    autoCalibrate(faceLandmarks);
    std::tie(roll, yaw, pitch) = rotationMatrixToEulerAngles(calibration_state.rotation.transpose());

    HeadPoseData headPoseData;
    headPoseData.yaw = yaw;
    headPoseData.pitch = pitch;
    headPoseData.roll = roll;
    headPoseData.timestamp = time(0);  // Current timestamp
    head_movement_history.push_back(headPoseData);

    int head_movement_kss = calculateHeadMovementKSS();

    std::stringstream stream;
    stream << std::fixed << std::setprecision(1); // Set precision to 1 decimal place

    stream.str(std::string());
    stream << yaw;
    std::string yaw_str = stream.str();
    stream.str(std::string());
    stream << pitch;
    std::string pitch_str = stream.str();
    stream.str(std::string());
    stream << roll;
    std::string roll_str = stream.str();
    std::vector<std::vector<std::string>> rows = {
        {"Yaw", yaw_str + " deg"},
        {"Pitch", pitch_str + " deg"},
        {"Roll", roll_str + " deg"},
        {"Head KSS", std::to_string(head_movement_kss)}
    };
    results.rows = rows;

    return results;
}

} // namespace my