# cmake_minimum_required(VERSION 3.14)
# project(bytetrack)

# # --- Standard C++ Settings ---
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED True)

# # --- Find Dependencies ---
# find_package(OpenCV REQUIRED)
# message(STATUS "Found OpenCV for bytetrack library: ${OpenCV_VERSION}")

# # --- THIS IS THE FIX ---
# # Find Eigen3. This will set Eigen3_FOUND and Eigen3_INCLUDE_DIRS if it exists.
# find_package(Eigen3 3.3 QUIET)

# # If Eigen3 was not found on the system, then download it.
# if(NOT Eigen3_FOUND)
#   message(STATUS "System Eigen not found. Downloading...")
#   include(FetchContent)
#   FetchContent_Declare(eigen
#     GIT_REPOSITORY  https://gitlab.com/libeigen/eigen.git
#     GIT_TAG         3.4.0
#     GIT_SHALLOW     ON
#   )
#   FetchContent_MakeAvailable(eigen)
#   # FetchContent sets the variable 'eigen_SOURCE_DIR'
#   set(EIGEN_INCLUDE_DIRS ${eigen_SOURCE_DIR})
#   message(STATUS "Eigen was downloaded and configured.")
# else()
#   # If it was found, use the variable set by find_package.
#   set(EIGEN_INCLUDE_DIRS ${Eigen3_INCLUDE_DIRS})
#   message(STATUS "Found system-installed Eigen version: ${EIGEN3_VERSION_STRING}")
# endif()
# # --- END OF FIX ---

# # --- Define the Library Target ---
# add_library(${PROJECT_NAME} SHARED
#   src/BYTETracker.cpp
#   src/KalmanFilter.cpp
#   src/lapjv.cpp
#   src/Object.cpp
#   src/Rect.cpp
#   src/STrack.cpp
# )

# # --- Include Directories ---
# # PUBLIC means that any target linking against this library will
# # automatically inherit these include paths.
# target_include_directories(${PROJECT_NAME} PUBLIC
#   "${CMAKE_CURRENT_SOURCE_DIR}/include"
#   ${EIGEN_INCLUDE_DIRS}   # <-- Use our new, unified variable
#   ${OpenCV_INCLUDE_DIRS}
# )

# # --- Linking for the Library ---
# target_link_libraries(${PROJECT_NAME} PRIVATE 
#     ${OpenCV_LIBS}
# )

# # --- Compiler Options ---
# target_compile_options(${PROJECT_NAME} PRIVATE 
#   -Wall -Wextra
#   $<$<CONFIG:Debug>:-g>
#   $<$<CONFIG:Release>:-O3>
# )

cmake_minimum_required(VERSION 3.14)
project(bytetrack)

# --- Standard C++ Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Find Dependencies ---
find_package(OpenCV REQUIRED)
message(STATUS "Found OpenCV for bytetrack library: ${OpenCV_VERSION}")

# --- Find or Fetch Eigen ---
find_package(Eigen3 3.3 QUIET)

# If Eigen3 was not found on the system, then download it.
if(NOT Eigen3_FOUND)
  message(STATUS "System Eigen not found. Downloading...")
  include(FetchContent)
  FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG        3.4.0
    GIT_SHALLOW    ON
  )
  FetchContent_MakeAvailable(eigen)
  
  # For FetchContent, the include directory is the source directory
  set(EIGEN_INCLUDE_DIRS ${eigen_SOURCE_DIR})

else()
  message(STATUS "Found system-installed Eigen version: ${EIGEN3_VERSION_STRING}")
  
  # --- THIS IS THE FIX ---
  # For system-installed Eigen, the include path is provided by the imported target.
  # We get the 'INTERFACE_INCLUDE_DIRECTORIES' property from the 'Eigen3::Eigen' target.
  get_target_property(EIGEN_INCLUDE_DIRS Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
  # --- END OF FIX ---
endif()
message(STATUS "Using Eigen include directories: ${EIGEN_INCLUDE_DIRS}")

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
target_include_directories(${PROJECT_NAME} PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  ${EIGEN_INCLUDE_DIRS}   # Now this variable is correct in both cases
  ${OpenCV_INCLUDE_DIRS}
)

# --- Linking for the Library ---
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${OpenCV_LIBS}
)

# --- Compiler Options ---
target_compile_options(${PROJECT_NAME} PRIVATE 
  -Wall -Wextra
  $<$<CONFIG:Debug>:-g>
  $<$<CONFIG:Release>:-O3>
)