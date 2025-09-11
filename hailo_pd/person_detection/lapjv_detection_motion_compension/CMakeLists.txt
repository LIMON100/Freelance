cmake_minimum_required(VERSION 3.16)
project(obj_det)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Find Required Packages ---
find_package(Threads REQUIRED)
find_package(HailoRT REQUIRED)
find_package(OpenCV REQUIRED)
message(STATUS "Found OpenCV version: ${OpenCV_VERSION}")

# --- Add and Build the Tracking Library ---
# This command executes tracking_lib/CMakeLists.txt, building the 'bytetrack' library
add_subdirectory(tracking_lib)

# --- Define the Main Executable ---
set(MAIN_SOURCES
    "object_detection/object_detection.cpp"
    "object_detection/utils/async_inference.cpp"
    "object_detection/utils/utils.cpp"
)
add_executable(${PROJECT_NAME} ${MAIN_SOURCES})

# --- Include Directories for the Executable ---
# We only need to specify paths unique to the executable.
# The 'bytetrack' and 'Eigen' include paths are inherited automatically.
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${OpenCV_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/object_detection/utils"
)

# --- Linking ---
# Link our executable against all the libraries it depends on.
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Threads::Threads 
    HailoRT::libhailort 
    ${OpenCV_LIBS}
    bytetrack      # <-- This is the crucial link to our new library
    stdc++fs
)

# --- Compiler Options ---
target_compile_options(${PROJECT_NAME} PRIVATE 
    -Wall -Wextra
    $<$<CONFIG:Release>:-O3> 
    $<$<CONFIG:Debug>:-g>
)

message(STATUS "Configured main executable '${PROJECT_NAME}' to link against 'bytetrack' library.")