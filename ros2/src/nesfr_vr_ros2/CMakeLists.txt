cmake_minimum_required(VERSION 3.5)
project(nesfr_vr_ros2)

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
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)

find_package(PkgConfig REQUIRED) #finding pkg-config is a helper tool

find_package(spdlog REQUIRED)

#using pkg-config to getting jsoncpp
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

#using pkg-config to getting Gstreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

set(PROJECT_ROOT_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    )

add_executable(nesfr_vr_ros2
    ${PROJECT_ROOT_DIR}/src/NesfrVR.cpp
    ${PROJECT_ROOT_DIR}/src/RoverController.cpp
    ${PROJECT_ROOT_DIR}/src/GimbalController.cpp
    ${PROJECT_ROOT_DIR}/video_streamer/src/V4L2Device.cpp
    ${PROJECT_ROOT_DIR}/video_streamer/src/V4L2Camera.cpp
    ${PROJECT_ROOT_DIR}/video_streamer/src/V4L2StereoCamera.cpp
    ${PROJECT_ROOT_DIR}/video_streamer/src/VideoStreamer.cpp
    ${PROJECT_ROOT_DIR}/video_streamer/src/CtrlClient.cpp
    ${PROJECT_ROOT_DIR}/nesfr_vr_ctrl/src/custom_crc16.c
    ${PROJECT_ROOT_DIR}/nesfr_vr_ctrl/src/custom_crc32.c
    ${PROJECT_ROOT_DIR}/nesfr_vr_ctrl/src/RS2Ctrl.cpp
    ${PROJECT_ROOT_DIR}/nesfr_vr_ctrl/src/RS2.cpp
    ${PROJECT_ROOT_DIR}/nesfr4_control/src/Joystick.cpp
    ${PROJECT_ROOT_DIR}/nesfr4_control/src/Wheel.cpp
    ${PROJECT_ROOT_DIR}/nesfr4_control/src/VRJoystick.cpp
    ${PROJECT_ROOT_DIR}/src/Nesfr7ROS2.cpp
    )

ament_target_dependencies(nesfr_vr_ros2
    "rclcpp"
    "geometry_msgs")

target_include_directories(nesfr_vr_ros2 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${PROJECT_ROOT_DIR}/include
    ${JSONCPP_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${PROJECT_ROOT_DIR}/video_streamer/include
    ${PROJECT_ROOT_DIR}/nesfr_vr_ctrl/include
    ${PROJECT_ROOT_DIR}/nesfr4_control/include
    )

target_compile_definitions(
    ${PROJECT_NAME} PUBLIC
    __SPDLOG__
    __GIT_BRANCH__="${GIT_BRANCH}"
    __GIT_COMMIT_HASH__="${GIT_COMMIT_HASH}"
    NESFR_VR_MAIN_FUNC
    _AUDIO_GUIDE_
    _ROS2_
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

install(TARGETS nesfr_vr_ros2
  DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})

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
