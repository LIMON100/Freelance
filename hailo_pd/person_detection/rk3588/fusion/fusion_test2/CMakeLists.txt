cmake_minimum_required(VERSION 3.16)
project(sky_hunter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Find Required Packages ---
find_package(Threads REQUIRED)
find_package(HailoRT REQUIRED)
find_package(OpenCV REQUIRED)
message(STATUS "Found OpenCV version: ${OpenCV_VERSION}")


# --- Find GStreamer ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)
if (NOT GSTREAMER_FOUND)
    message(FATAL_ERROR "GStreamer development libraries not found.")
endif()
message(STATUS "Found GStreamer libs: ${GSTREAMER_LIBRARIES}")
# --- END ---

# --- NEW: Add nlohmann/json for parsing calibration files ---
# This uses FetchContent to download the header-only library if it's not found.
include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.2 # A recent, stable version
  GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(nlohmann_json)
# --- END NEW ---

# --- Add and Build the Tracking Library ---
add_subdirectory(tracking_lib)

# --- Define the Main Executable ---
# --- UPDATED: Add lut.cpp to the list of source files ---
set(MAIN_SOURCES
    "object_detection/main.cpp"
    "object_detection/tracking_pipeline.cpp"
    "object_detection/utils/async_inference.cpp"
    "object_detection/utils/utils.cpp"
    "object_detection/global_stabilizer.cpp"
    "object_detection/utils/calibration_data.cpp"
)
add_executable(${PROJECT_NAME} ${MAIN_SOURCES})

# --- Include Directories for the Executable ---
# --- UPDATED: Add the include path for nlohmann/json ---
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${OpenCV_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/object_detection"     
    "${CMAKE_CURRENT_SOURCE_DIR}/object_detection/utils"
)

# --- Linking ---
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Threads::Threads 
    HailoRT::libhailort 
    ${OpenCV_LIBS}
    bytetrack
    stdc++fs
    nlohmann_json::nlohmann_json  # Add this line
    ${GSTREAMER_LIBRARIES} # Link GStreamer libraries

)

# --- Compiler Options ---
target_compile_options(${PROJECT_NAME} PRIVATE 
    -Wall -Wextra
    $<$<CONFIG:Release>:-O3> 
    $<$<CONFIG:Debug>:-g>
)

message(STATUS "Configured main executable '${PROJECT_NAME}' to link against 'bytetrack' library.")