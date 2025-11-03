#ifndef GLOBAL_STABILIZER_HPP
#define GLOBAL_STABILIZER_HPP

#include <opencv2/opencv.hpp>
#include <vector>

// Re-define the Trajectory struct here or move it to a common header
struct Trajectory
{
    Trajectory() : x(0), y(0), a(0) {}
    Trajectory(double _x, double _y, double _a) : x(_x), y(_y), a(_a) {}
    double x, y, a;
    Trajectory operator+(const Trajectory& other) { return Trajectory(x + other.x, y + other.y, a + other.a); }
    Trajectory operator-(const Trajectory& other) { return Trajectory(x - other.x, y - other.y, a - other.a); }
    Trajectory operator*(const Trajectory& other) { return Trajectory(x * other.x, y * other.y, a * other.a); }
    Trajectory operator/(const Trajectory& other) { return Trajectory(x / other.x, y / other.y, a / other.a); }
};

class GlobalStabilizer {
public:
    GlobalStabilizer();

    void stabilize(const cv::Mat& raw_frame, cv::Mat& stabilized_frame, cv::Mat& affine_matrix);

private:
    // Internal state variables
    cv::Mat m_prev_gray;
    
    // State for the camera motion Kalman Filter
    Trajectory m_X, m_P;
    double m_a, m_x, m_y;
    Trajectory m_Q, m_R;
    int m_k;
};

#endif // GLOBAL_STABILIZER_HPP
