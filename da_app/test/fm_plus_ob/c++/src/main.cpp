// #include "IrisLandmark.hpp"
// #include "BlinkDetector.hpp"
// #include "YawnDetector.hpp"
// #include "HeadPoseTracker.hpp"
// #include "inference.h" // Include YOLO headers
// #include <iostream>
// #include <opencv2/highgui.hpp>
// #include <opencv2/imgproc.hpp>
// #include <iomanip>
// #include <sstream>
// #include <thread>
// #include <queue>
// #include <atomic>
// #include <mutex>

// #define SHOW_FPS    (1)
// #define MAX_QUEUE_SIZE 5 // Limit the queue size to prevent excessive memory usage

// #if SHOW_FPS
// #include <chrono>
// #endif

// // Structure to hold YOLO results
// struct YOLO_Result {
//     cv::Mat frame;
//     std::vector<DL_RESULT> results;
// };

// // Mutexes for queue protection
// std::mutex queueMutex;
// std::mutex resultMutex;

// // YOLO Worker Function (to run in a separate thread)
// void yoloWorker(YOLO_V8* yoloDetector, std::queue<cv::Mat>& inputQueue, std::queue<YOLO_Result>& outputQueue, std::atomic<bool>& stopRequested) {
//     while (!stopRequested.load()) {
//         cv::Mat frame;
//         {
//             std::unique_lock<std::mutex> lock(queueMutex);
//             if (!inputQueue.empty()) {
//                 frame = inputQueue.front();
//                 inputQueue.pop();
//             } else {
//                 lock.unlock();
//                 std::this_thread::sleep_for(std::chrono::milliseconds(1));
//                 continue;
//             }
//         }
//         if (!frame.empty()) {
//             YOLO_Result yoloResult;
//             yoloResult.frame = frame.clone(); // Make a copy
//             yoloDetector->RunSession(yoloResult.frame, yoloResult.results);
//             {
//                 std::unique_lock<std::mutex> lock(resultMutex);
//                 if (outputQueue.size() >= MAX_QUEUE_SIZE) {
//                     outputQueue.pop(); // Drop the oldest result if the queue is full
//                 }
//                 outputQueue.push(yoloResult);
//             }
//         }
//     }
// }

// int main(int argc, char* argv[]) {
//     my::IrisLandmark irisLandmarker("");
//     my::HeadPoseTracker headPoseTracker;
//     cv::VideoCapture cap(0);
//     my::BlinkDetector blinkDetector;  // Use the my:: namespace
//     YawnDetector yawnDetector;

//     // Local variable definitions
//     std::queue<cv::Mat> yoloInputQueue;
//     std::queue<YOLO_Result> yoloOutputQueue;
//     std::atomic<bool> stopRequested(false); // Initialize atomic variable
//     YOLO_V8* yoloDetector = nullptr; // Declare
//     std::thread yoloThread; // Must declare to tell. It is thread

//     bool success = cap.isOpened();
//     if (!success) {
//         std::cerr << "Cannot open the camera." << std::endl;
//         return 1;
//     }

// #if SHOW_FPS
//     float sum = 0;
//     int count = 0;
// #endif

//     // YOLO Initialization
//     yoloDetector = new YOLO_V8;
//     yoloDetector->classes = {"cigarette", "drinking", "eating", "mobile"};  // Set YOLO classes

//     DL_INIT_PARAM params;
//     params.rectConfidenceThreshold = 0.2;
//     params.iouThreshold = 0.5;
//     params.modelPath = "/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/bface/dms/c++/test/iris_test/mediapipe_face_iris_ob/models/ob.onnx";  // Path to your custom YOLOv8 ONNX model
//     params.imgSize = {640, 640};  // Adjust this if your model uses a different input size
// #ifdef USE_CUDA
//     params.cudaEnable = true;
//     params.modelType = YOLO_DETECT_V8;  // Use YOLO_DETECT_V8 for FP32 inference
// #else
//     params.modelType = YOLO_DETECT_V8;  // Use YOLO_DETECT_V8 for CPU inference
//     params.cudaEnable = false;
// #endif
//     char* result = yoloDetector->CreateSession(params);
//     if (result != RET_OK) {
//         std::cerr << "YOLO initialization failed: " << result << std::endl;
//         stopRequested = true;
//         delete yoloDetector;
//         return 1;
//     }

//     // Create and start the YOLO thread
//     yoloThread = std::thread(yoloWorker, yoloDetector, std::ref(yoloInputQueue), std::ref(yoloOutputQueue), std::ref(stopRequested));

//     while (success) {
//         cv::Mat rframe, frame;
//         success = cap.read(rframe);

//         if (!success)
//             break;

//         cv::flip(rframe, rframe, 1);

//         // Declare variables for text size and position calculations
//         int baseline;
//         cv::Size textSize;
//         cv::Point textOrg;

// #if SHOW_FPS
//         auto start = std::chrono::steady_clock::now();
// #endif

//         irisLandmarker.loadImageToInput(rframe);
//         irisLandmarker.runInference();

//         blinkDetector.run(rframe, irisLandmarker);
//         std::vector<cv::Point> faceLandmarks = irisLandmarker.getAllFaceLandmarks();
//          bool isReferenceSet;
//         //Head pose Detection
//         my::HeadPoseTracker::HeadPoseResults headPoseResults = headPoseTracker.run(faceLandmarks);
//             isReferenceSet = headPoseResults.reference_set;
//         YawnDetector::YawnMetrics yawnMetrics;
//         if (faceLandmarks.size() > 10) {
//             yawnMetrics = yawnDetector.run(rframe, faceLandmarks);

//             std::unique_lock<std::mutex> lock(queueMutex);
//             if (yoloInputQueue.size() >= MAX_QUEUE_SIZE) {
//                 yoloInputQueue.pop(); // Drop the oldest frame if the queue is full
//             }
//             yoloInputQueue.push(rframe.clone()); // Put the frame in the input queue
//         }

//         YOLO_Result yoloResult;
//         bool hasYoloResults = false;
//         {
//             std::unique_lock<std::mutex> lock(resultMutex);
//             if (!yoloOutputQueue.empty()) {
//                 yoloResult = yoloOutputQueue.front();
//                 yoloOutputQueue.pop();
//                 hasYoloResults = true;
//             }
//         }

//         std::string leftEyeStatus = blinkDetector.isLeftEyeClosed() ? "Closed" : "Open";
//         std::string rightEyeStatus = blinkDetector.isRightEyeClosed() ? "Closed" : "Open";
//         float leftEAR = blinkDetector.getLeftEARValue();
//         float rightEAR = blinkDetector.getRightEARValue();
//         int blink_counts = blinkDetector.getBlinkCount();
//         double perclos = blinkDetector.getPerclos();

//         // Get the last blink duration
//         double lastBlinkDuration = blinkDetector.getLastBlinkDuration();

//         // Get the blink rate
//         double blinkRate = blinkDetector.getBlinkRate();

//         std::string detectedObjectsText;
//         if (hasYoloResults && faceLandmarks.size() > 10) {
//             std::vector<std::string> detectedObjects;
//             for (auto& re : yoloResult.results) {
//                 if (re.confidence >= 0.55) {
//                     std::string className = yoloDetector->classes[re.classId];
//                     detectedObjects.push_back(className);
//                 }
//             }

//             // Create the comma-separated list of detected objects
//             if (!detectedObjects.empty()) {
//                 detectedObjectsText = "Detected: ";
//                 for (size_t i = 0; i < detectedObjects.size(); ++i) {
//                     detectedObjectsText += detectedObjects[i];
//                     if (i < detectedObjects.size() - 1) {
//                         detectedObjectsText += ", ";
//                     }
//                 }
//             }
//         }
//         // Calculate the text size and position for top-center alignment
       
//         double textScaleObjects = 1.0; // Reduced text scale
//         cv::Scalar textColorObjects(0, 0, 255); // Green color
//         textSize = cv::getTextSize(detectedObjectsText, cv::FONT_HERSHEY_PLAIN, textScaleObjects, 2, &baseline);
//         textOrg = cv::Point((rframe.cols - textSize.width) / 2, textSize.height + 10); // Center horizontally, 10 pixels from top

//         // Display detectedObjectsText at the top center
//         cv::putText(rframe, detectedObjectsText, textOrg, cv::FONT_HERSHEY_PLAIN, textScaleObjects, textColorObjects, 2);

//         // Display text information
//         int textY = 30 + textSize.height + 10 + 10; // Starting Y position for text, move down by the height of the objects text
//         int lineHeight = 20; // Line height for each text line
//         cv::Scalar textColor(0, 25, 0); // Red color
//         double textScale = 0.7; // Smaller text scale
//         int thickness = 1; 

//         // cv::putText(rframe, "Left Eye: " + leftEyeStatus + " EAR: " + std::to_string(leftEAR), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//         // textY += lineHeight;
//         // cv::putText(rframe, "Right Eye: " + rightEyeStatus + " EAR: " + std::to_string(rightEAR), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//         // textY += lineHeight;
//         cv::putText(rframe, "PERCLOS: " + std::to_string(perclos), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, thickness);
//         textY += lineHeight;
//         cv::putText(rframe, "Blink counts: " + std::to_string(blink_counts), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//         textY += lineHeight;
//         cv::putText(rframe, "Last Blink Duration: " + std::to_string(lastBlinkDuration) + " s", cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//         textY += lineHeight;
//         cv::putText(rframe, "Blink Rate: " + std::to_string(blinkRate) + " BPM", cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//         textY += lineHeight;
//         cv::putText(rframe, "Yaw: " + headPoseResults.rows[0][1], cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//         textY += lineHeight;
//         cv::putText(rframe, "Pitch: " + headPoseResults.rows[1][1], cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//         textY += lineHeight;
//         cv::putText(rframe, "Roll: " + headPoseResults.rows[2][1], cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//         textY += lineHeight;
//         cv::putText(rframe, "Yawning: " + std::string(yawnMetrics.isYawning ? "Yes" : "No"), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//         textY += lineHeight;
//         cv::putText(rframe, "Yawn Frequency: " + std::to_string(yawnMetrics.yawnFrequency), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//         textY += lineHeight;
//         cv::putText(rframe, "Yawn Duration: " + std::to_string(yawnMetrics.yawnDuration), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//         textY += lineHeight;
//         cv::putText(rframe, "Yawn Count: " + std::to_string(yawnMetrics.yawnCount), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//         textY += lineHeight;
//         //  cv::putText(rframe, "Calibration: "+ std::to_string(isReferenceSet) , cv::Point(20, 360), cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(0, 255, 0), 2);


// #if SHOW_FPS
//         auto stop = std::chrono::steady_clock::now();
//         auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
//         float inferenceTime = duration.count() / 1e3;
//         sum += inferenceTime;
//         count += 1;
//         int fps = (int)1e3 / inferenceTime;

//         // Move FPS to top-right corner
//         std::string fpsText = std::to_string(fps);
//         textSize = cv::getTextSize(fpsText, cv::FONT_HERSHEY_PLAIN, 3, 2, &baseline);
//         textOrg = cv::Point((rframe.cols - textSize.width - 10), (textSize.height + 10));

//         cv::putText(rframe, fpsText, textOrg, cv::FONT_HERSHEY_PLAIN, 3, cv::Scalar(0, 196, 255), 2);
// #endif

//         cv::imshow("Face and Iris Landmarks", rframe);

//         if (cv::waitKey(10) == 27)
//             break;
//     }

//     // Clean up
//     stopRequested = true;
//     yoloThread.join();
//     if (yoloDetector)
//         delete yoloDetector;

//     cap.release();
//     cv::destroyAllWindows();
//     return 0;
// }


// main.cpp
#include "IrisLandmark.hpp"
#include "BlinkDetector.hpp"
#include "YawnDetector.hpp"
#include "HeadPoseTracker.hpp"
#include "inference.h" // Include YOLO headers
#include "KSSCalculator.hpp" // Include KSSCalculator
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iomanip>
#include <sstream>
#include <thread>
#include <queue>
#include <atomic>
#include <mutex>

#define SHOW_FPS    (1)
#define MAX_QUEUE_SIZE 5 // Limit the queue size to prevent excessive memory usage

#if SHOW_FPS
#include <chrono>
#endif

// Structure to hold YOLO results
struct YOLO_Result {
    cv::Mat frame;
    std::vector<DL_RESULT> results;
};

// Mutexes for queue protection
std::mutex queueMutex;
std::mutex resultMutex;

// YOLO Worker Function (to run in a separate thread)
void yoloWorker(YOLO_V8* yoloDetector, std::queue<cv::Mat>& inputQueue, std::queue<YOLO_Result>& outputQueue, std::atomic<bool>& stopRequested) {
    while (!stopRequested.load()) {
        cv::Mat frame;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (!inputQueue.empty()) {
                frame = inputQueue.front();
                inputQueue.pop();
            } else {
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
        }
        if (!frame.empty()) {
            YOLO_Result yoloResult;
            yoloResult.frame = frame.clone(); // Make a copy
            yoloDetector->RunSession(yoloResult.frame, yoloResult.results);
            {
                std::unique_lock<std::mutex> lock(resultMutex);
                if (outputQueue.size() >= MAX_QUEUE_SIZE) {
                    outputQueue.pop(); // Drop the oldest result if the queue is full
                }
                outputQueue.push(yoloResult);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    my::IrisLandmark irisLandmarker("");
    my::HeadPoseTracker headPoseTracker;
    cv::VideoCapture cap(0);
    my::BlinkDetector blinkDetector;  // Use the my:: namespace
    YawnDetector yawnDetector;

    // Initialize KSSCalculator
    KSSCalculator kssCalculator;

    // Local variable definitions
    std::queue<cv::Mat> yoloInputQueue;
    std::queue<YOLO_Result> yoloOutputQueue;
    std::atomic<bool> stopRequested(false); // Initialize atomic variable
    YOLO_V8* yoloDetector = nullptr; // Declare
    std::thread yoloThread; // Must declare to tell. It is thread

    bool success = cap.isOpened();
    if (!success) {
        std::cerr << "Cannot open the camera." << std::endl;
        return 1;
    }

#if SHOW_FPS
    float sum = 0;
    int count = 0;
#endif

    // YOLO Initialization
    yoloDetector = new YOLO_V8;
    yoloDetector->classes = {"cigarette", "drinking", "eating", "mobile"};  // Set YOLO classes

    DL_INIT_PARAM params;
    params.rectConfidenceThreshold = 0.2;
    params.iouThreshold = 0.5;
    params.modelPath = "/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/bface/dms/c++/test/iris_test/mediapipe_face_iris_ob/models/ob.onnx";  // Path to your custom YOLOv8 ONNX model
    params.imgSize = {640, 640};  // Adjust this if your model uses a different input size
#ifdef USE_CUDA
    params.cudaEnable = true;
    params.modelType = YOLO_DETECT_V8;  // Use YOLO_DETECT_V8 for FP32 inference
#else
    params.modelType = YOLO_DETECT_V8;  // Use YOLO_DETECT_V8 for CPU inference
    params.cudaEnable = false;
#endif
    char* result = yoloDetector->CreateSession(params);
    if (result != RET_OK) {
        std::cerr << "YOLO initialization failed: " << result << std::endl;
        stopRequested = true;
        delete yoloDetector;
        return 1;
    }

    // Create and start the YOLO thread
    yoloThread = std::thread(yoloWorker, yoloDetector, std::ref(yoloInputQueue), std::ref(yoloOutputQueue), std::ref(stopRequested));

    while (success) {
        cv::Mat rframe, frame;
        success = cap.read(rframe);

        if (!success)
            break;

        cv::flip(rframe, rframe, 1);

        // Declare variables for text size and position calculations
        int baseline;
        cv::Size textSize;
        cv::Point textOrg;

#if SHOW_FPS
        auto start = std::chrono::steady_clock::now();
#endif

        irisLandmarker.loadImageToInput(rframe);
        irisLandmarker.runInference();

        blinkDetector.run(rframe, irisLandmarker);
        std::vector<cv::Point> faceLandmarks = irisLandmarker.getAllFaceLandmarks();
        bool isReferenceSet;

        //Head pose Detection
        my::HeadPoseTracker::HeadPoseResults headPoseResults = headPoseTracker.run(faceLandmarks);
        isReferenceSet = headPoseResults.reference_set;
        YawnDetector::YawnMetrics yawnMetrics;
        if (faceLandmarks.size() > 10) {
            yawnMetrics = yawnDetector.run(rframe, faceLandmarks);

            std::unique_lock<std::mutex> lock(queueMutex);
            if (yoloInputQueue.size() >= MAX_QUEUE_SIZE) {
                yoloInputQueue.pop(); // Drop the oldest frame if the queue is full
            }
            yoloInputQueue.push(rframe.clone()); // Put the frame in the input queue
        }

        YOLO_Result yoloResult;
        bool hasYoloResults = false;
        {
            std::unique_lock<std::mutex> lock(resultMutex);
            if (!yoloOutputQueue.empty()) {
                yoloResult = yoloOutputQueue.front();
                yoloOutputQueue.pop();
                hasYoloResults = true;
            }
        }

        std::string leftEyeStatus = blinkDetector.isLeftEyeClosed() ? "Closed" : "Open";
        std::string rightEyeStatus = blinkDetector.isRightEyeClosed() ? "Closed" : "Open";
        float leftEAR = blinkDetector.getLeftEARValue();
        float rightEAR = blinkDetector.getRightEARValue();
        int blink_counts = blinkDetector.getBlinkCount();
        double perclos = blinkDetector.getPerclos();

        // Get the last blink duration
        double lastBlinkDuration = blinkDetector.getLastBlinkDuration();

        // Get the blink rate
        double blinkRate = blinkDetector.getBlinkRate();

        std::vector<std::string> detectedObjects;
        if (hasYoloResults && faceLandmarks.size() > 10) {
            for (auto& re : yoloResult.results) {
                if (re.confidence >= 0.55) {
                    std::string className = yoloDetector->classes[re.classId];
                    detectedObjects.push_back(className);
                }
            }
        }
        // Setting values for KSS Class
        kssCalculator.setPerclos(perclos);
        kssCalculator.setHeadPose(std::stod(headPoseResults.rows[0][1]), std::stod(headPoseResults.rows[1][1]), std::stod(headPoseResults.rows[2][1]));
        kssCalculator.setYawnMetrics(yawnMetrics.isYawning, yawnMetrics.yawnFrequency, yawnMetrics.yawnDuration);
        kssCalculator.setDetectedObjects(detectedObjects);

        // Calculate the composite KSS
        int compositeKSS = kssCalculator.calculateCompositeKSS();
        std::string kssStatus = kssCalculator.getKSSAlertStatus(compositeKSS);

        std::string detectedObjectsText;
        if (hasYoloResults && faceLandmarks.size() > 10) {
             if (!detectedObjects.empty()) {
                detectedObjectsText = "Detected: ";
                for (size_t i = 0; i < detectedObjects.size(); ++i) {
                    detectedObjectsText += detectedObjects[i];
                    if (i < detectedObjects.size() - 1) {
                        detectedObjectsText += ", ";
                    }
                }
            }
        }
        // Calculate the text size and position for top-center alignment
       
        double textScaleObjects = 1.0; // Reduced text scale
        cv::Scalar textColorObjects(0, 0, 255); // Green color
        textSize = cv::getTextSize(detectedObjectsText, cv::FONT_HERSHEY_PLAIN, textScaleObjects, 2, &baseline);
        textOrg = cv::Point((rframe.cols - textSize.width) / 2, textSize.height + 10); // Center horizontally, 10 pixels from top

        // Display detectedObjectsText at the top center
        cv::putText(rframe, detectedObjectsText, textOrg, cv::FONT_HERSHEY_PLAIN, textScaleObjects, textColorObjects, 2);

        // Display text information
        int textY = 30 + textSize.height + 10 + 10; // Starting Y position for text, move down by the height of the objects text
        int lineHeight = 20; // Line height for each text line
        cv::Scalar textColor(0, 255, 0); // Red color
        double textScale = 0.7; // Smaller text scale
        int thickness = 1; 

    
        cv::putText(rframe, "PERCLOS: " + std::to_string(perclos), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, thickness);
        textY += lineHeight;
        cv::putText(rframe, "Blink counts: " + std::to_string(blink_counts), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
        textY += lineHeight;
        cv::putText(rframe, "Last Blink Duration: " + std::to_string(lastBlinkDuration) + " s", cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
        textY += lineHeight;
        cv::putText(rframe, "Blink Rate: " + std::to_string(blinkRate) + " BPM", cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
        textY += lineHeight;
        cv::putText(rframe, "Yaw: " + headPoseResults.rows[0][1], cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
        textY += lineHeight;
        cv::putText(rframe, "Pitch: " + headPoseResults.rows[1][1], cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
        textY += lineHeight;
        cv::putText(rframe, "Roll: " + headPoseResults.rows[2][1], cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
        textY += lineHeight;
        cv::putText(rframe, "Yawning: " + std::string(yawnMetrics.isYawning ? "Yes" : "No"), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
        textY += lineHeight;
        cv::putText(rframe, "Yawn Frequency: " + std::to_string(yawnMetrics.yawnFrequency), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
        textY += lineHeight;
        cv::putText(rframe, "Yawn Duration: " + std::to_string(yawnMetrics.yawnDuration), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
        textY += lineHeight;
        cv::putText(rframe, "Yawn Count: " + std::to_string(yawnMetrics.yawnCount), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
        textY += lineHeight;
        // **Display the Composite KSS and Status**
        cv::putText(rframe, "Composite KSS: " + std::to_string(compositeKSS), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, thickness);
        textY += lineHeight;
        cv::putText(rframe, "KSS Status: " + kssStatus, cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, thickness);


        //  cv::putText(rframe, "Calibration: "+ std::to_string(isReferenceSet) , cv::Point(20, 360), cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(0, 255, 0), 2);


#if SHOW_FPS
        auto stop = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        float inferenceTime = duration.count() / 1e3;
        sum += inferenceTime;
        count += 1;
        int fps = (int)1e3 / inferenceTime;

        // Move FPS to top-right corner
        std::string fpsText = std::to_string(fps);
        textSize = cv::getTextSize(fpsText, cv::FONT_HERSHEY_PLAIN, 3, 2, &baseline);
        textOrg = cv::Point((rframe.cols - textSize.width - 10), (textSize.height + 10));

        cv::putText(rframe, fpsText, textOrg, cv::FONT_HERSHEY_PLAIN, 3, cv::Scalar(0, 196, 255), 2);
#endif

        cv::imshow("Face and Iris Landmarks", rframe);

        if (cv::waitKey(10) == 27)
            break;
    }

    // Clean up
    stopRequested = true;
    yoloThread.join();
    if (yoloDetector)
        delete yoloDetector;

    cap.release();
    cv::destroyAllWindows();
    return 0;
}