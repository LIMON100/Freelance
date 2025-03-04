# object_detector.py
import cv2
import torch
from ultralytics import YOLO

def detect_objects(frame, model_path="last.pt", confidence_threshold=0.5, iou_threshold=0.7):
    try:
        model = YOLO(model_path)
    except Exception as e:
        print(f"Error loading the object detection model: {e}")
        return []

    # Convert to grayscale
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    # "Fake" RGB conversion (duplicating the channel)
    rgb = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)  # Convert grayscale to 3-channel RGB

    results = model(rgb, conf=confidence_threshold, iou=iou_threshold)

    detections = []
    for result in results:
        boxes = result.boxes  # Access the boxes attribute directly
        for i in range(len(boxes)):
            box = boxes[i].xyxy[0].tolist()  # Get bounding box coordinates (xyxy format)
            confidence = float(boxes[i].conf[0])  # Get confidence score
            class_id = int(boxes[i].cls[0])  # Get class ID

            detections.append({
                'class_id': class_id,
                'confidence': confidence,
                'box': box
            })

    return detections

if __name__ == '__main__':
    # Example usage (for testing)
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Error: Could not open webcam")
        exit()

    while True:
        ret, frame = cap.read()
        if not ret:
            print("Error: Could not read frame")
            break

        detections = detect_objects(frame)

        for detection in detections:
            print(f"Detected: Class {detection['class_id']}, Confidence {detection['confidence']}, Box {detection['box']}")
            # Draw bounding boxes on the frame (optional)
            x1, y1, x2, y2 = detection['box']
            cv2.rectangle(frame, (int(x1), int(y1)), (int(x2), int(y2)), (0, 255, 0), 2)

        cv2.imshow("Object Detection Test", frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()