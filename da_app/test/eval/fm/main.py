import os
import time
import cv2
import numpy as np
import mediapipe as mp
from eye_closure_det import BlinkDetector
from head_pose_det import HeadPoseTracker
from yawn_detector import YawnDetector
from utils import draw_table_grid, scale_up_frame, sharpen_image

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


def process_image(image_path, face_mesh, blink_detector, head_pose_tracker, yawn_detector):
    try:
        frame = cv2.imread(image_path)
        if frame is None:
            raise ValueError(f"Error: Could not read image: {image_path}")

        h, w = frame.shape[:2]
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = face_mesh.process(rgb_frame)
        output_data = {}
        overlay_text = [] # List to store text for image overlay

        if results.multi_face_landmarks:
            yawn_status, frame = yawn_detector.process_frame(frame.copy(), results)
            eye_closure, eye_state, _ = blink_detector.run(frame.copy(), results)  # Get eye_state from BlinkDetector
            head_pose = head_pose_tracker.run(results)

            blink_detector.draw_table_grid(frame, 10, 20, eye_closure, [100, 150], thickness=1,font_scale=0.4)

            #eye_state = "open" if not any(item[1] == 'Yes' for item in eye_closure if item[0] in ('Eye_left', 'Eye_right')) else "closed" # REMOVE this line and use value from BlinkDetector
            mouth_state = "open" if any(item[1] == 'Yes' for item in yawn_status if item[0] == 'Yawning') else "closed"

            output_data["eye"] = eye_state
            output_data["mouth"] = mouth_state
            output_data.update(dict(head_pose))
            output_data.update(dict(yawn_status))

            # Append text to the overlay list
            overlay_text.append(f"Eye: {eye_state}")
            overlay_text.append(f"Mouth: {mouth_state}")

            # Extract head pose data and append to overlay
            head_pose_data = dict(head_pose) if head_pose else {} # create dict if head_pose is not None, also fix crash
            for k,v in head_pose_data.items():
                overlay_text.append(f"{k}:{v}")
            # Extract yawn status data and append to overlay
            yawn_status_data = dict(yawn_status) if yawn_status else {} # create dict if yawn_status is not None, also fix crash
            for k,v in yawn_status_data.items():
                overlay_text.append(f"{k}:{v}")

        else:
            print(f"Warning: No face detected in image: {image_path}")
            output_data["face_detected"] = False
            overlay_text.append("No face detected") # If no face is detected


        # Add overlay to the image
        y_offset = 150 # Start position of the text
        for i, text in enumerate(overlay_text):
            cv2.putText(frame, text, (10, y_offset + i * 20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1, cv2.LINE_AA)



        return output_data, frame


    except Exception as e:
        print(f"Error processing image {image_path}: {e}")
        return None, None

def main():
    image_folder = "/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/test_dataset/output_g/images/"
    output_folder = "output_images"

    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    face_mesh, mp_face_mesh, mp_drawing, mp_drawing_styles = initialize_mediapipe()
    blink_detector, head_pose_tracker, yawn_detector = initialize_models(mp_face_mesh, mp_drawing, mp_drawing_styles)

    for filename in os.listdir(image_folder):
        if filename.lower().endswith(('.png', '.jpg', '.jpeg')):
            image_path = os.path.join(image_folder, filename)
            output_data, frame = process_image(
                image_path, face_mesh, blink_detector, head_pose_tracker, yawn_detector)

            if frame is not None: # process the frame only it is valid.
                base_filename, _ = os.path.splitext(filename)
                output_text_file = os.path.join(output_folder, f"{base_filename}.txt")
                output_image_path = os.path.join(output_folder, filename)

                with open(output_text_file, "w") as outfile:
                    output_lines = [f"{key}: {value}" for key, value in output_data.items()]
                    outfile.write("\n".join(output_lines) + "\n")

                cv2.imwrite(output_image_path, frame)

    print(f"Results saved to {output_folder}")

if __name__ == "__main__":
    main()
