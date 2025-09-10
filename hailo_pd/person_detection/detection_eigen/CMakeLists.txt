cmake_minimum_required(VERSION 3.16)
project(obj_det)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)
find_package(HailoRT REQUIRED)
find_package(OpenCV REQUIRED)
message(STATUS "Found OpenCV version: ${OpenCV_VERSION}")

# This will build the ByteTrackEigen library and, thanks to our fix,
# will correctly set up its public dependencies (like Eigen).
add_subdirectory(tracking_lib)

# Define the source files for the main executable
set(MAIN_SOURCES
    "object_detection/object_detection.cpp"
    "object_detection/utils/async_inference.cpp"
    "object_detection/utils/utils.cpp"
)
add_executable(${PROJECT_NAME} ${MAIN_SOURCES})

# Include directories needed ONLY by the executable itself.
# Eigen will be inherited automatically from the library.
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${OpenCV_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/object_detection/utils"
)

# Link the executable against its required libraries.
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Threads::Threads 
    HailoRT::libhailort 
    ${OpenCV_LIBS}
    ByteTrackEigen      # Because this is linked, Eigen's include path is inherited
    stdc++fs
)

target_compile_options(${PROJECT_NAME} PRIVATE 
    -Wall -Wextra $<$<CONFIG:Release>:-O3> $<$<CONFIG:Debug>:-g>
)

message(STATUS "Configured main executable '${PROJECT_NAME}' to link against 'ByteTrackEigen' library.")