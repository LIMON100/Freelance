#ifndef BYTETRACKER_HPP
#define BYTETRACKER_HPP

#include "KalmanFilter.hpp"
#include "utils.hpp" // For NamedBbox
#include <vector>

// Enum to represent the state of a track
enum TrackState { New = 0, Tracked, Lost, Removed };

class STrack {
public:
    STrack(const cv::Rect2f& bbox, float score);
    
    void predict();
    void update(STrack &new_track, int frame_id);
    void mark_lost();
    void mark_removed();

    cv::Rect2f get_rect() const;
    
    // --- THE FIX IS HERE ---
    // Make next_id public and declare it inside the class
    static int next_id;
    static void reset_id_counter() { next_id = 1; }

    bool is_activated;
    int track_id;
    int frame_id;
    int tracklet_len;
    int start_frame;
    TrackState state;
    float score;

private:
    KalmanFilter kf;
    cv::Rect2f _tlwh; // Top-left, width, height
    // 'next_id' is no longer declared privately outside the class
};
class ByteTracker {
public:
    ByteTracker(int frame_rate = 30, int track_buffer = 30);
    std::vector<STrack> update(const std::vector<NamedBbox>& bboxes);

private:
    // Methods for association
    void associate(const std::vector<STrack>& tracks, const std::vector<STrack>& detections,
                   std::vector<std::pair<int, int>>& matches,
                   std::vector<int>& unmatched_tracks,
                   std::vector<int>& unmatched_detections);

    // State lists for tracks
    std::vector<STrack> tracked_stracks;
    std::vector<STrack> lost_stracks;
    std::vector<STrack> removed_stracks;

    int frame_count;
    int max_time_lost;

    // Configuration thresholds
    float track_thresh = 0.5; // Threshold to consider a detection as a potential track
    float high_thresh = 0.6;  // Threshold for a high-confidence detection
    float match_thresh = 0.5; // IoU threshold for matching
};

#endif // BYTETRACKER_HPP
