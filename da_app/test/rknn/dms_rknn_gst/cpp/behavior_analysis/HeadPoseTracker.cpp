// #include "HeadPoseTracker.hpp"
// #include <iostream>
// #include <iomanip>
// #include <cmath>
// #include <cmath> 
// #include <sstream> 
// #include <algorithm> 
// #include <Eigen/Dense>
// #include <opencv2/calib3d.hpp> 

// namespace my {

//     HeadPoseTracker::HeadPoseTracker() {
//     }
    
//     bool HeadPoseTracker::isNearPerfectAxes(const Eigen::Matrix3f& computedAxes, float tolerance) {
//         Eigen::Vector3f x_axis = computedAxes.col(0);
//         Eigen::Vector3f y_axis = computedAxes.col(1);
//         Eigen::Vector3f z_axis = computedAxes.col(2);
    
//         float dot_xy = x_axis.dot(y_axis);
//         float dot_yz = y_axis.dot(z_axis);
//         float dot_zx = z_axis.dot(x_axis);
    
//         float norm_x = x_axis.norm();
//         float norm_y = y_axis.norm();
//         float norm_z = z_axis.norm();
    
//         bool isOrthogonal = std::abs(dot_xy) < tolerance &&
//                             std::abs(dot_yz) < tolerance &&
//                             std::abs(dot_zx) < tolerance;
    
//         bool isUnitLength = std::abs(norm_x - 1.0f) < tolerance &&
//                             std::abs(norm_y - 1.0f) < tolerance &&
//                             std::abs(norm_z - 1.0f) < tolerance;
    
//         if (!isOrthogonal || !isUnitLength) {
//             std::cout << "isNearPerfectAxes failed:\n";
//             std::cout << "dot_xy: " << dot_xy << ", dot_yz: " << dot_yz << ", dot_zx: " << dot_zx << "\n";
//             std::cout << "norm_x: " << norm_x << ", norm_y: " << norm_y << ", norm_z: " << norm_z << "\n";
//         }
    
//         return isOrthogonal && isUnitLength;
//     }
    
//     Eigen::Matrix3f HeadPoseTracker::calculateAxes(const std::vector<cv::Point>& faceLandmarks) {
//         Eigen::Vector3f leftEye(faceLandmarks[LandmarkIndex::LEFT_EYE].x, faceLandmarks[LandmarkIndex::LEFT_EYE].y, 0.0f);
//         Eigen::Vector3f rightEye(faceLandmarks[LandmarkIndex::RIGHT_EYE].x, faceLandmarks[LandmarkIndex::RIGHT_EYE].y, 0.0f);
//         Eigen::Vector3f forehead(faceLandmarks[LandmarkIndex::FOREHEAD].x, faceLandmarks[LandmarkIndex::FOREHEAD].y, 0.0f);
//         Eigen::Vector3f mouth(faceLandmarks[LandmarkIndex::MOUTH_CENTER].x, faceLandmarks[LandmarkIndex::MOUTH_CENTER].y, -20.0f);
//         Eigen::Vector3f noseTip(faceLandmarks[LandmarkIndex::NOSE_TIP].x, faceLandmarks[LandmarkIndex::NOSE_TIP].y, 20.0f);
//         Eigen::Vector3f mouthLeft(faceLandmarks[LandmarkIndex::MOUTH_LEFT].x, faceLandmarks[LandmarkIndex::MOUTH_LEFT].y, -15.0f);
//         Eigen::Vector3f mouthRight(faceLandmarks[LandmarkIndex::MOUTH_RIGHT].x, faceLandmarks[LandmarkIndex::MOUTH_RIGHT].y, -15.0f);
    
//         Eigen::Vector3f z_axis = noseTip - (leftEye + rightEye) / 2.0f;
//         if (z_axis.norm() < 1e-6) {
//             std::cerr << "Warning: z_axis is too small, using default z_axis\n";
//             z_axis = Eigen::Vector3f(0, 0, 1);
//         }
//         z_axis.normalize();
    
//         Eigen::Vector3f x_axis = rightEye - leftEye;
//         if (x_axis.norm() < 1e-6) {
//             std::cerr << "Warning: x_axis is too small, using default x_axis\n";
//             x_axis = Eigen::Vector3f(1, 0, 0);
//         }
//         x_axis.normalize();
    
//         Eigen::Vector3f y_axis = z_axis.cross(x_axis);
//         if (y_axis.norm() < 1e-6) {
//             std::cerr << "Warning: y_axis is too small, using default y_axis\n";
//             y_axis = Eigen::Vector3f(0, 1, 0);
//         }
//         y_axis.normalize();
    
//         x_axis = y_axis.cross(z_axis);
//         x_axis.normalize();
//         z_axis = x_axis.cross(y_axis);
//         z_axis.normalize();
    
//         Eigen::Matrix3f axes;
//         axes.col(0) = x_axis;
//         axes.col(1) = y_axis;
//         axes.col(2) = z_axis;
    
//         float det = axes.determinant();
//         if (std::abs(det - 1.0f) > 1e-4) {
//             std::cerr << "Warning: Computed axes determinant is not 1: " << det << "\n";
//             return Eigen::Matrix3f::Identity();
//         }
    
//         return axes;
//     }
    
//     void HeadPoseTracker::autoCalibrate(const std::vector<cv::Point>& faceLandmarks) {
//         if (!calibration_state.reference_set) {
//             calibration_state.reference_origin = Eigen::Vector3f(faceLandmarks[LandmarkIndex::NOSE_TIP].x, faceLandmarks[LandmarkIndex::NOSE_TIP].y, 20.0f);
//             calibration_state.reference_rotation = calculateAxes(faceLandmarks);
//             if (isNearPerfectAxes(calibration_state.reference_rotation, 0.5)) {
//                 calibration_state.reference_set = true;
//                 std::cout << "Reference pose set successfully\n";
//             } else {
//                 std::cout << "Failed to set reference pose: axes not orthogonal\n";
//             }
//             return;
//         }
    
//         Eigen::Matrix3f currentRot = calculateAxes(faceLandmarks);
//         Eigen::Matrix3f R_rel = currentRot * calibration_state.reference_rotation.transpose();
//         Eigen::Matrix3f R_temp = (1 - SMOOTHING_FACTOR) * calibration_state.rotation + SMOOTHING_FACTOR * R_rel;
    
//         for (int i = 0; i < 3; i++) {
//             for (int j = 0; j < 3; j++) {
//                 R_temp(i, j) = std::max(-1e6f, std::min(1e6f, R_temp(i, j)));
//             }
//         }
    
//         Eigen::JacobiSVD<Eigen::Matrix3f> svd(R_temp, Eigen::ComputeFullU | Eigen::ComputeFullV);
//         calibration_state.rotation = svd.matrixU() * svd.matrixV().transpose();
    
//         float det = calibration_state.rotation.determinant();
//         if (std::abs(det - 1.0f) > 1e-4) {
//             std::cerr << "Warning: Corrected rotation matrix determinant is not 1: " << det << "\n";
//             calibration_state.rotation = Eigen::Matrix3f::Identity();
//         }
    
//         const cv::Point& right_eye = faceLandmarks[LandmarkIndex::RIGHT_EYE];
//         const cv::Point& left_eye  = faceLandmarks[LandmarkIndex::LEFT_EYE];
//         float eye_dist = std::hypot((float)(right_eye.x - left_eye.x), (float)(right_eye.y - left_eye.y));
//         calibration_state.scale = clamp(eye_dist * 1.5f, MIN_SCALE, MAX_SCALE);
//     }
    
//     std::tuple<float, float, float> HeadPoseTracker::rotationMatrixToEulerAngles(const Eigen::Matrix3f& R) {
//         float det = R.determinant();
//         if (std::abs(det - 1.0f) > 1e-4) {
//             std::cerr << "Warning: Rotation matrix determinant is not 1: " << det << std::endl;
//             return std::make_tuple(0.0f, 0.0f, 0.0f);
//         }
    
//         float pitch = std::asin(-R(2, 0));
//         float yaw, roll;
    
//         if (std::abs(R(2, 0)) < 0.99999f) {
//             yaw = std::atan2(R(1, 0), R(0, 0));
//             roll = std::atan2(R(2, 1), R(2, 2));
//         } else {
//             yaw = std::atan2(-R(0, 1), R(1, 1));
//             roll = 0.0f;
//         }
    
//         if (std::isnan(yaw) || std::isnan(pitch) || std::isnan(roll)) {
//             std::cerr << "Warning: Euler angles contain nan\n";
//             return std::make_tuple(0.0f, 0.0f, 0.0f);
//         }
    
//         yaw = yaw * 180.0f / M_PI;
//         pitch = pitch * 180.0f / M_PI;
//         roll = roll * 180.0f / M_PI;
    
//         if (yaw > 90.0f) yaw -= 180.0f;
//         if (yaw < -90.0f) yaw += 180.0f;
//         if (pitch > 90.0f) pitch -= 180.0f;
//         if (pitch < -90.0f) pitch += 180.0f;
//         if (roll > 90.0f) roll -= 180.0f;
//         if (roll < -90.0f) roll += 180.0f;
    
//         return std::make_tuple(yaw, pitch, roll);
//     }
    
//     HeadPoseTracker::HeadPoseResults HeadPoseTracker::run(const std::vector<cv::Point>& faceLandmarks) {
//         HeadPoseResults results;
//         results.reference_set = calibration_state.reference_set;
//         if (faceLandmarks.empty()) {
//             std::vector<std::vector<std::string>> rows = {
//                 {"Yaw", "N/A"},
//                 {"Pitch", "N/A"},
//                 {"Roll", "N/A"},
//                 {"Head KSS", "N/A"}
//             };
//             results.rows = rows;
//             return results;
//         }
    
//         autoCalibrate(faceLandmarks);
    
//         std::cout << "Rotation Matrix:\n" << calibration_state.rotation << std::endl;
    
//         std::tie(yaw, pitch, roll) = rotationMatrixToEulerAngles(calibration_state.rotation.transpose());
    
//         std::cout << "Euler Angles - Yaw: " << yaw << ", Pitch: " << pitch << ", Roll: " << roll << std::endl;
    
//         static float smoothed_yaw = 0.0f;
//         static float smoothed_pitch = 0.0f;
//         static float smoothed_roll = 0.0f;
//         const float alpha = 0.2f;
//         smoothed_yaw = (1.0f - alpha) * smoothed_yaw + alpha * yaw;
//         smoothed_pitch = (1.0f - alpha) * smoothed_pitch + alpha * pitch;
//         smoothed_roll = (1.0f - alpha) * smoothed_roll + alpha * roll;
//         yaw = smoothed_yaw;
//         pitch = smoothed_pitch;
//         roll = smoothed_roll;
    
//         HeadPoseData headPoseData;
//         headPoseData.yaw = yaw;
//         headPoseData.pitch = pitch;
//         headPoseData.roll = roll;
//         headPoseData.timestamp = time(0);
//         head_movement_history.push_back(headPoseData);
    
//         int head_movement_kss = calculateHeadMovementKSS();
    
//         std::stringstream stream;
//         stream << std::fixed << std::setprecision(1);
//         stream.str(std::string());
//         stream << yaw;
//         std::string yaw_str = stream.str();
//         stream.str(std::string());
//         stream << pitch;
//         std::string pitch_str = stream.str();
//         stream.str(std::string());
//         stream << roll;
//         std::string roll_str = stream.str();
//         std::vector<std::vector<std::string>> rows = {
//             {"Yaw", yaw_str + " deg"},
//             {"Pitch", pitch_str + " deg"},
//             {"Roll", roll_str + " deg"},
//             {"Head KSS", std::to_string(head_movement_kss)}
//         };
//         results.rows = rows;
    
//         return results;
//     }
    
//     int HeadPoseTracker::calculateHeadMovementKSS(int timeWindow, int minDuration) {
//         // Placeholder implementation
//         return 0;
//     }
    
// } // namespace my



// #include "HeadPoseTracker.hpp"
// #include <iostream>
// #include <iomanip>
// #include <cmath>

// namespace my {

// HeadPoseTracker::HeadPoseTracker() {
// }

// bool HeadPoseTracker::isNearPerfectAxes(const Eigen::Matrix3f& computedAxes, float tolerance) {
//     Eigen::Vector3f x_axis = computedAxes.col(0);
//     Eigen::Vector3f y_axis = computedAxes.col(1);
//     Eigen::Vector3f z_axis = computedAxes.col(2);

//     float dot_xy = x_axis.dot(y_axis);
//     float dot_yz = y_axis.dot(z_axis);
//     float dot_zx = z_axis.dot(x_axis);

//     float norm_x = x_axis.norm();
//     float norm_y = y_axis.norm();
//     float norm_z = z_axis.norm();

//     bool isOrthogonal = std::abs(dot_xy) < tolerance &&
//                         std::abs(dot_yz) < tolerance &&
//                         std::abs(dot_zx) < tolerance;

//     bool isUnitLength = std::abs(norm_x - 1.0f) < tolerance &&
//                         std::abs(norm_y - 1.0f) < tolerance &&
//                         std::abs(norm_z - 1.0f) < tolerance;

//     if (!isOrthogonal || !isUnitLength) {
//         std::cout << "isNearPerfectAxes failed:\n";
//         std::cout << "dot_xy: " << dot_xy << ", dot_yz: " << dot_yz << ", dot_zx: " << dot_zx << "\n";
//         std::cout << "norm_x: " << norm_x << ", norm_y: " << norm_y << ", norm_z: " << norm_z << "\n";
//     }

//     return isOrthogonal && isUnitLength;
// }

// Eigen::Matrix3f HeadPoseTracker::calculateAxes(const std::vector<cv::Point>& faceLandmarks) {
//     Eigen::Vector3f leftEye(faceLandmarks[LandmarkIndex::LEFT_EYE].x, faceLandmarks[LandmarkIndex::LEFT_EYE].y, 0.0f);
//     Eigen::Vector3f rightEye(faceLandmarks[LandmarkIndex::RIGHT_EYE].x, faceLandmarks[LandmarkIndex::RIGHT_EYE].y, 0.0f);
//     Eigen::Vector3f forehead(faceLandmarks[LandmarkIndex::FOREHEAD].x, faceLandmarks[LandmarkIndex::FOREHEAD].y, 0.0f);
//     Eigen::Vector3f mouth(faceLandmarks[LandmarkIndex::MOUTH_CENTER].x, faceLandmarks[LandmarkIndex::MOUTH_CENTER].y, -20.0f);
//     Eigen::Vector3f noseTip(faceLandmarks[LandmarkIndex::NOSE_TIP].x, faceLandmarks[LandmarkIndex::NOSE_TIP].y, 20.0f);
//     Eigen::Vector3f mouthLeft(faceLandmarks[LandmarkIndex::MOUTH_LEFT].x, faceLandmarks[LandmarkIndex::MOUTH_LEFT].y, -15.0f);
//     Eigen::Vector3f mouthRight(faceLandmarks[LandmarkIndex::MOUTH_RIGHT].x, faceLandmarks[LandmarkIndex::MOUTH_RIGHT].y, -15.0f);

//     Eigen::Vector3f z_axis = noseTip - (leftEye + rightEye) / 2.0f;
//     if (z_axis.norm() < 1e-6) {
//         std::cerr << "Warning: z_axis is too small, using default z_axis\n";
//         z_axis = Eigen::Vector3f(0, 0, 1);
//     }
//     z_axis.normalize();

//     Eigen::Vector3f x_axis = rightEye - leftEye;
//     if (x_axis.norm() < 1e-6) {
//         std::cerr << "Warning: x_axis is too small, using default x_axis\n";
//         x_axis = Eigen::Vector3f(1, 0, 0);
//     }
//     x_axis.normalize();

//     Eigen::Vector3f y_axis = z_axis.cross(x_axis);
//     if (y_axis.norm() < 1e-6) {
//         std::cerr << "Warning: y_axis is too small, using default y_axis\n";
//         y_axis = Eigen::Vector3f(0, 1, 0);
//     }
//     y_axis.normalize();

//     x_axis = y_axis.cross(z_axis);
//     x_axis.normalize();
//     z_axis = x_axis.cross(y_axis);
//     z_axis.normalize();

//     Eigen::Matrix3f axes;
//     axes.col(0) = x_axis;
//     axes.col(1) = y_axis;
//     axes.col(2) = z_axis;

//     float det = axes.determinant();
//     if (std::abs(det - 1.0f) > 1e-4) { // Fixed: Removed extra std::
//         std::cerr << "Warning: Computed axes determinant is not 1: " << det << "\n";
//         return Eigen::Matrix3f::Identity();
//     }

//     return axes;
// }

// void HeadPoseTracker::autoCalibrate(const std::vector<cv::Point>& faceLandmarks) {
//     if (!calibration_state.reference_set) {
//         calibration_state.reference_origin = Eigen::Vector3f(faceLandmarks[LandmarkIndex::NOSE_TIP].x, faceLandmarks[LandmarkIndex::NOSE_TIP].y, 20.0f);
//         calibration_state.reference_rotation = calculateAxes(faceLandmarks);
//         if (isNearPerfectAxes(calibration_state.reference_rotation, 0.5)) {
//             calibration_state.reference_set = true;
//             std::cout << "Reference pose set successfully\n";
//         } else {
//             std::cout << "Failed to set reference pose: axes not orthogonal\n";
//         }
//         return;
//     }

//     Eigen::Matrix3f currentRot = calculateAxes(faceLandmarks);
//     Eigen::Matrix3f R_rel = currentRot * calibration_state.reference_rotation.transpose();
//     Eigen::Matrix3f R_temp = (1 - SMOOTHING_FACTOR) * calibration_state.rotation + SMOOTHING_FACTOR * R_rel;

//     for (int i = 0; i < 3; i++) {
//         for (int j = 0; j < 3; j++) {
//             R_temp(i, j) = std::max(-1e6f, std::min(1e6f, R_temp(i, j)));
//         }
//     }

//     Eigen::JacobiSVD<Eigen::Matrix3f> svd(R_temp, Eigen::ComputeFullU | Eigen::ComputeFullV);
//     calibration_state.rotation = svd.matrixU() * svd.matrixV().transpose();

//     float det = calibration_state.rotation.determinant();
//     if (std::abs(det - 1.0f) > 1e-4) {
//         std::cerr << "Warning: Corrected rotation matrix determinant is not 1: " << det << "\n";
//         calibration_state.rotation = Eigen::Matrix3f::Identity();
//     }

//     const cv::Point& right_eye = faceLandmarks[LandmarkIndex::RIGHT_EYE];
//     const cv::Point& left_eye  = faceLandmarks[LandmarkIndex::LEFT_EYE];
//     float eye_dist = std::hypot((float)(right_eye.x - left_eye.x), (float)(right_eye.y - left_eye.y));
//     calibration_state.scale = clamp(eye_dist * 1.5f, MIN_SCALE, MAX_SCALE);
// }

// std::tuple<float, float, float> HeadPoseTracker::rotationMatrixToEulerAngles(const Eigen::Matrix3f& R) {
//     float det = R.determinant();
//     if (std::abs(det - 1.0f) > 1e-4) {
//         std::cerr << "Warning: Rotation matrix determinant is not 1: " << det << std::endl;
//         return std::make_tuple(0.0f, 0.0f, 0.0f);
//     }

//     float pitch = std::asin(-R(2, 0));
//     float yaw, roll;

//     if (std::abs(R(2, 0)) < 0.99999f) {
//         yaw = std::atan2(R(1, 0), R(0, 0));
//         roll = std::atan2(R(2, 1), R(2, 2));
//     } else {
//         yaw = std::atan2(-R(0, 1), R(1, 1));
//         roll = 0.0f;
//     }

//     if (std::isnan(yaw) || std::isnan(pitch) || std::isnan(roll)) {
//         std::cerr << "Warning: Euler angles contain nan\n";
//         return std::make_tuple(0.0f, 0.0f, 0.0f);
//     }

//     yaw = yaw * 180.0f / M_PI;
//     pitch = pitch * 180.0f / M_PI;
//     roll = roll * 180.0f / M_PI;

//     if (yaw > 90.0f) yaw -= 180.0f;
//     if (yaw < -90.0f) yaw += 180.0f;
//     if (pitch > 90.0f) pitch -= 180.0f;
//     if (pitch < -90.0f) pitch += 180.0f;
//     if (roll > 90.0f) roll -= 180.0f;
//     if (roll < -90.0f) roll += 180.0f;

//     return std::make_tuple(yaw, pitch, roll);
// }

// HeadPoseTracker::HeadDirection HeadPoseTracker::calculateHeadDirection(float yaw, float pitch) {
//     bool isLeft = yaw < -YAW_THRESHOLD;
//     bool isRight = yaw > YAW_THRESHOLD;
//     bool isUp = pitch > PITCH_THRESHOLD;
//     bool isDown = pitch < -PITCH_THRESHOLD;

//     if (isLeft && isUp) return HeadDirection::LEFT_UP;
//     if (isLeft && isDown) return HeadDirection::LEFT_DOWN;
//     if (isRight && isUp) return HeadDirection::RIGHT_UP;
//     if (isRight && isDown) return HeadDirection::RIGHT_DOWN;
//     if (isLeft) return HeadDirection::LEFT;
//     if (isRight) return HeadDirection::RIGHT;
//     if (isUp) return HeadDirection::UP;
//     if (isDown) return HeadDirection::DOWN;

//     return HeadDirection::NEUTRAL;
// }

// HeadPoseTracker::HeadPoseResults HeadPoseTracker::run(const std::vector<cv::Point>& faceLandmarks) {
//     HeadPoseResults results;
//     results.reference_set = calibration_state.reference_set;
//     if (faceLandmarks.empty()) {
//         std::vector<std::vector<std::string>> rows = {
//             {"Yaw", "N/A"},
//             {"Pitch", "N/A"},
//             {"Roll", "N/A"},
//             {"Head KSS", "N/A"},
//             {"Direction", "N/A"}
//         };
//         results.rows = rows;
//         return results;
//     }

//     autoCalibrate(faceLandmarks);

//     std::cout << "Rotation Matrix:\n" << calibration_state.rotation << std::endl;

//     std::tie(yaw, pitch, roll) = rotationMatrixToEulerAngles(calibration_state.rotation.transpose());

//     std::cout << "Euler Angles - Yaw: " << yaw << ", Pitch: " << pitch << ", Roll: " << roll << std::endl;

//     static float smoothed_yaw = 0.0f;
//     static float smoothed_pitch = 0.0f;
//     static float smoothed_roll = 0.0f;
//     const float alpha = 0.2f;
//     smoothed_yaw = (1.0f - alpha) * smoothed_yaw + alpha * yaw;
//     smoothed_pitch = (1.0f - alpha) * smoothed_pitch + alpha * pitch;
//     smoothed_roll = (1.0f - alpha) * smoothed_roll + alpha * roll;
//     yaw = smoothed_yaw;
//     pitch = smoothed_pitch;
//     roll = smoothed_roll;

//     // Calculate head direction
//     HeadDirection direction = calculateHeadDirection(yaw, pitch);
//     std::string direction_str;
//     switch (direction) {
//         case HeadDirection::NEUTRAL: direction_str = "Neutral"; break;
//         case HeadDirection::LEFT: direction_str = "Left"; break;
//         case HeadDirection::RIGHT: direction_str = "Right"; break;
//         case HeadDirection::UP: direction_str = "Up"; break;
//         case HeadDirection::DOWN: direction_str = "Down"; break;
//         case HeadDirection::LEFT_UP: direction_str = "Left-Up"; break;
//         case HeadDirection::LEFT_DOWN: direction_str = "Left-Down"; break;
//         case HeadDirection::RIGHT_UP: direction_str = "Right-Up"; break;
//         case HeadDirection::RIGHT_DOWN: direction_str = "Right-Down"; break;
//     }

//     HeadPoseData headPoseData;
//     headPoseData.yaw = yaw;
//     headPoseData.pitch = pitch;
//     headPoseData.roll = roll;
//     headPoseData.timestamp = time(0);
//     head_movement_history.push_back(headPoseData);

//     int head_movement_kss = calculateHeadMovementKSS();

//     std::stringstream stream;
//     stream << std::fixed << std::setprecision(1);
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
//         {"Head KSS", std::to_string(head_movement_kss)},
//         {"Direction", direction_str}
//     };
//     results.rows = rows;

//     return results;
// }

// int HeadPoseTracker::calculateHeadMovementKSS(int timeWindow, int minDuration) {
//     // --- Define NEW thresholds for magnitude ---
//     // Adjust these based on how much overall movement (sqrt(y^2+p^2+r^2))
//     // you consider risky, especially considering the more sensitive +/-15deg turns.
//     // Maybe make these slightly more sensitive too?
//     const float low_threshold = 20.0f;  // KSS=1 below this (Was 30)
//     const float mid_threshold = 30.0f;  // KSS=4 below this (Was 40)
//     const float high_threshold = 50.0f; // KSS=7 below this (Was 60)
//                                         // KSS=9 above high_threshold

//     // --- Define NEW thresholds for sustained pose check ---
//     const float sustained_yaw_threshold = 20.0f; // Trigger if abs(yaw) > 20 (Was 30)
//     const float sustained_yaw_delta = 10.0f;   // Allow 10 deg variation during sustained pose (Keep or adjust?)

//     // --- Basic Checks ---
//     if (head_movement_history.empty()) {
//         head_movement_kss = 1;
//         return head_movement_kss;
//     }

//     // --- Filter History ---
//     time_t current_time = time(0);
//     std::deque<HeadPoseData> recent_history;
//     for (const auto& data : head_movement_history) {
//         if (current_time - data.timestamp <= timeWindow) {
//             recent_history.push_back(data);
//         }
//     }

//     if (recent_history.empty()) {
//         head_movement_kss = 1;
//         return head_movement_kss;
//     }

//     // --- Track Sustained Head Positions (LEFT OR RIGHT) ---
//     std::vector<HeadPoseData> sustained_positions;
//     HeadPoseData prev_data = {};
//     time_t prev_timestamp = 0;
//     bool prev_data_set = false;

//     for (const auto& data : recent_history) {
//         time_t timestamp = data.timestamp;
//         float yaw = data.yaw;

//         // Check if turned significantly LEFT OR RIGHT
//         // Use std::abs to check magnitude against the NEW threshold
//         bool turned_significantly = std::abs(yaw) > sustained_yaw_threshold; // Now checks > 20 deg

//         if (turned_significantly) {
//             // If currently turned significantly...
//             if (prev_data_set &&
//                 (yaw * prev_data.yaw > 0) && // Check if still turned in the SAME direction (both pos or both neg)
//                 std::abs(yaw - prev_data.yaw) < sustained_yaw_delta) // Check if yaw hasn't changed too much
//             {
//                 // Still turned significantly in the SAME direction
//                 time_t duration = timestamp - prev_timestamp;
//                 if (duration >= minDuration) { // Has it been sustained long enough?
//                     sustained_positions.push_back(data);
//                     // Optimization: Can break early if only one instance needed for KSS=9
//                     // break;
//                 }
//             } else {
//                 // Start of a new potential sustained period OR direction changed OR yaw changed too much
//                 prev_data = data;
//                 prev_timestamp = timestamp;
//                 prev_data_set = true;
//             }
//         } else {
//             // Head is relatively forward, reset tracking
//             prev_data_set = false;
//         }
//     }

//     // --- Calculate Overall Movement Magnitudes ---
//     std::vector<float> magnitudes;
//     magnitudes.reserve(recent_history.size());
//     for (const auto& data : recent_history) {
//         magnitudes.push_back(std::sqrt(data.yaw * data.yaw + data.pitch * data.pitch + data.roll * data.roll));
//     }

//     // --- Determine KSS Score ---
//     float max_magnitude = 0.0f;
//     if (!magnitudes.empty()) {
//         max_magnitude = *std::max_element(magnitudes.begin(), magnitudes.end());
//     }

//     // Prioritize sustained awkward pose (override)
//     if (!sustained_positions.empty()) {
//         head_movement_kss = 9; // Sustained turn > 20deg for > 3sec -> KSS 9
//         printf("DEBUG KSS: Sustained pose triggered KSS=9\n"); // Add debug
//     }
//     // Otherwise, score based on the maximum movement magnitude using NEW thresholds
//     else if (max_magnitude < low_threshold) { // Check < 20
//         head_movement_kss = 1;
//         printf("DEBUG KSS: MaxMag=%.1f (<%.1f) -> KSS=1\n", max_magnitude, low_threshold); // Add debug
//     } else if (max_magnitude < mid_threshold) { // Check < 30
//         head_movement_kss = 4;
//         printf("DEBUG KSS: MaxMag=%.1f (<%.1f) -> KSS=4\n", max_magnitude, mid_threshold); // Add debug
//     } else if (max_magnitude < high_threshold) { // Check < 50
//         head_movement_kss = 7;
//         printf("DEBUG KSS: MaxMag=%.1f (<%.1f) -> KSS=7\n", max_magnitude, high_threshold); // Add debug
//     } else { // >= 50
//         head_movement_kss = 9;
//         printf("DEBUG KSS: MaxMag=%.1f (>=%.1f) -> KSS=9\n", max_magnitude, high_threshold); // Add debug
//     }

//     return head_movement_kss;
// }


// } // namespace my


#include "HeadPoseTracker.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>

namespace my {

HeadPoseTracker::HeadPoseTracker() {
}

bool HeadPoseTracker::isNearPerfectAxes(const Eigen::Matrix3f& computedAxes, float tolerance) {
    Eigen::Vector3f x_axis = computedAxes.col(0);
    Eigen::Vector3f y_axis = computedAxes.col(1);
    Eigen::Vector3f z_axis = computedAxes.col(2);

    float dot_xy = x_axis.dot(y_axis);
    float dot_yz = y_axis.dot(z_axis);
    float dot_zx = z_axis.dot(x_axis);

    float norm_x = x_axis.norm();
    float norm_y = y_axis.norm();
    float norm_z = z_axis.norm();

    bool isOrthogonal = std::abs(dot_xy) < tolerance &&
                        std::abs(dot_yz) < tolerance &&
                        std::abs(dot_zx) < tolerance;

    bool isUnitLength = std::abs(norm_x - 1.0f) < tolerance &&
                        std::abs(norm_y - 1.0f) < tolerance &&
                        std::abs(norm_z - 1.0f) < tolerance;

    if (!isOrthogonal || !isUnitLength) {
        std::cout << "isNearPerfectAxes failed:\n";
        std::cout << "dot_xy: " << dot_xy << ", dot_yz: " << dot_yz << ", dot_zx: " << dot_zx << "\n";
        std::cout << "norm_x: " << norm_x << ", norm_y: " << norm_y << ", norm_z: " << norm_z << "\n";
    }

    return isOrthogonal && isUnitLength;
}

Eigen::Matrix3f HeadPoseTracker::calculateAxes(const std::vector<cv::Point>& faceLandmarks) {
    Eigen::Vector3f leftEye(faceLandmarks[LandmarkIndex::LEFT_EYE].x, faceLandmarks[LandmarkIndex::LEFT_EYE].y, 0.0f);
    Eigen::Vector3f rightEye(faceLandmarks[LandmarkIndex::RIGHT_EYE].x, faceLandmarks[LandmarkIndex::RIGHT_EYE].y, 0.0f);
    Eigen::Vector3f forehead(faceLandmarks[LandmarkIndex::FOREHEAD].x, faceLandmarks[LandmarkIndex::FOREHEAD].y, 0.0f);
    Eigen::Vector3f mouth(faceLandmarks[LandmarkIndex::MOUTH_CENTER].x, faceLandmarks[LandmarkIndex::MOUTH_CENTER].y, -20.0f);
    Eigen::Vector3f noseTip(faceLandmarks[LandmarkIndex::NOSE_TIP].x, faceLandmarks[LandmarkIndex::NOSE_TIP].y, 20.0f);
    Eigen::Vector3f mouthLeft(faceLandmarks[LandmarkIndex::MOUTH_LEFT].x, faceLandmarks[LandmarkIndex::MOUTH_LEFT].y, -15.0f);
    Eigen::Vector3f mouthRight(faceLandmarks[LandmarkIndex::MOUTH_RIGHT].x, faceLandmarks[LandmarkIndex::MOUTH_RIGHT].y, -15.0f);

    Eigen::Vector3f z_axis = noseTip - (leftEye + rightEye) / 2.0f;
    if (z_axis.norm() < 1e-6) {
        std::cerr << "Warning: z_axis is too small, using default z_axis\n";
        z_axis = Eigen::Vector3f(0, 0, 1);
    }
    z_axis.normalize();

    Eigen::Vector3f x_axis = rightEye - leftEye;
    if (x_axis.norm() < 1e-6) {
        std::cerr << "Warning: x_axis is too small, using default x_axis\n";
        x_axis = Eigen::Vector3f(1, 0, 0);
    }
    x_axis.normalize();

    Eigen::Vector3f y_axis = z_axis.cross(x_axis);
    if (y_axis.norm() < 1e-6) {
        std::cerr << "Warning: y_axis is too small, using default y_axis\n";
        y_axis = Eigen::Vector3f(0, 1, 0);
    }
    y_axis.normalize();

    x_axis = y_axis.cross(z_axis);
    x_axis.normalize();
    z_axis = x_axis.cross(y_axis);
    z_axis.normalize();

    Eigen::Matrix3f axes;
    axes.col(0) = x_axis;
    axes.col(1) = y_axis;
    axes.col(2) = z_axis;

    float det = axes.determinant();
    if (std::abs(det - 1.0f) > 1e-4) {
        std::cerr << "Warning: Computed axes determinant is not 1: " << det << "\n";
        return Eigen::Matrix3f::Identity();
    }

    return axes;
}

void HeadPoseTracker::autoCalibrate(const std::vector<cv::Point>& faceLandmarks) {
    if (!calibration_state.reference_set) {
        calibration_state.reference_origin = Eigen::Vector3f(faceLandmarks[LandmarkIndex::NOSE_TIP].x, faceLandmarks[LandmarkIndex::NOSE_TIP].y, 20.0f);
        calibration_state.reference_rotation = calculateAxes(faceLandmarks);
        if (isNearPerfectAxes(calibration_state.reference_rotation, 0.5)) {
            calibration_state.reference_set = true;
            std::cout << "Reference pose set successfully\n";
        } else {
            std::cout << "Failed to set reference pose: axes not orthogonal\n";
        }
        return;
    }

    Eigen::Matrix3f currentRot = calculateAxes(faceLandmarks);
    Eigen::Matrix3f R_rel = currentRot * calibration_state.reference_rotation.transpose();
    Eigen::Matrix3f R_temp = (1 - SMOOTHING_FACTOR) * calibration_state.rotation + SMOOTHING_FACTOR * R_rel;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            R_temp(i, j) = std::max(-1e6f, std::min(1e6f, R_temp(i, j)));
        }
    }

    Eigen::JacobiSVD<Eigen::Matrix3f> svd(R_temp, Eigen::ComputeFullU | Eigen::ComputeFullV);
    calibration_state.rotation = svd.matrixU() * svd.matrixV().transpose();

    float det = calibration_state.rotation.determinant();
    if (std::abs(det - 1.0f) > 1e-4) {
        std::cerr << "Warning: Corrected rotation matrix determinant is not 1: " << det << "\n";
        calibration_state.rotation = Eigen::Matrix3f::Identity();
    }

    const cv::Point& right_eye = faceLandmarks[LandmarkIndex::RIGHT_EYE];
    const cv::Point& left_eye  = faceLandmarks[LandmarkIndex::LEFT_EYE];
    float eye_dist = std::hypot((float)(right_eye.x - left_eye.x), (float)(right_eye.y - left_eye.y));
    calibration_state.scale = clamp(eye_dist * 1.5f, MIN_SCALE, MAX_SCALE);
}

std::tuple<float, float, float> HeadPoseTracker::rotationMatrixToEulerAngles(const Eigen::Matrix3f& R) {
    float det = R.determinant();
    if (std::abs(det - 1.0f) > 1e-4) {
        std::cerr << "Warning: Rotation matrix determinant is not 1: " << det << std::endl;
        return std::make_tuple(0.0f, 0.0f, 0.0f);
    }

    float pitch = std::asin(-R(2, 0));
    float yaw, roll;

    if (std::abs(R(2, 0)) < 0.99999f) {
        yaw = std::atan2(R(1, 0), R(0, 0));
        roll = std::atan2(R(2, 1), R(2, 2));
    } else {
        yaw = std::atan2(-R(0, 1), R(1, 1));
        roll = 0.0f;
    }

    if (std::isnan(yaw) || std::isnan(pitch) || std::isnan(roll)) {
        std::cerr << "Warning: Euler angles contain nan\n";
        return std::make_tuple(0.0f, 0.0f, 0.0f);
    }

    yaw = yaw * 180.0f / M_PI;
    pitch = pitch * 180.0f / M_PI;
    roll = roll * 180.0f / M_PI;

    if (yaw > 90.0f) yaw -= 180.0f;
    if (yaw < -90.0f) yaw += 180.0f;
    if (pitch > 90.0f) pitch -= 180.0f;
    if (pitch < -90.0f) pitch += 180.0f;
    if (roll > 90.0f) roll -= 180.0f;
    if (roll < -90.0f) roll += 180.0f;

    return std::make_tuple(yaw, pitch, roll);
}

HeadPoseTracker::HeadDirection HeadPoseTracker::calculateHeadDirection(float yaw, float pitch) {
    bool isLeft = yaw < -YAW_THRESHOLD;
    bool isRight = yaw > YAW_THRESHOLD;
    bool isUp = pitch > PITCH_THRESHOLD;
    bool isDown = pitch < -PITCH_THRESHOLD;

    if (isLeft && isUp) return HeadDirection::LEFT_UP;
    if (isLeft && isDown) return HeadDirection::LEFT_DOWN;
    if (isRight && isUp) return HeadDirection::RIGHT_UP;
    if (isRight && isDown) return HeadDirection::RIGHT_DOWN;
    if (isLeft) return HeadDirection::LEFT;
    if (isRight) return HeadDirection::RIGHT;
    if (isUp) return HeadDirection::UP;
    if (isDown) return HeadDirection::DOWN;

    return HeadDirection::NEUTRAL;
}

int HeadPoseTracker::calculateHeadMovementKSS(int timeWindow, int minDuration) {
    // Define thresholds for head turns
    const float threshold_15 = 15.0f; // Head turn ≥ 15°
    const float threshold_30 = 30.0f; // Head turn ≥ 30°
    const float threshold_45 = 45.0f; // Head turn ≥ 45°

    // Count head turns within the time window
    time_t current_time = time(0);
    std::deque<HeadTurnEvent> recent_events;

    // Filter head turn events to keep only those within the 5-minute window
    for (const auto& event : head_turn_events) {
        if (current_time - event.timestamp <= timeWindow) {
            recent_events.push_back(event);
        }
    }

    // Remove old events from the history
    while (!head_turn_events.empty() && (current_time - head_turn_events.front().timestamp > timeWindow + 10)) {
        head_turn_events.pop_front();
    }

    if (recent_events.empty()) {
        head_movement_kss = 1;
        return head_movement_kss;
    }

    // Count head turns at each threshold
    int count_15 = 0; // Head turns ≥ 15°
    int count_30 = 0; // Head turns ≥ 30°
    int count_45 = 0; // Head turns ≥ 45°

    for (const auto& event : recent_events) {
        if (event.angle >= threshold_45) {
            count_45++;
            count_30++;
            count_15++;
        } else if (event.angle >= threshold_30) {
            count_30++;
            count_15++;
        } else if (event.angle >= threshold_15) {
            count_15++;
        }
    }

    // Determine KSS score based on the criteria
    if (count_45 >= 5) {
        head_movement_kss = 7; // KSS 7-9 (we'll use 7 as the base, can adjust to 9 if needed)
        printf("DEBUG KSS: %d head turns >= 45° in 5 min -> KSS=7\n", count_45);
    } else if (count_30 >= 3) {
        head_movement_kss = 4; // KSS 4-6 (we'll use 4 as the base, can adjust to 6 if needed)
        printf("DEBUG KSS: %d head turns >= 30° in 5 min -> KSS=4\n", count_30);
    } else if (count_15 >= 3) {
        head_movement_kss = 1; // KSS 1-3 (we'll use 1 as the base, can adjust to 3 if needed)
        printf("DEBUG KSS: %d head turns >= 15° in 5 min -> KSS=1\n", count_15);
    } else {
        head_movement_kss = 1; // Default KSS if no criteria are met
        printf("DEBUG KSS: No significant head turns -> KSS=1\n");
    }

    return head_movement_kss;
}

HeadPoseTracker::HeadPoseResults HeadPoseTracker::run(const std::vector<cv::Point>& faceLandmarks) {
    HeadPoseResults results;
    results.reference_set = calibration_state.reference_set;
    if (faceLandmarks.empty()) {
        std::vector<std::vector<std::string>> rows = {
            {"Yaw", "N/A"}, {"Pitch", "N/A"}, {"Roll", "N/A"},
            {"Head KSS", "N/A"}, {"Direction", "N/A"}
        };
        results.rows = rows;
        return results;
    }

    autoCalibrate(faceLandmarks);

    std::tie(yaw, pitch, roll) = rotationMatrixToEulerAngles(calibration_state.rotation.transpose());

    // Smoothing angles
    static float smoothed_yaw = 0.0f, smoothed_pitch = 0.0f, smoothed_roll = 0.0f;
    const float alpha = 0.2f; // Smoothing factor
    smoothed_yaw = (1.0f - alpha) * smoothed_yaw + alpha * yaw;
    smoothed_pitch = (1.0f - alpha) * smoothed_pitch + alpha * pitch;
    smoothed_roll = (1.0f - alpha) * smoothed_roll + alpha * roll;
    yaw = smoothed_yaw;
    pitch = smoothed_pitch;
    roll = smoothed_roll;

    // Record current pose data
    HeadPoseData headPoseData;
    headPoseData.yaw = yaw;
    headPoseData.pitch = pitch;
    headPoseData.roll = roll;
    headPoseData.timestamp = time(0);
    head_movement_history.push_back(headPoseData);

    // Detect head turn events
    static float prev_yaw = 0.0f, prev_pitch = 0.0f;
    static bool was_below_15 = true; // Track if the head was below the 15° threshold
    static time_t last_event_time = 0;
    const float min_time_between_events = 1.0f; // Minimum 1 second between head turn events to avoid overcounting

    float max_angle = std::max(std::abs(yaw), std::abs(pitch));
    time_t current_time = time(0);

    // Check if the head has crossed a threshold (e.g., from < 15° to ≥ 15°)
    if (max_angle >= 15.0f && was_below_15 && (current_time - last_event_time) >= min_time_between_events) {
        HeadTurnEvent event;
        event.angle = max_angle;
        event.timestamp = current_time;
        head_turn_events.push_back(event);
        last_event_time = current_time;
        was_below_15 = false;
    } else if (max_angle < 15.0f) {
        was_below_15 = true; // Reset when head returns to neutral
    }

    // Calculate head direction
    HeadDirection direction = calculateHeadDirection(yaw, pitch);
    std::string direction_str;
    switch (direction) {
        case HeadDirection::NEUTRAL: direction_str = "Neutral"; break;
        case HeadDirection::LEFT: direction_str = "Left"; break;
        case HeadDirection::RIGHT: direction_str = "Right"; break;
        case HeadDirection::UP: direction_str = "Up"; break;
        case HeadDirection::DOWN: direction_str = "Down"; break;
        case HeadDirection::LEFT_UP: direction_str = "Left-Up"; break;
        case HeadDirection::LEFT_DOWN: direction_str = "Left-Down"; break;
        case HeadDirection::RIGHT_UP: direction_str = "Right-Up"; break;
        case HeadDirection::RIGHT_DOWN: direction_str = "Right-Down"; break;
    }

    // Calculate KSS Score after updating history
    int head_movement_kss_score = calculateHeadMovementKSS();

    // Format output strings
    std::stringstream stream;
    stream << std::fixed << std::setprecision(1);
    stream.str(std::string()); stream << yaw; std::string yaw_str = stream.str();
    stream.str(std::string()); stream << pitch; std::string pitch_str = stream.str();
    stream.str(std::string()); stream << roll; std::string roll_str = stream.str();

    std::vector<std::vector<std::string>> result_rows = {
        {"Yaw", yaw_str + " deg"},
        {"Pitch", pitch_str + " deg"},
        {"Roll", roll_str + " deg"},
        {"Head KSS", std::to_string(head_movement_kss_score)},
        {"Direction", direction_str}
    };
    results.rows = result_rows;

    return results;
}

} // namespace my