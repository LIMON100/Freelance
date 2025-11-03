#pragma once
#include <string>
#include <vector>
#include <map>
#include <opencv2/opencv.hpp>
struct ImageShot { std::string rgb_path, thermal_path; double zoom=0, focus=0; int pose_id=0; double lens_temp=0, rig_temp=0; };
struct ChessboardSpec { int cols; int rows; double square; };
struct CornerObs { std::vector<cv::Point2f> rgb_corners, thermal_corners; bool has_rgb=false, has_thermal=false; std::vector<cv::Point3f> board_pts; double zoom=0, focus=0; int pose_id=0; };
struct CalibDetections { std::vector<ImageShot> shots; std::vector<CornerObs> corner_sets; };
struct InitialGuess { cv::Mat KZ, distZ, KT, distT, R_ZT, t_ZT; cv::Vec3d laser_p0{0,0,0}, laser_d{0,0,1}; std::map<int, cv::Mat> RZ_B, tZ_B, RT_B, tT_B; };
std::vector<ImageShot> loadShots(const std::string& data_root);
void detectAllCorners(const std::vector<ImageShot>& shots, const ChessboardSpec& board, CalibDetections& out);
void buildInitialGuess(const CalibDetections& det, InitialGuess& guess);