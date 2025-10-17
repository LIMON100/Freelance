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


cmake_minimum_required(VERSION 3.16)
project(sky_hunter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Find Required Packages ---
find_package(Threads REQUIRED)
find_package(HailoRT REQUIRED)
find_package(OpenCV REQUIRED)

# --- NEW: Find GStreamer and RGA ---
find_package(PkgConfig REQUIRED)
pkg_search_module(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)
find_library(RGA_LIBRARY NAMES rga) # Finds librga.so
if(NOT RGA_LIBRARY)
    message(FATAL_ERROR "librga not found. Please ensure Rockchip's MPP/MAPI is installed.")
endif()
message(STATUS "Found librga at: ${RGA_LIBRARY}")
# --- END NEW ---

message(STATUS "Found OpenCV version: ${OpenCV_VERSION}")

add_subdirectory(tracking_lib)

set(MAIN_SOURCES
    "object_detection/main.cpp"
    "object_detection/tracking_pipeline.cpp"
    "object_detection/utils/async_inference.cpp"
    "object_detection/utils/utils.cpp"
    "object_detection/global_stabilizer.cpp"
)
add_executable(${PROJECT_NAME} ${MAIN_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${OpenCV_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS} # <-- NEW
    "${CMAKE_CURRENT_SOURCE_DIR}/object_detection"     
    "${CMAKE_CURRENT_SOURCE_DIR}/object_detection/utils"
    # Rockchip RGA headers are often in a standard system path like /usr/include/rga
    "/usr/include/rga" # <-- NEW
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Threads::Threads 
    HailoRT::libhailort 
    ${OpenCV_LIBS}
    ${GSTREAMER_LIBRARIES} # <-- NEW
    ${RGA_LIBRARY}         # <-- NEW
    bytetrack
    stdc++fs
)

target_compile_options(${PROJECT_NAME} PRIVATE 
    -Wall -Wextra
    $<$<CONFIG:Release>:-O3> 
    $<$<CONFIG:Debug>:-g>
)

message(STATUS "Configured main executable '${PROJECT_NAME}' to link against 'bytetrack' library.")