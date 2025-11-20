// object_detection/utils/calibration_data.hpp
#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "../config.hpp"  // Add this include

struct ZoomLUT {
    std::vector<double> zoom_bins;
    std::vector<double> focus_bins;
    std::vector<std::vector<double>> fx, fy, cx, cy;
    std::vector<std::vector<double>> k1, k2, p1, p2, k3;
};

struct ThermalIntrinsics {
    cv::Mat K;
    cv::Mat dist;
};

struct Extrinsics {
    cv::Mat R; // 3x3 rotation matrix
    cv::Mat t; // 3x1 translation vector
    cv::Vec3d laser_p0; // Laser origin point
    cv::Vec3d laser_d;  // Laser direction vector
};

struct CalibrationData {
    ZoomLUT zoom_lut;
    ThermalIntrinsics thermal_intrinsics;
    Extrinsics extrinsics;
    bool is_loaded = false;
};

bool loadCalibrationData(const PipelineConfig& config, CalibrationData& calib);
void bilinearInterpolate(const ZoomLUT& lut, double zoom, double focus, 
                        double& fx, double& fy, double& cx, double& cy);