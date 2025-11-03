#include "calib_data_io.hpp"
#include <iostream>
static std::vector<cv::Point3f> board3D(const ChessboardSpec& b){ std::vector<cv::Point3f> o; o.reserve(b.cols*b.rows); for(int r=0;r<b.rows;++r) for(int c=0;c<b.cols;++c) o.emplace_back(c*b.square, r*b.square, 0); return o; }
static bool detect(const std::string& path, const ChessboardSpec& b, std::vector<cv::Point2f>& c) {
    cv::Mat im=cv::imread(path, cv::IMREAD_GRAYSCALE); if(im.empty()) return false;
    bool ok=cv::findChessboardCorners(im, {b.cols,b.rows}, c, cv::CALIB_CB_ADAPTIVE_THRESH|cv::CALIB_CB_FAST_CHECK|cv::CALIB_CB_NORMALIZE_IMAGE);
    if(ok) cv::cornerSubPix(im, c, {11,11}, {-1,-1}, {cv::TermCriteria::EPS+cv::TermCriteria::MAX_ITER, 30, 0.01});
    return ok;
}
std::vector<ImageShot> loadShots(const std::string& root){
    std::vector<ImageShot> v;
    // TODO: implement your dataset loader; placeholder leaves empty
    return v;
}
void detectAllCorners(const std::vector<ImageShot>& shots, const ChessboardSpec& b, CalibDetections& out){
    out.shots=shots;
    auto obj=board3D(b);
    for(const auto& s: shots){
        CornerObs co; co.board_pts=obj; co.zoom=s.zoom; co.focus=s.focus; co.pose_id=s.pose_id;
        if(!s.rgb_path.empty()) co.has_rgb = detect(s.rgb_path, b, co.rgb_corners);
        if(!s.thermal_path.empty()) co.has_thermal = detect(s.thermal_path, b, co.thermal_corners);
        if(co.has_rgb||co.has_thermal) out.corner_sets.push_back(std::move(co));
    }
}
void buildInitialGuess(const CalibDetections&, InitialGuess& guess){
    std::cout<<"[TODO] buildInitialGuess\n";
    guess.KT = cv::Mat::eye(3,3,CV_64F); guess.distT=cv::Mat::zeros(1,5,CV_64F);
    guess.R_ZT = cv::Mat::eye(3,3,CV_64F); guess.t_ZT=cv::Mat::zeros(3,1,CV_64F);
}