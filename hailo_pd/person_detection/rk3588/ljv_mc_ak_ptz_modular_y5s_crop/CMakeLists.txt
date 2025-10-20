# cmake_minimum_required(VERSION 3.16)
# project(sky_hunter) # Renamed project as per client request

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED True)

# # --- Find Required Packages ---
# find_package(Threads REQUIRED)
# find_package(HailoRT REQUIRED)
# find_package(OpenCV REQUIRED)
# message(STATUS "Found OpenCV version: ${OpenCV_VERSION}")

# # --- Add and Build the Tracking Library ---
# add_subdirectory(tracking_lib)

# # --- Define the Main Executable ---
# # It's now built from our new modular files
# set(MAIN_SOURCES
#     "object_detection/main.cpp"
#     "object_detection/tracking_pipeline.cpp"
#     "object_detection/utils/async_inference.cpp"
#     "object_detection/utils/utils.cpp"
#     "object_detection/global_stabilizer.cpp"
# )
# add_executable(${PROJECT_NAME} ${MAIN_SOURCES})

# # --- Include Directories for the Executable ---
# target_include_directories(${PROJECT_NAME} PRIVATE 
#     ${OpenCV_INCLUDE_DIRS}
#     "${CMAKE_CURRENT_SOURCE_DIR}/object_detection"     
#     "${CMAKE_CURRENT_SOURCE_DIR}/object_detection/utils"
# )

# # --- Linking ---
# target_link_libraries(${PROJECT_NAME} PRIVATE 
#     Threads::Threads 
#     HailoRT::libhailort 
#     ${OpenCV_LIBS}
#     bytetrack
#     stdc++fs
# )

# # --- Compiler Options ---
# target_compile_options(${PROJECT_NAME} PRIVATE 
#     -Wall -Wextra
#     $<$<CONFIG:Release>:-O3> 
#     $<$<CONFIG:Debug>:-g>
# )

# message(STATUS "Configured main executable '${PROJECT_NAME}' to link against 'bytetrack' library.")


# File: CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(sky_hunter) # Renamed project as per client request

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Find Required Packages ---
find_package(Threads REQUIRED)
find_package(HailoRT REQUIRED)
find_package(OpenCV REQUIRED)
message(STATUS "Found OpenCV version: ${OpenCV_VERSION}")

# --- Find GStreamer ---
# This is crucial for GStreamer integration.
# You might need to adjust PKG_CONFIG_PATH if GStreamer is not in a standard location.
find_package(PkgConfig REQUIRED)
# CORRECTED: Changed 'gstreamer-1.0-app' to 'gstreamer-app-1.0'
# Also, standard practice uses spaces as delimiters for pkg_check_modules arguments.
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-plugins-base-1.0)
if (NOT GSTREAMER_FOUND)
    message(FATAL_ERROR "GStreamer development libraries not found. Please install them or set PKG_CONFIG_PATH.")
endif()
message(STATUS "Found GStreamer version: ${GSTREAMER_VERSION}")
message(STATUS "GStreamer include dirs: ${GSTREAMER_INCLUDE_DIRS}")
message(STATUS "GStreamer libraries: ${GSTREAMER_LIBRARIES}")
# --- End Find GStreamer ---


# --- Add and Build the Tracking Library ---
add_subdirectory(tracking_lib)

# --- Define the Main Executable ---
set(MAIN_SOURCES
    "object_detection/main.cpp"
    "object_detection/tracking_pipeline.cpp"
    "object_detection/utils/async_inference.cpp"
    "object_detection/utils/utils.cpp"
    "object_detection/global_stabilizer.cpp"
)
add_executable(${PROJECT_NAME} ${MAIN_SOURCES})

# --- Include Directories for the Executable ---
target_include_directories(${PROJECT_NAME} PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS} # Add GStreamer include directories
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
    ${GSTREAMER_LIBRARIES} # Link against GStreamer libraries
)

# --- Compiler Options ---
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-g>
)

message(STATUS "Configured main executable '${PROJECT_NAME}' to link against 'bytetrack' library.")
message(STATUS "Configured main executable '${PROJECT_NAME}' to link against GStreamer libraries.")