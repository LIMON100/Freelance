cmake_minimum_required(VERSION 3.14)
project(bytetrack)

# --- Standard C++ Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Find or Download Eigen ---
# This makes the library self-contained. It will use a system Eigen if found,
# otherwise it will download it.
find_package(Eigen3 3.3 QUIET)
if(NOT EIGEN3_FOUND)
  include(FetchContent)
  FetchContent_Declare(eigen
    GIT_REPOSITORY  https://gitlab.com/libeigen/eigen.git
    GIT_TAG         3.4.0 # Using a slightly newer, stable version
    GIT_SHALLOW     ON
  )
  FetchContent_MakeAvailable(eigen)
  # EIGEN3_INCLUDE_DIR is set by FetchContent
  message(STATUS "Eigen was not found, downloaded and configured.")
else()
  message(STATUS "Found system-installed Eigen version: ${EIGEN3_VERSION_STRING}")
endif()

# --- Define the Library Target ---
add_library(${PROJECT_NAME} SHARED
  src/BYTETracker.cpp
  src/KalmanFilter.cpp
  src/lapjv.cpp
  src/Object.cpp
  src/Rect.cpp
  src/STrack.cpp
)

# --- Include Directories ---
# PUBLIC means that any target linking against this library will
# automatically inherit these include paths. This is the key to fixing the build.
target_include_directories(${PROJECT_NAME} PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  ${EIGEN3_INCLUDE_DIR}
)

# --- Compiler Options ---
target_compile_options(${PROJECT_NAME} PRIVATE 
  -Wall -Wextra
  $<$<CONFIG:Debug>:-g>
  $<$<CONFIG:Release>:-O3>
)