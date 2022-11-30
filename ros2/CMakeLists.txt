cmake_minimum_required(VERSION 3.5)
project(nesfrvr_ros2)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)

find_package(PkgConfig REQUIRED) #finding pkg-config is a helper tool

find_package(spdlog REQUIRED)

#using pkg-config to getting jsoncpp
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

#using pkg-config to getting Gstreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

add_executable(nesfrvr_ros2
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/NesfrVR.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/RoverController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/GimbalController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../nesfr_vr_video_streamer/src/V4L2Device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../nesfr_vr_video_streamer/src/V4L2Camera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../nesfr_vr_video_streamer/src/V4L2StereoCamera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../nesfr_vr_video_streamer/src/VideoStreamer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../nesfr_vr_video_streamer/src/CtrlClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../nesfr_vr_ctrl/src/custom_crc16.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../nesfr_vr_ctrl/src/custom_crc32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../nesfr_vr_ctrl/src/RS2Ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../nesfr_vr_ctrl/src/RS2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../nesfr4_control/src/Joystick.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../nesfr4_control/src/Wheel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../nesfr4_control/src/VRJoystick.cpp
    )
target_include_directories(nesfrvr_ros2 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${JSONCPP_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../nesfr_vr_video_streamer/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../nesfr_vr_ctrl/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../nesfr4_control/include
    )

target_compile_definitions(
    ${PROJECT_NAME} PUBLIC
    __SPDLOG__
    __GIT_BRANCH__="${GIT_BRANCH}"
    __GIT_COMMIT_HASH__="${GIT_COMMIT_HASH}"
    NESFR_VR_MAIN_FUNC
    _AUDIO_GUIDE_
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

install(TARGETS nesfrvr_ros2
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # uncomment the line when a copyright and license is not present in all source files
  #set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # uncomment the line when this package is not in a git repo
  #set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
