cmake_minimum_required(VERSION 3.15)
project(KoreanCommandPredictorV2 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Define paths to your dependencies ---
set(ONNXRUNTIME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/onnxruntime)

# --- Configure ONNX Runtime ---
include_directories(${ONNXRUNTIME_DIR}/include)
link_directories(${ONNXRUNTIME_DIR}/lib)

# --- Configure single-header libraries ---
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/single_header)

# --- Create the executable ---
add_executable(predictor_v2 voice_command_small.cpp)

# --- Link libraries ---
target_link_libraries(predictor_v2 PRIVATE onnxruntime)

# On Windows, copy the .dll for convenience
if(WIN32)
    add_custom_command(
        TARGET predictor_v2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ONNXRUNTIME_DIR}/lib/onnxruntime.dll"
        $<TARGET_FILE_DIR:predictor_v2>
    )
endif()