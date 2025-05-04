// KSSCalculator.hpp
#ifndef KSSCALCULATOR_HPP
#define KSSCALCULATOR_HPP

#include <string>
#include <vector>
#include <deque>  // <-- Ensure deque is included if needed by ObjectEvent
#include <chrono> // <-- Ensure chrono is included if needed by ObjectEvent

class KSSCalculator {
public:
    KSSCalculator();


    // FOR UI
    // ADDED: Getters for consecutive frame counts
    int getConsecutiveMobileFrames() const { return consecutiveMobileFrames; }
    int getConsecutiveEatDrinkFrames() const { return consecutiveEatDrinkFrames; }
    int getConsecutiveSmokeFrames() const { return consecutiveSmokeFrames; }

    // ADDED: Getters for event counts (optional, might be complex to show all)
    // Example: Get count of level 3 mobile events in last 10 mins
    int getMobileEventsL3_10m(double currentTimeSeconds) {
        // Note: Need to call countEventsInWindow here or store result from calculateObjectDetectionKSS
        // For simplicity, let's just return the stored value if calculateObjectDetectionKSS updates it.
        // Alternatively, KSSCalculator could store the last calculated counts.
        // Let's assume calculateObjectDetectionKSS stores these internally for now.
        // We'll add placeholder members and getters.
         return mobile_event_count_L3_10m; // Need to compute and store this in calculateObjectDetectionKSS
    }

    // UNTIL THIS


    // Setters for input factors
    void setPerclos(double perclos);
    void setHeadPose(int headPoseKSS); // Setter remains the same
    void setYawnMetrics(bool isYawning, double yawnFrequency, double yawnDuration);
    void setDetectedObjects(const std::vector<std::string>& currentFrameObjects, double currentTimeSeconds);
    void setBlinksLastMinute(int count);

    // **** MODIFIED RETURN TYPE ****
    // Calculate the composite KSS score and return detailed breakdown
    std::vector<std::vector<std::string>> calculateCompositeKSS();

    // Get KSS alert status string (remains the same)
    std::string getKSSAlertStatus(int compositeKSS);

private:
    // --- Constants ---
    // Duration thresholds (in frames, assuming ~30 FPS)
    const int DURATION_THRESHOLD_1S = 30;
    const int DURATION_THRESHOLD_2S = 60;
    const int DURATION_THRESHOLD_3S = 90;
    // Time windows (in seconds)
    const double WINDOW_1_MIN = 60.0;
    const double WINDOW_5_MIN = 300.0;
    const double WINDOW_10_MIN = 600.0;
    // KSS Scores mapping (example, adjust if needed)
    const int KSS_LEVEL_1 = 1; // Corresponds to base KSS 1-3
    const int KSS_LEVEL_2 = 4; // Corresponds to base KSS 4-6
    const int KSS_LEVEL_3 = 7; // Corresponds to base KSS 7-9

    // Helper functions to calculate individual KSS scores
    int calculateBlinkKSS();
    int calculateBlinkCountKSS();
    int calculateYawnKSS();
    int calculateObjectDetectionKSS(double currentTimeSeconds); // *** MODIFIED: Needs current time

    // --- Event Tracking ---
    struct ObjectEvent {
        double timestamp; // Time the duration threshold was met
        int durationLevel; // 1, 2, or 3 (corresponding to 1s, 2s, 3s)
    };
    std::deque<ObjectEvent> mobileEvents;
    std::deque<ObjectEvent> eatDrinkEvents;
    std::deque<ObjectEvent> smokeEvents;

    // --- Consecutive Detection Tracking ---
    int consecutiveMobileFrames = 0;
    int consecutiveEatDrinkFrames = 0;
    int consecutiveSmokeFrames = 0;
    // Flags to prevent multiple event triggers for the same continuous detection
    bool triggeredMobileLevel1 = false;
    bool triggeredMobileLevel2 = false;
    bool triggeredMobileLevel3 = false;
    bool triggeredEatDrinkLevel1 = false;
    bool triggeredEatDrinkLevel2 = false;
    bool triggeredEatDrinkLevel3 = false;
    bool triggeredSmokeLevel1 = false;
    bool triggeredSmokeLevel2 = false;
    bool triggeredSmokeLevel3 = false;

    // --- Helper to count events in a window ---
    int countEventsInWindow(const std::deque<ObjectEvent>& eventQueue, double windowSeconds, double currentTimeSeconds, int minDurationLevel = 1);

    // Individual KSS scores (keep as member variables if needed internally)
    int blinkKSS;
    int blinkCountKSS;
    int headPoseKSS; // Still set externally via setter
    int yawnKSS;
    int objectDetectionKSS;

    // Input factors
    double perclos;
    int blinksLastMinute;
    bool isYawning;
    double yawnFrequency;
    double yawnDuration;
    // No longer need to store detectedObjects list here




    // FOR UI
    int mobile_event_count_L3_10m = 0;
    int mobile_event_count_L2_5m = 0;
    int mobile_event_count_L1_1m = 0;
    int eat_drink_event_count_L3_10m = 0;
};

#endif // KSSCALCULATOR_HPP