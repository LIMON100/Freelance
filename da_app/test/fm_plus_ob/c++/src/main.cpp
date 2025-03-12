// #include "IrisLandmark.hpp"
// #include "BlinkDetector.hpp"
// #include "YawnDetector.hpp"
// #include "HeadPoseTracker.hpp"
// #include "inference.h" // Include YOLO headers
// #include "KSSCalculator.hpp" // Include KSSCalculator
// #include <iostream>
// #include <opencv2/highgui.hpp>
// #include <opencv2/imgproc.hpp>
// #include <iomanip>
// #include <sstream>
// #include <thread>
// #include <queue>
// #include <atomic>
// #include <mutex>
// #include <gst/gst.h>
// #include <gst/app/gstappsink.h> // Corrected include

// #define SHOW_FPS    (1)
// #define MAX_QUEUE_SIZE 5 // Limit the queue size to prevent excessive memory usage

// // Declare sum and count outside the #if SHOW_FPS block
// #if SHOW_FPS
// #include <chrono>
//     float sum = 0;
//     int count = 0;
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

// // Function to create a GStreamer pipeline string for webcam capture
// std::string createGStreamerPipeline(int captureWidth, int captureHeight, int displayWidth, int displayHeight, int framerate) {
// #ifdef _WIN32
//     // Windows pipeline (example, may need adjustments)
//     return "ksvideosrc ! videoconvert ! video/x-raw,format=BGRx,width=" + std::to_string(captureWidth) + ",height=" + std::to_string(captureHeight) + ",framerate=" + std::to_string(framerate) + "/1 ! videoconvert ! video/x-raw,format=BGR ! appsink name=sink";
// #else
//     // Linux pipeline (v4l2src)
//     return "v4l2src ! videoconvert ! video/x-raw,format=BGR,width=" + std::to_string(captureWidth) + ",height=" + std::to_string(captureHeight) + ",framerate=" + std::to_string(framerate) + "/1 ! appsink name=sink";
// #endif
// }

// int main(int argc, char* argv[]) {
//     // Initialize GStreamer
//     gst_init(&argc, &argv);

//     my::IrisLandmark irisLandmarker("");
//     my::HeadPoseTracker headPoseTracker;
//     my::BlinkDetector blinkDetector;  // Use the my:: namespace
//     YawnDetector yawnDetector;

//     // Initialize KSSCalculator
//     KSSCalculator kssCalculator;

//     // Local variable definitions
//     std::queue<cv::Mat> yoloInputQueue;
//     std::queue<YOLO_Result> yoloOutputQueue;
//     std::atomic<bool> stopRequested(false); // Initialize atomic variable
//     YOLO_V8* yoloDetector = nullptr; // Declare
//     std::thread yoloThread; // Must declare to tell. It is thread

//     // Define capture and display parameters
//     int captureWidth = 640;
//     int captureHeight = 480;
//     int displayWidth = 640;
//     int displayHeight = 480;
//     int framerate = 30;

//     // Create GStreamer pipeline
//     std::string pipelineString = createGStreamerPipeline(captureWidth, captureHeight, displayWidth, displayHeight, framerate);
//     std::cout << "Using pipeline: " << pipelineString << std::endl;

//     // Create GStreamer pipeline from string
//     GstElement* pipeline = gst_parse_launch(pipelineString.c_str(), nullptr);
//     if (!pipeline) {
//         std::cerr << "Failed to create pipeline" << std::endl;
//         return -1;
//     }

//     // Get the appsink
//     GstElement* appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
//     if (!appsink) {
//         std::cerr << "Failed to get appsink" << std::endl;
//         gst_object_unref(pipeline);
//         return -1;
//     }

//     // Configure the appsink to emit buffers that OpenCV can read
//     GstCaps* caps = gst_caps_new_simple("video/x-raw",
//         "format", G_TYPE_STRING, "BGR",
//         "width", G_TYPE_INT, displayWidth,
//         "height", G_TYPE_INT, displayHeight,
//         NULL);
//     gst_app_sink_set_caps(GST_APP_SINK(appsink), caps);
//     gst_caps_unref(caps);


//     // Set the pipeline to playing state
//     gst_element_set_state(pipeline, GST_STATE_PLAYING);

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

//     // Declare these variables *before* the main loop

//     // KSS values
//     int compositeKSS;
//     std::string kssStatus;

//     //Object detection text
//     double textScaleObjects = 1.0; // Reduced text scale
//     cv::Scalar textColorObjects(0, 0, 255); // color
//     std::string detectedObjectsText;

//      // Formatted Values as string
//     std::stringstream perclos_ss, lastBlinkDuration_ss, blinkRate_ss, yawnFrequency_ss, yawnDuration_ss;
//     std::string yawnCountStr ;// Formatted number as String
//     //For printing green color when no alert
//     cv::Scalar textColor(0, 255, 0); //Red Green is the best

//     // Main loop (GStreamer)
//     GstSample* sample;
//     GstBuffer* buffer;
//     GstMapInfo mapInfo;
//     cv::Mat frame;
//     while (!stopRequested) {
//         // Get the sample from appsink
//         sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
//         if (sample) {
//             buffer = gst_sample_get_buffer(sample);
//             if (buffer) {
//                 gst_buffer_map(buffer, &mapInfo, GST_MAP_READ);
//                 frame = cv::Mat(cv::Size(displayWidth, displayHeight), CV_8UC3, mapInfo.data, cv::Mat::AUTO_STEP);

//                 if (!frame.empty()) {
//                     cv::flip(frame, frame, 1);

//                     int baseline;
//                     cv::Size textSize;
//                     cv::Point textOrg;

// #if SHOW_FPS
//                     auto start = std::chrono::steady_clock::now();
// #endif

//                     irisLandmarker.loadImageToInput(frame);
//                     irisLandmarker.runInference();

//                     blinkDetector.run(frame, irisLandmarker);
//                     std::vector<cv::Point> faceLandmarks = irisLandmarker.getAllFaceLandmarks();
//                     bool isReferenceSet;

//                     //Head pose Detection
//                     my::HeadPoseTracker::HeadPoseResults headPoseResults = headPoseTracker.run(faceLandmarks);
//                     isReferenceSet = headPoseResults.reference_set;
//                     YawnDetector::YawnMetrics yawnMetrics;
//                     if (faceLandmarks.size() > 10) {
//                         yawnMetrics = yawnDetector.run(frame, faceLandmarks);

//                         std::unique_lock<std::mutex> lock(queueMutex);
//                         if (yoloInputQueue.size() >= MAX_QUEUE_SIZE) {
//                             yoloInputQueue.pop(); // Drop the oldest frame if the queue is full
//                         }
//                         yoloInputQueue.push(frame.clone()); // Put the frame in the input queue
//                     }

//                     YOLO_Result yoloResult;
//                     bool hasYoloResults = false;
//                     {
//                         std::unique_lock<std::mutex> lock(resultMutex);
//                         if (!yoloOutputQueue.empty()) {
//                             yoloResult = yoloOutputQueue.front();
//                             yoloOutputQueue.pop();
//                             hasYoloResults = true;
//                         }
//                     }

//                     std::string leftEyeStatus = blinkDetector.isLeftEyeClosed() ? "Closed" : "Open";
//                     std::string rightEyeStatus = blinkDetector.isRightEyeClosed() ? "Closed" : "Open";
//                     float leftEAR = blinkDetector.getLeftEARValue();
//                     float rightEAR = blinkDetector.getRightEARValue();
//                     int blink_counts = blinkDetector.getBlinkCount();
//                     double perclos = blinkDetector.getPerclos();

//                     // Get the last blink duration
//                     double lastBlinkDuration = blinkDetector.getLastBlinkDuration();

//                     // Get the blink rate
//                     double blinkRate = blinkDetector.getBlinkRate();

//                     std::vector<std::string> detectedObjects;
//                     if (hasYoloResults && faceLandmarks.size() > 10) {
//                         for (auto& re : yoloResult.results) {
//                             if (re.confidence >= 0.55) {
//                                 std::string className = yoloDetector->classes[re.classId];
//                                 detectedObjects.push_back(className);
//                             }
//                         }
//                         if (!detectedObjects.empty()) {
//                             detectedObjectsText = "Detected: ";
//                             for (size_t i = 0; i < detectedObjects.size(); ++i) {
//                                 detectedObjectsText += detectedObjects[i];
//                                 if (i < detectedObjects.size() - 1) {
//                                     detectedObjectsText += ", ";
//                                 }
//                             }
//                         }
//                     }
//                      else{
//                          detectedObjectsText = "";//Set is none when empty. To use default values
//                      }

//                     // Setting values for KSS Class
//                     kssCalculator.setPerclos(perclos);
//                     //Check if headPoseResults.rows is greater than 0 before accessing its elements
//                     if(headPoseResults.rows.size() > 0){
//                       try {
//                             kssCalculator.setHeadPose(std::stod(headPoseResults.rows[0][1]), std::stod(headPoseResults.rows[1][1]), std::stod(headPoseResults.rows[2][1]));
//                         } catch (const std::invalid_argument& e) {
//                             std::cerr << "Caught invalid_argument: " << e.what() << std::endl;
//                             // Handle the error - perhaps set a default value or skip this KSS calculation
//                             kssCalculator.setHeadPose(0.0, 0.0, 0.0); // Set default values
//                         }
//                     }else {
//                          kssCalculator.setHeadPose(0.0, 0.0, 0.0); // Set default values
//                     }

//                     kssCalculator.setYawnMetrics(yawnMetrics.isYawning, yawnMetrics.yawnFrequency, yawnMetrics.yawnDuration);
//                     kssCalculator.setDetectedObjects(detectedObjects);

//                     // Calculate the composite KSS
//                     compositeKSS = kssCalculator.calculateCompositeKSS();
//                     kssStatus = kssCalculator.getKSSAlertStatus(compositeKSS);

//                      // Format the floating-point numbers to two decimal places
//                     perclos_ss.str(""); //Clear for storing next text
//                     lastBlinkDuration_ss.str("");
//                     blinkRate_ss.str("");
//                     yawnFrequency_ss.str("");
//                     yawnDuration_ss.str("");

//                     perclos_ss << std::fixed << std::setprecision(2) << perclos;
//                     lastBlinkDuration_ss << std::fixed << std::setprecision(2) << lastBlinkDuration;
//                     blinkRate_ss << std::fixed << std::setprecision(2) << blinkRate;
//                     yawnFrequency_ss << std::fixed << std::setprecision(2) << yawnMetrics.yawnFrequency;
//                     yawnDuration_ss << std::fixed << std::setprecision(2) << yawnMetrics.yawnDuration;

//                     //To convert yawn count to string
//                      yawnCountStr = std::to_string(static_cast<int>(yawnMetrics.yawnCount));

//                     // Calculate the text size and position for top-center alignment
//                    //Moved Text outside as well

//                     textSize = cv::getTextSize(detectedObjectsText, cv::FONT_HERSHEY_PLAIN, textScaleObjects, 2, &baseline);
//                     textOrg = cv::Point((frame.cols - textSize.width) / 2, textSize.height + 10); // Center horizontally, 10 pixels from top


//                     // Display KSS Status at the top-left (if compositeKSS is 4 or higher)
//                     textColor=cv::Scalar(0,255,0);
                   
//                     if (compositeKSS >= 4) {
//                        textColor=cv::Scalar(0,0,255); // Red color
//                       cv::putText(frame, "" + kssStatus, cv::Point(20, 30), cv::FONT_HERSHEY_PLAIN, 1.5, textColor, 2);
//                     }

//                      // Display detectedObjectsText at the top center
                  
//                     // Display detectedObjectsText at the top center
//                     cv::putText(frame, detectedObjectsText, textOrg, cv::FONT_HERSHEY_PLAIN, textScaleObjects, textColorObjects, 2);

//                     // Display text information

//                     int textY = 60 ;//Set the intial position of txt
//                     int lineHeight = 20 ; // Distance
//                     double textScale = 0.7; // Smaller text scale
//                     int thickness = 1; 
//                     // Display text information
//                     cv::putText(frame, "PERCLOS: " + perclos_ss.str(), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, thickness);
//                      textY+=lineHeight;
//                     cv::putText(frame, "Blink counts: " + std::to_string(blink_counts), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                     textY+=lineHeight;
//                     cv::putText(frame, "Last Blink Duration: " + lastBlinkDuration_ss.str() + " s", cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                      textY+=lineHeight;
//                     cv::putText(frame, "Blink Rate: " + blinkRate_ss.str() + " BPM", cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                     textY+=lineHeight;
//                     //Accessing yaw, pitch and roll only when its greater than 0
//                     if(headPoseResults.rows.size() > 0){
//                     cv::putText(frame, "Yaw: " + headPoseResults.rows[0][1], cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                      textY+=lineHeight;
//                     cv::putText(frame, "Pitch: " + headPoseResults.rows[1][1], cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                     textY+=lineHeight;
//                     cv::putText(frame, "Roll: " + headPoseResults.rows[2][1], cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                      textY+=lineHeight;
//                      }
//                     cv::putText(frame, "Yawning: " + std::string(yawnMetrics.isYawning ? "Yes" : "No"), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                       textY+=lineHeight;
//                     cv::putText(frame, "Yawn Frequency: " + yawnFrequency_ss.str(), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                      textY+=lineHeight;
//                     cv::putText(frame, "Yawn Duration: " + yawnDuration_ss.str(), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                      textY+=lineHeight;
//                     cv::putText(frame, "Yawn Count: " +  yawnCountStr, cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                     textY+=lineHeight;
//                     cv::putText(frame, "Composite KSS: " + std::to_string(compositeKSS), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, thickness);
                   

// #if SHOW_FPS
//                     auto stop = std::chrono::steady_clock::now();
//                     auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
//                     float inferenceTime = duration.count() / 1e3;
//                     sum += inferenceTime;
//                     count += 1;
//                     int fps = (int)1e3 / inferenceTime;

//                     // Move FPS to top-right corner
//                     std::string fpsText = std::to_string(fps);
//                     textSize = cv::getTextSize(fpsText, cv::FONT_HERSHEY_PLAIN, 3, 2, &baseline);
//                     textOrg = cv::Point((frame.cols - textSize.width - 10), (textSize.height + 10));

//                     cv::putText(frame, fpsText, textOrg, cv::FONT_HERSHEY_PLAIN, 3, cv::Scalar(0, 196, 255), 2);
// #endif

//                     cv::imshow("Face and Iris Landmarks", frame);
//                 }
//                 gst_buffer_unmap(buffer, &mapInfo);
//             }
//             gst_sample_unref(sample);
//         } else{
//              break;
//         }


//         if (cv::waitKey(10) == 27)
//             break;
//     }

//     // Clean up
//     std::cout << "Exit loop" << std::endl;
//     stopRequested = true;
//     yoloThread.join();
//     if (yoloDetector)
//         delete yoloDetector;

//     // Stop the pipeline
//     gst_element_set_state(pipeline, GST_STATE_NULL);
//     // Free resources
//     gst_object_unref(appsink);
//     gst_object_unref(pipeline);
//     gst_deinit();

//     return 0;
// }


// #include "IrisLandmark.hpp"
// #include "BlinkDetector.hpp"
// #include "YawnDetector.hpp"
// #include "HeadPoseTracker.hpp"
// #include "inference.h" 
// #include "KSSCalculator.hpp" 
// #include <iostream>
// #include <opencv2/highgui.hpp>
// #include <opencv2/imgproc.hpp>
// #include <iomanip>
// #include <sstream>
// #include <thread>
// #include <queue>
// #include <atomic>
// #include <mutex>
// #include <gst/gst.h>
// #include <gst/app/gstappsink.h> // Corrected include
// #include <memory> // Include for shared_ptr

// #ifdef _WIN32
// #include <Windows.h> // For Windows thread priority
// #else
// #include <pthread.h> // For POSIX thread priority (Linux, macOS)
// #include <sched.h>
// #endif

// #define SHOW_FPS    (1)
// #define MAX_QUEUE_SIZE 5 // Limit the queue size to prevent excessive memory usage

// // Declare sum and count outside the #if SHOW_FPS block
// #if SHOW_FPS
// #include <chrono>
//     float sum = 0;
//     int count = 0;
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
// // YOLO Worker with Frame Skipping
// void yoloWorker(YOLO_V8* yoloDetector, std::queue<std::shared_ptr<cv::Mat>>& inputQueue, std::queue<YOLO_Result>& outputQueue, std::atomic<bool>& stopRequested) {
//     while (!stopRequested.load()) {
//         std::shared_ptr<cv::Mat> framePtr;
//         {
//             std::unique_lock<std::mutex> lock(queueMutex);
//             if (!inputQueue.empty()) {
//                 framePtr = inputQueue.front();
//                 inputQueue.pop();
//             } else {
//                 lock.unlock();
//                 std::this_thread::sleep_for(std::chrono::milliseconds(1));
//                 continue;
//             }
//         }
//         if (framePtr) {
//             YOLO_Result yoloResult;
//             yoloResult.frame = *framePtr; // Access cv::Mat through shared pointer
//             yoloDetector->RunSession(yoloResult.frame, yoloResult.results); // Use asynchronous inference
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

// // Function to create a GStreamer pipeline string for webcam capture
// std::string createGStreamerPipeline(int captureWidth, int captureHeight, int displayWidth, int displayHeight, int framerate) {
// #ifdef _WIN32
//     // Windows pipeline (example, may need adjustments)
//     return "ksvideosrc ! videoconvert ! video/x-raw,format=BGRx,width=" + std::to_string(captureWidth) + ",height=" + std::to_string(captureHeight) + ",framerate=" + std::to_string(framerate) + "/1 ! videoconvert ! video/x-raw,format=BGR ! appsink name=sink";
// #else
//     // Linux pipeline (v4l2src)
//     return "v4l2src ! videoconvert ! video/x-raw,format=BGR,width=" + std::to_string(captureWidth) + ",height=" + std::to_string(captureHeight) + ",framerate=" + std::to_string(framerate) + "/1 ! appsink name=sink";
// #endif
// }

// int main(int argc, char* argv[]) {
//     // Initialize GStreamer
//     gst_init(&argc, &argv);

//     my::IrisLandmark irisLandmarker("");
//     my::HeadPoseTracker headPoseTracker;
//     my::BlinkDetector blinkDetector;  // Use the my:: namespace
//     YawnDetector yawnDetector;

//     // Initialize KSSCalculator
//     KSSCalculator kssCalculator;

//     // Local variable definitions
//     std::queue<std::shared_ptr<cv::Mat>> yoloInputQueue;
//     std::queue<YOLO_Result> yoloOutputQueue;
//     std::atomic<bool> stopRequested(false); // Initialize atomic variable
//     YOLO_V8* yoloDetector = nullptr; // Declare
//     std::thread yoloThread; // Must declare to tell. It is thread

//     // Define capture and display parameters
//     int captureWidth = 640;
//     int captureHeight = 480;
//     int displayWidth = 640;
//     int displayHeight = 480;
//     int framerate = 30;

//     // Create GStreamer pipeline
//     std::string pipelineString = createGStreamerPipeline(captureWidth, captureHeight, displayWidth, displayHeight, framerate);
//     std::cout << "Using pipeline: " << pipelineString << std::endl;

//     // Create GStreamer pipeline from string
//     GstElement* pipeline = gst_parse_launch(pipelineString.c_str(), nullptr);
//     if (!pipeline) {
//         std::cerr << "Failed to create pipeline" << std::endl;
//         return -1;
//     }

//     // Get the appsink
//     GstElement* appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
//     if (!appsink) {
//         std::cerr << "Failed to get appsink" << std::endl;
//         gst_object_unref(pipeline);
//         return -1;
//     }

//     // Configure the appsink to emit buffers that OpenCV can read
//     GstCaps* caps = gst_caps_new_simple("video/x-raw",
//         "format", G_TYPE_STRING, "BGR",
//         "width", G_TYPE_INT, displayWidth,
//         "height", G_TYPE_INT, displayHeight,
//         NULL);
//     gst_app_sink_set_caps(GST_APP_SINK(appsink), caps);
//     gst_caps_unref(caps);


//     // Set the pipeline to playing state
//     gst_element_set_state(pipeline, GST_STATE_PLAYING);

//     // YOLO Initialization
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
//     yoloDetector = new YOLO_V8(params);  // Create YOLO detector with the parameters
//     yoloDetector->classes = {"cigarette", "drinking", "eating", "mobile"};  // Set YOLO classes

//     //After create yoloDetector, before create thread

//     // Create and start the YOLO thread
//     yoloThread = std::thread(yoloWorker, yoloDetector, std::ref(yoloInputQueue), std::ref(yoloOutputQueue), std::ref(stopRequested));

          
//     #ifdef _WIN32
//         SetThreadPriority(yoloThread.native_handle(), THREAD_PRIORITY_ABOVE_NORMAL);
//     #else
//         sched_param paramsThread;
//         paramsThread.sched_priority = sched_get_priority_max(SCHED_FIFO)/2; //Set to 1/2 max value
//         pthread_setschedparam(yoloThread.native_handle(), SCHED_FIFO, &paramsThread);
//     #endif



//     // Declare these variables *before* the main loop

//     // KSS values
//     int compositeKSS;
//     std::string kssStatus;

//     //Object detection text
//     double textScaleObjects = 1.0; // Reduced text scale
//     cv::Scalar textColorObjects(0, 0, 255); // color
//     std::string detectedObjectsText;

//      // Formatted Values as string
//     std::stringstream perclos_ss, lastBlinkDuration_ss, blinkRate_ss, yawnFrequency_ss, yawnDuration_ss;
//     std::string yawnCountStr ;// Formatted number as String
//     //For printing green color when no alert
//     cv::Scalar textColor(0, 255, 0); //Red Green is the best

//     // Main loop (GStreamer)
//     GstSample* sample;
//     GstBuffer* buffer;
//     GstMapInfo mapInfo;
//     cv::Mat frame;
//     while (!stopRequested) {
//         // Get the sample from appsink
//         sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
//         if (sample) {
//             buffer = gst_sample_get_buffer(sample);
//             if (buffer) {
//                 gst_buffer_map(buffer, &mapInfo, GST_MAP_READ);
//                 frame = cv::Mat(cv::Size(displayWidth, displayHeight), CV_8UC3, mapInfo.data, cv::Mat::AUTO_STEP);

//                 if (!frame.empty()) {
//                     cv::flip(frame, frame, 1);

//                     int baseline;
//                     cv::Size textSize;
//                     cv::Point textOrg;

// #if SHOW_FPS
//                     auto start = std::chrono::steady_clock::now();
// #endif

//                     irisLandmarker.loadImageToInput(frame);
//                     irisLandmarker.runInference();

//                     blinkDetector.run(frame, irisLandmarker);
//                     std::vector<cv::Point> faceLandmarks = irisLandmarker.getAllFaceLandmarks();
//                     bool isReferenceSet;

//                     //Head pose Detection
//                     my::HeadPoseTracker::HeadPoseResults headPoseResults = headPoseTracker.run(faceLandmarks);
//                     isReferenceSet = headPoseResults.reference_set;
//                     YawnDetector::YawnMetrics yawnMetrics;
                    
//                    std::shared_ptr<cv::Mat> framePtr = std::make_shared<cv::Mat>(frame.clone()); // Create a shared pointer
//                     if (faceLandmarks.size() > 10) {
//                         yawnMetrics = yawnDetector.run(frame, faceLandmarks);

//                         std::unique_lock<std::mutex> lock(queueMutex);
//                         if (yoloInputQueue.size() >= MAX_QUEUE_SIZE) {
//                             yoloInputQueue.pop(); // Drop the oldest frame if the queue is full
//                         }
//                         yoloInputQueue.push(framePtr); // Put the frame in the input queue
//                     }

//                     YOLO_Result yoloResult;
//                     bool hasYoloResults = false;
//                     {
//                         std::unique_lock<std::mutex> lock(resultMutex);
//                         if (!yoloOutputQueue.empty()) {
//                             yoloResult = yoloOutputQueue.front();
//                             yoloOutputQueue.pop();
//                             hasYoloResults = true;
//                         }
//                     }
//  //add this line here
//  bool isCalibratedHeadPose = headPoseTracker.isCalibrated();

// std::vector<std::string> detectedObjects;
//                     //Add if isCalibrated
//                     if (hasYoloResults && faceLandmarks.size() > 10 && isCalibratedHeadPose) {
//                         for (auto& re : yoloResult.results) {
//                             if (re.confidence >= 0.55) {
//                                 std::string className = yoloDetector->classes[re.classId];
//                                 detectedObjects.push_back(className);
//                             }
//                         }
//                         if (!detectedObjects.empty()) {
//                             detectedObjectsText = "Detected: ";
//                             for (size_t i = 0; i < detectedObjects.size(); ++i) {
//                                 detectedObjectsText += detectedObjects[i];
//                                 if (i < detectedObjects.size() - 1) {
//                                     detectedObjectsText += ", ";
//                                 }
//                             }
//                         }
//                     }
//                      else{
//                          detectedObjectsText = "";//Set is none when empty. To use default values
//                      }

//                     // Setting values for KSS Class
//                     kssCalculator.setPerclos(blinkDetector.getPerclos());
//                     //Check if headPoseResults.rows is greater than 0 before accessing its elements
//                     if(headPoseResults.rows.size() > 0){
//                       try {
//                             kssCalculator.setHeadPose(std::stod(headPoseResults.rows[0][1]), std::stod(headPoseResults.rows[1][1]), std::stod(headPoseResults.rows[2][1]));
//                         } catch (const std::invalid_argument& e) {
//                             std::cerr << "Caught invalid_argument: " << e.what() << std::endl;
//                             // Handle the error - perhaps set a default value or skip this KSS calculation
//                             kssCalculator.setHeadPose(0.0, 0.0, 0.0); // Set default values
//                         }
//                     }else {
//                          kssCalculator.setHeadPose(0.0, 0.0, 0.0); // Set default values
//                     }

//                     kssCalculator.setYawnMetrics(yawnMetrics.isYawning, yawnMetrics.yawnFrequency, yawnMetrics.yawnDuration);
//                     kssCalculator.setDetectedObjects(detectedObjects);

//                     // Calculate the composite KSS
//                     compositeKSS = kssCalculator.calculateCompositeKSS();
//                     kssStatus = kssCalculator.getKSSAlertStatus(compositeKSS);

//                      // Format the floating-point numbers to two decimal places
//                     perclos_ss.str(""); //Clear for storing next text
//                     lastBlinkDuration_ss.str("");
//                     blinkRate_ss.str("");
//                     yawnFrequency_ss.str("");
//                     yawnDuration_ss.str("");

//                     perclos_ss << std::fixed << std::setprecision(2) << blinkDetector.getPerclos();
//                     lastBlinkDuration_ss << std::fixed << std::setprecision(2) << blinkDetector.getLastBlinkDuration();
//                     blinkRate_ss << std::fixed << std::setprecision(2) << blinkDetector.getBlinkRate();
//                     yawnFrequency_ss << std::fixed << std::setprecision(2) << yawnMetrics.yawnFrequency;
//                     yawnDuration_ss << std::fixed << std::setprecision(2) << yawnMetrics.yawnDuration;

//                     //To convert yawn count to string
//                      yawnCountStr = std::to_string(static_cast<int>(yawnMetrics.yawnCount));

//                     // Calculate the text size and position for top-center alignment
//                    //Moved Text outside as well

//                     textSize = cv::getTextSize(detectedObjectsText, cv::FONT_HERSHEY_PLAIN, textScaleObjects, 2, &baseline);
//                     textOrg = cv::Point((frame.cols - textSize.width) / 2, textSize.height + 10); // Center horizontally, 10 pixels from top


//                     // Display KSS Status at the top-left (if compositeKSS is 4 or higher)
//                     textColor=cv::Scalar(0,255,0);
                   
//                     if (compositeKSS >= 4) {
//                        textColor=cv::Scalar(0,0,255); // Red color
//                       cv::putText(frame, "" + kssStatus, cv::Point(20, 30), cv::FONT_HERSHEY_PLAIN, 1.5, textColor, 2);
//                     }

//                      // Display detectedObjectsText at the top center
                  
//                     // Display detectedObjectsText at the top center
//                     cv::putText(frame, detectedObjectsText, textOrg, cv::FONT_HERSHEY_PLAIN, textScaleObjects, textColorObjects, 2);

//                     // Display text information

//                     int textY = 60 ;//Set the intial position of txt
//                     int lineHeight = 20 ; // Distance
//                     double textScale = 0.7; // Smaller text scale
//                     int thickness = 1; 
//                     // Display text information
//                     cv::putText(frame, "PERCLOS: " + perclos_ss.str(), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, thickness);
//                      textY+=lineHeight;
//                     cv::putText(frame, "Blink counts: " + std::to_string(blinkDetector.getBlinkCount()), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                     textY+=lineHeight;
//                     cv::putText(frame, "Last Blink Duration: " + lastBlinkDuration_ss.str() + " s", cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                      textY+=lineHeight;
//                     cv::putText(frame, "Blink Rate: " + blinkRate_ss.str() + " BPM", cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                     textY+=lineHeight;
//                     //Accessing yaw, pitch and roll only when its greater than 0
//                     if(headPoseResults.rows.size() > 0){
//                     cv::putText(frame, "Yaw: " + headPoseResults.rows[0][1], cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                      textY+=lineHeight;
//                     cv::putText(frame, "Pitch: " + headPoseResults.rows[1][1], cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                      textY+=lineHeight;
//                     cv::putText(frame, "Roll: " + headPoseResults.rows[2][1], cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                      textY+=lineHeight;
//                      }
//                     cv::putText(frame, "Yawning: " + std::string(yawnMetrics.isYawning ? "Yes" : "No"), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                       textY+=lineHeight;
//                     cv::putText(frame, "Yawn Frequency: " + yawnFrequency_ss.str(), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                      textY+=lineHeight;
//                     cv::putText(frame, "Yawn Duration: " + yawnDuration_ss.str(), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                      textY+=lineHeight;
//                     cv::putText(frame, "Yawn Count: " +  yawnCountStr, cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
//                     textY+=lineHeight;
//                     cv::putText(frame, "Composite KSS: " + std::to_string(compositeKSS), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, thickness);
                   

// #if SHOW_FPS
//                     auto stop = std::chrono::steady_clock::now();
//                     auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
//                     float inferenceTime = duration.count() / 1e3;
//                     sum += inferenceTime;
//                     count += 1;
//                     int fps = (int)1e3 / inferenceTime;

//                     // Move FPS to top-right corner
//                     std::string fpsText = std::to_string(fps);
//                     textSize = cv::getTextSize(fpsText, cv::FONT_HERSHEY_PLAIN, 3, 2, &baseline);
//                     textOrg = cv::Point((frame.cols - textSize.width - 10), (textSize.height + 10));

//                     cv::putText(frame, fpsText, textOrg, cv::FONT_HERSHEY_PLAIN, 3, cv::Scalar(0, 196, 255), 2);
// #endif

//                     cv::imshow("Face and Iris Landmarks", frame);
//                 }
//                 gst_buffer_unmap(buffer, &mapInfo);
//             }
//             gst_sample_unref(sample);
//         } else{
//              break;
//         }


//         if (cv::waitKey(10) == 27)
//             break;
//     }

//     // Clean up
//     std::cout << "Exit loop" << std::endl;
//     stopRequested = true;
//     yoloThread.join();
//     if (yoloDetector)
//         delete yoloDetector;

//     // Stop the pipeline
//     gst_element_set_state(pipeline, GST_STATE_NULL);
//     // Free resources
//     gst_object_unref(appsink);
//     gst_object_unref(pipeline);
//     gst_deinit();

//     return 0;
// }



#include "IrisLandmark.hpp"
#include "BlinkDetector.hpp"
#include "YawnDetector.hpp"
#include "HeadPoseTracker.hpp"
#include "inference.h" 
#include "KSSCalculator.hpp"
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iomanip>
#include <sstream>
#include <thread>
#include <queue>
#include <atomic>
#include <mutex>
#include <gst/gst.h>
#include <gst/app/gstappsink.h> 
#include <memory> 
#include <chrono>

#ifdef _WIN32
#include <Windows.h> // For Windows thread priority
#else
#include <pthread.h> // For POSIX thread priority (Linux)
#include <sched.h>
#endif

#define SHOW_FPS    (1)
#define MAX_QUEUE_SIZE 5 // Limit the queue size to prevent excessive memory usage
const int CALIBRATION_TIME = 5;

// Declare sum and count outside 
#if SHOW_FPS
#include <chrono>
    float sum = 0;
    int count = 0;
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
// YOLO Worker with Frame Skipping
void yoloWorker(YOLO_V8* yoloDetector, std::queue<std::shared_ptr<cv::Mat>>& inputQueue, std::queue<YOLO_Result>& outputQueue, std::atomic<bool>& stopRequested) {
    while (!stopRequested.load()) {
        std::shared_ptr<cv::Mat> framePtr;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (!inputQueue.empty()) {
                framePtr = inputQueue.front();
                inputQueue.pop();
            } else {
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
        }
        if (framePtr) {
            YOLO_Result yoloResult;
            yoloResult.frame = *framePtr; // Access cv::Mat through shared pointer
            yoloDetector->RunSession(yoloResult.frame, yoloResult.results); // Use asynchronous inference
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

// Function to create a GStreamer pipeline string for webcam capture
std::string createGStreamerPipeline(int captureWidth, int captureHeight, int displayWidth, int displayHeight, int framerate) {
#ifdef _WIN32
    // Windows pipeline (example, may need adjustments)
    return "ksvideosrc ! videoconvert ! video/x-raw,format=BGRx,width=" + std::to_string(captureWidth) + ",height=" + std::to_string(captureHeight) + ",framerate=" + std::to_string(framerate) + "/1 ! videoconvert ! video/x-raw,format=BGR ! appsink name=sink";
#else
    // Linux pipeline (v4l2src)
    return "v4l2src ! videoconvert ! video/x-raw,format=BGR,width=" + std::to_string(captureWidth) + ",height=" + std::to_string(captureHeight) + ",framerate=" + std::to_string(framerate) + "/1 ! appsink name=sink";
#endif
}

int main(int argc, char* argv[]) {
    // Initialize GStreamer
    gst_init(&argc, &argv);

    my::IrisLandmark irisLandmarker("");
    my::HeadPoseTracker headPoseTracker;
    my::BlinkDetector blinkDetector;  
    YawnDetector yawnDetector;

    // Initialize KSSCalculator
    KSSCalculator kssCalculator;

    // Local variable definitions
    std::queue<std::shared_ptr<cv::Mat>> yoloInputQueue;
    std::queue<YOLO_Result> yoloOutputQueue;
    std::atomic<bool> stopRequested(false); // Initialize atomic variable
    YOLO_V8* yoloDetector = nullptr; // Declare
    std::thread yoloThread; // Must declare to tell. It is thread

    // Define capture and display parameters
    int captureWidth = 640;
    int captureHeight = 480;
    int displayWidth = 640;
    int displayHeight = 480;
    int framerate = 30;

    // Create GStreamer pipeline
    std::string pipelineString = createGStreamerPipeline(captureWidth, captureHeight, displayWidth, displayHeight, framerate);
    std::cout << "Using pipeline: " << pipelineString << std::endl;

    // Create GStreamer pipeline from string
    GstElement* pipeline = gst_parse_launch(pipelineString.c_str(), nullptr);
    if (!pipeline) {
        std::cerr << "Failed to create pipeline" << std::endl;
        return -1;
    }

    // Get the appsink
    GstElement* appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    if (!appsink) {
        std::cerr << "Failed to get appsink" << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    // Configure the appsink to emit buffers that OpenCV can read
    GstCaps* caps = gst_caps_new_simple("video/x-raw",
        "format", G_TYPE_STRING, "BGR",
        "width", G_TYPE_INT, displayWidth,
        "height", G_TYPE_INT, displayHeight,
        NULL);
    gst_app_sink_set_caps(GST_APP_SINK(appsink), caps);
    gst_caps_unref(caps);


    // Set the pipeline to playing state
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // YOLO Initialization
    DL_INIT_PARAM params;
    params.rectConfidenceThreshold = 0.2;
    params.iouThreshold = 0.5;
    params.modelPath = "../models/ob.onnx";
    params.imgSize = {640, 640}; 
#ifdef USE_CUDA
    params.cudaEnable = true;
    params.modelType = YOLO_DETECT_V8;  
#else
    params.modelType = YOLO_DETECT_V8; 
    params.cudaEnable = false;
#endif
    yoloDetector = new YOLO_V8(params); 
    yoloDetector->classes = {"cigarette", "drinking", "eating", "mobile"}; 

    // Create and start the YOLO thread
    yoloThread = std::thread(yoloWorker, yoloDetector, std::ref(yoloInputQueue), std::ref(yoloOutputQueue), std::ref(stopRequested));

        
    #ifdef _WIN32
        SetThreadPriority(yoloThread.native_handle(), THREAD_PRIORITY_ABOVE_NORMAL);
    #else
        sched_param paramsThread;
        paramsThread.sched_priority = sched_get_priority_max(SCHED_FIFO)/2; //Set to 1/2 max value
        pthread_setschedparam(yoloThread.native_handle(), SCHED_FIFO, &paramsThread);
    #endif


    // Calibration variables
    bool calibration_done = false;
    auto calibration_start_time = std::chrono::steady_clock::now();

    // Declare these variables *before* the main loop

    // KSS values
    int compositeKSS;
    std::string kssStatus;

    //Object detection text
    double textScaleObjects = 1.0; // Reduced text scale
    cv::Scalar textColorObjects(0, 0, 255); // color
    std::string detectedObjectsText;

    // Formatted Values as string
    std::stringstream perclos_ss, lastBlinkDuration_ss, blinkRate_ss, yawnFrequency_ss, yawnDuration_ss;
    std::string yawnCountStr ;// Formatted number as String
    //For printing green color when no alert
    cv::Scalar textColor(0, 255, 0); //Red Green is the best

    // Main loop (GStreamer)
    GstSample* sample;
    GstBuffer* buffer;
    GstMapInfo mapInfo;
    cv::Mat frame;
    while (!stopRequested) {
        // Get the sample from appsink
        sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
        if (sample) {
            buffer = gst_sample_get_buffer(sample);
            if (buffer) {
                gst_buffer_map(buffer, &mapInfo, GST_MAP_READ);
                frame = cv::Mat(cv::Size(displayWidth, displayHeight), CV_8UC3, mapInfo.data, cv::Mat::AUTO_STEP);

                if (!frame.empty()) {
                    cv::flip(frame, frame, 1);

                    int baseline;
                    cv::Size textSize;
                    cv::Point textOrg;

#if SHOW_FPS
                    auto start = std::chrono::steady_clock::now();
#endif

                    irisLandmarker.loadImageToInput(frame);
                    irisLandmarker.runInference();

                    blinkDetector.run(frame, irisLandmarker);
                    std::vector<cv::Point> faceLandmarks = irisLandmarker.getAllFaceLandmarks();
                    bool isReferenceSet;

                    //Head pose Detection
                    my::HeadPoseTracker::HeadPoseResults headPoseResults = headPoseTracker.run(faceLandmarks);
                    isReferenceSet = headPoseResults.reference_set;

                    //Add this condition
                if (!calibration_done) {
                    if (faceLandmarks.size() > 10) {
                        auto now = std::chrono::steady_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - calibration_start_time).count();

                        if (duration >= CALIBRATION_TIME) {
                             headPoseTracker.run(faceLandmarks);
                            if (headPoseResults.reference_set) {
                                calibration_done = true;
                                std::cout << "Calibration Complete!" << std::endl;
                            }
                        } else {
                            // Display Calibration Message
                            cv::putText(frame, "Calibrating...", cv::Point(20, 30), cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(0, 0, 255), 2);
                        }
                    } else {
                        calibration_start_time = std::chrono::steady_clock::now(); // Reset timer if no face
                    }
                }
YawnDetector::YawnMetrics yawnMetrics;
 //start to process only when calibration is done
                     std::shared_ptr<cv::Mat> framePtr = std::make_shared<cv::Mat>(frame.clone());
                   if (calibration_done) {

 if (faceLandmarks.size() > 10) {
                        yawnMetrics = yawnDetector.run(frame, faceLandmarks);

                        std::unique_lock<std::mutex> lock(queueMutex);
                        if (yoloInputQueue.size() >= MAX_QUEUE_SIZE) {
                            yoloInputQueue.pop(); // Drop the oldest frame if the queue is full
                        }
                        yoloInputQueue.push(framePtr); // Put the frame in the input queue
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

bool isCalibratedHeadPose = headPoseTracker.isCalibrated();

std::vector<std::string> detectedObjects;
                    //Add if isCalibrated
                    if (hasYoloResults && faceLandmarks.size() > 10 && isCalibratedHeadPose) {
                        for (auto& re : yoloResult.results) {
                            if (re.confidence >= 0.55) {
                                std::string className = yoloDetector->classes[re.classId];
                                detectedObjects.push_back(className);
                            }
                        }
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
                     else{
                         detectedObjectsText = "";//Set is none when empty. To use default values
                     }

                    // Setting values for KSS Class
                    kssCalculator.setPerclos(blinkDetector.getPerclos());
                    //Check if headPoseResults.rows is greater than 0 before accessing its elements
                    if(headPoseResults.rows.size() > 0){
                      try {
                            kssCalculator.setHeadPose(std::stod(headPoseResults.rows[0][1]), std::stod(headPoseResults.rows[1][1]), std::stod(headPoseResults.rows[2][1]));
                        } catch (const std::invalid_argument& e) {
                            std::cerr << "Caught invalid_argument: " << e.what() << std::endl;
                            // Handle the error - perhaps set a default value or skip this KSS calculation
                            kssCalculator.setHeadPose(0.0, 0.0, 0.0); // Set default values
                        }
                    }else {
                         kssCalculator.setHeadPose(0.0, 0.0, 0.0); // Set default values
                    }

                    kssCalculator.setYawnMetrics(yawnMetrics.isYawning, yawnMetrics.yawnFrequency, yawnMetrics.yawnDuration);
                    kssCalculator.setDetectedObjects(detectedObjects);

                    // Calculate the composite KSS
                    compositeKSS = kssCalculator.calculateCompositeKSS();
                    kssStatus = kssCalculator.getKSSAlertStatus(compositeKSS);

                     // Format the floating-point numbers to two decimal places
                    perclos_ss.str(""); //Clear for storing next text
                    lastBlinkDuration_ss.str("");
                    blinkRate_ss.str("");
                    yawnFrequency_ss.str("");
                    yawnDuration_ss.str("");

                    perclos_ss << std::fixed << std::setprecision(2) << blinkDetector.getPerclos();
                    lastBlinkDuration_ss << std::fixed << std::setprecision(2) << blinkDetector.getLastBlinkDuration();
                    blinkRate_ss << std::fixed << std::setprecision(2) << blinkDetector.getBlinkRate();
                    yawnFrequency_ss << std::fixed << std::setprecision(2) << yawnMetrics.yawnFrequency;
                    yawnDuration_ss << std::fixed << std::setprecision(2) << yawnMetrics.yawnDuration;

                    //To convert yawn count to string
                     yawnCountStr = std::to_string(static_cast<int>(yawnMetrics.yawnCount));

                    // Calculate the text size and position for top-center alignment

                    textSize = cv::getTextSize(detectedObjectsText, cv::FONT_HERSHEY_PLAIN, textScaleObjects, 2, &baseline);
                    textOrg = cv::Point((frame.cols - textSize.width) / 2, textSize.height + 10); // Center horizontally, 10 pixels from top


                    // Display KSS Status at the top-left (if compositeKSS is 4 or higher)
                    textColor=cv::Scalar(0,255,0);
                   
                    if (compositeKSS >= 4) {
                       textColor=cv::Scalar(0,0,255); // Red color
                      cv::putText(frame, "" + kssStatus, cv::Point(20, 30), cv::FONT_HERSHEY_PLAIN, 1.5, textColor, 2);
                    }

        
                    // Display detectedObjectsText at the top center
                    cv::putText(frame, detectedObjectsText, textOrg, cv::FONT_HERSHEY_PLAIN, textScaleObjects, textColorObjects, 2);

                    // Display text information

                    int textY = 60 ;
                    int lineHeight = 20;
                    double textScale = 0.7; 
                    int thickness = 1; 
                    // Display text information
                    cv::putText(frame, "PERCLOS: " + perclos_ss.str(), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, thickness);
                     textY+=lineHeight;
                    cv::putText(frame, "Blink counts: " + std::to_string(blinkDetector.getBlinkCount()), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
                    textY+=lineHeight;
                    cv::putText(frame, "Last Blink Duration: " + lastBlinkDuration_ss.str() + " s", cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
                     textY+=lineHeight;
                    cv::putText(frame, "Blink Rate: " + blinkRate_ss.str() + " BPM", cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
                    textY+=lineHeight;
                    //Accessing yaw, pitch and roll only when its greater than 0
                    if(headPoseResults.rows.size() > 0){
                    cv::putText(frame, "Yaw: " + headPoseResults.rows[0][1], cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
                     textY+=lineHeight;
                    cv::putText(frame, "Pitch: " + headPoseResults.rows[1][1], cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
                     textY+=lineHeight;
                    cv::putText(frame, "Roll: " + headPoseResults.rows[2][1], cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
                     textY+=lineHeight;
                     }
                    cv::putText(frame, "Yawning: " + std::string(yawnMetrics.isYawning ? "Yes" : "No"), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
                      textY+=lineHeight;
                    cv::putText(frame, "Yawn Frequency: " + yawnFrequency_ss.str(), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
                     textY+=lineHeight;
                    cv::putText(frame, "Yawn Duration: " + yawnDuration_ss.str(), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
                     textY+=lineHeight;
                    cv::putText(frame, "Yawn Count: " +  yawnCountStr, cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, 1);
                    textY+=lineHeight;
                    cv::putText(frame, "Composite KSS: " + std::to_string(compositeKSS), cv::Point(20, textY), cv::FONT_HERSHEY_PLAIN, textScale, textColor, thickness);
                   
                  }

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
                    textOrg = cv::Point((frame.cols - textSize.width - 10), (textSize.height + 10));

                    cv::putText(frame, fpsText, textOrg, cv::FONT_HERSHEY_PLAIN, 3, cv::Scalar(0, 196, 255), 2);
#endif

                    cv::imshow("Face and Iris Landmarks", frame);
                }
                gst_buffer_unmap(buffer, &mapInfo);
            }
            gst_sample_unref(sample);
        } else{
             break;
        }


        if (cv::waitKey(10) == 27)
            break;
    }

    // Clean up
    std::cout << "Exit loop" << std::endl;
    stopRequested = true;
    yoloThread.join();
    if (yoloDetector)
        delete yoloDetector;

    // Stop the pipeline
    gst_element_set_state(pipeline, GST_STATE_NULL);
    
    // Free resources
    gst_object_unref(appsink);
    gst_object_unref(pipeline);
    gst_deinit();

    return 0;
}