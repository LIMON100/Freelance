cmake_minimum_required(VERSION 3.10)
project(RealTimePredictor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Find system-installed libraries ---
find_library(FFTW3F_LIBRARY NAMES fftw3f REQUIRED)
find_library(ASOUND_LIBRARY NAMES asound REQUIRED)

# --- Define paths to LOCAL libraries ---
set(ONNXRUNTIME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/onnxruntime)
set(SINGLE_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/single_header)

# --- Create the executable ---
add_executable(realtime_predictor combined_wav_gk.cpp)

# --- Configure Include Directories ---
target_include_directories(realtime_predictor PRIVATE
    ${ONNXRUNTIME_DIR}/include
    ${SINGLE_HEADER_DIR}
    # FFTW headers are found system-wide now
)

# --- Configure Link Libraries ---
target_link_libraries(realtime_predictor
    PRIVATE
    ${ONNXRUNTIME_DIR}/lib/libonnxruntime.so
    ${FFTW3F_LIBRARY}
    ${ASOUND_LIBRARY} 
)








### MULTI combined wav file

# cmake_minimum_required(VERSION 3.14)
# project(CommandDetector LANGUAGES CXX)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3") # Add optimization flags for release

# # ===================================================================
# #  Define Paths to Local Libraries
# #  This makes the project self-contained.
# # ===================================================================

# # Set the path to our local 'libs' directory
# set(LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

# # --- ONNX Runtime ---
# # Define the include and library paths for ONNX Runtime
# set(ONNXRUNTIME_INCLUDE_DIR ${LIBS_DIR}/onnxruntime/include)
# find_library(ONNXRUNTIME_LIBRARY
#     NAMES onnxruntime # The name of the library file (e.g., libonnxruntime.so)
#     PATHS ${LIBS_DIR}/onnxruntime/lib
#     REQUIRED
# )
# message(STATUS "Found ONNX Runtime Library: ${ONNXRUNTIME_LIBRARY}")
# message(STATUS "Found ONNX Runtime Include Dir: ${ONNXRUNTIME_INCLUDE_DIR}")


# # --- FFTW3 ---
# # You need to download FFTW3 and place it in libs/fftw3
# # Download from: https://www.fftw.org/download.html
# # For Linux, you can often install it with: sudo apt-get install libfftw3-dev
# # If you install it system-wide, CMake can find it automatically.
# # If you place it in the libs folder, we find it here.
# set(FFTW3_INCLUDE_DIR ${LIBS_DIR}/fftw3/include)
# find_library(FFTW3_LIBRARY
#     NAMES fftw3f # Find the float version (libfftw3f.so)
#     PATHS ${LIBS_DIR}/fftw3/lib
#     REQUIRED
# )
# message(STATUS "Found FFTW3 Library: ${FFTW3_LIBRARY}")
# message(STATUS "Found FFTW3 Include Dir: ${FFTW3_INCLUDE_DIR}")


# # ===================================================================
# #  Define Executable and Link Libraries
# # ===================================================================

# # Create the executable from our main C++ file
# add_executable(command_detector voice_command.cpp)

# # Specify the include directories our program needs
# target_include_directories(command_detector PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}       # Allows including files from the root
#     ${ONNXRUNTIME_INCLUDE_DIR}
#     ${FFTW3_INCLUDE_DIR}
#     # dr_wav.h is already found because we add the root dir
# )

# # Link the executable against the required libraries
# target_link_libraries(command_detector PRIVATE
#     ${ONNXRUNTIME_LIBRARY}
#     ${FFTW3_LIBRARY}
#     # On Linux, you might need to link against the math library and pthreads
#     # if you get linker errors.
#     m
#     pthread
# )

# # ===================================================================
# #  Post-Build Step: Copy Assets
# #  This copies the model and label files to the build directory so the
# #  program can find them when you run it from there.
# # ===================================================================
# file(COPY
#     "${CMAKE_CURRENT_SOURCE_DIR}/models/smaller_grok.onnx"
#     "${CMAKE_CURRENT_SOURCE_DIR}/models/labels.txt"
#     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
# )

# # --- Optional: Install step ---
# # This defines what happens when you run `make install`
# install(TARGETS command_detector DESTINATION bin)
# install(FILES
#     "${CMAKE_CURRENT_BINARY_DIR}/models/smaller_grok.onnx"
#     "${CMAKE_CURRENT_BINARY_DIR}/models/labels.txt"
#     DESTINATION bin
# )

# # --- Print a helpful message after configuration ---
# message(STATUS "Configuration complete. Run 'make' to build.")
# message(STATUS "To run the program from the build directory, use:")
# message(STATUS "./command_detector ../your_test_file.wav")