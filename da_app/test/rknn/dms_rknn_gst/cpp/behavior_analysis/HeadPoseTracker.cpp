#include "HeadPoseTracker.hpp"
// #include <opencv2/core/core.hpp> 
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

// void HeadPoseTracker::autoCalibrate(const std::vector<cv::Point>& faceLandmarks) {
//     if (!calibration_state.reference_set) {
//         calibration_state.reference_origin = Vector3f(faceLandmarks[NOSE_TIP].x, faceLandmarks[NOSE_TIP].y,0);
//         calibration_state.reference_rotation = calculateAxes(faceLandmarks);
//         if (isNearPerfectAxes(calibration_state.reference_rotation)) {
//             calibration_state.reference_set = true;
//         }
//         return;
//     }

//     Eigen::Matrix3f currentRot = calculateAxes(faceLandmarks);
//     Eigen::Matrix3f R_rel = currentRot * calibration_state.reference_rotation.transpose();

//     Eigen::Matrix3f R_temp = (1 - SMOOTHING_FACTOR) * calibration_state.rotation + SMOOTHING_FACTOR * R_rel;
//     Eigen::JacobiSVD<Eigen::Matrix3f> svd(R_temp, Eigen::ComputeFullU | Eigen::ComputeFullV);
//     calibration_state.rotation = svd.matrixU() * svd.matrixV().transpose();

//     float eye_dist = cv::norm(cv::Mat(faceLandmarks[RIGHT_EYE]), cv::Mat(faceLandmarks[LEFT_EYE]));
//     calibration_state.scale =  clamp(eye_dist * 1.5f, MIN_SCALE, MAX_SCALE);
// }


// In HeadPoseTracker.cpp
void HeadPoseTracker::autoCalibrate(const std::vector<cv::Point>& faceLandmarks) {
    Eigen::Matrix3f currentRot = calculateAxes(faceLandmarks); // Calculate current axes regardless

    if (!calibration_state.reference_set) { // If not calibrated yet...
        printf("***** DEBUG: autoCalibrate - Attempting to set reference... *****\n");
        if (isNearPerfectAxes(currentRot)) { // Check if CURRENT pose is good
            calibration_state.reference_origin = Vector3f(faceLandmarks[NOSE_TIP].x, faceLandmarks[NOSE_TIP].y, 0);
            calibration_state.reference_rotation = currentRot; // Use the current good pose as reference
            calibration_state.reference_set = true;
            calibration_state.rotation = Eigen::Matrix3f::Identity(); // Start relative pose at identity
            printf("***** DEBUG: autoCalibrate - Achieved Near Perfect Axes! Setting reference_set=TRUE *****\n");
        } else {
            printf("***** DEBUG: autoCalibrate - Current axes NOT near perfect. Waiting... *****\n");
             // Do nothing, wait for the next frame
             return; // Exit autoCalibrate for this frame if not calibrated and not perfect yet
        }
    }

    // --- This block only runs AFTER reference_set is TRUE ---
    // Calculate rotation relative to the stored reference
    Eigen::Matrix3f R_rel = currentRot * calibration_state.reference_rotation.transpose();

    // Apply smoothing
    Eigen::Matrix3f R_temp = (1 - SMOOTHING_FACTOR) * calibration_state.rotation + SMOOTHING_FACTOR * R_rel;
    Eigen::JacobiSVD<Eigen::Matrix3f> svd(R_temp, Eigen::ComputeFullU | Eigen::ComputeFullV);
    calibration_state.rotation = svd.matrixU() * svd.matrixV().transpose();

    // Calculate scale
    const cv::Point& right_eye = faceLandmarks[RIGHT_EYE];
    const cv::Point& left_eye  = faceLandmarks[LEFT_EYE];
    float eye_dist = std::hypot( (float)(right_eye.x - left_eye.x),
                                (float)(right_eye.y - left_eye.y) );
    calibration_state.scale = clamp(eye_dist * 1.5f, MIN_SCALE, MAX_SCALE);
}


// void HeadPoseTracker::autoCalibrate(const std::vector<cv::Point>& faceLandmarks) {
//     if (!calibration_state.reference_set) {
//         // ... (setting reference_origin, reference_rotation) ...
//         return;
//     }

//     // ... (calculating R_rel, R_temp, SVD) ...

//     // Calculate eye distance using standard math
//     const cv::Point& right_eye = faceLandmarks[RIGHT_EYE];
//     const cv::Point& left_eye  = faceLandmarks[LEFT_EYE];
//     float eye_dist = std::hypot( (float)(right_eye.x - left_eye.x),
//                                 (float)(right_eye.y - left_eye.y) );

//     calibration_state.scale = clamp(eye_dist * 1.5f, MIN_SCALE, MAX_SCALE); // Use your clamp function
// }

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

// HeadPoseTracker::HeadPoseResults HeadPoseTracker::run(const std::vector<cv::Point>& faceLandmarks) {
//     HeadPoseResults results;
//     results.reference_set = calibration_state.reference_set;
//     // if (faceLandmarks.empty()) {
//     if (faceLandmarks.empty() || faceLandmarks.size() <= RIGHT_EYE ){
//         std::vector<std::vector<std::string>> rows = {
//             {"Yaw", "N/A"},
//             {"Pitch", "N/A"},
//             {"Roll", "N/A"},
//             {"Head KSS", "N/A"}
//         };
//         results.rows = rows;
//         printf("DEBUG HeadPoseTracker::run - Received empty or too few landmarks.\n"); 
//         return results;
//     }

//     printf("DEBUG HeadPoseTracker Landmarks Input:\n");
//     printf("  Left Eye (33): (%d, %d)\n", faceLandmarks[LEFT_EYE].x, faceLandmarks[LEFT_EYE].y);
//     printf("  Right Eye (263): (%d, %d)\n", faceLandmarks[RIGHT_EYE].x, faceLandmarks[RIGHT_EYE].y);
//     printf("  Nose Tip (1): (%d, %d)\n", faceLandmarks[NOSE_TIP].x, faceLandmarks[NOSE_TIP].y);
//     printf("  Mouth Ctr (13): (%d, %d)\n", faceLandmarks[MOUTH_CENTER].x, faceLandmarks[MOUTH_CENTER].y);
//     printf("  Forehead (10): (%d, %d)\n", faceLandmarks[FOREHEAD].x, faceLandmarks[FOREHEAD].y);

//     auto landmarks = faceLandmarks;

//     autoCalibrate(faceLandmarks);
//     std::tie(roll, yaw, pitch) = rotationMatrixToEulerAngles(calibration_state.rotation.transpose());

//     HeadPoseData headPoseData;
//     headPoseData.yaw = yaw;
//     headPoseData.pitch = pitch;
//     headPoseData.roll = roll;
//     headPoseData.timestamp = time(0);  // Current timestamp
//     head_movement_history.push_back(headPoseData);

//     int head_movement_kss = calculateHeadMovementKSS();

//     std::stringstream stream;
//     stream << std::fixed << std::setprecision(1); // Set precision to 1 decimal place

//     stream.str(std::string());
//     stream << yaw;
//     std::string yaw_str = stream.str();
//     stream.str(std::string());
//     stream << pitch;
//     std::string pitch_str = stream.str();
//     stream.str(std::string());
//     stream << roll;
//     std::string roll_str = stream.str();
//     std::vector<std::vector<std::string>> rows = {
//         {"Yaw", yaw_str + " deg"},
//         {"Pitch", pitch_str + " deg"},
//         {"Roll", roll_str + " deg"},
//         {"Head KSS", std::to_string(head_movement_kss)}
//     };
//     results.rows = rows;

//     return results;
// }

HeadPoseTracker::HeadPoseResults HeadPoseTracker::run(const std::vector<cv::Point>& faceLandmarks) {
    HeadPoseResults results;
    results.reference_set = calibration_state.reference_set; // Report current calibration state

    // --- Handle cases with insufficient landmarks ---
    if (faceLandmarks.empty() || faceLandmarks.size() <= RIGHT_EYE) { // Ensure max index is valid
        // Prepare default "N/A" results if no landmarks
        std::vector<std::vector<std::string>> rows = {
            {"Yaw", "N/A"},
            {"Pitch", "N/A"},
            {"Roll", "N/A"},
            {"Head KSS", "N/A"} // Or maybe keep previous KSS? Depends on desired behavior
        };
        results.rows = rows;
        printf("DEBUG HeadPoseTracker::run - Received empty or too few landmarks (%zu).\n", faceLandmarks.size());
        return results; // Exit early
    }

    // --- Print the Input Landmarks used for Axis Calculation (Keep this from previous step) ---
    printf("DEBUG HeadPoseTracker Landmarks Input:\n");
    printf("  Left Eye (33): (%d, %d)\n", faceLandmarks[LEFT_EYE].x, faceLandmarks[LEFT_EYE].y);
    printf("  Right Eye (263): (%d, %d)\n", faceLandmarks[RIGHT_EYE].x, faceLandmarks[RIGHT_EYE].y);
    printf("  Nose Tip (1): (%d, %d)\n", faceLandmarks[NOSE_TIP].x, faceLandmarks[NOSE_TIP].y);
    printf("  Mouth Ctr (13): (%d, %d)\n", faceLandmarks[MOUTH_CENTER].x, faceLandmarks[MOUTH_CENTER].y);
    printf("  Forehead (10): (%d, %d)\n", faceLandmarks[FOREHEAD].x, faceLandmarks[FOREHEAD].y);
    // --- End Input Landmark Print ---

    // --- Perform Calibration / Update Rotation ---
    // This function updates calibration_state.reference_set and calibration_state.rotation
    autoCalibrate(faceLandmarks);

    // --- Debug Print Rotation Matrices ---
    // Add these lines to inspect the matrices involved
    Eigen::Matrix3f rel_rotation_smoothed = calibration_state.rotation;
    Eigen::Matrix3f matrix_for_euler = rel_rotation_smoothed.transpose(); // The matrix actually passed

    printf("DEBUG Smoothed Relative Rotation (calibration_state.rotation):\n");
    // Use std::cout because Eigen overloads the << operator for easy printing
    std::cout << rel_rotation_smoothed << std::endl;

    printf("DEBUG Matrix Passed to Euler Conversion (Transposed):\n");
    std::cout << matrix_for_euler << std::endl;
    // --- End Debug Print Rotation Matrices ---

    // --- Calculate Euler Angles ---
    // Use the matrix that was printed just above
    // std::tie(roll, yaw, pitch) = rotationMatrixToEulerAngles(matrix_for_euler);

    Eigen::Matrix3f matrix_to_convert = calibration_state.rotation.transpose(); // Keep transpose
    std::tie(roll, yaw, pitch) = rotationMatrixToEulerAngles(matrix_to_convert);
    printf("DEBUG HeadPoseTracker Calculated Angles (WITH TRANSPOSE): Yaw=%.1f, Pitch=%.1f, Roll=%.1f\n", yaw, pitch, roll);

    // --- Debug Print Calculated Angles ---
    printf("DEBUG HeadPoseTracker Calculated Angles: Yaw=%.1f, Pitch=%.1f, Roll=%.1f\n", yaw, pitch, roll);
    // --- End Debug Print Calculated Angles ---


    // --- Update History and Calculate KSS ---
    HeadPoseData headPoseData;
    headPoseData.yaw = yaw;
    headPoseData.pitch = pitch;
    headPoseData.roll = roll;
    headPoseData.timestamp = time(0);  // Current timestamp
    head_movement_history.push_back(headPoseData); // Add to history deque

    int head_movement_kss = calculateHeadMovementKSS(); // Calculate KSS based on history

    // --- Format Results for Display ---
    std::stringstream stream;
    stream << std::fixed << std::setprecision(1); // Set precision to 1 decimal place

    stream.str(std::string()); // Clear stream
    stream << yaw;
    std::string yaw_str = stream.str();

    stream.str(std::string()); // Clear stream
    stream << pitch;
    std::string pitch_str = stream.str();

    stream.str(std::string()); // Clear stream
    stream << roll;
    std::string roll_str = stream.str();

    // Update the rows in the results struct
    std::vector<std::vector<std::string>> result_rows = {
        {"Yaw", yaw_str + " deg"},
        {"Pitch", pitch_str + " deg"},
        {"Roll", roll_str + " deg"},
        {"Head KSS", std::to_string(head_movement_kss)}
    };
    results.rows = result_rows; // Assign the formatted rows

    return results; // Return the final results struct
}

} // namespace my