cmake_minimum_required(VERSION 3.15)
project(KoreanCommandPredictor CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Define paths to your dependencies ---
# Path to the extracted ONNX Runtime directory
set(ONNXRUNTIME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/onnxruntime)

# --- Configure ONNX Runtime ---
include_directories(${ONNXRUNTIME_DIR}/include)
link_directories(${ONNXRUNTIME_DIR}/lib)

# --- Configure single-header libraries ---
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/single_header)

# --- Create the executable ---
add_executable(predictor main.cpp)

# --- Link libraries ---
# For Linux/macOS, the library is called onnxruntime. For Windows, onnxruntime.lib
if(WIN32)
    target_link_libraries(predictor PRIVATE onnxruntime)
else()
    target_link_libraries(predictor PRIVATE onnxruntime)
endif()

# On Windows, you might also need to copy the .dll file
if(WIN32)
    add_custom_command(
        TARGET predictor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ONNXRUNTIME_DIR}/lib/onnxruntime.dll"
        $<TARGET_FILE_DIR:predictor>
    )
endif()
