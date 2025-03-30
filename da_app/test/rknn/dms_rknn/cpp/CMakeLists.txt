# cmake_minimum_required(VERSION 3.10)

# project(rknn_multi_model_demo)

# if (ENABLE_ASAN)
# 	message(STATUS "BUILD WITH ADDRESS SANITIZER")
# 	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
# 	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
# 	set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
# endif ()

# # --- Find Eigen3 ---
# find_package(Eigen3 REQUIRED)
# if(NOT Eigen3_FOUND)
#     message(FATAL_ERROR "Eigen3 not found! Please install libeigen3-dev or set Eigen3_DIR.")
# endif()
# # --- End Find Eigen3 ---


# # --- Use relative paths from the OLD CMakeLists ---
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/ 3rdparty.out)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/ utils.out)
# # --- End path fix ---

# set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

# set(file_utils_file ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/file_utils.c)

# # --- Select correct YOLO implementation based on SOC ---
# # ... (YOLO SOC selection logic remains the same) ...
# set(rknpu_yolo11_impl_file yolo_detector/rknpu2/yolo11.cc) # Default
# if (TARGET_SOC STREQUAL "rv1106" OR TARGET_SOC STREQUAL "rv1103") add_definitions(-DRV1106_1103); set(rknpu_yolo11_impl_file yolo_detector/rknpu2/yolo11_rv1106_1103.cc); include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/allocator/dma); endif()
# if(TARGET_SOC STREQUAL "rk1808" OR TARGET_SOC STREQUAL "rv1109" OR TARGET_SOC STREQUAL "rv1126") add_definitions(-DRKNPU1); set(rknpu_yolo11_impl_file yolo_detector/rknpu1/yolo11.cc); endif()
# # --- End YOLO implementation selection ---


# # Define executable and source files
# add_executable(${PROJECT_NAME}
#     main.cc                         # Main application logic
#     face_analyzer/face_analyzer.cc  # Face/Iris analysis implementation
#     yolo_detector/postprocess.cc    # YOLO postprocessing
#     ${rknpu_yolo11_impl_file}       # Selected YOLO implementation
#     # --- Add Behavior Analysis Sources ---
#     behavior_analysis/BlinkDetector.cpp
#     behavior_analysis/YawnDetector.cpp
#     behavior_analysis/HeadPoseTracker.cpp
#     behavior_analysis/KSSCalculator.cpp
#     # --- End Behavior Analysis Sources ---
# )

# # Link libraries
# target_link_libraries(${PROJECT_NAME}
#     imageutils
#     fileutils
#     imagedrawing
#     # Add other libs from 3rdparty if needed
#     ${LIBRKNNRT}
#     dl
# )

# # Platform-specific libraries
# if (CMAKE_SYSTEM_NAME STREQUAL "Android")
#     target_link_libraries(${PROJECT_NAME} log)
# endif()
# if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
#     set(THREADS_PREFER_PTHREAD_FLAG ON)
#     find_package(Threads REQUIRED)
#     target_link_libraries(${PROJECT_NAME} Threads::Threads)
# endif()

# # Include directories
# target_include_directories(${PROJECT_NAME} PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}
#     ${CMAKE_CURRENT_SOURCE_DIR}/face_analyzer
#     ${CMAKE_CURRENT_SOURCE_DIR}/yolo_detector
#     ${CMAKE_CURRENT_SOURCE_DIR}/behavior_analysis # <-- Add behavior analysis header path
#     ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils
#     # ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/include # Add if needed
#     ${LIBRKNNRT_INCLUDES}
#     ${EIGEN3_INCLUDE_DIRS} # <-- Add Eigen include path
# )

# # --- Install Rules ---
# # ... (Install rules remain the same, ensure all models/txt are listed) ...
# install(TARGETS ${PROJECT_NAME} DESTINATION .)
# install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/test.jpg ${CMAKE_CURRENT_SOURCE_DIR}/../model/bus.jpg DESTINATION model)
# install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/faceD.rknn ${CMAKE_CURRENT_SOURCE_DIR}/../model/faceL.rknn ${CMAKE_CURRENT_SOURCE_DIR}/../model/faceI.rknn ${CMAKE_CURRENT_SOURCE_DIR}/../model/od.rknn ${CMAKE_CURRENT_SOURCE_DIR}/../model/custom_class.txt DESTINATION model OPTIONAL)




cmake_minimum_required(VERSION 3.10)

project(rknn_multi_model_demo)

if (ENABLE_ASAN)
	message(STATUS "BUILD WITH ADDRESS SANITIZER")
	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif ()

# --- Find Eigen3 ---
find_package(Eigen3 REQUIRED)
if(NOT Eigen3_FOUND)
    message(FATAL_ERROR "Eigen3 not found! Please install libeigen3-dev or set Eigen3_DIR.")
endif()
# --- End Find Eigen3 ---


# --- Use relative paths from the OLD CMakeLists ---
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/ 3rdparty.out)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/ utils.out)
# --- End path fix ---

set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

# --- Select correct YOLO implementation based on SOC ---
set(rknpu_yolo11_impl_file yolo_detector/rknpu2/yolo11.cc) # Default

if (TARGET_SOC STREQUAL "rv1106" OR TARGET_SOC STREQUAL "rv1103")
    add_definitions(-DRV1106_1103)
    set(rknpu_yolo11_impl_file yolo_detector/rknpu2/yolo11_rv1106_1103.cc)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/allocator/dma)
endif() # <-- Make sure endif is on its own line

if(TARGET_SOC STREQUAL "rk1808" OR TARGET_SOC STREQUAL "rv1109" OR TARGET_SOC STREQUAL "rv1126")
    add_definitions(-DRKNPU1) # <-- Make sure add_definitions starts on a new line
    set(rknpu_yolo11_impl_file yolo_detector/rknpu1/yolo11.cc)
endif() # <-- Make sure endif is on its own line
# --- End YOLO implementation selection ---


# Define executable and source files
add_executable(${PROJECT_NAME} # <-- Ensure this starts on a new line after the last endif()
    main.cc                         # Main application logic
    face_analyzer/face_analyzer.cc  # Face/Iris analysis implementation
    yolo_detector/postprocess.cc    # YOLO postprocessing
    ${rknpu_yolo11_impl_file}       # Selected YOLO implementation
    # --- Add Behavior Analysis Sources ---
    behavior_analysis/BlinkDetector.cpp
    behavior_analysis/YawnDetector.cpp
    behavior_analysis/HeadPoseTracker.cpp
    behavior_analysis/KSSCalculator.cpp
    # --- End Behavior Analysis Sources ---
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    imageutils
    fileutils
    imagedrawing
    # Add other libs from 3rdparty if needed
    ${LIBRKNNRT}
    dl
    # Add OpenCV libs if needed for linking (unlikely if only headers used)
    # Example: opencv_core opencv_imgproc etc. (These MUST match the actual library names)
)

# Platform-specific libraries
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(${PROJECT_NAME} log)
endif()
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} Threads::Threads)
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/face_analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/yolo_detector
    ${CMAKE_CURRENT_SOURCE_DIR}/behavior_analysis
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils
    # --- Add OpenCV Include Path ---
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/opencv/opencv-linux-aarch64/include
    # --- End OpenCV Include Path ---
    # ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/include # Add other 3rdparty includes if needed
    ${LIBRKNNRT_INCLUDES}
    ${EIGEN3_INCLUDE_DIRS} # Add Eigen include path
)

# --- Install Rules ---
# ... (Install rules remain the same) ...
install(TARGETS ${PROJECT_NAME} DESTINATION .)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/test.jpg ${CMAKE_CURRENT_SOURCE_DIR}/../model/bus.jpg DESTINATION model)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/faceD.rknn ${CMAKE_CURRENT_SOURCE_DIR}/../model/faceL.rknn ${CMAKE_CURRENT_SOURCE_DIR}/../model/iris.rknn ${CMAKE_CURRENT_SOURCE_DIR}/../model/yolo11.rknn ${CMAKE_CURRENT_SOURCE_DIR}/../model/custom_class.txt DESTINATION model OPTIONAL)