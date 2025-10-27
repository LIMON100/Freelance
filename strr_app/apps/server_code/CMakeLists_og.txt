cmake_minimum_required(VERSION 3.5)

# --- Original Project Name ---
set(PROJECT_NAME selector_test)

project(
    ${PROJECT_NAME}
    VERSION 1.0
    LANGUAGES CXX C)

set(CMAKE_INSTALL_ALWAYS 1)

set(TARGET_PROJECT_DIR ${CMAKE_SYSROOT})
set(TARGET_PATH_LIB ${TARGET_PROJECT_DIR}/usr/lib)
set(TARGET_PATH_INCLUDE ${TARGET_PROJECT_DIR}/usr/include)

message("Let's configure ARM64")
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("backend is wl")
set(BACKEND_LINK wayland-client wayland-cursor wayland-egl)

add_compile_definitions(EMBEDDED_DEVICE)

find_package(PkgConfig) 
 
pkg_search_module(LIBXML2 REQUIRED libxml-2.0)
pkg_search_module(GST REQUIRED gstreamer-1.0) 
pkg_search_module(GST_VIDEO REQUIRED gstreamer-video-1.0) 

# --- NEW: Added the required GStreamer RTSP Server library ---
# This line is added to find the necessary headers and libraries for the server.
pkg_search_module(GST_RTSP_SERVER REQUIRED gstreamer-rtsp-server-1.0)


include_directories(
${TARGET_PATH_INCLUDE}/
${TARGET_PATH_INCLUDE}/apriltag/common/

${TARGET_PATH_INCLUDE}/glib-2.0/
${TARGET_PATH_INCLUDE}/glib-2.0/glib/

${GST_INCLUDE_DIRS}
${GST_VIDEO_INCLUDE_DIRS}
# --- NEW: Added the RTSP Server include path ---
${GST_RTSP_SERVER_INCLUDE_DIRS}
)

include_directories(
${PROJECT_SOURCE_DIR}/include/
)

# --- UPDATED: Changed the source file to your new server code ---
# Note: The project name from add_executable() remains the same as you had it.
set(MAIN_FILES
    ${PROJECT_SOURCE_DIR}/src/robot_server.cpp
)

add_executable(${PROJECT_NAME}
               ${MAIN_FILES}
              )


message("Library Path is [${TARGET_PATH_LIB}]")

target_link_directories(${PROJECT_NAME}  PUBLIC
            ${TARGET_PROJECT_DIR}/lib/
            ${TARGET_PROJECT_DIR}/usr/lib/
            ${TARGET_PROJECT_DIR}/usr/local/lib/
            )

target_link_libraries(${PROJECT_NAME} PUBLIC
    m stdc++ pthread dl 
    libgbm.so
    libglib-2.0.so.0
    libgobject-2.0.so.0
    libgthread-2.0.so
    libpthread.so
    libmali.so

    ${BACKEND_LINK}
    ${GST_LIBRARIES} 
    ${GST_VIDEO_LIBRARIES} # Note: In some systems this is GST_VIDEO_LIBRARIES, not GST_VIDEOLIBRARIES
    
    # --- NEW: Link against the RTSP server library ---
    ${GST_RTSP_SERVER_LIBRARIES}
)

target_compile_definitions(${PROJECT_NAME}  PUBLIC
            PROJECT_SRC_DIR="${CMAKE_SOURCE_DIR}"
            ${BACKEND_OPTION} ${SRC}
            )

target_compile_options(${PROJECT_NAME}  PUBLIC -Wall -Wno-poison-system-directories -O3 -rdynamic -DCALIBRATION_APP)