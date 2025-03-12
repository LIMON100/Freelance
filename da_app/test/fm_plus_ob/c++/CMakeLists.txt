# cmake_minimum_required(VERSION 3.16..3.21)
# project(FaceMeshCpp)

# set(CMAKE_CXX_STANDARD 14)

# # Find OpenCV libraries
# find_package(OpenCV REQUIRED)
# if (NOT OpenCV_FOUND)
#     message(FATAL_ERROR "OpenCV was not found")
# endif()

# # Find Eigen3
# find_package(Eigen3 REQUIRED)
# if(NOT Eigen3_FOUND)
#    message(FATAL_ERROR "Eigen3 not found. Please install Eigen3.")
# endif()

# # Set 3rd party path
# set(TFLite_PATH "/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/bface/dms/c++/test/iris_test/tensorflowlite")
# set(TFLite_INCLUDE_DIRS "${TFLite_PATH}/include")
# set(TFLite_LIB_DIR "${TFLite_PATH}/libs/linux_x86_64")
# set(TFLite_LIBS "${TFLite_LIB_DIR}/libtensorflowlite.so")

# # -------------- ONNXRUNTIME  ------------------#
# # Use system-installed ONNX Runtime
# include_directories(/usr/local/include/onnxruntime/include)  # Add this line
# link_directories(/usr/local/lib)

# set(SOURCES
#     src/main.cpp
#     src/FaceLandmark.cpp
#     src/BlinkDetector.cpp
#     src/YawnDetector.cpp
#     src/inference.cpp
#     src/FaceDetection.cpp
#     src/ModelLoader.cpp
#     src/DetectionPostProcess.cpp
#     src/IrisLandmark.cpp 
# )

# add_executable(${PROJECT_NAME} ${SOURCES})

# include_directories(
#     ${OpenCV_INCLUDE_DIRS}
#     ${TFLite_INCLUDE_DIRS}
#    #${EIGEN3_INCLUDE_DIR} REMOVE IT for not using fornow
# )

# target_link_libraries(${PROJECT_NAME}
#     ${OpenCV_LIBS}
#     ${TFLite_LIBS}
#     onnxruntime
#     )

# # target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} onnxruntime)

# # For Windows system, copy onnxruntime.dll to the same folder of the executable file
# if (WIN32)
#     add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#             COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             "${ONNXRUNTIME_ROOT}/lib/onnxruntime.dll"
#             $<TARGET_FILE_DIR:${PROJECT_NAME}>)
# endif ()

# # Create folder name images in the same folder of the executable file
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/images
# )

cmake_minimum_required(VERSION 3.16..3.21)
project(FaceMeshCpp)

set(CMAKE_CXX_STANDARD 17)

# Find OpenCV libraries
find_package(OpenCV REQUIRED)
if (NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV was not found")
endif()

# Find Eigen3
find_package(Eigen3 REQUIRED)
if(NOT Eigen3_FOUND)
   message(FATAL_ERROR "Eigen3 not found. Please install Eigen3.")
endif()

# Set 3rd party path
set(TFLite_PATH "/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/bface/dms/c++/test/iris_test/tensorflowlite")
set(TFLite_INCLUDE_DIRS "${TFLite_PATH}/include")
set(TFLite_LIB_DIR "${TFLite_PATH}/libs/linux_x86_64")
set(TFLite_LIBS "${TFLite_LIB_DIR}/libtensorflowlite.so")

# -------------- ONNXRUNTIME  ------------------#
# Use system-installed ONNX Runtime
include_directories(/usr/local/include/onnxruntime/include)  # Add this line
link_directories(/usr/local/lib)

set(SOURCES
    src/main.cpp
    src/FaceLandmark.cpp
    src/BlinkDetector.cpp
    src/YawnDetector.cpp
    src/inference.cpp
    src/FaceDetection.cpp
    src/ModelLoader.cpp
    src/DetectionPostProcess.cpp
    src/IrisLandmark.cpp
    src/HeadPoseTracker.cpp
    src/KSSCalculator.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${TFLite_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR} # Uncommnet
)

target_link_libraries(${PROJECT_NAME}
    ${OpenCV_LIBS}
    ${TFLite_LIBS}
    onnxruntime
    )

# target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} onnxruntime)

# For Windows system, copy onnxruntime.dll to the same folder of the executable file
if (WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_ROOT}/lib/onnxruntime.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif ()

# Create folder name images in the same folder of the executable file
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/images
)