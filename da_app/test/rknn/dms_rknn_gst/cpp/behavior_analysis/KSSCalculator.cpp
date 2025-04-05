// KSSCalculator.cpp
// #include "KSSCalculator.hpp"
// #include <algorithm>
// #include <vector> 
// #include <cmath>  

// KSSCalculator::KSSCalculator() :
//     blinkKSS(1),
//     headPoseKSS(1),
//     yawnKSS(1),
//     objectDetectionKSS(0),
//     perclos(0.0),
//     yaw(0.0),
//     pitch(0.0),
//     roll(0.0),
//     isYawning(false),
//     yawnFrequency(0.0),
//     yawnDuration(0.0),
//     detectedObjects({}) {} //Initialize detectedObjects

// void KSSCalculator::setPerclos(double perclos) {
//     this->perclos = perclos;
// }

// void KSSCalculator::setHeadPose(double yaw, double pitch, double roll) {
//     this->yaw = yaw;
//     this->pitch = pitch;
//     this->roll = roll;
// }

// void KSSCalculator::setYawnMetrics(bool isYawning, double yawnFrequency, double yawnDuration) {
//     this->isYawning = isYawning;
//     this->yawnFrequency = yawnFrequency;
//     this->yawnDuration = yawnDuration;
// }

// //Make sure the function has correct type
// void KSSCalculator::setDetectedObjects(const std::vector<std::string>& detectedObjects) {
//     this->detectedObjects = detectedObjects;
// }

// int KSSCalculator::calculateCompositeKSS() {
//     blinkKSS = calculateBlinkKSS();
//     headPoseKSS = calculateHeadPoseKSS();
//     yawnKSS = calculateYawnKSS();
//     objectDetectionKSS = calculateObjectDetectionKSS();

//     return blinkKSS + headPoseKSS + yawnKSS + objectDetectionKSS;
// }

// int KSSCalculator::calculateBlinkKSS() {
//     if (perclos < 10) {
//         return 1;
//     } else if (perclos <= 20) {
//         return 4;
//     } else if (perclos <= 30) {
//         return 7;
//     } else {
//         return 9;
//     }
// }

// int KSSCalculator::calculateHeadPoseKSS() {
//     double headMovementMagnitude = sqrt(yaw * yaw + pitch * pitch + roll * roll);
//     // Implement logic for head pose KSS (example)
//     if (headMovementMagnitude < 30) {
//         return 1;
//     } else if (headMovementMagnitude < 40) {
//         return 4;
//     } else {
//         return 7;
//     }
// }

// int KSSCalculator::calculateYawnKSS() {
//     if (yawnFrequency < 3) {
//         return 1;
//     } else if (yawnFrequency < 5) {
//         return 4;
//     } else if (yawnFrequency < 7) {
//         return 7;
//     } else {
//         return 9;
//     }
// }

// int KSSCalculator::calculateObjectDetectionKSS() {
//     int score = 0;
//     // Increase score for each detected object (example)
//     for (const auto& object : detectedObjects) {
//         if (object == "mobile" || object == "cigarette" || object == "drinking" || object == "eating") {
//             score += 2;  // Add 2 for each detected object
//         }
//     }
//     // Limit the maximum score to avoid excessively high values
//     return std::min(score, 6);
// }

// // std::string KSSCalculator::getKSSAlertStatus(int compositeKSS) {
// //     if (compositeKSS >= 10) {
// //         return "Extreme Risk";
// //     } else if (compositeKSS >= 7) {
// //         return "High Risk";
// //     } else if (compositeKSS >= 4) {
// //         return "Moderate Risk";
// //     } else {
// //         return "Low Risk";
// //     }
// // }

// std::string KSSCalculator::getKSSAlertStatus(int kss) {
//     if (kss <= 3) {
//         return "";
//     } else if (kss >= 4 && kss <= 7) {
//         return "Moderate Alert";
//     } else if (kss >= 8 && kss <= 9) {
//         return "HIGH ALERT";
//     } else if (kss >= 10) {
//         return "EXTREME ALERT";
//     }
//     // Default case (shouldn't be reached with proper KSS values)
//     return "Unknown";
// }


#include "KSSCalculator.hpp"
#include <algorithm>
#include <vector>
#include <cmath>

KSSCalculator::KSSCalculator() :
    blinkKSS(1),
    headPoseKSS(1),
    yawnKSS(1),
    objectDetectionKSS(0),
    perclos(0.0),
    isYawning(false),
    yawnFrequency(0.0),
    yawnDuration(0.0),
    detectedObjects({}) {}

void KSSCalculator::setPerclos(double perclos) {
    this->perclos = perclos;
}

void KSSCalculator::setHeadPose(int headPoseKSS) {
    this->headPoseKSS = headPoseKSS;
}

void KSSCalculator::setYawnMetrics(bool isYawning, double yawnFrequency, double yawnDuration) {
    this->isYawning = isYawning;
    this->yawnFrequency = yawnFrequency;
    this->yawnDuration = yawnDuration;
}

void KSSCalculator::setDetectedObjects(const std::vector<std::string>& detectedObjects) {
    this->detectedObjects = detectedObjects;
}

int KSSCalculator::calculateCompositeKSS() {
    blinkKSS = calculateBlinkKSS();
    yawnKSS = calculateYawnKSS();
    objectDetectionKSS = calculateObjectDetectionKSS();

    // headPoseKSS is already set by setHeadPose, no need to calculate it
    return blinkKSS + headPoseKSS + yawnKSS + objectDetectionKSS;
}

int KSSCalculator::calculateBlinkKSS() {
    if (perclos < 10) {
        return 1;
    } else if (perclos <= 20) {
        return 4;
    } else if (perclos <= 30) {
        return 7;
    } else {
        return 9;
    }
}

int KSSCalculator::calculateYawnKSS() {
    if (yawnFrequency < 3) {
        return 1;
    } else if (yawnFrequency < 5) {
        return 4;
    } else if (yawnFrequency < 7) {
        return 7;
    } else {
        return 9;
    }
}

int KSSCalculator::calculateObjectDetectionKSS() {
    int score = 0;
    for (const auto& object : detectedObjects) {
        if (object == "mobile" || object == "cigarette" || object == "drinking" || object == "eating") {
            score += 2;
        }
    }
    return std::min(score, 6);
}

std::string KSSCalculator::getKSSAlertStatus(int kss) {
    if (kss <= 3) {
        return "";
    } else if (kss >= 4 && kss <= 7) {
        return "Moderate Alert";
    } else if (kss >= 8 && kss <= 9) {
        return "HIGH ALERT";
    } else if (kss >= 10) {
        return "EXTREME ALERT";
    }
    return "Unknown";
}