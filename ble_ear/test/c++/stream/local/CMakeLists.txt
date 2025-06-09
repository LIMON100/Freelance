# cmake_minimum_required(VERSION 3.10)
# project(VoskSpeechRecognition)

# # Set C++ standard
# set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # Specify the path to Vosk
# set(VOSK_DIR "${CMAKE_SOURCE_DIR}/vosk-linux-x86_64-0.3.45")

# # Include Vosk headers
# include_directories(${VOSK_DIR})

# # Find Vosk library
# find_library(VOSK_LIB NAMES vosk libvosk PATHS ${VOSK_DIR})

# # Check if Vosk library was found
# if(NOT VOSK_LIB)
#     message(FATAL_ERROR "Vosk library not found in ${VOSK_DIR}")
# endif()

# # Add executable
# add_executable(vosk_recognizer vosk_recognizer.cpp)

# # Link libraries
# target_link_libraries(vosk_recognizer PRIVATE ${VOSK_LIB} pthread)

# # Set output directory for the executable
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


# cmake_minimum_required(VERSION 3.10)
# project(VoskLiveSpeechRecognition CXX)

# set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_STANDARD_REQUIRED True)

# # --- Vosk SDK Configuration ---
# # Adjust this path to where your 'vosk-api' directory is
# set(VOSK_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vosk-linux-x86_64-0.3.45") # Assumes vosk-api is in the same dir

# if(NOT EXISTS "${VOSK_SDK_DIR}/vosk_api.h" OR NOT EXISTS "${VOSK_SDK_DIR}/libvosk.so") # Adjust lib name for OS
#     message(FATAL_ERROR "Vosk SDK not found or vosk_api.h/libvosk.so missing in ${VOSK_SDK_DIR}.")
# endif()

# include_directories("${VOSK_SDK_DIR}")
# link_directories("${VOSK_SDK_DIR}")


# # --- Find SDL2 Library ---
# # This tells CMake to find SDL2 using its own find modules or a config file
# # It will set SDL2_INCLUDE_DIRS and SDL2_LIBRARIES
# find_package(SDL2 REQUIRED)
# include_directories(${SDL2_INCLUDE_DIRS})


# # --- Find Required System Libraries ---
# find_package(Threads REQUIRED)

# # --- Define Your Executabule ---
# # Change the source file name if you saved the C++ code with a different name
# add_executable(vosk_recognizer vosk_recognizer.cpp)

# # --- Link Libraries to Your Executable ---
# target_link_libraries(vosk_recognizer
#     PRIVATE
#     vosk
#     ${SDL2_LIBRARIES}       # Link with SDL2
#     Threads::Threads
#     dl
# )

# # --- Set RPATH (Runtime Path) for Vosk ---
# set_target_properties(vosk_recognizer PROPERTIES
#     INSTALL_RPATH "${VOSK_SDK_DIR}"
#     BUILD_WITH_INSTALL_RPATH TRUE
# )

# # Set output directory for the executable
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# message(STATUS "Vosk SDK Directory: ${VOSK_SDK_DIR}")
# message(STATUS "SDL2 Include Dirs: ${SDL2_INCLUDE_DIRS}")
# message(STATUS "SDL2 Libraries: ${SDL2_LIBRARIES}")
# message(STATUS "Configuring VoskLiveSpeechRecognition done.")



#  BLE SDL2
# cmake_minimum_required(VERSION 3.10)
# project(VoskLiveSpeechRecognition CXX)

# set(CMAKE_CXX_STANDARD 11) # Or CMAKE_CXX_STANDARD 17 if you use std::filesystem or C++17 features
# set(CMAKE_CXX_STANDARD_REQUIRED True)

# # --- Vosk SDK Configuration ---
# set(VOSK_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vosk-linux-x86_64-0.3.45") # UPDATE THIS PATH

# if(NOT EXISTS "${VOSK_SDK_DIR}/vosk_api.h" OR NOT EXISTS "${VOSK_SDK_DIR}/libvosk.so")
#     message(FATAL_ERROR "Vosk SDK not found or vosk_api.h/libvosk.so missing in ${VOSK_SDK_DIR}.")
# endif()

# include_directories("${VOSK_SDK_DIR}")
# link_directories("${VOSK_SDK_DIR}")

# # --- Find SDL2 Library ---
# find_package(SDL2 REQUIRED)
# include_directories(${SDL2_INCLUDE_DIRS})

# # --- Find Required System Libraries ---
# find_package(Threads REQUIRED)

# # --- Define Your Executable ---
# add_executable(vosk_recognizer vosk_recognizer_ble.cpp) # USE THE NEW CPP FILENAME

# # --- Link Libraries to Your Executable ---
# target_link_libraries(vosk_recognizer
#     PRIVATE
#     vosk
#     ${SDL2_LIBRARIES}
#     Threads::Threads
#     dl
# )

# # --- Set RPATH (Runtime Path) for Vosk ---
# set_target_properties(vosk_recognizer PROPERTIES
#     INSTALL_RPATH "${VOSK_SDK_DIR}" # This should point to dir with libvosk.so
#     BUILD_WITH_INSTALL_RPATH TRUE
# )

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Output to build/bin

# message(STATUS "Vosk SDK Directory: ${VOSK_SDK_DIR}")
# message(STATUS "SDL2 Include Dirs: ${SDL2_INCLUDE_DIRS}")
# message(STATUS "SDL2 Libraries: ${SDL2_LIBRARIES}")
# message(STATUS "Configuring VoskLiveSpeechRecognition done.")




# Ble alsa
cmake_minimum_required(VERSION 3.10)
project(VoskAlsaLiveRecognizer CXX) # Project name

# Set C++ standard (e.g., C++11, C++14, C++17)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Vosk SDK Configuration ---
# OPTION 1: Vosk SDK is a subdirectory (e.g., "vosk-sdk") next to this CMakeLists.txt
# set(VOSK_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vosk-linux-x86_64-0.3.45") # Example: UPDATE THIS PATH

# OPTION 2: Specify an absolute path or allow user to set it via CMake variable
# If VOSK_SDK_DIR is not set, default to a common relative path.
# Users can override with: cmake .. -DVOSK_SDK_DIR=/path/to/vosk-sdk
if(NOT VOSK_SDK_DIR)
    set(VOSK_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vosk-linux-x86_64-0.3.45" CACHE PATH "Path to the Vosk SDK directory")
endif()
message(STATUS "Using Vosk SDK from: ${VOSK_SDK_DIR}")

# Check if essential Vosk files exist
if(NOT EXISTS "${VOSK_SDK_DIR}/vosk_api.h")
    message(FATAL_ERROR "vosk_api.h not found in ${VOSK_SDK_DIR}. Please set VOSK_SDK_DIR correctly.")
endif()
# Adjust the library name if it's different (e.g., on Windows it would be .dll, macOS .dylib)
if(NOT EXISTS "${VOSK_SDK_DIR}/libvosk.so")
    message(FATAL_ERROR "libvosk.so not found in ${VOSK_SDK_DIR}. Please set VOSK_SDK_DIR correctly.")
endif()

# Add Vosk include directory
include_directories("${VOSK_SDK_DIR}")

# Add Vosk library directory
link_directories("${VOSK_SDK_DIR}")


# --- Find ALSA Library (libasound) ---
# CMake's FindALSA module should locate the ALSA library and headers
find_package(ALSA REQUIRED)
# Add ALSA include directory (usually /usr/include or similar)
include_directories(${ALSA_INCLUDE_DIRS})


# --- Find PThreads (Required by Vosk) ---
find_package(Threads REQUIRED)


# --- Define the Executable ---
add_executable(vosk_recognizer vosk_recognizer_pa.cpp)


# --- Link Libraries to the Executable ---
target_link_libraries(vosk_recognizer
    PRIVATE
    vosk                 # Link with libvosk.so
    ${ALSA_LIBRARIES}    # Link with libasound (ALSA)
    Threads::Threads     # Link with pthreads
    dl                   # For dynamic linking functions (dlopen, dlsym) used by Vosk
)


# --- Set RPATH (Runtime Path) for Vosk Library ---
# This helps the executable find libvosk.so at runtime without needing LD_LIBRARY_PATH
# It embeds the path to the Vosk SDK directory into the executable.
set_target_properties(vosk_recognizer PROPERTIES
    INSTALL_RPATH "${VOSK_SDK_DIR}" # This should be the directory containing libvosk.so
    BUILD_WITH_INSTALL_RPATH TRUE   # Also use this RPATH during build (from build tree)
)
# If libvosk.so is in a 'lib' subdirectory of VOSK_SDK_DIR, use:
# INSTALL_RPATH "${VOSK_SDK_DIR}/lib"


# --- Set Output Directory for the Executable (Optional) ---
# This will place the compiled executable in 'your_project_dir/build/bin/'
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


# --- Informational Messages ---
message(STATUS "CMake CXX Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "ALSA Include Directories: ${ALSA_INCLUDE_DIRS}")
message(STATUS "ALSA Libraries: ${ALSA_LIBRARIES}")
message(STATUS "Threads Library: ${CMAKE_THREAD_LIBS_INIT}")
message(STATUS "Configuration for VoskAlsaLiveRecognizer complete.")