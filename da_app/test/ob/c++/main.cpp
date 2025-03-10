#include <iostream>
#include <iomanip>
#include "inference.h"
#include <filesystem>
#include <fstream>
#include <random>
#include <chrono>  // For FPS calculation

void Detector(YOLO_V8*& p) {
    cv::VideoCapture cap(0);  // Open the default camera (webcam)
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open webcam." << std::endl;
        return;
    }

    cv::Mat frame;
    auto startTime = std::chrono::steady_clock::now();  // Start time for FPS calculation
    int frameCount = 0;  // Frame counter for FPS calculation

    while (true) {
        cap >> frame;  // Capture a frame from the webcam
        if (frame.empty()) {
            std::cerr << "Error: Captured frame is empty." << std::endl;
            break;
        }

        std::vector<DL_RESULT> res;
        p->RunSession(frame, res);

        for (auto& re : res) {
            // Check confidence threshold (55%)
            if (re.confidence < 0.55) {
                continue;  // Skip this detection if confidence is below 55%
            }

            cv::RNG rng(cv::getTickCount());
            cv::Scalar color(rng.uniform(0, 256), rng.uniform(0, 256), rng.uniform(0, 256));

            cv::rectangle(frame, re.box, color, 3);

            float confidence = floor(100 * re.confidence) / 100;
            std::cout << std::fixed << std::setprecision(2);

            // Abbreviate class names
            std::string className = p->classes[re.classId];
            std::string abbreviatedName;
            if (className == "Mouth_Open") {
                abbreviatedName = "mo";
            } else if (className == "Mouth_Close") {
                abbreviatedName = "mc";
            } else if (className == "Eye_Open") {
                abbreviatedName = "eo";
            } else if (className == "Eye_Close") {
                abbreviatedName = "ec";
            } else {
                abbreviatedName = className;  // Default to full name if no abbreviation is defined
            }

            std::string label = abbreviatedName + " " + std::to_string(confidence).substr(0, std::to_string(confidence).size() - 4);

            cv::rectangle(
                frame,
                cv::Point(re.box.x, re.box.y - 25),
                cv::Point(re.box.x + label.length() * 15, re.box.y),
                color,
                cv::FILLED
            );

            cv::putText(
                frame,
                label,
                cv::Point(re.box.x, re.box.y - 5),
                cv::FONT_HERSHEY_SIMPLEX,
                0.75,
                cv::Scalar(0, 0, 0),
                2
            );
        }

        // Calculate FPS
        frameCount++;
        auto endTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        double fps = frameCount / (elapsedTime / 1000.0);

        // Display FPS on the frame
        // std::string fpsText = "FPS: " + std::to_string(static_cast<int>(fps));
        // cv::putText(
        //     frame,
        //     fpsText,
        //     cv::Point(10, 30),  // Position of the FPS text
        //     cv::FONT_HERSHEY_SIMPLEX,
        //     1.0,
        //     cv::Scalar(0, 255, 0),  // Green color
        //     2
        // );

        // Show the frame
        cv::imshow("Result of Detection", frame);

        // Exit on 'q' key press
        if (cv::waitKey(1) == 'q') {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
}

void DetectTest()
{
    YOLO_V8* yoloDetector = new YOLO_V8;
    // Hardcode class names for your custom model
    yoloDetector->classes = {"Face", "Eye_Open", "Eye_Close", "Mouth_Open", "Mouth_Close", "Cigar", "Phone"};  // Add your class names here

    DL_INIT_PARAM params;
    params.rectConfidenceThreshold = 0.1;
    params.iouThreshold = 0.6;
    params.modelPath = "best.onnx";  // Path to your custom YOLOv11 ONNX model
    params.imgSize = { 416, 416 };  // Adjust this if your model uses a different input size
#ifdef USE_CUDA
    params.cudaEnable = true;
    params.modelType = YOLO_DETECT_V8;  // Use YOLO_DETECT_V8 for FP32 inference
#else
    params.modelType = YOLO_DETECT_V8;  // Use YOLO_DETECT_V8 for CPU inference
    params.cudaEnable = false;
#endif
    yoloDetector->CreateSession(params);
    Detector(yoloDetector);
}

int main()
{
    DetectTest();  // Run object detection on webcam
}