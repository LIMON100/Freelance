import cv2
import mediapipe as mp
import math
import time

class YawnDetector:
    def __init__(self):
        self.mp_face_mesh = mp.solutions.face_mesh
        self.face_mesh = self.mp_face_mesh.FaceMesh(
            static_image_mode=False,
            max_num_faces=1,
            refine_landmarks=True,
            min_detection_confidence=0.5,
            min_tracking_confidence=0.5
        )

        # Mouth landmarks indices
        self.MOUTH_TOP = 13
        self.MOUTH_BOTTOM = 14

        # Yawn detection parameters
        self.YAWN_THRESHOLD = 25
        self.MIN_YAWN_DURATION = .3
        self.YAWN_COOLDOWN = .1

        # Tracking variables
        self.yawns = [{'start': 0, 'end': 0, 'duration': 0}]
        self.is_yawn = False
        self.yawn_start_time = None
        self.last_yawn_time = 0

    def calculate_pixel_distance(self, landmark1, landmark2, frame_shape):
        x1 = int(landmark1.x * frame_shape[1])
        y1 = int(landmark1.y * frame_shape[0])
        x2 = int(landmark2.x * frame_shape[1])
        y2 = int(landmark2.y * frame_shape[0])
        return math.hypot(x2 - x1, y2 - y1)

    def process_frame(self, frame, results):
        current_time = time.time()

        if results.multi_face_landmarks:
            for face_landmarks in results.multi_face_landmarks:
                mouth_top = face_landmarks.landmark[self.MOUTH_TOP]
                mouth_bottom = face_landmarks.landmark[self.MOUTH_BOTTOM]

                # Calculate mouth openness
                mouth_distance = self.calculate_pixel_distance(
                    mouth_top, mouth_bottom, frame.shape
                )

                # Yawn detection logic
                if mouth_distance > self.YAWN_THRESHOLD:
                    if self.yawn_start_time is None:
                        self.yawn_start_time = current_time
                        self.is_yawn = True  # Set is_yawn to True when mouth opens
                else:
                    if self.yawn_start_time is not None:
                        yawn_duration = current_time - self.yawn_start_time
                        if yawn_duration >= self.MIN_YAWN_DURATION:
                            if (current_time - self.last_yawn_time) > self.YAWN_COOLDOWN:
                                self.yawns.append({
                                    "start": self.yawn_start_time,
                                    "end": current_time,
                                    "duration": yawn_duration
                                })
                                self.last_yawn_time = current_time
                        # Reset yawn_start_time and is_yawn after logging the yawn
                        self.yawn_start_time = None
                        self.is_yawn = False

                # Draw landmarks and info
                self._draw_annotations(frame, mouth_top, mouth_bottom)

                # Prepare rows for display
        if self.yawns:
            rows = [
                ["Yawning", "Yes" if self.is_yawn else "No"],
                ["Yawn count", f"{len(self.yawns)-1}"],
                ["Yawn Dur", f"{self.yawns[-1]['duration']:.1f}s"],
            ]
        else:
            rows = [
                ["Yawning", "No"],
                ["Yawn count", "0"],
                ["Yawn Dur", "0.0s"],
            ]

        return rows, frame

    def _draw_annotations(self, frame, mouth_top, mouth_bottom):
        h, w, _ = frame.shape
        x1 = int(mouth_top.x * w)
        y1 = int(mouth_top.y * h)
        x2 = int(mouth_bottom.x * w)
        y2 = int(mouth_bottom.y * h)

        # Draw mouth line
        cv2.line(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
        cv2.circle(frame, (x1, y1), 3, (0, 0, 255), -1)
        cv2.circle(frame, (x2, y2), 3, (0, 0, 255), -1)

        # Draw yawn info
        # cv2.putText(frame, f"Total Yawns: {len(self.yawns)}", (10, 30),
        #         cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

        if self.yawn_start_time is not None:
            cv2.putText(frame, f"Yawning", (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
            progress = min((time.time() - self.yawn_start_time) / self.MIN_YAWN_DURATION, 1.0)
            cv2.rectangle(frame, (10, 50), (150, 70), (0, 0, 0), -1)
            cv2.rectangle(frame, (10, 50), (50 + int(100 * progress), 70), (0, 255, 255), -1)

        # if self.is_yawn:
        #     current_duration = time.time() - self.yawn_start_time
        #     cv2.putText(frame, f"Yawning: {current_duration:.1f}s", (10, 60),
        #             cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)

    def get_yawn_stats(self):
        total = len(self.yawns)
        durations = [y["duration"] for y in self.yawns]
        return {
            "total_yawns": total,
            "average_duration": sum(durations) / total if total > 0 else 0,
            "yawn_list": self.yawns
        }