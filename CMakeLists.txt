cmake_minimum_required(VERSION 3.10)

project(video_stream) #Project name

set(CMAKE_CXX_STANDARD 14) #setting C++ 14 standard
find_package(PkgConfig) #finding pkg-config is a helper tool

set (CMAKE_CXX_FLAGS "-lstdc++fs -fpermissive")

#using pkg-config to getting Gstreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)


#including GStreamer header files directory
include_directories(
    ./include
    ${GSTREAMER_INCLUDE_DIRS}
)

#linking GStreamer library directory
link_directories(
    ${GSTREAMER_LIBRARY_DIRS}
)

#building target executable
add_executable(${PROJECT_NAME} src/video_stream.cpp src/CtrlClient.cpp)

#linking Gstreamer library with target executable
target_link_libraries(${PROJECT_NAME} ${GSTREAMER_LIBRARIES})

#building target executable
add_executable(VideoStreamTest src/VideoStreamTest.cpp src/VideoStreamer.cpp src/CtrlClient.cpp)

#linking Gstreamer library with target executable
target_link_libraries(VideoStreamTest ${GSTREAMER_LIBRARIES})
