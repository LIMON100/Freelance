cmake_minimum_required(VERSION 3.10)
project(FinalPredictor CXX)

# Set the C++ standard for modern features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Define paths to all our libraries ---

# Path to the RKNN API library inside our project
set(RKNN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/rknn_api)
# Path to the single-header libraries inside our project
set(SINGLE_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/single_header)

# --- THE FIX: Use the EXACT path where you installed FFTW ---
set(FFTW_INSTALL_DIR /userdata/libb/install_fftw)


# --- Create the executable ---
add_executable(final_predictor main.cpp)


# --- Configure Include Directories ---
# Tell the compiler where to find all the necessary .h header files.
target_include_directories(final_predictor PRIVATE
    ${RKNN_API_DIR}/include
    ${SINGLE_HEADER_DIR}
    ${FFTW_INSTALL_DIR}/include  # <-- Add the include path for your custom FFTW installation
)


# --- Configure Link Libraries ---
# Tell the linker where to find all the necessary .so library files.
target_link_libraries(final_predictor
    PRIVATE
    ${RKNN_API_DIR}/lib/librknnrt.so      # Link directly to the RKNN library
    ${FFTW_INSTALL_DIR}/lib/libfftw3f.so   # Link directly to the FFTW library in your custom path
)