#include "ByteTracker.hpp"
#include <vector>
#include <algorithm>


// Correctly define and initialize the public static member
int STrack::next_id = 1;

// --- Helper function for IoU calculation ---
float calculate_iou(const cv::Rect2f& rect1, const cv::Rect2f& rect2) {
    cv::Rect2f intersection = rect1 & rect2;
    float intersection_area = intersection.area();
    if (intersection_area <= 0) return 0.0f;
    float union_area = rect1.area() + rect2.area() - intersection_area;
    return (union_area > 0) ? (intersection_area / union_area) : 0.0f;
}

// --- STrack implementation ---
STrack::STrack(const cv::Rect2f& bbox, float score) :
    is_activated(false), track_id(0), frame_id(0), tracklet_len(0),
    start_frame(0), state(TrackState::New), score(score) {
    _tlwh = bbox;
    kf.init(bbox);
}

void STrack::predict() {
    if (this->state != TrackState::Tracked) {
        // If the state is not tracked, maybe its velocity is zero
        _tlwh.x += 0;
        _tlwh.y += 0;
    }
    _tlwh = kf.predict();
}

void STrack::update(STrack &new_track, int current_frame_id) {
    kf.update(new_track._tlwh);
    this->state = TrackState::Tracked;
    this->is_activated = true;
    this->score = new_track.score;
    this->tracklet_len++;
    this->frame_id = current_frame_id;
}

void STrack::mark_lost() {
    this->state = TrackState::Lost;
}

void STrack::mark_removed() {
    this->state = TrackState::Removed;
}

cv::Rect2f STrack::get_rect() const {
    return _tlwh;
}

// --- ByteTracker implementation ---
ByteTracker::ByteTracker(int frame_rate, int track_buffer) {
    max_time_lost = static_cast<int>(frame_rate * track_buffer / 30.0);
    frame_count = 0;
    STrack::reset_id_counter();
}

std::vector<STrack> ByteTracker::update(const std::vector<NamedBbox>& bboxes) {
    this->frame_count++;

    // --- Prepare Track and Detection Pools ---
    std::vector<STrack> activated_stracks;
    std::vector<STrack> refind_stracks;
    std::vector<STrack> lost_stracks_pool;
    std::vector<STrack> removed_stracks_pool;

    // Separate existing tracks into tracked and lost pools
    for (const auto& track : this->tracked_stracks) {
        if (track.state == TrackState::Tracked) activated_stracks.push_back(track);
        else lost_stracks_pool.push_back(track);
    }
    for (const auto& track : this->lost_stracks) {
        lost_stracks_pool.push_back(track);
    }
    
    // Predict new locations for all tracks
    for (auto& track : activated_stracks) track.predict();
    for (auto& track : lost_stracks_pool) track.predict();

    // Separate detections into high and low confidence
    std::vector<STrack> high_conf_dets, low_conf_dets;
    for (const auto& named_bbox : bboxes) {
        float w = named_bbox.bbox.x_max - named_bbox.bbox.x_min;
        float h = named_bbox.bbox.y_max - named_bbox.bbox.y_min;
        cv::Rect2f normalized_bbox(named_bbox.bbox.x_min, named_bbox.bbox.y_min, w, h);

        if (named_bbox.bbox.score >= this->high_thresh) {
            high_conf_dets.emplace_back(normalized_bbox, named_bbox.bbox.score);
        } else if (named_bbox.bbox.score >= this->track_thresh) {
            low_conf_dets.emplace_back(normalized_bbox, named_bbox.bbox.score);
        }
    }

    // --- First Association (High Confidence Detections) ---
    std::vector<std::pair<int, int>> matches;
    std::vector<int> unmatched_tracks, unmatched_dets;
    associate(activated_stracks, high_conf_dets, matches, unmatched_tracks, unmatched_dets);

    for (const auto& match : matches) {
        activated_stracks[match.first].update(high_conf_dets[match.second], this->frame_count);
        refind_stracks.push_back(activated_stracks[match.first]);
    }

    // --- Second Association (Low Confidence Detections with Unmatched Tracks) ---
    std::vector<STrack> unmatched_tracks_pool;
    for (int idx : unmatched_tracks) {
        unmatched_tracks_pool.push_back(activated_stracks[idx]);
    }
    
    std::vector<std::pair<int, int>> matches_low;
    std::vector<int> unmatched_tracks_low, unmatched_dets_low;
    associate(unmatched_tracks_pool, low_conf_dets, matches_low, unmatched_tracks_low, unmatched_dets_low);

    for (const auto& match : matches_low) {
        unmatched_tracks_pool[match.first].update(low_conf_dets[match.second], this->frame_count);
        refind_stracks.push_back(unmatched_tracks_pool[match.first]);
    }
    
    // --- Third Association (Lost Tracks with remaining High Confidence Detections) ---
    std::vector<STrack> remaining_high_dets;
    for (int idx : unmatched_dets) {
        remaining_high_dets.push_back(high_conf_dets[idx]);
    }

    std::vector<std::pair<int, int>> matches_lost;
    std::vector<int> unmatched_lost, unmatched_dets_final;
    associate(lost_stracks_pool, remaining_high_dets, matches_lost, unmatched_lost, unmatched_dets_final);

    for (const auto& match : matches_lost) {
        lost_stracks_pool[match.first].update(remaining_high_dets[match.second], this->frame_count);
        refind_stracks.push_back(lost_stracks_pool[match.first]);
    }

    // --- Manage Track Lifecycles ---
    // Mark remaining unmatched tracked stracks as lost
    for (int idx : unmatched_tracks_low) {
        unmatched_tracks_pool[idx].mark_lost();
        lost_stracks_pool.push_back(unmatched_tracks_pool[idx]);
    }

    // Create new tracks from remaining high-confidence detections
    for (int idx : unmatched_dets_final) {
        STrack& new_det = remaining_high_dets[idx];
        if (new_det.score >= this->high_thresh) {
            new_det.track_id = STrack::next_id++;
            new_det.is_activated = true;
            new_det.frame_id = this->frame_count;
            new_det.start_frame = this->frame_count;
            refind_stracks.push_back(new_det);
        }
    }

    // Prune lost tracks that have been gone for too long
    for (auto& track : lost_stracks_pool) {
        if (this->frame_count - track.frame_id > this->max_time_lost) {
            track.mark_removed();
            removed_stracks_pool.push_back(track);
        }
    }
    
    // --- Update Global Track Lists ---
    this->tracked_stracks = refind_stracks;
    this->lost_stracks.clear();
    for(const auto& track : lost_stracks_pool) {
        if(track.state != TrackState::Removed) {
            this->lost_stracks.push_back(track);
        }
    }

    // --- Prepare Output ---
    std::vector<STrack> output_stracks;
    for (const auto& track : this->tracked_stracks) {
        if (track.is_activated) {
            output_stracks.push_back(track);
        }
    }
    return output_stracks;
}


// Simple greedy IoU-based association
void ByteTracker::associate(const std::vector<STrack>& tracks, const std::vector<STrack>& detections,
                               std::vector<std::pair<int, int>>& matches,
                               std::vector<int>& unmatched_tracks,
                               std::vector<int>& unmatched_detections) {
    if (tracks.empty() || detections.empty()) {
        for (size_t i = 0; i < tracks.size(); ++i) unmatched_tracks.push_back(i);
        for (size_t i = 0; i < detections.size(); ++i) unmatched_detections.push_back(i);
        return;
    }

    std::vector<bool> det_matched(detections.size(), false);
    std::vector<bool> track_matched(tracks.size(), false);

    for (size_t i = 0; i < tracks.size(); ++i) {
        float max_iou = 0.0f;
        int best_match_idx = -1;

        for (size_t j = 0; j < detections.size(); ++j) {
            if (!det_matched[j]) {
                float iou = calculate_iou(tracks[i].get_rect(), detections[j].get_rect());
                if (iou > max_iou) {
                    max_iou = iou;
                    best_match_idx = j;
                }
            }
        }
        if (max_iou > match_thresh) {
            matches.push_back({(int)i, best_match_idx});
            det_matched[best_match_idx] = true;
            track_matched[i] = true;
        }
    }

    for (size_t i = 0; i < tracks.size(); ++i) {
        if (!track_matched[i]) unmatched_tracks.push_back(i);
    }
    for (size_t j = 0; j < detections.size(); ++j) {
        if (!det_matched[j]) unmatched_detections.push_back(j);
    }
}
