#head_pose_det.py

import cv2
import mediapipe as mp
import numpy as np

from utils import draw_table_grid

class HeadPoseTracker:
    def __init__(self):
        self.mp_face_mesh = mp.solutions.face_mesh
        self.face_mesh = self.mp_face_mesh.FaceMesh(static_image_mode=False, max_num_faces=1, refine_landmarks=True)

        # Landmark indices
        self.LANDMARK_INDICES = {
            "left_eye": 33,
            "right_eye": 263,
            "nose_tip": 1,
            "mouth_center": 13,
            "forehead": 10
        }

        # Calibration parameters
        self.SMOOTHING_FACTOR = 0.2
        self.MIN_SCALE = 0.05
        self.MAX_SCALE = 0.2

        # Calibration state
        self.calibration_state = {
            "reference_set": False,
            "reference_origin": np.zeros(3),
            "reference_rotation": np.eye(3),
            "rotation": np.eye(3)
        }

    def calculate_axes(self, landmarks):
        left_eye = np.array(landmarks["left_eye"], dtype=float)
        right_eye = np.array(landmarks["right_eye"], dtype=float)
        forehead = np.array(landmarks["forehead"], dtype=float)
        mouth = np.array(landmarks["mouth_center"], dtype=float)

        z_axis = forehead - mouth
        z_axis /= np.linalg.norm(z_axis)

        x_axis = right_eye - left_eye
        x_axis /= np.linalg.norm(x_axis)

        y_axis = np.cross(z_axis, x_axis)
        y_axis /= np.linalg.norm(y_axis)

        x_axis = np.cross(y_axis, z_axis)
        x_axis /= np.linalg.norm(x_axis)

        return np.column_stack((x_axis, y_axis, z_axis))

    def auto_calibrate(self, landmarks):
        if not self.calibration_state["reference_set"]:
            self.calibration_state["reference_origin"] = np.array(landmarks["nose_tip"], dtype=float)
            self.calibration_state["reference_rotation"] = self.calculate_axes(landmarks)
            self.calibration_state["reference_set"] = True
            return

        current_rot = self.calculate_axes(landmarks)
        R_rel = current_rot @ self.calibration_state["reference_rotation"].T

        R_temp = (1 - self.SMOOTHING_FACTOR) * self.calibration_state["rotation"] + self.SMOOTHING_FACTOR * R_rel
        U, _, Vt = np.linalg.svd(R_temp)
        self.calibration_state["rotation"] = U @ Vt

        eye_dist = np.linalg.norm(np.array(landmarks["right_eye"]) - np.array(landmarks["left_eye"]))
        self.calibration_state["scale"] = np.clip(eye_dist * 1.5, self.MIN_SCALE, self.MAX_SCALE)

    def rotation_matrix_to_euler_angles(self, R):
        sy = np.sqrt(R[0, 0]**2 + R[1, 0]**2)
        singular = sy < 1e-6

        if not singular:
            yaw = np.arctan2(R[1, 0], R[0, 0])
            pitch = np.arctan2(-R[2, 0], sy)
            roll = np.arctan2(R[2, 1], R[2, 2])
        else:
            yaw = np.arctan2(-R[1, 2], R[1, 1])
            pitch = np.arctan2(-R[2, 0], sy)
            roll = 0

        return np.degrees(yaw), np.degrees(pitch), np.degrees(roll)


    def run(self, results):
        # Initialize rows with a default value (empty list or "No Face Detected")
        rows = []
        if results.multi_face_landmarks:
            face = results.multi_face_landmarks[0]
            landmarks = {}
            for name, idx in self.LANDMARK_INDICES.items():
                lm = face.landmark[idx]
                landmarks[name] = [lm.x, lm.y, lm.z]

            self.auto_calibrate(landmarks)
            roll, yaw, pitch = self.rotation_matrix_to_euler_angles(self.calibration_state["rotation"].T)

            roll = round(roll, 1)
            yaw = round(yaw, 1)
            pitch = round(pitch, 1)

            column_widths = [100, 150]
            rows = [
                ["Yaw", f"{yaw:.1f} deg"],
                ["Pitch", f"{pitch:.1f} deg"],
                ["Roll", f"{roll:.1f} deg"]
            ]
        return rows