cmake_minimum_required(VERSION 3.5)
project(AudioVisualizer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option 1: Try to find PortAudio using modern CMake config
find_package(PortAudio QUIET)

if(NOT PortAudio_FOUND)
    # Option 2: Try to find PortAudio using pkg-config (common on Linux)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PORTAUDIO QUIET portaudio-2.0)
        if(PORTAUDIO_FOUND)
            add_library(PortAudio::PortAudio INTERFACE IMPORTED)
            target_include_directories(PortAudio::PortAudio INTERFACE ${PORTAUDIO_INCLUDE_DIRS})
            target_link_libraries(PortAudio::PortAudio INTERFACE ${PORTAUDIO_LIBRARIES})
            set(PortAudio_FOUND TRUE)
        endif()
    endif()
endif()

if(NOT PortAudio_FOUND)
    # Option 3: Try to find PortAudio the traditional way
    find_path(PORTAUDIO_INCLUDE_DIR portaudio.h
        PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
    )

    find_library(PORTAUDIO_LIBRARY
        NAMES portaudio libportaudio portaudio-2.0
        PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
    )

    if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY)
        add_library(PortAudio::PortAudio INTERFACE IMPORTED)
        target_include_directories(PortAudio::PortAudio INTERFACE ${PORTAUDIO_INCLUDE_DIR})
        target_link_libraries(PortAudio::PortAudio INTERFACE ${PORTAUDIO_LIBRARY})
        set(PortAudio_FOUND TRUE)
    endif()
endif()

if(NOT PortAudio_FOUND)
    message(FATAL_ERROR "PortAudio not found. Please install PortAudio development package.")
endif()

# Add executable
add_executable(audio_visualizer
    main.cpp
)

# Link PortAudio to the executable
target_link_libraries(audio_visualizer
    PRIVATE
    PortAudio::PortAudio
)

# Install target (optional)
install(TARGETS audio_visualizer DESTINATION bin)