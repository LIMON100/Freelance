# #eye_closure_det.py

# import cv2
# import mediapipe as mp
# import numpy as np
# import time
# from collections import deque

# class BlinkDetector:
#     def __init__(self, face_mesh, mp_drawing, mp_drawing_styles,  calibration_frames=50):
#         # Define eye landmark indices for EAR calculation
#         self.LEFT_EYE_POINTS = [33, 160, 158, 133, 153, 144]
#         self.RIGHT_EYE_POINTS = [362, 385, 387, 263, 380, 373]

#         self.mp_face_mesh = face_mesh
#         self.mp_drawing = mp_drawing
#         self.mp_drawing_styles = mp_drawing_styles

#         # Blink tracking variables
#         self.blink_counter = 0
#         self.eye_closed = 0
#         self.ear_history = deque(maxlen=5)

#         # Blink duration and speed tracking
#         self.blink_durations = []
#         self.blink_speeds = []
#         self.blink_start_time = None
#         self.blink_rate = 0
#         self.last_blink_time = time.time()
#         self.blink_timestamps = deque(maxlen=5)

#         # PERCLOS variables
#         self.PERCLOS_WINDOW = 60
#         self.eye_closed_time = 0
#         self.perclos_start_time = time.time()
#         self.perclos_durations = []

#         # Calibration variables
#         self.calibrating = True
#         self.calibration_frames = calibration_frames
#         self.open_eye_ears = []
#         self.BLINK_THRESHOLD = 0.30  # Initial threshold
#         self.CLOSED_FRAMES = 2
#         self.frame_count = 0

#         # Optional blink speed thresholds (not used in current logic)
#         self.FAST_BLINK_THRESHOLD = 4.0
#         self.SLOW_BLINK_THRESHOLD = 1.0

#     def calculate_ear(self, landmarks, eye_points, w, h):
#         """Calculate the Eye Aspect Ratio (EAR) for an eye."""
#         try:
#             # Extract the six landmarks for the eye and convert to pixel coordinates
#             points = [landmarks.landmark[i] for i in eye_points]
#             coords = [(int(p.x * w), int(p.y * h)) for p in points]
#             p1, p2, p3, p4, p5, p6 = coords

#             # Calculate distances
#             vertical_1 = np.linalg.norm(np.array(p2) - np.array(p6))
#             vertical_2 = np.linalg.norm(np.array(p3) - np.array(p5))
#             horizontal = np.linalg.norm(np.array(p1) - np.array(p4))

#             ear = (vertical_1 + vertical_2) / (2.0 * horizontal)
#             return ear
#         except Exception as e:
#             return None

#     def draw_table_grid(self, img, x_offset, y_offset, rows, column_widths, row_height=30, padding=5,
#                         grid_color=(0, 255, 0), text_color=(0, 0, 0), thickness=1, font_scale=0.3,
#                         font=cv2.FONT_HERSHEY_SIMPLEX, alignment='left'):
#         for i in range(len(rows) + 1):
#             cv2.line(img, (x_offset, y_offset + i * row_height),
#                      (x_offset + sum(column_widths), y_offset + i * row_height), grid_color, thickness)
#         for i in range(len(column_widths) + 1):
#             x = x_offset + sum(column_widths[:i])
#             cv2.line(img, (x, y_offset), (x, y_offset + len(rows) * row_height), grid_color, thickness)

#         for i, row in enumerate(rows):
#             for j, cell_text in enumerate(row):
#                 x = x_offset + sum(column_widths[:j]) + padding
#                 y = y_offset + i * row_height + row_height - padding
#                 (text_width, text_height), _ = cv2.getTextSize(cell_text, font, font_scale, thickness)

#                 if alignment == 'left':
#                     text_x = x
#                 elif alignment == 'center':
#                     text_x = x + (column_widths[j] - text_width) // 2
#                 elif alignment == 'right':
#                     text_x = x + column_widths[j] - text_width - padding
#                 else:
#                     raise ValueError("Alignment must be 'left', 'center', or 'right'")

#                 text_y = y - (row_height - text_height) // 2
#                 cv2.putText(img, cell_text, (text_x, text_y + 10), font, font_scale, text_color, thickness)

#     def run(self, frame, results):
#         h, w = frame.shape[:2]

#         if results.multi_face_landmarks:
#             for face_landmarks in results.multi_face_landmarks:
#                 self.mp_drawing.draw_landmarks(
#                     image=frame,
#                     landmark_list=face_landmarks,
#                     connections=self.mp_face_mesh.FACEMESH_TESSELATION,
#                     landmark_drawing_spec=None,
#                     connection_drawing_spec=self.mp_drawing_styles.get_default_face_mesh_tesselation_style()
#                 )

#                 left_ear = self.calculate_ear(face_landmarks, self.LEFT_EYE_POINTS, w, h)
#                 right_ear = self.calculate_ear(face_landmarks, self.RIGHT_EYE_POINTS, w, h)
#                 if left_ear is None or right_ear is None:
#                     continue

#                 avg_ear = (left_ear + right_ear) / 2.0

#                 if self.calibrating:
#                     if avg_ear < 0.20:
#                         continue
#                     self.open_eye_ears.append(avg_ear)
#                     #print('calibrating')
#                     self.frame_count += 1

#                     if self.frame_count >= self.calibration_frames:
#                         mean_open_eye_ear = np.mean(self.open_eye_ears)
#                         self.BLINK_THRESHOLD = mean_open_eye_ear * 0.80
#                         self.calibrating = False
#                         print(f"Calibration done! New blink threshold: {self.BLINK_THRESHOLD:.2f}")
#                 else:
#                     self.ear_history.append(avg_ear)
#                     smoothed_ear = np.mean(self.ear_history)

#                     # Blink detection logic
#                     if smoothed_ear < self.BLINK_THRESHOLD:
#                         self.eye_closed_time += (1 / 30)
#                         if self.eye_closed == 0:
#                             self.blink_start_time = time.time()
#                         self.eye_closed += 1
#                     else:
#                         if self.eye_closed >= self.CLOSED_FRAMES:
#                             self.blink_counter += 1
#                             blink_duration = time.time() - self.blink_start_time
#                             self.blink_durations.append(blink_duration)

#                             # Compute blink speed (blinks per second)
#                             blink_speed = 1 / blink_duration if blink_duration > 0 else 0
#                             self.blink_speeds.append(blink_speed)

#                             current_time = time.time()
#                             self.blink_timestamps.append(current_time)

#                             if len(self.blink_timestamps) > 1:
#                                 avg_interval = np.mean(np.diff(self.blink_timestamps))
#                                 self.blink_rate = 60 / avg_interval if avg_interval > 0 else 0

#                         self.eye_closed = 0

#                     current_time = time.time()
#                     if current_time - self.perclos_start_time >= 1:
#                         total_time = current_time - self.perclos_start_time
#                         perclos = (self.eye_closed_time / total_time) * 100
#                         self.perclos_durations.append(perclos)
#                         self.eye_closed_time = 0
#                         self.perclos_start_time = current_time


#         rows = [
#             ["Blinks", str(self.blink_counter)],
#             ["Last Blink", f"{self.blink_durations[-1]:.2f}s" if self.blink_durations else "N/A"],
#             ["Blink Rate", f"{self.blink_rate:.2f} BPM"],
#             ["PERCLOS", f"{self.perclos_durations[-1]:.2f}%" if self.perclos_durations else "N/A"]
#         ]

#         return rows, self.calibrating


import cv2
import mediapipe as mp
import numpy as np
import time
from collections import deque

class BlinkDetector:
    def __init__(self, face_mesh, mp_drawing, mp_drawing_styles, calibration_frames=50):
        # Define eye landmark indices for EAR calculation
        self.LEFT_EYE_POINTS = [33, 160, 158, 133, 153, 144]
        self.RIGHT_EYE_POINTS = [362, 385, 387, 263, 380, 373]

        self.mp_face_mesh = face_mesh
        self.mp_drawing = mp_drawing
        self.mp_drawing_styles = mp_drawing_styles

        # Blink tracking variables
        self.blink_counter = 0
        self.eye_closed = 0
        self.ear_history = deque(maxlen=5)

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
        self.BLINK_THRESHOLD_PERCENTAGE = 0.80  # Adjust this
        self.BLINK_THRESHOLD_MIN = 0.15          # Minimum threshold
        self.CLOSED_FRAMES = 2
        self.frame_count = 0
        self.previous_ear = None

        # Optional blink speed thresholds (not used in current logic)
        self.FAST_BLINK_THRESHOLD = 4.0
        self.SLOW_BLINK_THRESHOLD = 1.0


        self.BLINK_THRESHOLD = 0.30 
    

    def calculate_ear(self, landmarks, eye_points, w, h):
        """Calculate the Eye Aspect Ratio (EAR) for an eye."""
        try:
            # Extract the six landmarks for the eye and convert to pixel coordinates
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

                left_ear = self.calculate_ear(face_landmarks, self.LEFT_EYE_POINTS, w, h)
                right_ear = self.calculate_ear(face_landmarks, self.RIGHT_EYE_POINTS, w, h)
                if left_ear is None or right_ear is None:
                    continue

                avg_ear = (left_ear + right_ear) / 2.0

                if self.calibrating:
                    if avg_ear < 0.20:
                        continue
                    self.open_eye_ears.append(avg_ear)
                    #print('calibrating')
                    self.frame_count += 1

                    if self.frame_count >= self.calibration_frames:
                        mean_open_eye_ear = np.mean(self.open_eye_ears)
                        # Get percentage of mean
                        percentage_threshold = mean_open_eye_ear * self.BLINK_THRESHOLD_PERCENTAGE
                        #Get highest
                        self.BLINK_THRESHOLD = max(self.BLINK_THRESHOLD_MIN, percentage_threshold)
                        self.calibrating = False
                        print(f"Calibration done! New blink threshold: {self.BLINK_THRESHOLD:.2f}")
                else:
                    self.ear_history.append(avg_ear)
                    smoothed_ear = np.mean(self.ear_history)

                    # Blink detection logic
                    if smoothed_ear < self.BLINK_THRESHOLD:
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


        rows = [
            ["Blinks", str(self.blink_counter)],
            ["Last Blink", f"{self.blink_durations[-1]:.2f}s" if self.blink_durations else "N/A"],
            ["Blink Rate", f"{self.blink_rate:.2f} BPM"],
            ["PERCLOS", f"{self.perclos_durations[-1]:.2f}%" if self.perclos_durations else "N/A"],
            ["Blink Thresh",f"{self.BLINK_THRESHOLD:.2f}"] #Added threshold value
        ]

        return rows, self.calibrating