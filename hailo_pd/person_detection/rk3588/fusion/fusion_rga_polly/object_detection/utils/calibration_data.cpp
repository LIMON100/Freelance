// object_detection/utils/calibration_data.cpp
#include "calibration_data.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// Remove the config.hpp include here since it's already in the header

bool loadCalibrationData(const PipelineConfig& config, CalibrationData& calib) {
    try {
        // Load zoom intrinsics
        std::ifstream zoom_file(config.intrinsics_zoom_path);
        if (!zoom_file.is_open()) {
            std::cerr << "Failed to open zoom intrinsics file: " << config.intrinsics_zoom_path << std::endl;
            return false;
        }
        
        json zoom_json;
        zoom_file >> zoom_json;
        
        calib.zoom_lut.zoom_bins = zoom_json["zoom_bins"].get<std::vector<double>>();
        calib.zoom_lut.focus_bins = zoom_json["focus_bins"].get<std::vector<double>>();
        calib.zoom_lut.fx = zoom_json["fx"].get<std::vector<std::vector<double>>>();
        calib.zoom_lut.fy = zoom_json["fy"].get<std::vector<std::vector<double>>>();
        calib.zoom_lut.cx = zoom_json["cx"].get<std::vector<std::vector<double>>>();
        calib.zoom_lut.cy = zoom_json["cy"].get<std::vector<std::vector<double>>>();
        
        // Load thermal intrinsics
        std::ifstream thermal_file(config.intrinsics_thermal_path);
        if (!thermal_file.is_open()) {
            std::cerr << "Failed to open thermal intrinsics file: " << config.intrinsics_thermal_path << std::endl;
            return false;
        }
        
        json thermal_json;
        thermal_file >> thermal_json;
        
        calib.thermal_intrinsics.K = cv::Mat(3, 3, CV_64F);
        calib.thermal_intrinsics.dist = cv::Mat(1, 5, CV_64F);
        
        auto K = thermal_json["K"];
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                calib.thermal_intrinsics.K.at<double>(i, j) = K[i][j];
            }
        }
        
        auto dist = thermal_json["dist"];
        for (int i = 0; i < 5; i++) {
            calib.thermal_intrinsics.dist.at<double>(i) = dist[i];
        }
        
        // Load extrinsics
        std::ifstream extrinsics_file(config.extrinsics_path);
        if (!extrinsics_file.is_open()) {
            std::cerr << "Failed to open extrinsics file: " << config.extrinsics_path << std::endl;
            return false;
        }
        
        json extrinsics_json;
        extrinsics_file >> extrinsics_json;
        
        auto Z_T = extrinsics_json["Z_T"];
        auto R = Z_T["R"];
        auto t = Z_T["t"];
        
        calib.extrinsics.R = cv::Mat(3, 3, CV_64F);
        calib.extrinsics.t = cv::Mat(3, 1, CV_64F);
        
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                calib.extrinsics.R.at<double>(i, j) = R[i][j];
            }
            calib.extrinsics.t.at<double>(i) = t[i];
        }
        
        auto laser = extrinsics_json["laser_in_Z"];
        auto p0 = laser["p0"];
        auto d = laser["d"];
        
        calib.extrinsics.laser_p0 = cv::Vec3d(p0[0], p0[1], p0[2]);
        calib.extrinsics.laser_d = cv::Vec3d(d[0], d[1], d[2]);
        
        calib.is_loaded = true;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading calibration data: " << e.what() << std::endl;
        return false;
    }
}

void bilinearInterpolate(const ZoomLUT& lut, double zoom, double focus, 
                        double& fx, double& fy, double& cx, double& cy) {
    // Find zoom indices
    int zi = 0;
    while (zi < (int)lut.zoom_bins.size() - 1 && zoom > lut.zoom_bins[zi]) zi++;
    if (zi > 0) zi--;
    
    // Find focus indices
    int fi = 0;
    while (fi < (int)lut.focus_bins.size() - 1 && focus > lut.focus_bins[fi]) fi++;
    if (fi > 0) fi--;
    
    // Clamp to valid range
    zi = std::max(0, std::min(zi, (int)lut.zoom_bins.size() - 2));
    fi = std::max(0, std::min(fi, (int)lut.focus_bins.size() - 2));
    
    // Calculate interpolation weights
    double z0 = lut.zoom_bins[zi];
    double z1 = lut.zoom_bins[zi + 1];
    double f0 = lut.focus_bins[fi];
    double f1 = lut.focus_bins[fi + 1];
    
    double tz = (z1 == z0) ? 0 : (zoom - z0) / (z1 - z0);
    double tf = (f1 == f0) ? 0 : (focus - f0) / (f1 - f0);
    
    // Bilinear interpolation for each parameter
    auto interp = [&](const std::vector<std::vector<double>>& param) {
        double v00 = param[fi][zi];
        double v01 = param[fi][zi + 1];
        double v10 = param[fi + 1][zi];
        double v11 = param[fi + 1][zi + 1];
        
        double v0 = v00 * (1 - tz) + v01 * tz;
        double v1 = v10 * (1 - tz) + v11 * tz;
        
        return v0 * (1 - tf) + v1 * tf;
    };
    
    fx = interp(lut.fx);
    fy = interp(lut.fy);
    cx = interp(lut.cx);
    cy = interp(lut.cy);
}