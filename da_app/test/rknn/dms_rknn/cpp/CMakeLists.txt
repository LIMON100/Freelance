cmake_minimum_required(VERSION 3.10)

project(rknn_multi_model_demo) # Combined project name

if (ENABLE_ASAN)
	message(STATUS "BUILD WITH ADDRESS SANITIZER")
	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif ()

# --- Use relative paths from the OLD CMakeLists ---
# Link common utils and 3rdparty (Assuming they are 3 levels up)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/ 3rdparty.out)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/ utils.out)
# --- End path fix ---

set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib") # Keep this as it likely refers to the build/install location

set(file_utils_file ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/file_utils.c)

# --- Select correct YOLO implementation based on SOC ---
set(rknpu_yolo11_impl_file yolo_detector/rknpu2/yolo11.cc) # Default

if (TARGET_SOC STREQUAL "rv1106" OR TARGET_SOC STREQUAL "rv1103")
    add_definitions(-DRV1106_1103)
    set(rknpu_yolo11_impl_file yolo_detector/rknpu2/yolo11_rv1106_1103.cc)
    # Include dma allocator headers if needed for this platform
    # Adjust path relative to THIS CMakeLists.txt if allocator is within 3rdparty
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/allocator/dma)
endif()

if(TARGET_SOC STREQUAL "rk1808" OR TARGET_SOC STREQUAL "rv1109" OR TARGET_SOC STREQUAL "rv1126")
    add_definitions(-DRKNPU1)
    set(rknpu_yolo11_impl_file yolo_detector/rknpu1/yolo11.cc)
endif()
# --- End YOLO implementation selection ---


# Define executable and source files
add_executable(${PROJECT_NAME}
    main.cc                         # Main application logic
    face_analyzer/face_analyzer.cc  # Face/Iris analysis implementation
    yolo_detector/postprocess.cc    # YOLO postprocessing
    ${rknpu_yolo11_impl_file}       # Selected YOLO implementation
    # REMOVED: ${file_utils_file} - No need to add .c files directly if linked via utils library
)

# Link libraries (Combine from both projects)
# The libraries 'imageutils', 'fileutils', 'imagedrawing' should be targets
# created by the add_subdirectory calls for 'utils' and potentially '3rdparty'
target_link_libraries(${PROJECT_NAME}
    imageutils      # From utils subdirectory
    fileutils       # From utils subdirectory
    imagedrawing    # From utils subdirectory
    # Add other libs from 3rdparty if needed
    ${LIBRKNNRT}    # RKNN Runtime
    dl              # Common
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

# Include directories (Add subdirectories AND utils/3rdparty)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/face_analyzer # Add face analyzer header path
    ${CMAKE_CURRENT_SOURCE_DIR}/yolo_detector   # Add yolo detector header path
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils  # Add path to utils headers <-- FIX
    # Add path to 3rdparty headers if needed
    # ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/include # Example path
    ${LIBRKNNRT_INCLUDES}
)

# --- Install Rules ---
# Install the executable
install(TARGETS ${PROJECT_NAME} DESTINATION .)

# Install example images (add more if needed)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../model/test.jpg # Example face image
    ${CMAKE_CURRENT_SOURCE_DIR}/../model/bus.jpg  # Example YOLO image
    DESTINATION model
)

# Install models and YOLO class file
# Adjust filenames as necessary
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../model/faceD.rknn
    ${CMAKE_CURRENT_SOURCE_DIR}/../model/faceL.rknn
    ${CMAKE_CURRENT_SOURCE_DIR}/../model/faceI.rknn  # Use correct iris model name
    ${CMAKE_CURRENT_SOURCE_DIR}/../model/od.rknn # Use correct YOLO model name
    ${CMAKE_CURRENT_SOURCE_DIR}/../model/custom_class.txt
    DESTINATION model
    OPTIONAL # Use OPTIONAL in case some models don't exist during certain builds
)

# NOTE: Zero-copy target from YOLO CMakeLists is omitted here for simplicity.