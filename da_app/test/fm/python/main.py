import time
import cv2
import numpy as np
import mediapipe as mp
from eye_closure_det import BlinkDetector
from head_pose_det import HeadPoseTracker
from yawn_detector import YawnDetector
from utils import draw_table_grid

def initialize_mediapipe():
    mp_face_mesh = mp.solutions.face_mesh
    # face_mesh = mp_face_mesh.FaceMesh(
    #     static_image_mode=False,
    #     max_num_faces=1,
    #     refine_landmarks=True
    # )
    face_mesh = mp_face_mesh.FaceMesh(
        static_image_mode=False,
        max_num_faces=1,
        refine_landmarks=True,  
        min_detection_confidence=0.3, 
        min_tracking_confidence=0.3
    )
    mp_drawing = mp.solutions.drawing_utils
    mp_drawing_styles = mp.solutions.drawing_styles
    return face_mesh, mp_face_mesh, mp_drawing, mp_drawing_styles

def initialize_models(mp_face_mesh, mp_drawing, mp_drawing_styles):
    blink_detector = BlinkDetector(mp_face_mesh, mp_drawing, mp_drawing_styles)
    head_pose_tracker = HeadPoseTracker()
    yawn_detector = YawnDetector()  # Initialize YawnDetector
    return blink_detector, head_pose_tracker, yawn_detector

def process_frame(frame, face_mesh, blink_detector, head_pose_tracker, yawn_detector, calibrate_done, calibration_end_time):
    h, w = frame.shape[:2]
    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = face_mesh.process(rgb_frame)
    yawn_status_, frame = yawn_detector.process_frame(frame, results)

    eye_closure_, calibrating = blink_detector.run(frame, results)

    if calibrating:
        cv2.putText(frame, 'Calibrating...', (30, 30), cv2.FONT_HERSHEY_COMPLEX, 0.6, (0, 0, 255), 2)
    elif not calibrating and not calibrate_done:
        calibrate_done = True
        calibration_end_time = time.time()

    if calibrate_done and time.time() - calibration_end_time <= 2:
        cv2.putText(frame, 'Calibration done!', (30, 30), cv2.FONT_HERSHEY_COMPLEX, 0.6, (0, 50, 170), 2)

    head_pose_ = head_pose_tracker.run(results)

    rows = eye_closure_ + head_pose_ + yawn_status_
    return frame, rows, calibrate_done, calibration_end_time


def display_results(frame, rows, h):
    text_frame = np.ones((h, 400, 3), dtype=np.uint8) * 0
    column_widths = [100, 150]
    draw_table_grid(text_frame, 10, 20, rows, column_widths, row_height=40, padding=10,
                    grid_color=(0, 255, 0), text_color=(255, 255, 255), thickness=2,
                    font_scale=0.5, alignment='left')
    combined_frame = np.hstack((frame, text_frame))

    cv2.imshow("Facial Behaviour", combined_frame)

def main():
    face_mesh, mp_face_mesh, mp_drawing, mp_drawing_styles = initialize_mediapipe()
    blink_detector, head_pose_tracker, yawn_detector = initialize_models(mp_face_mesh, mp_drawing, mp_drawing_styles)

    cap = cv2.VideoCapture(0)
    calibrate_done = False
    calibration_end_time = None

    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            break

        h, w = frame.shape[:2]
        frame, rows, calibrate_done, calibration_end_time = process_frame(
            frame, face_mesh, blink_detector, head_pose_tracker, yawn_detector, calibrate_done, calibration_end_time
        )

        display_results(frame, rows, h)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()