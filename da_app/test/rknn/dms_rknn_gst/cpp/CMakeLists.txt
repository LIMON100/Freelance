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

# find_package(PkgConfig) 
 
# pkg_search_module(LIBXML2 REQUIRED libxml-2.0)
# pkg_search_module(GST REQUIRED gstreamer-1.0) 
# pkg_search_module(GST_VIDEO REQUIRED gstreamer-video-1.0) 
# pkg_search_module(GST_RTSP REQUIRED gstreamer-rtsp-1.0) 
# pkg_search_module(GST_RTSP_SERVER REQUIRED gstreamer-rtsp-server-1.0)
# pkg_search_module(GST_GL REQUIRED gstreamer-gl-1.0) 
# pkg_search_module(GST_GL_EGL REQUIRED gstreamer-gl-egl-1.0) 
# pkg_search_module(CV REQUIRED opencv4) 

# # --- Use relative paths from the OLD CMakeLists ---
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/ 3rdparty.out)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/ utils.out)
# # --- End path fix ---

# set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

# # --- Select correct YOLO implementation based on SOC ---
# set(rknpu_yolo11_impl_file yolo_detector/rknpu2/yolo11.cc) # Default

# if (TARGET_SOC STREQUAL "rv1106" OR TARGET_SOC STREQUAL "rv1103")
#     add_definitions(-DRV1106_1103)
#     set(rknpu_yolo11_impl_file yolo_detector/rknpu2/yolo11_rv1106_1103.cc)
#     include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/allocator/dma)
# endif() # <-- Make sure endif is on its own line

# if(TARGET_SOC STREQUAL "rk1808" OR TARGET_SOC STREQUAL "rv1109" OR TARGET_SOC STREQUAL "rv1126")
#     add_definitions(-DRKNPU1) # <-- Make sure add_definitions starts on a new line
#     set(rknpu_yolo11_impl_file yolo_detector/rknpu1/yolo11.cc)
# endif() # <-- Make sure endif is on its own line
# # --- End YOLO implementation selection ---


# # Define executable and source files
# add_executable(${PROJECT_NAME} # <-- Ensure this starts on a new line after the last endif()
#     main.cc                         # Main application logic
#     face_analyzer/face_analyzer.cc  # Face/Iris analysis implementation
#     yolo_detector/postprocess.cc    # YOLO postprocessing
#     ${rknpu_yolo11_impl_file}       # Selected YOLO implementation
#     # --- Add Behavior Analysis Sources ---
#     behavior_analysis/BlinkDetector.cpp
#     behavior_analysis/YawnDetector.cpp
#     behavior_analysis/HeadPoseTracker.cpp
#     behavior_analysis/KSSCalculator.cpp
# )

# # Link libraries
# target_link_libraries(${PROJECT_NAME}
#     imageutils
#     fileutils
#     imagedrawing
#     dl
#     gstapp-1.0 
#     librga.so librknnrt.so
#     ${GST_LIBRARIES} 
#     ${GST_VIDEOLIBRARIES}
#     ${GST_RTSP_LIBRARIES}
#     ${GST_RTSP_SERVER_LIBRARIES}   
#     ${GST_GL_LIBRARIES} 
#     ${GST_APP_LIBRARIES}
#     ${GST_GL_EGL_LIBRARIES} 
#     ${CV_LIBRARIES}
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
#     ${CMAKE_CURRENT_SOURCE_DIR}/behavior_analysis
#     ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils
#     ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/opencv/opencv-linux-aarch64/include
#     # ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/include # Add other 3rdparty includes if needed
#     # ${LIBRKNNRT_INCLUDES}
#     ${EIGEN3_INCLUDE_DIRS} # Add Eigen include path
 
# )

# # --- Install Rules ---
# # ... (Install rules remain the same) ...
# install(TARGETS ${PROJECT_NAME} DESTINATION .)
# install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/test.jpg ${CMAKE_CURRENT_SOURCE_DIR}/../model/5.jpg DESTINATION model)
# install(FILES
#     ${CMAKE_CURRENT_SOURCE_DIR}/../model/faceD.rknn
#     ${CMAKE_CURRENT_SOURCE_DIR}/../model/faceL.rknn
#     ${CMAKE_CURRENT_SOURCE_DIR}/../model/faceI.rknn  # Use correct iris model name
#     ${CMAKE_CURRENT_SOURCE_DIR}/../model/od.rknn # Use correct YOLO model name
#     ${CMAKE_CURRENT_SOURCE_DIR}/../model/custom_class.txt
#     DESTINATION model
#     OPTIONAL
# )

cmake_minimum_required(VERSION 3.10)

project(rknn_multi_model_demo)

if (ENABLE_ASAN)
    message(STATUS "BUILD WITH ADDRESS SANITIZER")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif ()

# --- Find Eigen3 ---
find_package(Eigen3 REQUIRED)
if(NOT Eigen3_FOUND)
    message(FATAL_ERROR "Eigen3 not found! Please install libeigen3-dev or set Eigen3_DIR.")
endif()

# --- Find PkgConfig ---
find_package(PkgConfig REQUIRED)

# --- Find GStreamer and related modules ---
pkg_search_module(LIBXML2 REQUIRED libxml-2.0)
pkg_search_module(GST REQUIRED gstreamer-1.0)
pkg_search_module(GST_VIDEO REQUIRED gstreamer-video-1.0)
pkg_search_module(GST_RTSP REQUIRED gstreamer-rtsp-1.0)
pkg_search_module(GST_RTSP_SERVER REQUIRED gstreamer-rtsp-server-1.0)
pkg_search_module(GST_GL REQUIRED gstreamer-gl-1.0)
pkg_search_module(GST_GL_EGL REQUIRED gstreamer-gl-egl-1.0)
pkg_search_module(GST_APP REQUIRED gstreamer-app-1.0) # Added for appsink
pkg_search_module(CV REQUIRED opencv4)
pkg_search_module(GST_ALLOCATORS REQUIRED gstreamer-allocators-1.0)

# --- Use relative paths from the OLD CMakeLists ---
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/ 3rdparty.out)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/ utils.out)

set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

# --- Select correct YOLO implementation based on SOC ---
set(rknpu_yolo11_impl_file yolo_detector/rknpu2/yolo11.cc) # Default

if (TARGET_SOC STREQUAL "rv1106" OR TARGET_SOC STREQUAL "rv1103")
    add_definitions(-DRV1106_1103)
    set(rknpu_yolo11_impl_file yolo_detector/rknpu2/yolo11_rv1106_1103.cc)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/allocator/dma)
endif()

if(TARGET_SOC STREQUAL "rk1808" OR TARGET_SOC STREQUAL "rv1109" OR TARGET_SOC STREQUAL "rv1126")
    add_definitions(-DRKNPU1)
    set(rknpu_yolo11_impl_file yolo_detector/rknpu1/yolo11.cc)
endif()

# Define executable and source files
add_executable(${PROJECT_NAME}
    main.cc                         # Main application logic
    face_analyzer/face_analyzer.cc  # Face/Iris analysis implementation
    yolo_detector/postprocess.cc    # YOLO postprocessing
    ${rknpu_yolo11_impl_file}       # Selected YOLO implementation
    # --- Add Behavior Analysis Sources ---
    behavior_analysis/BlinkDetector.cpp
    behavior_analysis/YawnDetector.cpp
    behavior_analysis/HeadPoseTracker.cpp
    behavior_analysis/KSSCalculator.cpp
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    imageutils
    fileutils
    imagedrawing
    dl
    librga.so
    librknnrt.so
    ${GST_LIBRARIES}
    ${GST_VIDEO_LIBRARIES}
    ${GST_RTSP_LIBRARIES}
    ${GST_RTSP_SERVER_LIBRARIES}
    ${GST_GL_LIBRARIES}
    ${GST_APP_LIBRARIES}  # Added for appsink
    ${GST_GL_EGL_LIBRARIES}
    ${CV_LIBRARIES}
    ${GST_ALLOCATORS_LIBRARIES}
)

# Platform-specific libraries
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
    ${GST_INCLUDE_DIRS}         # GStreamer include dirs
    ${GST_VIDEO_INCLUDE_DIRS}   # GStreamer video include dirs
    ${GST_RTSP_INCLUDE_DIRS}    # GStreamer RTSP include dirs
    ${GST_RTSP_SERVER_INCLUDE_DIRS}  # GStreamer RTSP server include dirs
    ${GST_GL_INCLUDE_DIRS}      # GStreamer GL include dirs
    ${GST_APP_INCLUDE_DIRS}     # GStreamer app include dirs
    ${GST_GL_EGL_INCLUDE_DIRS}  # GStreamer GL EGL include dirs
    ${CV_INCLUDE_DIRS}          # OpenCV include dirs
    ${EIGEN3_INCLUDE_DIRS}      # Eigen include path
    ${GST_ALLOCATORS_INCLUDE_DIRS}
)

# --- Install Rules ---
install(TARGETS ${PROJECT_NAME} DESTINATION .)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/test.jpg ${CMAKE_CURRENT_SOURCE_DIR}/../model/5.jpg DESTINATION model)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../model/faceD.rknn
    ${CMAKE_CURRENT_SOURCE_DIR}/../model/faceL.rknn
    ${CMAKE_CURRENT_SOURCE_DIR}/../model/faceI.rknn
    ${CMAKE_CURRENT_SOURCE_DIR}/../model/od.rknn
    ${CMAKE_CURRENT_SOURCE_DIR}/../model/custom_class.txt
    DESTINATION model
    OPTIONAL
)