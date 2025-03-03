# import time
# import cv2
# import numpy as np
# import mediapipe as mp
# from eye_closure_det import BlinkDetector
# from head_pose_det import HeadPoseTracker
# from yawn_detector import YawnDetector
# from utils import draw_table_grid

# def initialize_mediapipe():
#     mp_face_mesh = mp.solutions.face_mesh
#     face_mesh = mp_face_mesh.FaceMesh(
#         static_image_mode=False,
#         max_num_faces=1,
#         refine_landmarks=True
#     )
#     mp_drawing = mp.solutions.drawing_utils
#     mp_drawing_styles = mp.solutions.drawing_styles
#     return face_mesh, mp_face_mesh, mp_drawing, mp_drawing_styles

# def initialize_models(mp_face_mesh, mp_drawing, mp_drawing_styles):
#     blink_detector = BlinkDetector(mp_face_mesh, mp_drawing, mp_drawing_styles)
#     head_pose_tracker = HeadPoseTracker()
#     yawn_detector = YawnDetector()  # Initialize YawnDetector
#     return blink_detector, head_pose_tracker, yawn_detector

# def calculate_kss_score(eye_closure_data, head_pose_data, yawn_data, head_pose_angles, head_turn_history, yawn_detector, face_occluded, landmark_confidence):
#     kss_score = 1  # Start with a base KSS score of 1 (fully alert)
    
#     # --- Eye Closure & Blink Rate ---
#     blink_rate = float(eye_closure_data[2][1].split(' ')[0])  # Blink Rate
#     perclos_str = eye_closure_data[3][1].split('%')[0]
#     perclos = float(perclos_str) if perclos_str != "N/A" else 0  # PERCLOS
#     eye_closure_duration = float(eye_closure_data[4][1].split('s')[0]) if len(eye_closure_data) > 4 else 0.0

#     # Scale PERCLOS
#     if perclos >= 20:
#         kss_score += 3
#     elif perclos >= 10:
#         kss_score += 2
#     elif perclos > 0:
#         kss_score += 1

#     # Scale Blink Rate
#     if blink_rate > 30:
#         kss_score += 3
#     elif blink_rate > 20:
#         kss_score += 2
#     elif blink_rate > 0:
#         kss_score += 1

#     # --- Eye Closure Duration ---
#     if eye_closure_duration >= 1.5:
#         kss_score += 3  # Insensitive
#     elif eye_closure_duration >= 1.0:
#         kss_score += 2  # Normal
#     elif eye_closure_duration >= 0.5:
#         kss_score += 1  # Sensitive

#     # --- Head Movement ---
#     try:
#         if head_pose_data[0][1] and head_pose_data[1][1]:
#             head_horizontal = head_pose_data[0][1]
#             head_vertical = head_pose_data[1][1]
#         else:
#             head_horizontal = ""
#             head_vertical = ""
#     except:
#         head_horizontal = ""
#         head_vertical = ""

#     # --- Head Pose Thresholds ---
#     yaw = head_pose_angles[0]  # Assuming yaw is the first element
#     pitch = head_pose_angles[1]  # Assuming pitch is the second element

#     if abs(yaw) >= 45:  # Insensitive
#         kss_score += 3
#     elif abs(yaw) >= 30:  # Normal
#         kss_score += 2
#     elif abs(yaw) >= 15:  # Sensitive
#         kss_score += 1

#     if abs(pitch) >= 45:  # Insensitive
#         kss_score += 3
#     elif abs(pitch) >= 30:  # Normal
#         kss_score += 2
#     elif abs(pitch) >= 15:  # Sensitive
#         kss_score += 1

#     # --- Yawning ---
#     yawning = yawn_data[0][1]
#     yawn_count = int(yawn_data[1][1])
#     yawn_duration_str = yawn_data[2][1].split('s')[0]
#     yawn_duration = float(yawn_duration_str)

#     if yawning == "Yes":
#         kss_score += 1
#     if yawn_count > 3:
#         kss_score += 3
#     elif yawn_count > 1:
#         kss_score += 2
#     if yawn_duration > 2:
#         kss_score += 2

#     # --- Yawning Frequency ---
#     # Track yawning frequency within specific time windows
#     YAWN_TIME_WINDOW_SENSITIVE = 5 * 60  # 5 minutes in seconds
#     YAWN_TIME_WINDOW_NORMAL = 10 * 60  # 10 minutes in seconds
#     YAWN_TIME_WINDOW_INSENSITIVE = 10 * 60  # 10 minutes in seconds

#     current_time = time.time()
#     yawn_history = yawn_detector.get_yawn_stats()["yawn_list"]  # Access yawn history from yawn_detector

#     # Filter yawns within the time windows
#     yawns_in_sensitive_window = [y for y in yawn_history if current_time - y["start"] <= YAWN_TIME_WINDOW_SENSITIVE]
#     yawns_in_normal_window = [y for y in yawn_history if current_time - y["start"] <= YAWN_TIME_WINDOW_NORMAL]
#     yawns_in_insensitive_window = [y for y in yawn_history if current_time - y["start"] <= YAWN_TIME_WINDOW_INSENSITIVE]

#     # Assign KSS scores based on yawning frequency
#     if len(yawns_in_insensitive_window) >= 5 and any(y["duration"] >= 4 for y in yawns_in_insensitive_window):
#         kss_score += 3  # Insensitive
#     elif len(yawns_in_normal_window) >= 3 and any(y["duration"] >= 3 for y in yawns_in_normal_window):
#         kss_score += 2  # Normal
#     elif len(yawns_in_sensitive_window) >= 3 and any(y["duration"] >= 2 for y in yawns_in_sensitive_window):
#         kss_score += 1  # Sensitive

#     # --- Passenger Interaction (Head Turns) ---
#     YAW_THRESHOLD_LIGHT = 15
#     YAW_THRESHOLD_MODERATE = 30
#     YAW_THRESHOLD_SEVERE = 45
#     TIME_WINDOW_LIGHT = 5 * 60  # 5 minutes in seconds
#     TIME_WINDOW_MODERATE = 10 * 60  # 10 minutes in seconds
#     TIME_WINDOW_SEVERE = 10 * 60  # 10 minutes in seconds

#     current_yaw = head_pose_angles[0]  # Assuming yaw is the first element
#     current_time = time.time()
#     head_turn_history.append((current_time, current_yaw))

#     # Filter out old head turns outside the time windows
#     head_turn_history[:] = [(t, yaw) for (t, yaw) in head_turn_history if current_time - t <= max(TIME_WINDOW_LIGHT, TIME_WINDOW_MODERATE, TIME_WINDOW_SEVERE)]

#     # Count head turns within the time windows and above thresholds
#     light_turns = sum(1 for (t, yaw) in head_turn_history if current_time - t <= TIME_WINDOW_LIGHT and abs(yaw) >= YAW_THRESHOLD_LIGHT)
#     moderate_turns = sum(1 for (t, yaw) in head_turn_history if current_time - t <= TIME_WINDOW_MODERATE and abs(yaw) >= YAW_THRESHOLD_MODERATE)
#     severe_turns = sum(1 for (t, yaw) in head_turn_history if current_time - t <= TIME_WINDOW_SEVERE and abs(yaw) >= YAW_THRESHOLD_SEVERE)

#     # Assign KSS score based on head turn frequency and severity
#     if severe_turns >= 5:
#         kss_score += 3
#     elif moderate_turns >= 3:
#         kss_score += 2
#     elif light_turns >= 3:
#         kss_score += 1

#     # --- Robustness Enhancements (Eyeglasses/Masks) ---
#     if landmark_confidence < 0.7:  # Eyes may not be visible
#         if head_vertical == "Head Down":
#             kss_score += 2  # Add more KSS if head is down, likely drowsy

#     if face_occluded:
#         if head_horizontal != "Facing Forward" or head_vertical != "Facing Forward":
#             kss_score += 1
#         #Rely more on eye and head for face is occluded.
#         if (perclos >= 10 or blink_rate>20):
#             kss_score +=1

#     # Cap the KSS score
#     kss_score = min(kss_score, 9)  # KSS scale is from 1 to 9
#     return kss_score

# def process_frame(frame, face_mesh, blink_detector, head_pose_tracker, yawn_detector, calibrate_done, calibration_end_time, head_turn_history):
#     h, w = frame.shape[:2]
#     rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
#     results = face_mesh.process(rgb_frame)
#     yawn_status_, frame = yawn_detector.process_frame(frame, results)
#     eye_closure_, calibrating = blink_detector.run(frame, results)

#     if calibrating:
#         cv2.putText(frame, 'Calibrating...', (30, 30), cv2.FONT_HERSHEY_COMPLEX, 0.6, (0, 0, 255), 2)
#     elif not calibrating and not calibrate_done:
#         calibrate_done = True
#         calibration_end_time = time.time()

#     if calibrate_done and time.time() - calibration_end_time <= 2:
#         cv2.putText(frame, 'Calibration done!', (30, 30), cv2.FONT_HERSHEY_COMPLEX, 0.6, (0, 50, 170), 2)

#     # Get landmark confidence from eye closure data
#     landmark_confidence = float(eye_closure_[-1][1])  # Assuming it's the last element

#     head_pose_ = head_pose_tracker.run(results)
#     # Extract yaw, pitch, roll
#     try:
#         yaw = float(head_pose_[0][1].split(' ')[0])
#         pitch = float(head_pose_[1][1].split(' ')[0])
#         roll = float(head_pose_[2][1].split(' ')[0])
#         head_pose_angles = (yaw, pitch, roll)
#     except:
#         head_pose_angles = (0.0, 0.0, 0.0)  # default

#     # Determine if the face is occluded
#     face_occluded = len(yawn_status_) == 0 and (head_pose_[3][1] != "Facing Forward" or head_pose_[4][1] != "Facing Forward") #head_horizontal, head_vertical
#     rows = eye_closure_ + head_pose_ + yawn_status_
#     kss_score = calculate_kss_score(eye_closure_, head_pose_, yawn_status_, head_pose_angles, head_turn_history, yawn_detector, face_occluded, landmark_confidence)
#     rows.append(["KSS Score", str(kss_score)])  # Adding KSS to last row
#     return frame, rows, calibrate_done, calibration_end_time


# def display_results(frame, rows, h):
#     text_frame = np.ones((h, 400, 3), dtype=np.uint8) * 0
#     column_widths = [100, 150]
#     draw_table_grid(text_frame, 10, 20, rows, column_widths, row_height=40, padding=10,
#                     grid_color=(0, 255, 0), text_color=(255, 255, 255), thickness=2,
#                     font_scale=0.5, alignment='left')
#     combined_frame = np.hstack((frame, text_frame))

#     cv2.imshow("Facial Behaviour", combined_frame)

# def main():
#     face_mesh, mp_face_mesh, mp_drawing, mp_drawing_styles = initialize_mediapipe()
#     blink_detector, head_pose_tracker, yawn_detector = initialize_models(mp_face_mesh, mp_drawing, mp_drawing_styles)

#     cap = cv2.VideoCapture(0)
#     calibrate_done = False
#     calibration_end_time = None

#     head_turn_history = []  # Initialize head turn history outside the loop

#     while cap.isOpened():
#         ret, frame = cap.read()
#         if not ret:
#             break

#         h, w = frame.shape[:2]
#         frame, rows, calibrate_done, calibration_end_time = process_frame(
#             frame, face_mesh, blink_detector, head_pose_tracker, yawn_detector, calibrate_done, calibration_end_time, head_turn_history
#         )

#         display_results(frame, rows, h)

#         if cv2.waitKey(1) & 0xFF == ord('q'):
#             break

#     cap.release()
#     cv2.destroyAllWindows()

# if __name__ == "__main__":
#     main()


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
    face_mesh = mp_face_mesh.FaceMesh(
        static_image_mode=False,
        max_num_faces=1,
        refine_landmarks=True
    )
    mp_drawing = mp.solutions.drawing_utils
    mp_drawing_styles = mp.solutions.drawing_styles
    return face_mesh, mp_face_mesh, mp_drawing, mp_drawing_styles

def initialize_models(mp_face_mesh, mp_drawing, mp_drawing_styles):
    blink_detector = BlinkDetector(mp_face_mesh, mp_drawing, mp_drawing_styles)
    head_pose_tracker = HeadPoseTracker()
    yawn_detector = YawnDetector()  # Initialize YawnDetector
    return blink_detector, head_pose_tracker, yawn_detector

def calculate_kss_score(eye_closure_data, head_pose_data, yawn_data, head_pose_angles, head_turn_history, yawn_detector, face_occluded, landmark_confidence):
    kss_score = 1  # Start with a base KSS score of 1 (fully alert)
    
    # --- Eye Closure & Blink Rate ---
    blink_rate = float(eye_closure_data[2][1].split(' ')[0])  # Blink Rate
    perclos_str = eye_closure_data[3][1].split('%')[0]
    perclos = float(perclos_str) if perclos_str != "N/A" else 0  # PERCLOS
    eye_closure_duration = float(eye_closure_data[4][1].split('s')[0]) if len(eye_closure_data) > 4 else 0.0

    # Scale PERCLOS
    if perclos >= 20:
        kss_score += 3
    elif perclos >= 10:
        kss_score += 2
    elif perclos > 0:
        kss_score += 1

    # Scale Blink Rate
    if blink_rate > 30:
        kss_score += 3
    elif blink_rate > 20:
        kss_score += 2
    elif blink_rate > 0:
        kss_score += 1

    # --- Eye Closure Duration ---
    if eye_closure_duration >= 1.5:
        kss_score += 3  # Insensitive
    elif eye_closure_duration >= 1.0:
        kss_score += 2  # Normal
    elif eye_closure_duration >= 0.5:
        kss_score += 1  # Sensitive

    # --- Head Movement ---
    try:
        if head_pose_data[0][1] and head_pose_data[1][1]:
            head_horizontal = head_pose_data[0][1]
            head_vertical = head_pose_data[1][1]
        else:
            head_horizontal = ""
            head_vertical = ""
    except:
        head_horizontal = ""
        head_vertical = ""

    # --- Head Pose Thresholds ---
    yaw = head_pose_angles[0]  # Assuming yaw is the first element
    pitch = head_pose_angles[1]  # Assuming pitch is the second element

    if abs(yaw) >= 45:  # Insensitive
        kss_score += 3
    elif abs(yaw) >= 30:  # Normal
        kss_score += 2
    elif abs(yaw) >= 15:  # Sensitive
        kss_score += 1

    if abs(pitch) >= 45:  # Insensitive
        kss_score += 3
    elif abs(pitch) >= 30:  # Normal
        kss_score += 2
    elif abs(pitch) >= 15:  # Sensitive
        kss_score += 1

    # --- Yawning ---
    yawning = yawn_data[0][1]
    yawn_count = int(yawn_data[1][1])
    yawn_duration_str = yawn_data[2][1].split('s')[0]
    yawn_duration = float(yawn_duration_str)

    if yawning == "Yes" and yawn_duration >= 1.0:  # Only count if duration is >= 1 second
        kss_score += 1
    if yawn_count > 3 and  yawn_duration >= 1.0:  # Only count if duration is >= 1 second
        kss_score += 3
    elif yawn_count > 1 and  yawn_duration >= 1.0:  # Only count if duration is >= 1 second
        kss_score += 2
    if yawn_duration > 2:
        kss_score += 2

    # --- Yawning Frequency ---
    # Track yawning frequency within specific time windows
    YAWN_TIME_WINDOW_SENSITIVE = 5 * 60  # 5 minutes in seconds
    YAWN_TIME_WINDOW_NORMAL = 10 * 60  # 10 minutes in seconds
    YAWN_TIME_WINDOW_INSENSITIVE = 10 * 60  # 10 minutes in seconds

    current_time = time.time()
    yawn_history = yawn_detector.get_yawn_stats()["yawn_list"]  # Access yawn history from yawn_detector

    # Filter yawns within the time windows
    yawns_in_sensitive_window = [y for y in yawn_history if current_time - y["start"] <= YAWN_TIME_WINDOW_SENSITIVE]
    yawns_in_normal_window = [y for y in yawn_history if current_time - y["start"] <= YAWN_TIME_WINDOW_NORMAL]
    yawns_in_insensitive_window = [y for y in yawn_history if current_time - y["start"] <= YAWN_TIME_WINDOW_INSENSITIVE]

    # Assign KSS scores based on yawning frequency
    if len(yawns_in_insensitive_window) >= 5 and all(y["duration"] >= 1.0 for y in yawns_in_insensitive_window) and any(y["duration"] >= 4 for y in yawns_in_insensitive_window): #added all for minimum time count
        kss_score += 3  # Insensitive
    elif len(yawns_in_normal_window) >= 3 and all(y["duration"] >= 1.0 for y in yawns_in_normal_window) and any(y["duration"] >= 3 for y in yawns_in_normal_window):#added all for minimum time count
        kss_score += 2  # Normal
    elif len(yawns_in_sensitive_window) >= 3 and all(y["duration"] >= 1.0 for y in yawns_in_sensitive_window) and any(y["duration"] >= 2 for y in yawns_in_sensitive_window):#added all for minimum time count
        kss_score += 1  # Sensitive

    # --- Passenger Interaction (Head Turns) ---
    YAW_THRESHOLD_LIGHT = 15
    YAW_THRESHOLD_MODERATE = 30
    YAW_THRESHOLD_SEVERE = 45
    TIME_WINDOW_LIGHT = 5 * 60  # 5 minutes in seconds
    TIME_WINDOW_MODERATE = 10 * 60  # 10 minutes in seconds
    TIME_WINDOW_SEVERE = 10 * 60  # 10 minutes in seconds

    current_yaw = head_pose_angles[0]  # Assuming yaw is the first element
    current_time = time.time()
    head_turn_history.append((current_time, current_yaw))

    # Filter out old head turns outside the time windows
    head_turn_history[:] = [(t, yaw) for (t, yaw) in head_turn_history if current_time - t <= max(TIME_WINDOW_LIGHT, TIME_WINDOW_MODERATE, TIME_WINDOW_SEVERE)]

    # Count head turns within the time windows and above thresholds
    light_turns = sum(1 for (t, yaw) in head_turn_history if current_time - t <= TIME_WINDOW_LIGHT and abs(yaw) >= YAW_THRESHOLD_LIGHT)
    moderate_turns = sum(1 for (t, yaw) in head_turn_history if current_time - t <= TIME_WINDOW_MODERATE and abs(yaw) >= YAW_THRESHOLD_MODERATE)
    severe_turns = sum(1 for (t, yaw) in head_turn_history if current_time - t <= TIME_WINDOW_SEVERE and abs(yaw) >= YAW_THRESHOLD_SEVERE)

    # Assign KSS score based on head turn frequency and severity
    if severe_turns >= 5:
        kss_score += 3
    elif moderate_turns >= 3:
        kss_score += 2
    elif light_turns >= 3:
        kss_score += 1

    # --- Robustness Enhancements (Eyeglasses/Masks) ---
    if landmark_confidence < 0.7:  # Eyes may not be visible
        if head_vertical == "Head Down":
            kss_score += 2  # Add more KSS if head is down, likely drowsy

    if face_occluded:
        if head_horizontal != "Facing Forward" or head_vertical != "Facing Forward":
            kss_score += 1
        #Rely more on eye and head for face is occluded.
        if (perclos >= 10 or blink_rate>20):
            kss_score +=1

    # Cap the KSS score
    kss_score = min(kss_score, 9)  # KSS scale is from 1 to 9
    return kss_score

def process_frame(frame, face_mesh, blink_detector, head_pose_tracker, yawn_detector, calibrate_done, calibration_end_time, head_turn_history):
    h, w = frame.shape[:2]
    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = face_mesh.process(rgb_frame)
    yawn_status_, frame = yawn_detector.process_frame(frame, results)
    eye_closure_, calibrating = blink_detector.run(frame, results)

    # Get landmark confidence from eye closure data
    landmark_confidence = float(eye_closure_[-1][1])  # Assuming it's the last element

    head_pose_ = head_pose_tracker.run(results)
    # Extract yaw, pitch, roll
    try:
        yaw = float(head_pose_[0][1].split(' ')[0])
        pitch = float(head_pose_[1][1].split(' ')[0])
        roll = float(head_pose_[2][1].split(' ')[0])
        head_pose_angles = (yaw, pitch, roll)
    except:
        head_pose_angles = (0.0, 0.0, 0.0)  # default

    # Determine if the face is occluded
    face_occluded = len(yawn_status_) == 0 and (head_pose_[3][1] != "Facing Forward" or head_pose_[4][1] != "Facing Forward") #head_horizontal, head_vertical
    rows = eye_closure_ + head_pose_ + yawn_status_
    kss_score = calculate_kss_score(eye_closure_, head_pose_, yawn_status_, head_pose_angles, head_turn_history, yawn_detector, face_occluded, landmark_confidence)
    rows.append(["KSS Score", str(kss_score)])  # Adding KSS to last row
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

    head_turn_history = []  # Initialize head turn history outside the loop

    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            break

        h, w = frame.shape[:2]
        frame, rows, calibrate_done, calibration_end_time = process_frame(
            frame, face_mesh, blink_detector, head_pose_tracker, yawn_detector, calibrate_done, calibration_end_time, head_turn_history
        )

        display_results(frame, rows, h)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()