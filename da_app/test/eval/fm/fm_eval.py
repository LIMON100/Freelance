# import os
# import time
# import cv2
# import numpy as np
# import mediapipe as mp
# from eye_closure_det import BlinkDetector
# from head_pose_det import HeadPoseTracker
# from yawn_detector import YawnDetector
# from utils import draw_table_grid, scale_up_frame, sharpen_image

# # Ground truth for each folder
# GROUND_TRUTH = {
#     "Folder-1": {
#         "Eyes_open": 120,
#         "Eyes_closed": 60,
#         "Mouth_open": 100,
#         "Mouth_closed": 50
#     },
#     "Folder-2": {
#         "Head_left": 11,
#         "Head_right": 6
#     },
#     "Folder-3": {
#         "Yawn": 16
#     },
#     "Folder-4": {
#         "Head_left": 11,
#         "Head_right": 6
#     },
# }

# # Initialize MediaPipe and models
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

# # Process an image and return output data and annotated frame
# def process_image(image_path, face_mesh, blink_detector, head_pose_tracker, yawn_detector):
#     try:
#         frame = cv2.imread(image_path)
#         if frame is None:
#             raise ValueError(f"Error: Could not read image: {image_path}")

#         h, w = frame.shape[:2]
#         rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
#         results = face_mesh.process(rgb_frame)
#         output_data = {}
#         overlay_text = []  # List to store text for image overlay

#         if results.multi_face_landmarks:
#             yawn_status, frame = yawn_detector.process_frame(frame.copy(), results)
#             eye_closure, _ = blink_detector.run(frame.copy(), results)
#             head_pose = head_pose_tracker.run(results)

#             eye_closed_value = int(next(item[1] for item in eye_closure if item[0] == 'Eye Closed'))
#             eye_state = "closed" if eye_closed_value > 0 else "open"
#             mouth_state = "open" if any(item[1] == 'Yes' for item in yawn_status if item[0] == 'Yawning') else "closed"

#             output_data["eye"] = eye_state
#             output_data["mouth"] = mouth_state
#             output_data.update(dict(head_pose))
#             output_data.update(dict(yawn_status))

#             # Append text to the overlay list
#             overlay_text.append(f"Eye: {eye_state}")
#             overlay_text.append(f"Mouth: {mouth_state}")

#             # Extract head pose data and append to overlay
#             head_pose_data = dict(head_pose) if head_pose else {}
#             for k, v in head_pose_data.items():
#                 overlay_text.append(f"{k}:{v}")
#             # Extract yawn status data and append to overlay
#             yawn_status_data = dict(yawn_status) if yawn_status else {}
#             for k, v in yawn_status_data.items():
#                 overlay_text.append(f"{k}:{v}")

#         else:
#             print(f"Warning: No face detected in image: {image_path}")
#             output_data["face_detected"] = False
#             overlay_text.append("No face detected")  # If no face is detected

#         # Add overlay to the image
#         y_offset = 150  # Start position of the text
#         for i, text in enumerate(overlay_text):
#             cv2.putText(frame, text, (10, y_offset + i * 20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2, cv2.LINE_AA)

#         return output_data, frame

#     except Exception as e:
#         print(f"Error processing image {image_path}: {e}")
#         return None, None

# # Evaluate predictions against ground truth
# def evaluate_predictions(predictions, ground_truth):
#     metrics = {
#         "TP": 0,  
#         "FP": 0,  
#         "FN": 0,  
#         "precision": 0,
#         "recall": 0,
#         "accuracy": 0
#     }

#     for key, true_count in ground_truth.items():
#         pred_count = predictions.get(key, 0)
#         if pred_count > true_count:
#             metrics["FP"] += pred_count - true_count
#             metrics["TP"] += true_count
#         elif pred_count < true_count:
#             metrics["FN"] += true_count - pred_count
#             metrics["TP"] += pred_count
#         else:
#             metrics["TP"] += pred_count

#     # Calculate precision, recall, and accuracy
#     metrics["precision"] = metrics["TP"] / (metrics["TP"] + metrics["FP"]) if (metrics["TP"] + metrics["FP"]) > 0 else 0
#     metrics["recall"] = metrics["TP"] / (metrics["TP"] + metrics["FN"]) if (metrics["TP"] + metrics["FN"]) > 0 else 0
#     metrics["accuracy"] = metrics["TP"] / (metrics["TP"] + metrics["FP"] + metrics["FN"]) if (metrics["TP"] + metrics["FP"] + metrics["FN"]) > 0 else 0

#     return metrics

# # Main function to process folders and evaluate results
# def main():
#     folders = ["Folder-1", "Folder-2", "Folder-3", "Folder-4"]
#     output_base_folder = "output_images"

#     face_mesh, mp_face_mesh, mp_drawing, mp_drawing_styles = initialize_mediapipe()
#     blink_detector, head_pose_tracker, yawn_detector = initialize_models(mp_face_mesh, mp_drawing, mp_drawing_styles)

#     # Initialize overall metrics
#     overall_metrics = {
#         "TP": 0,
#         "FP": 0,
#         "FN": 0
#     }

#     # Initialize class-wise metrics
#     class_wise_metrics = {
#         "Eyes_open": {"TP": 0, "FP": 0, "FN": 0},
#         "Eyes_closed": {"TP": 0, "FP": 0, "FN": 0},
#         "Mouth_open": {"TP": 0, "FP": 0, "FN": 0},
#         "Mouth_closed": {"TP": 0, "FP": 0, "FN": 0},
#         "Head_left": {"TP": 0, "FP": 0, "FN": 0},
#         "Head_right": {"TP": 0, "FP": 0, "FN": 0},
#         "Yawn": {"TP": 0, "FP": 0, "FN": 0}
#     }

#     for folder in folders:
#         image_folder = os.path.join("/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/test_dataset/eval_face_mesh/", folder)
#         output_folder = os.path.join(output_base_folder, folder)

#         if not os.path.exists(output_folder):
#             os.makedirs(output_folder)

#         predictions = {
#             "Eyes_open": 0,
#             "Eyes_closed": 0,
#             "Mouth_open": 0,
#             "Mouth_closed": 0,
#             "Head_left": 0,
#             "Head_right": 0,
#             "Yawn": 0
#         }

#         for filename in os.listdir(image_folder):
#             if filename.lower().endswith(('.png', '.jpg', '.jpeg')):
#                 image_path = os.path.join(image_folder, filename)
#                 output_data, frame = process_image(
#                     image_path, face_mesh, blink_detector, head_pose_tracker, yawn_detector)

#                 if frame is not None:  # Process the frame only if it is valid
#                     base_filename, _ = os.path.splitext(filename)
#                     output_text_file = os.path.join(output_folder, f"{base_filename}.txt")
#                     output_image_path = os.path.join(output_folder, filename)

#                     with open(output_text_file, "w") as outfile:
#                         output_lines = [f"{key}: {value}" for key, value in output_data.items()]
#                         outfile.write("\n".join(output_lines) + "\n")

#                     cv2.imwrite(output_image_path, frame)

#                     # Update predictions based on output_data
#                     if output_data.get("eye") == "open":
#                         predictions["Eyes_open"] += 1
#                     elif output_data.get("eye") == "closed":
#                         predictions["Eyes_closed"] += 1

#                     if output_data.get("mouth") == "open":
#                         predictions["Mouth_open"] += 1
#                     elif output_data.get("mouth") == "closed":
#                         predictions["Mouth_closed"] += 1

#                     if output_data.get("Head Direction", "").lower() == "looking left":
#                         predictions["Head_left"] += 1
#                     elif output_data.get("Head Direction", "").lower() == "looking right":
#                         predictions["Head_right"] += 1

#                     if output_data.get("Yawning") == "Yes":
#                         predictions["Yawn"] += 1

#         # Evaluate predictions against ground truth
#         ground_truth = GROUND_TRUTH[folder]
#         metrics = evaluate_predictions(predictions, ground_truth)

#         # Update overall metrics
#         overall_metrics["TP"] += metrics["TP"]
#         overall_metrics["FP"] += metrics["FP"]
#         overall_metrics["FN"] += metrics["FN"]

#         # Update class-wise metrics
#         for key in class_wise_metrics.keys():
#             if key in ground_truth:
#                 true_count = ground_truth[key]
#                 pred_count = predictions.get(key, 0)
#                 if pred_count > true_count:
#                     class_wise_metrics[key]["FP"] += pred_count - true_count
#                     class_wise_metrics[key]["TP"] += true_count
#                 elif pred_count < true_count:
#                     class_wise_metrics[key]["FN"] += true_count - pred_count
#                     class_wise_metrics[key]["TP"] += pred_count
#                 else:
#                     class_wise_metrics[key]["TP"] += pred_count

#         print(f"\nMetrics for {folder}:")
#         print(f"True Positives (TP): {metrics['TP']}")
#         print(f"False Positives (FP): {metrics['FP']}")
#         print(f"False Negatives (FN): {metrics['FN']}")
#         print(f"Precision: {metrics['precision']:.2f}")
#         print(f"Recall: {metrics['recall']:.2f}")
#         print(f"Accuracy: {metrics['accuracy']:.2f}")

#     # Calculate overall accuracy
#     overall_accuracy = overall_metrics["TP"] / (overall_metrics["TP"] + overall_metrics["FP"] + overall_metrics["FN"]) if (overall_metrics["TP"] + overall_metrics["FP"] + overall_metrics["FN"]) > 0 else 0

#     print("\nOverall Metrics:")
#     print(f"True Positives (TP): {overall_metrics['TP']}")
#     print(f"False Positives (FP): {overall_metrics['FP']}")
#     print(f"False Negatives (FN): {overall_metrics['FN']}")
#     print(f"Overall Accuracy: {overall_accuracy:.2f}")

#     # Calculate class-wise accuracy
#     print("\nClass-wise Metrics:")
#     for key, metrics in class_wise_metrics.items():
#         accuracy = metrics["TP"] / (metrics["TP"] + metrics["FP"] + metrics["FN"]) if (metrics["TP"] + metrics["FP"] + metrics["FN"]) > 0 else 0
#         print(f"{key}:")
#         print(f"  True Positives (TP): {metrics['TP']}")
#         print(f"  False Positives (FP): {metrics['FP']}")
#         print(f"  False Negatives (FN): {metrics['FN']}")
#         print(f"  Accuracy: {accuracy:.2f}")

# if __name__ == "__main__":
#     main()



import os
import time
import cv2
import numpy as np
import mediapipe as mp
from eye_closure_det import BlinkDetector
from head_pose_det import HeadPoseTracker
from yawn_detector import YawnDetector
from utils import draw_table_grid, scale_up_frame, sharpen_image

# Ground truth for each folder
GROUND_TRUTH = {
    "Folder-1": {
        "Eyes_open": 120,
        "Eyes_closed": 60,
        "Mouth_open": 100,
        "Mouth_closed": 50
    },
    "Folder-2": {
        "Head_left": 11,
        "Head_right": 6
    },
    "Folder-3": {
        "Yawn": 16
    },
    "Folder-4": {
        "Head_left": 11,
        "Head_right": 6
    },
}

# Initialize MediaPipe and models
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

# Process an image and return output data and annotated frame
def process_image(image_path, face_mesh, blink_detector, head_pose_tracker, yawn_detector):
    try:
        frame = cv2.imread(image_path)
        if frame is None:
            raise ValueError(f"Error: Could not read image: {image_path}")

        h, w = frame.shape[:2]
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = face_mesh.process(rgb_frame)
        output_data = {}
        overlay_text = []  # List to store text for image overlay

        if results.multi_face_landmarks:
            yawn_status, frame = yawn_detector.process_frame(frame.copy(), results)
            eye_closure, _ = blink_detector.run(frame.copy(), results)
            head_pose = head_pose_tracker.run(results)

            eye_closed_value = int(next(item[1] for item in eye_closure if item[0] == 'Eye Closed'))
            eye_state = "closed" if eye_closed_value > 0 else "open"
            mouth_state = "open" if any(item[1] == 'Yes' for item in yawn_status if item[0] == 'Yawning') else "closed"

            output_data["eye"] = eye_state
            output_data["mouth"] = mouth_state
            output_data.update(dict(head_pose))
            output_data.update(dict(yawn_status))

            # Append text to the overlay list
            overlay_text.append(f"Eye: {eye_state}")
            overlay_text.append(f"Mouth: {mouth_state}")

            # Extract head pose data and append to overlay
            head_pose_data = dict(head_pose) if head_pose else {}
            for k, v in head_pose_data.items():
                overlay_text.append(f"{k}:{v}")
            # Extract yawn status data and append to overlay
            yawn_status_data = dict(yawn_status) if yawn_status else {}
            for k, v in yawn_status_data.items():
                overlay_text.append(f"{k}:{v}")

        else:
            print(f"Warning: No face detected in image: {image_path}")
            output_data["face_detected"] = False
            overlay_text.append("No face detected")  # If no face is detected

        # Add overlay to the image
        y_offset = 150  # Start position of the text
        for i, text in enumerate(overlay_text):
            cv2.putText(frame, text, (10, y_offset + i * 20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2, cv2.LINE_AA)

        return output_data, frame

    except Exception as e:
        print(f"Error processing image {image_path}: {e}")
        return None, None

# Evaluate predictions against ground truth
def evaluate_predictions(predictions, ground_truth):
    metrics = {
        "TP": 0,  
        "FP": 0,  
        "FN": 0,  
        "precision": 0,
        "recall": 0,
        "accuracy": 0
    }

    for key, true_count in ground_truth.items():
        pred_count = predictions.get(key, 0)
        if pred_count > true_count:
            metrics["FP"] += pred_count - true_count
            metrics["TP"] += true_count
        elif pred_count < true_count:
            metrics["FN"] += true_count - pred_count
            metrics["TP"] += pred_count
        else:
            metrics["TP"] += pred_count

    # Calculate precision, recall, and accuracy
    metrics["precision"] = metrics["TP"] / (metrics["TP"] + metrics["FP"]) if (metrics["TP"] + metrics["FP"]) > 0 else 0
    metrics["recall"] = metrics["TP"] / (metrics["TP"] + metrics["FN"]) if (metrics["TP"] + metrics["FN"]) > 0 else 0
    metrics["accuracy"] = metrics["TP"] / (metrics["TP"] + metrics["FP"] + metrics["FN"]) if (metrics["TP"] + metrics["FP"] + metrics["FN"]) > 0 else 0

    return metrics

# Generate HTML report
def generate_html_report(overall_metrics, class_wise_metrics, folder_metrics):
    html_content = """
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Evaluation Report</title>
        <style>
            body {{
                font-family: Arial, sans-serif;
                margin: 20px;
            }}
            h1, h2 {{
                color: #333;
            }}
            table {{
                width: 100%;
                border-collapse: collapse;
                margin-bottom: 20px;
            }}
            th, td {{
                padding: 10px;
                border: 1px solid #ddd;
                text-align: left;
            }}
            th {{
                background-color: #f4f4f4;
            }}
            .metrics {{
                margin-bottom: 30px;
            }}
        </style>
    </head>
    <body>
        <h1>Evaluation Report</h1>
        <div class="metrics">
            <h2>Overall Metrics</h2>
            <table>
                <tr>
                    <th>Metric</th>
                    <th>Value</th>
                </tr>
                <tr>
                    <td>True Positives (TP)</td>
                    <td>{overall_tp}</td>
                </tr>
                <tr>
                    <td>False Positives (FP)</td>
                    <td>{overall_fp}</td>
                </tr>
                <tr>
                    <td>False Negatives (FN)</td>
                    <td>{overall_fn}</td>
                </tr>
                <tr>
                    <td>Overall Accuracy</td>
                    <td>{overall_accuracy:.2f}</td>
                </tr>
            </table>
        </div>
        <div class="metrics">
            <h2>Class-wise Metrics</h2>
            <table>
                <tr>
                    <th>Class</th>
                    <th>True Positives (TP)</th>
                    <th>False Positives (FP)</th>
                    <th>False Negatives (FN)</th>
                    <th>Accuracy</th>
                </tr>
                {class_wise_rows}
            </table>
        </div>
        <div class="metrics">
            <h2>Folder-wise Metrics</h2>
            <table>
                <tr>
                    <th>Folder</th>
                    <th>True Positives (TP)</th>
                    <th>False Positives (FP)</th>
                    <th>False Negatives (FN)</th>
                    <th>Accuracy</th>
                </tr>
                {folder_wise_rows}
            </table>
        </div>
    </body>
    </html>
    """

    # Generate class-wise rows
    class_wise_rows = ""
    for key, metrics in class_wise_metrics.items():
        accuracy = metrics["TP"] / (metrics["TP"] + metrics["FP"] + metrics["FN"]) if (metrics["TP"] + metrics["FP"] + metrics["FN"]) > 0 else 0
        class_wise_rows += f"""
        <tr>
            <td>{key}</td>
            <td>{metrics['TP']}</td>
            <td>{metrics['FP']}</td>
            <td>{metrics['FN']}</td>
            <td>{accuracy:.2f}</td>
        </tr>
        """

    # Generate folder-wise rows
    folder_wise_rows = ""
    for folder, metrics in folder_metrics.items():
        accuracy = metrics["TP"] / (metrics["TP"] + metrics["FP"] + metrics["FN"]) if (metrics["TP"] + metrics["FP"] + metrics["FN"]) > 0 else 0
        folder_wise_rows += f"""
        <tr>
            <td>{folder}</td>
            <td>{metrics['TP']}</td>
            <td>{metrics['FP']}</td>
            <td>{metrics['FN']}</td>
            <td>{accuracy:.2f}</td>
        </tr>
        """

    # Fill the HTML template
    html_content = html_content.format(
        overall_tp=overall_metrics["TP"],
        overall_fp=overall_metrics["FP"],
        overall_fn=overall_metrics["FN"],
        overall_accuracy=overall_metrics["TP"] / (overall_metrics["TP"] + overall_metrics["FP"] + overall_metrics["FN"]) if (overall_metrics["TP"] + overall_metrics["FP"] + overall_metrics["FN"]) > 0 else 0,
        class_wise_rows=class_wise_rows,
        folder_wise_rows=folder_wise_rows
    )

    # Save the HTML report
    with open("evaluation_report.html", "w") as f:
        f.write(html_content)

# Main function to process folders and evaluate results
def main():
    folders = ["Folder-1", "Folder-2", "Folder-3", "Folder-4"]
    output_base_folder = "output_images"

    face_mesh, mp_face_mesh, mp_drawing, mp_drawing_styles = initialize_mediapipe()
    blink_detector, head_pose_tracker, yawn_detector = initialize_models(mp_face_mesh, mp_drawing, mp_drawing_styles)

    # Initialize overall metrics
    overall_metrics = {
        "TP": 0,
        "FP": 0,
        "FN": 0
    }

    # Initialize class-wise metrics
    class_wise_metrics = {
        "Eyes_open": {"TP": 0, "FP": 0, "FN": 0},
        "Eyes_closed": {"TP": 0, "FP": 0, "FN": 0},
        "Mouth_open": {"TP": 0, "FP": 0, "FN": 0},
        "Mouth_closed": {"TP": 0, "FP": 0, "FN": 0},
        "Head_left": {"TP": 0, "FP": 0, "FN": 0},
        "Head_right": {"TP": 0, "FP": 0, "FN": 0},
        "Yawn": {"TP": 0, "FP": 0, "FN": 0}
    }

    # Initialize folder-wise metrics
    folder_metrics = {}

    for folder in folders:
        image_folder = os.path.join("/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/test_dataset/eval_face_mesh/", folder)
        output_folder = os.path.join(output_base_folder, folder)

        if not os.path.exists(output_folder):
            os.makedirs(output_folder)

        predictions = {
            "Eyes_open": 0,
            "Eyes_closed": 0,
            "Mouth_open": 0,
            "Mouth_closed": 0,
            "Head_left": 0,
            "Head_right": 0,
            "Yawn": 0
        }

        for filename in os.listdir(image_folder):
            if filename.lower().endswith(('.png', '.jpg', '.jpeg')):
                image_path = os.path.join(image_folder, filename)
                output_data, frame = process_image(
                    image_path, face_mesh, blink_detector, head_pose_tracker, yawn_detector)

                if frame is not None:  # Process the frame only if it is valid
                    base_filename, _ = os.path.splitext(filename)
                    output_text_file = os.path.join(output_folder, f"{base_filename}.txt")
                    output_image_path = os.path.join(output_folder, filename)

                    with open(output_text_file, "w") as outfile:
                        output_lines = [f"{key}: {value}" for key, value in output_data.items()]
                        outfile.write("\n".join(output_lines) + "\n")

                    cv2.imwrite(output_image_path, frame)

                    # Update predictions based on output_data
                    if output_data.get("eye") == "open":
                        predictions["Eyes_open"] += 1
                    elif output_data.get("eye") == "closed":
                        predictions["Eyes_closed"] += 1

                    if output_data.get("mouth") == "open":
                        predictions["Mouth_open"] += 1
                    elif output_data.get("mouth") == "closed":
                        predictions["Mouth_closed"] += 1

                    if output_data.get("Head Direction", "").lower() == "looking left":
                        predictions["Head_left"] += 1
                    elif output_data.get("Head Direction", "").lower() == "looking right":
                        predictions["Head_right"] += 1

                    if output_data.get("Yawning") == "Yes":
                        predictions["Yawn"] += 1

        # Evaluate predictions against ground truth
        ground_truth = GROUND_TRUTH[folder]
        metrics = evaluate_predictions(predictions, ground_truth)

        # Update overall metrics
        overall_metrics["TP"] += metrics["TP"]
        overall_metrics["FP"] += metrics["FP"]
        overall_metrics["FN"] += metrics["FN"]

        # Update class-wise metrics
        for key in class_wise_metrics.keys():
            if key in ground_truth:
                true_count = ground_truth[key]
                pred_count = predictions.get(key, 0)
                if pred_count > true_count:
                    class_wise_metrics[key]["FP"] += pred_count - true_count
                    class_wise_metrics[key]["TP"] += true_count
                elif pred_count < true_count:
                    class_wise_metrics[key]["FN"] += true_count - pred_count
                    class_wise_metrics[key]["TP"] += pred_count
                else:
                    class_wise_metrics[key]["TP"] += pred_count

        # Save folder-wise metrics
        folder_metrics[folder] = metrics

        print(f"\nMetrics for {folder}:")
        print(f"True Positives (TP): {metrics['TP']}")
        print(f"False Positives (FP): {metrics['FP']}")
        print(f"False Negatives (FN): {metrics['FN']}")
        print(f"Precision: {metrics['precision']:.2f}")
        print(f"Recall: {metrics['recall']:.2f}")
        print(f"Accuracy: {metrics['accuracy']:.2f}")

    # Calculate overall accuracy
    overall_accuracy = overall_metrics["TP"] / (overall_metrics["TP"] + overall_metrics["FP"] + overall_metrics["FN"]) if (overall_metrics["TP"] + overall_metrics["FP"] + overall_metrics["FN"]) > 0 else 0

    print("\nOverall Metrics:")
    print(f"True Positives (TP): {overall_metrics['TP']}")
    print(f"False Positives (FP): {overall_metrics['FP']}")
    print(f"False Negatives (FN): {overall_metrics['FN']}")
    print(f"Overall Accuracy: {overall_accuracy:.2f}")

    # Calculate class-wise accuracy
    print("\nClass-wise Metrics:")
    for key, metrics in class_wise_metrics.items():
        accuracy = metrics["TP"] / (metrics["TP"] + metrics["FP"] + metrics["FN"]) if (metrics["TP"] + metrics["FP"] + metrics["FN"]) > 0 else 0
        print(f"{key}:")
        print(f"  True Positives (TP): {metrics['TP']}")
        print(f"  False Positives (FP): {metrics['FP']}")
        print(f"  False Negatives (FN): {metrics['FN']}")
        print(f"  Accuracy: {accuracy:.2f}")

    # Generate HTML report
    generate_html_report(overall_metrics, class_wise_metrics, folder_metrics)
    print("\nHTML report generated: evaluation_report.html")

if __name__ == "__main__":
    main()