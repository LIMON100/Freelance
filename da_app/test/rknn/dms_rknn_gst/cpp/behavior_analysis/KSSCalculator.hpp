// // KSSCalculator.hpp
// #ifndef KSSCALCULATOR_HPP
// #define KSSCALCULATOR_HPP

// #include <string>
// #include <vector> 

// class KSSCalculator {
// public:
//     KSSCalculator();
//     ~KSSCalculator() = default;

//     // Setters for input factors
//     void setPerclos(double perclos);
//     void setHeadPose(double yaw, double pitch, double roll);
//     void setYawnMetrics(bool isYawning, double yawnFrequency, double yawnDuration);
//     void setDetectedObjects(const std::vector<std::string>& detectedObjects);

//     // Calculate the composite KSS score
//     int calculateCompositeKSS();

//     //String get KSS alert status
//     std::string getKSSAlertStatus(int compositeKSS);

// private:
//     // Individual KSS scores
//     int blinkKSS;
//     int headPoseKSS;
//     int yawnKSS;
//     int objectDetectionKSS;

//     // Input factors
//     double perclos;
//     double yaw;
//     double pitch;
//     double roll;
//     bool isYawning;
//     double yawnFrequency;
//     double yawnDuration;
//     std::vector<std::string> detectedObjects;

//     // Helper functions to calculate individual KSS scores
//     int calculateBlinkKSS();
//     int calculateHeadPoseKSS();
//     int calculateYawnKSS();
//     int calculateObjectDetectionKSS();
// };

// #endif // KSSCALCULATOR_HPP

#ifndef KSSCALCULATOR_H
#define KSSCALCULATOR_H

#include <string>
#include <vector>

class KSSCalculator {
public:
    KSSCalculator();

    void setPerclos(double perclos);
    void setHeadPose(int headPoseKSS); // Updated to accept headPoseKSS directly
    void setYawnMetrics(bool isYawning, double yawnFrequency, double yawnDuration);
    void setDetectedObjects(const std::vector<std::string>& detectedObjects);

    int calculateCompositeKSS();
    std::string getKSSAlertStatus(int compositeKSS);

private:
    int calculateBlinkKSS();
    int calculateYawnKSS();
    int calculateObjectDetectionKSS();

    int blinkKSS;
    int headPoseKSS;
    int yawnKSS;
    int objectDetectionKSS;

    double perclos;
    bool isYawning;
    double yawnFrequency;
    double yawnDuration;
    std::vector<std::string> detectedObjects;
};

#endif // KSSCALCULATOR_H