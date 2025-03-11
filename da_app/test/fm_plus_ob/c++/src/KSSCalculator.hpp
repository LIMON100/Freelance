// KSSCalculator.hpp
#ifndef KSSCALCULATOR_HPP
#define KSSCALCULATOR_HPP

#include <string>
#include <vector> // ADD THIS LINE

class KSSCalculator {
public:
    KSSCalculator();
    ~KSSCalculator() = default;

    // Setters for input factors
    void setPerclos(double perclos);
    void setHeadPose(double yaw, double pitch, double roll);
    void setYawnMetrics(bool isYawning, double yawnFrequency, double yawnDuration);
    void setDetectedObjects(const std::vector<std::string>& detectedObjects);

    // Calculate the composite KSS score
    int calculateCompositeKSS();

    //String get KSS alert status
    std::string getKSSAlertStatus(int compositeKSS);

private:
    // Individual KSS scores
    int blinkKSS;
    int headPoseKSS;
    int yawnKSS;
    int objectDetectionKSS;

    // Input factors
    double perclos;
    double yaw;
    double pitch;
    double roll;
    bool isYawning;
    double yawnFrequency;
    double yawnDuration;
    std::vector<std::string> detectedObjects;

    // Helper functions to calculate individual KSS scores
    int calculateBlinkKSS();
    int calculateHeadPoseKSS();
    int calculateYawnKSS();
    int calculateObjectDetectionKSS();
};

#endif // KSSCALCULATOR_HPP