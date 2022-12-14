#
# references
#  - https://kubasejdak.com/modern-cmake-is-like-inheritance
#
cmake_minimum_required(VERSION 3.10)

project(nesfr_vr_system_cli) #Project name

# reference: https://stackoverflow.com/questions/51727566/how-to-make-git-commit-hash-available-in-c-code-without-needless-recompiling
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# Get the latest commit hash
execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

find_package(PkgConfig REQUIRED) #finding pkg-config is a helper tool

find_package(spdlog REQUIRED)

#using pkg-config to getting jsoncpp
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

#using pkg-config to getting Gstreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

#building target executable
add_executable(
    ${PROJECT_NAME}
    ../src/NesfrVR.cpp
    ../src/RoverController.cpp
    ../src/GimbalController.cpp
    ../video_streamer/src/V4L2Device.cpp
    ../video_streamer/src/V4L2Camera.cpp
    ../video_streamer/src/V4L2StereoCamera.cpp
    ../video_streamer/src/VideoStreamer.cpp
    ../video_streamer/src/CtrlClient.cpp
    ../nesfr_vr_ctrl/src/custom_crc16.c
    ../nesfr_vr_ctrl/src/custom_crc32.c
    ../nesfr_vr_ctrl/src/RS2Ctrl.cpp
    ../nesfr_vr_ctrl/src/RS2.cpp
    ../nesfr4_control/src/Joystick.cpp
    ../nesfr4_control/src/Wheel.cpp
    ../nesfr4_control/src/VRJoystick.cpp
    )

set_property(
    TARGET ${PROJECT_NAME}
    PROPERTY CXX_STANDARD 17
    )

set_property(
    TARGET ${PROJECT_NAME}
    PROPERTY CXX_STANDARD_REQUIRED ON
    )

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ../include
    ./include
    ${JSONCPP_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ../video_streamer/include
    ../nesfr_vr_ctrl/include
    ../nesfr4_control/include
    )

target_compile_definitions(
    ${PROJECT_NAME} PUBLIC
    __SPDLOG__
    __GIT_BRANCH__="${GIT_BRANCH}"
    __GIT_COMMIT_HASH__="${GIT_COMMIT_HASH}"
    NESFR_VR_MAIN_FUNC
    _AUDIO_GUIDE_
    )

target_compile_options(
    ${PROJECT_NAME} PUBLIC
    -Wall -Werror
    )

#linking library with target executable
target_link_libraries(
    ${PROJECT_NAME}
    ${JSONCPP_LIBRARIES}
    ${GSTREAMER_LIBRARIES}
    stdc++fs
    spdlog::spdlog
    pthread
    )

