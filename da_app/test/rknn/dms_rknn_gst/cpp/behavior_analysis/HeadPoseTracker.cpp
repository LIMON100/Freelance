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
            // std::cout << "Reference pose set successfully\n";
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
    const float threshold_30 = 20.0f; // Head turn ≥ 30°
    const float threshold_45 = 35.0f; // Head turn ≥ 45°

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
    } else if (count_30 >= 3) {
        head_movement_kss = 4; // KSS 4-6 (we'll use 4 as the base, can adjust to 6 if needed)
    } else if (count_15 >= 3) {
        head_movement_kss = 1; // KSS 1-3 (we'll use 1 as the base, can adjust to 3 if needed)
    } else {
        head_movement_kss = 1; // Default KSS if no criteria are met
    }


    // ADDED: Store counts for getters
    // NEW UI
    this->count_15_last_calc = count_15;
    this->count_30_last_calc = count_30;
    this->count_45_last_calc = count_45;
    // END ADDED

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