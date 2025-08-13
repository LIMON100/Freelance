# CMake version and project name
cmake_minimum_required(VERSION 3.16)
project(hailo_speech_inference CXX)

# Use C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- THIS IS THE FIX ---
# Tell CMake to look for helper scripts in our 'cmake' directory
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Find necessary packages
find_package(Threads REQUIRED)
find_package(HailoRT REQUIRED)
find_package(OpenCV REQUIRED)
find_package(FFTW3 REQUIRED) 

# Add include directories for all libraries
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HAILORT_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${FFTW3_INCLUDE_DIRS}
)

# Define the executable from our main source file
add_executable(${PROJECT_NAME} main.cpp)

# Link the executable against all the required libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Threads::Threads
    HailoRT::libhailort
    ${OpenCV_LIBS}
    ${FFTW3f_LIBRARIES}
)