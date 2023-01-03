cmake_minimum_required(VERSION 3.10)

project(ip_identifier) #Project name

set(CMAKE_CXX_STANDARD 14) #setting C++ 14 standard
find_package(PkgConfig) #finding pkg-config is a helper tool

set (CMAKE_CXX_FLAGS "-lstdc++fs")


include_directories(
)

link_directories(
)

#building target executable
add_executable(${PROJECT_NAME} ip_identifier.cpp)

target_link_libraries(${PROJECT_NAME})

