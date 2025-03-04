import cv2
import mediapipe as mp
import numpy as np
import time
from collections import deque
import math

class BlinkDetector:
    def __init__(self, face_mesh, mp_drawing, mp_drawing_styles, calibration_frames=50):
        self.LEFT_EYE_POINTS = [33, 160, 158, 133, 153, 144]  # Left eye landmarks
        self.RIGHT_EYE_POINTS = [362, 385, 387, 263, 380, 373]  # Right eye landmarks

        self.LEFT_IRIS = [474, 475, 476, 477]  # Left iris landmarks
        self.RIGHT_IRIS = [469, 470, 471, 472]  # Right iris landmarks

        self.mp_face_mesh = face_mesh
        self.mp_drawing = mp_drawing
        self.mp_drawing_styles = mp_drawing_styles

        # Blink tracking variables
        self.blink_counter = 0
        self.eye_closed = 0
        self.ear_history = deque(maxlen=5)  # Moving average for EAR

        # Blink duration and speed tracking
        self.blink_durations = []
        self.blink_speeds = []
        self.blink_start_time = None
        self.blink_rate = 0
        self.last_blink_time = time.time()
        self.blink_timestamps = deque(maxlen=5)

        # PERCLOS variables
        self.PERCLOS_WINDOW = 60
        self.eye_closed_time = 0
        self.perclos_start_time = time.time()
        self.perclos_durations = []

        # Calibration variables
        self.calibrating = True
        self.calibration_frames = calibration_frames
        self.open_eye_ears = []
        self.BLINK_THRESHOLD = 0.30  # Initial threshold
        self.CLOSED_FRAMES = 2
        self.frame_count = 0

        # Iris visibility threshold 
        self.IRIS_THRESHOLD = 0.55  # If iris visibility is below this, eyes are likely closed

    def calculate_ear(self, landmarks, eye_points, w, h):
        """Calculate the Eye Aspect Ratio (EAR) for an eye."""
        try:
            # Extract the landmarks for the eye and convert to pixel coordinates
            points = [landmarks.landmark[i] for i in eye_points]
            coords = [(int(p.x * w), int(p.y * h)) for p in points]
            p1, p2, p3, p4, p5, p6 = coords

            # Calculate distances
            vertical_1 = np.linalg.norm(np.array(p2) - np.array(p6))
            vertical_2 = np.linalg.norm(np.array(p3) - np.array(p5))
            horizontal = np.linalg.norm(np.array(p1) - np.array(p4))

            ear = (vertical_1 + vertical_2) / (2.0 * horizontal)
            return ear
        except Exception as e:
            print(f"Error calculating EAR: {e}")
            return None
        
    
    def draw_table_grid(self, img, x_offset, y_offset, rows, column_widths, row_height=30, padding=5, 
                        grid_color=(0, 255, 0), text_color=(0, 0, 0), thickness=1, font_scale=0.3, 
                        font=cv2.FONT_HERSHEY_SIMPLEX, alignment='left'):
        for i in range(len(rows) + 1):
            cv2.line(img, (x_offset, y_offset + i * row_height),
                     (x_offset + sum(column_widths), y_offset + i * row_height), grid_color, thickness)
        for i in range(len(column_widths) + 1):
            x = x_offset + sum(column_widths[:i])
            cv2.line(img, (x, y_offset), (x, y_offset + len(rows) * row_height), grid_color, thickness)

        for i, row in enumerate(rows):
            for j, cell_text in enumerate(row):
                x = x_offset + sum(column_widths[:j]) + padding
                y = y_offset + i * row_height + row_height - padding
                (text_width, text_height), _ = cv2.getTextSize(cell_text, font, font_scale, thickness)
                
                if alignment == 'left':
                    text_x = x
                elif alignment == 'center':
                    text_x = x + (column_widths[j] - text_width) // 2
                elif alignment == 'right':
                    text_x = x + column_widths[j] - text_width - padding
                else:
                    raise ValueError("Alignment must be 'left', 'center', or 'right'")
                
                text_y = y - (row_height - text_height) // 2
                cv2.putText(img, cell_text, (text_x, text_y + 10), font, font_scale, text_color, thickness)

    def calculate_iris_visibility(self, landmarks, iris_points, eye_points, w, h):
        """Calculate the visibility ratio of the iris."""
        try:
            # Create a mask for the eye region
            eye_mask = np.zeros((h, w), dtype=np.uint8)
            eye_contour = np.array([(int(landmarks.landmark[i].x * w), int(landmarks.landmark[i].y * h)) for i in eye_points])
            cv2.fillPoly(eye_mask, [eye_contour], 1)

            # Create a mask for the iris region
            iris_mask = np.zeros((h, w), dtype=np.uint8)
            iris_center = np.mean([(int(landmarks.landmark[i].x * w), int(landmarks.landmark[i].y * h)) for i in iris_points], axis=0)
            iris_radius = int(np.linalg.norm(np.array(iris_center) - np.array((int(landmarks.landmark[iris_points[0]].x * w), int(landmarks.landmark[iris_points[0]].y * h)))))
            cv2.circle(iris_mask, tuple(map(int, iris_center)), iris_radius, 1, -1)

            # Calculate visibility ratio
            visibility_ratio = np.sum(cv2.bitwise_and(eye_mask, iris_mask)) / np.sum(iris_mask)
            return visibility_ratio
        except Exception as e:
            print(f"Error calculating iris visibility: {e}")
            return None

    def run(self, frame, results):
        h, w = frame.shape[:2]

        if results.multi_face_landmarks:
            for face_landmarks in results.multi_face_landmarks:
                self.mp_drawing.draw_landmarks(
                    image=frame,
                    landmark_list=face_landmarks,
                    connections=self.mp_face_mesh.FACEMESH_TESSELATION,
                    landmark_drawing_spec=None,
                    connection_drawing_spec=self.mp_drawing_styles.get_default_face_mesh_tesselation_style()
                )

                # Calculate EAR for left and right eyes
                left_ear = self.calculate_ear(face_landmarks, self.LEFT_EYE_POINTS, w, h)
                right_ear = self.calculate_ear(face_landmarks, self.RIGHT_EYE_POINTS, w, h)

                # Calculate iris visibility for left and right eyes
                left_iris_visibility = self.calculate_iris_visibility(face_landmarks, self.LEFT_IRIS, self.LEFT_EYE_POINTS, w, h)
                right_iris_visibility = self.calculate_iris_visibility(face_landmarks, self.RIGHT_IRIS, self.RIGHT_EYE_POINTS, w, h)

                if left_ear is None or right_ear is None or left_iris_visibility is None or right_iris_visibility is None:
                    continue

                # Combine EAR values (average)
                avg_ear = (left_ear + right_ear) / 2.0

                # Check iris visibility
                iris_visibility = (left_iris_visibility + right_iris_visibility) / 2.0

                if self.calibrating:
                    landmark_confidence = 1.0 #initiate landmark confidence
                    if avg_ear < 0.20:
                        continue
                    self.open_eye_ears.append(avg_ear)
                    self.frame_count += 1

                    if self.frame_count >= self.calibration_frames:
                        mean_open_eye_ear = np.mean(self.open_eye_ears)
                        self.BLINK_THRESHOLD = mean_open_eye_ear * 0.80
                        self.calibrating = False
                        print(f"Calibration done! New blink threshold: {self.BLINK_THRESHOLD:.2f}")
                else:
                    landmark_confidence = 1.0 #initiate landmark confidence
                    self.ear_history.append(avg_ear)
                    smoothed_ear = np.mean(self.ear_history)

                    # Blink detection logic (combine EAR and iris visibility)
                    if smoothed_ear < self.BLINK_THRESHOLD and iris_visibility < self.IRIS_THRESHOLD:
                        self.eye_closed_time += (1 / 30)
                        if self.eye_closed == 0:
                            self.blink_start_time = time.time()
                        self.eye_closed += 1
                    else:
                        if self.eye_closed >= self.CLOSED_FRAMES:
                            self.blink_counter += 1
                            blink_duration = time.time() - self.blink_start_time
                            self.blink_durations.append(blink_duration)

                            # Compute blink speed (blinks per second)
                            blink_speed = 1 / blink_duration if blink_duration > 0 else 0
                            self.blink_speeds.append(blink_speed)

                            current_time = time.time()
                            self.blink_timestamps.append(current_time)

                            if len(self.blink_timestamps) > 1:
                                avg_interval = np.mean(np.diff(self.blink_timestamps))
                                self.blink_rate = 60 / avg_interval if avg_interval > 0 else 0

                        self.eye_closed = 0

                    current_time = time.time()
                    if current_time - self.perclos_start_time >= 1:
                        total_time = current_time - self.perclos_start_time
                        perclos = (self.eye_closed_time / total_time) * 100
                        self.perclos_durations.append(perclos)
                        self.eye_closed_time = 0
                        self.perclos_start_time = current_time

                if left_ear is None or right_ear is None:
                    landmark_confidence = 0.5  # Reduce if EAR is unreliable
                if iris_visibility < 0.3:  #arbitrary amount
                    landmark_confidence *= 0.7  # Further reduce if iris is hard to see

        rows = [
            ["Blinks", str(self.blink_counter)],
            ["Last Blink", f"{self.blink_durations[-1]:.2f}s" if self.blink_durations else "N/A"],
            ["Blink Rate", f"{self.blink_rate:.2f} BPM"],
            ["PERCLOS", f"{self.perclos_durations[-1]:.2f}%" if self.perclos_durations else "N/A"],
            ["EyeC Dur", f"{self.eye_closed_time:.2f}"], 
        ]

        return rows, self.calibrating