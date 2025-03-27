# cmake_minimum_required(VERSION 3.10)

# project(rknn_retinaface_demo)

# if (ENABLE_ASAN)
# 	message(STATUS "BUILD WITH ADDRESS SANITIZER")
# 	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
# 	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
# 	set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
# endif ()

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/ 3rdparty.out)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/ utils.out)

# set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

# if (TARGET_SOC STREQUAL "rk1808" OR TARGET_SOC STREQUAL "rv1109" OR TARGET_SOC STREQUAL "rv1126")
#     set(retinaface_file rknpu1/retinaface.cc)
# else()
#     set(retinaface_file rknpu2/retinaface.cc)
# endif()

# file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

# add_executable(${PROJECT_NAME}
#     main.cc
#     ${retinaface_file}
# )

# target_link_libraries(${PROJECT_NAME}
#     fileutils
#     imageutils
#     imagedrawing
#     ${LIBRKNNRT}
#     dl
# )

# if (CMAKE_SYSTEM_NAME STREQUAL "Android")
#     target_link_libraries(${PROJECT_NAME} log )
# endif()

# if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
#     set(THREADS_PREFER_PTHREAD_FLAG ON)
#     find_package(Threads REQUIRED)
#     target_link_libraries(${PROJECT_NAME} Threads::Threads)
# endif()



# target_include_directories(${PROJECT_NAME} PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}
#     ${LIBRKNNRT_INCLUDES}
# )

# install(TARGETS ${PROJECT_NAME} DESTINATION .)
# install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/test.jpg DESTINATION model)
# set(file_path ${CMAKE_CURRENT_SOURCE_DIR}/../model/RetinaFace.rknn)
# if (EXISTS ${file_path})
#     install(FILES ${file_path} DESTINATION model)
# endif()

# cmake_minimum_required(VERSION 3.10)

# project(rknn_retinaface_demo)

# if (ENABLE_ASAN)
#     message(STATUS "BUILD WITH ADDRESS SANITIZER")
#     set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#     set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#     set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
# endif ()

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/ 3rdparty.out)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/ utils.out)

# set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

# set(faced_file faced_facel.cc)

# file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

# add_executable(${PROJECT_NAME}
#     main.cc
#     ${faced_file}
# )

# target_link_libraries(${PROJECT_NAME}
#     fileutils
#     imageutils
#     imagedrawing
#     ${LIBRKNNRT}
#     dl
# )

# if (CMAKE_SYSTEM_NAME STREQUAL "Android")
#     target_link_libraries(${PROJECT_NAME} log)
# endif()

# if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
#     set(THREADS_PREFER_PTHREAD_FLAG ON)
#     find_package(Threads REQUIRED)
#     target_link_libraries(${PROJECT_NAME} Threads::Threads)
# endif()

# target_include_directories(${PROJECT_NAME} PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}
#     ${LIBRKNNRT_INCLUDES}
# )

# install(TARGETS ${PROJECT_NAME} DESTINATION .)
# install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/test.jpg DESTINATION model)
# set(model_path ${CMAKE_CURRENT_SOURCE_DIR}/../model/retinaface.rknn)
# if (EXISTS ${model_path})
#     install(FILES ${model_path} DESTINATION model)
# endif()


cmake_minimum_required(VERSION 3.10)

project(rknn_retinaface_demo)

if (ENABLE_ASAN)
    message(STATUS "BUILD WITH ADDRESS SANITIZER")
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif ()

# Add subdirectories for 3rdparty and utils
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/ 3rdparty.out)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/ utils.out)

# Set the RPATH for the executable to find shared libraries
set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

# Define the source files
set(faced_facel_file faced_facel.cc)
set(file_utils_file ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/file_utils.c)  # Add file_utils.c directly

# Gather all .cc source files
file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

# Define the executable
add_executable(${PROJECT_NAME}
    main.cc
    ${faced_facel_file}
    ${file_utils_file}  # Include file_utils.c in the executable
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    imageutils
    imagedrawing
    ${LIBRKNNRT}
    dl
)

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(${PROJECT_NAME} log)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} Threads::Threads)
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils  # Include utils for file_utils.h
    ${LIBRKNNRT_INCLUDES}
)

# Install rules
install(TARGETS ${PROJECT_NAME} DESTINATION .)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/test.jpg DESTINATION model)

# Install the RKNN models
set(faced_model_path ${CMAKE_CURRENT_SOURCE_DIR}/../model/faceD.rknn)
set(facel_model_path ${CMAKE_CURRENT_SOURCE_DIR}/../model/faceL.rknn)
if (EXISTS ${faced_model_path})
    install(FILES ${faced_model_path} DESTINATION model)
endif()
if (EXISTS ${facel_model_path})
    install(FILES ${facel_model_path} DESTINATION model)
endif()