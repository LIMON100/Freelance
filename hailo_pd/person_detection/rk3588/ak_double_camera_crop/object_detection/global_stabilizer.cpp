#include "global_stabilizer.hpp"

GlobalStabilizer::GlobalStabilizer() 
    : m_P(1, 1, 1),
      m_a(0), m_x(0), m_y(0),
      m_Q(4e-3, 4e-3, 4e-3), // Process noise
      m_R(0.25, 0.25, 0.25),   // Measurement noise
      m_k(1)
{}

void GlobalStabilizer::stabilize(const cv::Mat& raw_frame, cv::Mat& stabilized_frame, cv::Mat& affine_matrix)
{
    cv::Mat cur_gray;
    cv::cvtColor(raw_frame, cur_gray, cv::COLOR_BGR2GRAY);

    // Default to identity transform if stabilization fails or on the first frame
    affine_matrix = cv::Mat::eye(2, 3, CV_64F);
    
    if (m_prev_gray.empty()) {
        cur_gray.copyTo(m_prev_gray);
        raw_frame.copyTo(stabilized_frame);
        return;
    }

    std::vector<cv::Point2f> prev_corner, cur_corner;
    cv::goodFeaturesToTrack(m_prev_gray, prev_corner, 200, 0.01, 30);
    
    if (prev_corner.size() > 10) { 
        std::vector<uchar> status;
        std::vector<float> err;
        cv::calcOpticalFlowPyrLK(m_prev_gray, cur_gray, prev_corner, cur_corner, status, err);

        std::vector<cv::Point2f> prev_corner2, cur_corner2;
        for(size_t i=0; i < status.size(); i++) {
            if(status[i]) {
                prev_corner2.push_back(prev_corner[i]);
                cur_corner2.push_back(cur_corner[i]);
            }
        }
        
        if (prev_corner2.size() > 10) {
            cv::Mat T = cv::estimateAffine2D(prev_corner2, cur_corner2);

            double dx = 0, dy = 0, da = 0;
            if (!T.empty()) {
                dx = T.at<double>(0, 2);
                dy = T.at<double>(1, 2);
                da = atan2(T.at<double>(1, 0), T.at<double>(0, 0));
            }

            m_x += dx; m_y += dy; m_a += da;
            Trajectory z(m_x, m_y, m_a);

            if (m_k == 1) {
                m_X = Trajectory(0, 0, 0);
                m_P = Trajectory(1, 1, 1);
            } else {
                Trajectory X_ = m_X;
                Trajectory P_ = m_P + m_Q;
                Trajectory K = P_ / (P_ + m_R);
                m_X = X_ + K * (z - X_);
                m_P = (Trajectory(1, 1, 1) - K) * P_;
            }
            m_k++;

            double diff_x = m_X.x - m_x;
            double diff_y = m_X.y - m_y;
            double diff_a = m_X.a - m_a;

            double new_dx = dx + diff_x;
            double new_dy = dy + diff_y;
            double new_da = da + diff_a;

            affine_matrix.at<double>(0, 0) = cos(new_da);
            affine_matrix.at<double>(0, 1) = -sin(new_da);
            affine_matrix.at<double>(1, 0) = sin(new_da);
            affine_matrix.at<double>(1, 1) = cos(new_da);
            affine_matrix.at<double>(0, 2) = new_dx;
            affine_matrix.at<double>(1, 2) = new_dy;
        }
    }
    
    cv::warpAffine(raw_frame, stabilized_frame, affine_matrix, raw_frame.size());
    cur_gray.copyTo(m_prev_gray);
}