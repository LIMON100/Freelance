#include "KSSCalculator.hpp"
#include <algorithm>
#include <vector>
#include <cmath>
#include <iostream>
#include <set>
#include <chrono> // Need this for getting current time

// Constructor remains the same
KSSCalculator::KSSCalculator() :
    blinkKSS(1),
    blinkCountKSS(1),
    headPoseKSS(1),
    yawnKSS(1),
    objectDetectionKSS(0),
    perclos(0.0),
    blinksLastMinute(0),
    isYawning(false),
    yawnFrequency(0.0),
    yawnDuration(0.0)
    {}

// Setters remain the same
void KSSCalculator::setPerclos(double perclos) { this->perclos = perclos; }
void KSSCalculator::setHeadPose(int headPoseKSS) { this->headPoseKSS = std::max(1, headPoseKSS); }
void KSSCalculator::setYawnMetrics(bool isYawning, double yawnFrequency, double yawnDuration) { this->isYawning = isYawning; this->yawnFrequency = yawnFrequency; this->yawnDuration = yawnDuration; }
void KSSCalculator::setBlinksLastMinute(int count) { this->blinksLastMinute = count; }
void KSSCalculator::setDetectedObjects(const std::vector<std::string>& currentFrameObjects, double currentTimeSeconds) { /* ... implementation remains the same ... */
    // Use a set for efficient lookup of objects detected in *this* frame
    std::set<std::string> detectedSet(currentFrameObjects.begin(), currentFrameObjects.end());

    // --- Update Mobile Phone ---
    if (detectedSet.count("mobile")) {
        consecutiveMobileFrames++;
        // Trigger events when thresholds are crossed *for the first time* during this continuous detection
        if (consecutiveMobileFrames >= DURATION_THRESHOLD_3S && !triggeredMobileLevel3) {
            mobileEvents.push_back({currentTimeSeconds, 3});
            triggeredMobileLevel3 = true; // Prevent re-triggering level 3
        }
        if (consecutiveMobileFrames >= DURATION_THRESHOLD_2S && !triggeredMobileLevel2) {
             mobileEvents.push_back({currentTimeSeconds, 2});
             triggeredMobileLevel2 = true;
        }
        if (consecutiveMobileFrames >= DURATION_THRESHOLD_1S && !triggeredMobileLevel1) {
             mobileEvents.push_back({currentTimeSeconds, 1});
             triggeredMobileLevel1 = true;
        }
    } else {
        // Reset counter and triggers when mobile is *not* detected
        consecutiveMobileFrames = 0;
        triggeredMobileLevel1 = false;
        triggeredMobileLevel2 = false;
        triggeredMobileLevel3 = false;
    }

    // --- Update Eating/Drinking ---
    bool eatDrinkDetected = detectedSet.count("eating") || detectedSet.count("drinking");
    if (eatDrinkDetected) {
        consecutiveEatDrinkFrames++;
        if (consecutiveEatDrinkFrames >= DURATION_THRESHOLD_3S && !triggeredEatDrinkLevel3) {
            eatDrinkEvents.push_back({currentTimeSeconds, 3});
            triggeredEatDrinkLevel3 = true;
        }
         if (consecutiveEatDrinkFrames >= DURATION_THRESHOLD_2S && !triggeredEatDrinkLevel2) {
             eatDrinkEvents.push_back({currentTimeSeconds, 2});
             triggeredEatDrinkLevel2 = true;
        }
        if (consecutiveEatDrinkFrames >= DURATION_THRESHOLD_1S && !triggeredEatDrinkLevel1) {
             eatDrinkEvents.push_back({currentTimeSeconds, 1});
             triggeredEatDrinkLevel1 = true;
        }
    } else {
        consecutiveEatDrinkFrames = 0;
        triggeredEatDrinkLevel1 = false;
        triggeredEatDrinkLevel2 = false;
        triggeredEatDrinkLevel3 = false;
    }

    // --- Update Smoking ---
    if (detectedSet.count("cigarette")) { // Assuming class name is "cigarette"
        consecutiveSmokeFrames++;
         if (consecutiveSmokeFrames >= DURATION_THRESHOLD_3S && !triggeredSmokeLevel3) {
            smokeEvents.push_back({currentTimeSeconds, 3});
            triggeredSmokeLevel3 = true;
        }
        if (consecutiveSmokeFrames >= DURATION_THRESHOLD_2S && !triggeredSmokeLevel2) {
             smokeEvents.push_back({currentTimeSeconds, 2});
             triggeredSmokeLevel2 = true;
        }
        if (consecutiveSmokeFrames >= DURATION_THRESHOLD_1S && !triggeredSmokeLevel1) {
             smokeEvents.push_back({currentTimeSeconds, 1});
             triggeredSmokeLevel1 = true;
        }
    } else {
        consecutiveSmokeFrames = 0;
        triggeredSmokeLevel1 = false;
        triggeredSmokeLevel2 = false;
        triggeredSmokeLevel3 = false;
    }

    // --- Clean up old events (Optional but recommended) ---
    // Remove events older than the largest window (10 minutes) + a small buffer
    double cleanupThreshold = currentTimeSeconds - (WINDOW_10_MIN + 10.0);
    while (!mobileEvents.empty() && mobileEvents.front().timestamp < cleanupThreshold) mobileEvents.pop_front();
    while (!eatDrinkEvents.empty() && eatDrinkEvents.front().timestamp < cleanupThreshold) eatDrinkEvents.pop_front();
    while (!smokeEvents.empty() && smokeEvents.front().timestamp < cleanupThreshold) smokeEvents.pop_front();
}

// --- Helper to count events ---
int KSSCalculator::countEventsInWindow(const std::deque<ObjectEvent>& eventQueue, double windowSeconds, double currentTimeSeconds, int minDurationLevel) {
    int count = 0;
    double windowStartTime = currentTimeSeconds - windowSeconds;
    // Iterate from newest to oldest (deque allows efficient front access)
    for (const auto& event : eventQueue) {
        if (event.timestamp >= windowStartTime && event.durationLevel >= minDurationLevel) {
            count++;
        } else if (event.timestamp < windowStartTime) {
             // break; // Optimization - uncomment if sure about order
        }
    }
    return count;
}


// **** MODIFIED IMPLEMENTATION ****
std::vector<std::vector<std::string>> KSSCalculator::calculateCompositeKSS() {
    // Get current time once for consistency
    double now = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();

    // Calculate individual components (store in member variables as before)
    blinkKSS = calculateBlinkKSS();
    blinkCountKSS = calculateBlinkCountKSS();
    yawnKSS = calculateYawnKSS();
    objectDetectionKSS = calculateObjectDetectionKSS(now);
    // headPoseKSS is assumed to be already set via setHeadPose()

    // Calculate the total score
    // int totalKSS = blinkKSS + blinkCountKSS + headPoseKSS + yawnKSS + objectDetectionKSS;
    int totalKSS = blinkKSS + headPoseKSS + yawnKSS + objectDetectionKSS;

    // Create the result vector
    std::vector<std::vector<std::string>> kssBreakdown;

    // Populate the vector
    kssBreakdown.push_back({"PERCLOS KSS", std::to_string(blinkKSS)});
    kssBreakdown.push_back({"Blink Count KSS", std::to_string(blinkCountKSS)});
    kssBreakdown.push_back({"Head Pose KSS", std::to_string(headPoseKSS)});
    kssBreakdown.push_back({"Yawn KSS", std::to_string(yawnKSS)});
    kssBreakdown.push_back({"Object KSS", std::to_string(objectDetectionKSS)});
    kssBreakdown.push_back({"Composite KSS", std::to_string(totalKSS)}); // Add the total

    return kssBreakdown;
}
// **** END MODIFIED IMPLEMENTATION ****


// Individual calculation functions remain the same
int KSSCalculator::calculateBlinkKSS() {
    // Based on PERCLOS
    if (perclos < 5.0) {
        return 0;
    }else if (perclos >= 5.0 && perclos < 10.0) { // Use >= for start of range
        return 1;
    } else if (perclos >= 10.0 && perclos <= 20.0) {
        return 4;
    } else if (perclos > 25.0 && perclos <= 35.0) {
        return 7;
    } else if (perclos > 35.0){ // perclos > 30.0
        return 9;
    }
}

int KSSCalculator::calculateBlinkCountKSS() {
    if (blinksLastMinute > 30) {
        return 7;
    }
    else if (blinksLastMinute > 20 && blinksLastMinute <= 30) {
        return 4;
    }
    else if (blinksLastMinute > 10 && blinksLastMinute <= 20) {
        return 1;
    }
    else{
        return 0;
    }
}


// int KSSCalculator::calculateYawnKSS() {
//     if (yawnFrequency < 1.0) {
//         return 0;
//     } else if (yawnFrequency > 1.0 && yawnFrequency < 3.0) {
//         return 1;
//     }else if (yawnFrequency > 3.0 && yawnFrequency < 5.0) {
//         return 4;
//     } else if (yawnFrequency > 5.0 && yawnFrequency < 7.0) {
//         return 7;
//     } else if(yawnFrequency >= 7.0) { 
//         return 9;
//     }
// }

int KSSCalculator::calculateYawnKSS() {
    if (yawnFrequency > 7.0) {
        return 9;
    } else if (yawnFrequency > 5.0) {
        return 7;
    } else if (yawnFrequency > 3.0) {
        return 4;
    } else if (yawnFrequency > 1.0) {
        return 1;
    } else {
        return 0;
    }
}

int KSSCalculator::calculateObjectDetectionKSS(double currentTimeSeconds) {
    int mobileKSS = 0;
    int eatDrinkKSS = 0;
    int smokeKSS = 0;

    // --- Mobile Phone Scoring ---
    this->mobile_event_count_L1_1m  = countEventsInWindow(mobileEvents, WINDOW_1_MIN, currentTimeSeconds, 1);   // >=1s events in 1min
    this->mobile_event_count_L2_5m  = countEventsInWindow(mobileEvents, WINDOW_5_MIN, currentTimeSeconds, 2);  // >=2s events in 5min
    this->mobile_event_count_L3_10m = countEventsInWindow(mobileEvents, WINDOW_10_MIN, currentTimeSeconds, 3); // >=3s events in 10min

    if (this->mobile_event_count_L3_10m >= 5) mobileKSS = KSS_LEVEL_3;
    else if (this->mobile_event_count_L2_5m >= 3) mobileKSS = KSS_LEVEL_2;
    else if (this->mobile_event_count_L1_1m >= 2) mobileKSS = KSS_LEVEL_1;

    // --- Eating/Drinking Scoring ---
    this->eat_drink_event_count_L1_5m  = countEventsInWindow(eatDrinkEvents, WINDOW_5_MIN, currentTimeSeconds, 1);  // >=1s events in 5min
    this->eat_drink_event_count_L2_10m = countEventsInWindow(eatDrinkEvents, WINDOW_10_MIN, currentTimeSeconds, 2); // >=2s events in 10min
    this->eat_drink_event_count_L3_10m = countEventsInWindow(eatDrinkEvents, WINDOW_10_MIN, currentTimeSeconds, 3); // >=3s events in 10min

    if (this->eat_drink_event_count_L3_10m >= 3) eatDrinkKSS = KSS_LEVEL_3;
    else if (this->eat_drink_event_count_L2_10m >= 2) eatDrinkKSS = KSS_LEVEL_2;
    else if (this->eat_drink_event_count_L1_5m >= 1) eatDrinkKSS = KSS_LEVEL_1;

    // --- Smoking Scoring ---
    this->smoke_event_count_L1_5m  = countEventsInWindow(smokeEvents, WINDOW_5_MIN, currentTimeSeconds, 1);  // >=1s events in 5min
    this->smoke_event_count_L2_10m = countEventsInWindow(smokeEvents, WINDOW_10_MIN, currentTimeSeconds, 2); // >=2s events in 10min
    this->smoke_event_count_L3_10m = countEventsInWindow(smokeEvents, WINDOW_10_MIN, currentTimeSeconds, 3); // >=3s events in 10min

    if (this->smoke_event_count_L3_10m >= 3) smokeKSS = KSS_LEVEL_3;
    else if (this->smoke_event_count_L2_10m >= 2) smokeKSS = KSS_LEVEL_2;
    else if (this->smoke_event_count_L1_5m >= 1) smokeKSS = KSS_LEVEL_1;

    // --- Combine and Cap ---
    int combinedObjectKSS = std::max({mobileKSS, eatDrinkKSS, smokeKSS});
    int finalObjectKSS = 0;
    if (combinedObjectKSS == KSS_LEVEL_3) finalObjectKSS = 6;
    else if (combinedObjectKSS == KSS_LEVEL_2) finalObjectKSS = 4;
    else if (combinedObjectKSS == KSS_LEVEL_1) finalObjectKSS = 2;
    else finalObjectKSS = 0;

    // Store the final KSS component value (already done for 'objectDetectionKSS' in calculateCompositeKSS)
    // this->objectDetectionKSS = finalObjectKSS; // This will be set in calculateCompositeKSS

    return finalObjectKSS;
}


// int KSSCalculator::calculateObjectDetectionKSS(double currentTimeSeconds) {
//     int mobileKSS = 0;
//     int eatDrinkKSS = 0;
//     int smokeKSS = 0;

//     // --- Mobile Phone Scoring ---
//     int mobileCountL3_10m = countEventsInWindow(mobileEvents, WINDOW_10_MIN, currentTimeSeconds, 3); // ≥3s events in 10min
//     int mobileCountL2_5m  = countEventsInWindow(mobileEvents, WINDOW_5_MIN, currentTimeSeconds, 2);  // ≥2s events in 5min
//     int mobileCountL1_1m  = countEventsInWindow(mobileEvents, WINDOW_1_MIN, currentTimeSeconds, 1);   // ≥1s events in 1min


//     // ADDED: Store counts
//     // NEW UI
//     this->mobile_event_count_L3_10m = mobileCountL3_10m;
//     this->mobile_event_count_L2_5m = mobileCountL2_5m;
//     this->mobile_event_count_L1_1m = mobileCountL1_1m;
//     // END ADDED

//     if (mobileCountL3_10m >= 5) mobileKSS = KSS_LEVEL_3; // Highest priority
//     else if (mobileCountL2_5m >= 3) mobileKSS = KSS_LEVEL_2;
//     else if (mobileCountL1_1m >= 2) mobileKSS = KSS_LEVEL_1;
//     // printf("DEBUG Mobile KSS: L3_10m=%d, L2_5m=%d, L1_1m=%d -> KSS=%d\n", mobileCountL3_10m, mobileCountL2_5m, mobileCountL1_1m, mobileKSS);


//     // --- Eating/Drinking Scoring ---
//     int eatDrinkCountL3_10m = countEventsInWindow(eatDrinkEvents, WINDOW_10_MIN, currentTimeSeconds, 3); // ≥3s events in 10min
//     int eatDrinkCountL2_10m = countEventsInWindow(eatDrinkEvents, WINDOW_10_MIN, currentTimeSeconds, 2); // ≥2s events in 10min
//     int eatDrinkCountL1_5m  = countEventsInWindow(eatDrinkEvents, WINDOW_5_MIN, currentTimeSeconds, 1);  // ≥1s events in 5min

//     // ADDED: Store counts
//     this->eat_drink_event_count_L3_10m = eatDrinkCountL3_10m;
//     // ... store others if needed ...

//     if (eatDrinkCountL3_10m >= 3) eatDrinkKSS = KSS_LEVEL_3;
//     else if (eatDrinkCountL2_10m >= 2) eatDrinkKSS = KSS_LEVEL_2;
//     else if (eatDrinkCountL1_5m >= 1) eatDrinkKSS = KSS_LEVEL_1;
//     // printf("DEBUG Eat/Drink KSS: L3_10m=%d, L2_10m=%d, L1_5m=%d -> KSS=%d\n", eatDrinkCountL3_10m, eatDrinkCountL2_10m, eatDrinkCountL1_5m, eatDrinkKSS);


//     // --- Smoking Scoring ---
//     int smokeCountL3_10m = countEventsInWindow(smokeEvents, WINDOW_10_MIN, currentTimeSeconds, 3); // ≥3s events in 10min
//     int smokeCountL2_10m = countEventsInWindow(smokeEvents, WINDOW_10_MIN, currentTimeSeconds, 2); // ≥2s events in 10min
//     int smokeCountL1_5m  = countEventsInWindow(smokeEvents, WINDOW_5_MIN, currentTimeSeconds, 1);  // ≥1s events in 5min

//     if (smokeCountL3_10m >= 3) smokeKSS = KSS_LEVEL_3;
//     else if (smokeCountL2_10m >= 2) smokeKSS = KSS_LEVEL_2;
//     else if (smokeCountL1_5m >= 1) smokeKSS = KSS_LEVEL_1;
//      // printf("DEBUG Smoke KSS: L3_10m=%d, L2_10m=%d, L1_5m=%d -> KSS=%d\n", smokeCountL3_10m, smokeCountL2_10m, smokeCountL1_5m, smokeKSS);


//     // --- Combine and Cap ---
//     // Take the MAXIMUM KSS score from any of the object categories
//     int combinedObjectKSS = std::max({mobileKSS, eatDrinkKSS, smokeKSS});
//     int finalObjectKSS = 0;
//     if (combinedObjectKSS == KSS_LEVEL_3) finalObjectKSS = 6; // Level 3 maps to 6 KSS points (max)
//     else if (combinedObjectKSS == KSS_LEVEL_2) finalObjectKSS = 4; // Level 2 maps to 4 KSS points
//     else if (combinedObjectKSS == KSS_LEVEL_1) finalObjectKSS = 2; // Level 1 maps to 2 KSS points
//     else finalObjectKSS = 0; // No object criteria met

//     // Update the member variable before returning
//     this->objectDetectionKSS = finalObjectKSS; 
//     // Ensure member is updated if needed elsewhere

//     return finalObjectKSS; // Return the calculated & potentially mapped score
// }


// getKSSAlertStatus remains the same
std::string KSSCalculator::getKSSAlertStatus(int kss) {
    if (kss <= 3) {
        return ""; // No alert text for low risk
    } else if (kss >= 4 && kss <= 7) {
        return "Moderate Alert";
    } else if (kss >= 8 && kss <= 9) {
        return "HIGH ALERT";
    } else { // kss >= 10
        return "EXTREME ALERT";
    }
}