cmake_minimum_required(VERSION 3.20)
project(byte-track-eigen VERSION 1.0.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Add library
add_library(ByteTrackEigen SHARED
    src/BaseTrack.cpp
    src/BoundingBoxTrackUtils.cpp
    src/BYTETracker.cpp
    src/HungarianAlgorithmEigen.cpp
    src/KalmanBBoxTrack.cpp
    src/KalmanFilter.cpp
    src/LinearAssignment.cpp
    src/BoundingBoxIoUMatching.cpp
)

# Compiler options for different build types
target_compile_options(ByteTrackEigen PRIVATE 
  $<$<CONFIG:Debug>:-DDEBUG -g>
  $<$<CONFIG:Release>:-O3>
)

# Set different runtime output directories for Debug and Release
set_target_properties(ByteTrackEigen PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
)

# --- EIGEN SETUP (No changes here) ---
set(EIGEN3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/eigen")
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/external/")
endif()
if(NOT EXISTS ${EIGEN3_INCLUDE_DIR})
    set(EIGEN3_URL "https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.zip")
    set(EIGEN3_ZIP "${CMAKE_BINARY_DIR}/eigen-3.4.0.zip")
    message(STATUS "Downloading Eigen to ${EIGEN3_ZIP}")
    file(DOWNLOAD ${EIGEN3_URL} ${EIGEN3_ZIP} STATUS EIGEN3_DOWNLOAD_STATUS TIMEOUT 60)
    list(GET EIGEN3_DOWNLOAD_STATUS 0 EIGEN3_DOWNLOAD_RESULT)
    if(EIGEN3_DOWNLOAD_RESULT EQUAL 0)
        message(STATUS "Extracting Eigen")
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${EIGEN3_ZIP} WORKING_DIRECTORY ${CMAKE_BINARY_DIR} RESULT_VARIABLE EIGEN3_EXTRACT_RESULT)
        if(EIGEN3_EXTRACT_RESULT EQUAL 0)
            file(RENAME "${CMAKE_BINARY_DIR}/eigen-3.4.0" ${EIGEN3_INCLUDE_DIR})
        else()
            message(FATAL_ERROR "Failed to extract Eigen")
        endif()
    else()
        message(FATAL_ERROR "Failed to download Eigen")
    endif()
endif()

# --- INCLUDE DIRECTORIES (THE FIX IS HERE) ---

# The library's own public headers are in the 'include' directory.
# The PUBLIC keyword means "anyone who links to me also needs this include path."
target_include_directories(ByteTrackEigen PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)

# The library needs Eigen to build itself, and anyone using the library's
# headers also needs Eigen. Therefore, this should also be PUBLIC.
target_include_directories(ByteTrackEigen PUBLIC ${EIGEN3_INCLUDE_DIR})


# Definitions for DLL export, if needed
target_compile_definitions(ByteTrackEigen PRIVATE BUILDING_BYTE_TRACK_EIGEN)