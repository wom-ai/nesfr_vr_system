cmake_minimum_required(VERSION 3.10)

project(video_dev_identifier) #Project name

set(CMAKE_CXX_STANDARD 14) #setting C++ 14 standard
find_package(PkgConfig) #finding pkg-config is a helper tool

set (CMAKE_CXX_FLAGS "-lstdc++fs -fpermissive")

#using pkg-config to getting libv4l2
pkg_check_modules(LIBV4L2 REQUIRED libv4l2)

#including GStreamer header files directory
include_directories(
   ${LIBV4L2_INCLUDE_DIRS}
)

#linking GStreamer library directory
link_directories(
   ${LIBV4L2_LIBRARY_DIRS}
)

#building target executable
add_executable(${PROJECT_NAME} video_dev_identifier.cpp)

#linking Gstreamer library with target executable
target_link_libraries(${PROJECT_NAME} ${LIBV4L2_LIBRARIES})


add_executable(test_app test.cpp)

#linking Gstreamer library with target executable
target_link_libraries(test_app ${LIBV4L2_LIBRARIES})
