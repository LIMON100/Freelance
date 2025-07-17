cmake_minimum_required(VERSION 3.15)
project(RknnCommandPredictor CXX)

# Set the C++ standard to at least 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Configure paths to our libraries ---
# Path to our custom RKNN API libs
set(RKNN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/rknn_api)
include_directories(${RKNN_API_DIR}/include)
# The linker needs to know where to find the .so file
link_directories(${RKNN_API_DIR}/lib)

# Single header libs
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/single_header)

# --- Create the executable from our main.cpp file ---
add_executable(rknn_predictor main.cpp)

# --- Link the required libraries ---
# This is the key change: link to rknnrt
target_link_libraries(rknn_predictor PRIVATE rknnrt)