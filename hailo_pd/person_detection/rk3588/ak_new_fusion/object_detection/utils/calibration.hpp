// File: object_detection/utils/calibration.hpp

#ifndef CALIBRATION_HPP
#define CALIBRATION_HPP

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

// --- THIS IS THE FIX ---
// Include lut.hpp to get the definitions for LUTGrid and LUTParam
// instead of redefining them here.
#include "lut.hpp"
// --- END OF FIX ---

// For the thermal camera's fixed intrinsics
struct PinholeIntrinsics {
    cv::Mat K = cv::Mat::eye(3, 3, CV_64F);
    cv::Mat dist = cv::Mat::zeros(1, 5, CV_64F);
};

// For the extrinsics between cameras
struct Extrinsics {
    cv::Mat R = cv::Mat::eye(3, 3, CV_64F); // Rotation
    cv::Mat t = cv::Mat::zeros(3, 1, CV_64F); // Translation
};

// A single struct to hold all calibration data.
// It now correctly uses the LUTGrid and LUTParam types defined in lut.hpp.
struct HawkEyeCalibration {
    PinholeIntrinsics thermal_intrinsics;
    LUTGrid zoom_lut_grid;
    LUTParam zoom_lut_params;
    Extrinsics thermal_to_zoom_extrinsics; // Z_T from the file
};

// Declare the loading function that will be defined in a .cpp file.
bool loadHawkEyeCalibration(const std::string& calib_dir, HawkEyeCalibration& calib);

// NOTE: The declaration for bilinearLUT is removed from here because
// it is already correctly declared in lut.hpp.

#endif // CALIBRATION_HPP