#
# references
#  - https://kubasejdak.com/modern-cmake-is-like-inheritance
#
cmake_minimum_required(VERSION 3.10)

project(nesfr_vr_system_cli) #Project name

find_package(PkgConfig REQUIRED) #finding pkg-config is a helper tool

#using pkg-config to getting jsoncpp
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

#building target executable
add_executable(
    ${PROJECT_NAME}
    src/nesfr_vr_system_cli.cpp
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
    ./include
    ${JSONCPP_INCLUDE_DIRS}
    )

target_compile_options(
    ${PROJECT_NAME} PUBLIC
    -Wall -Werror -fpermissive
    )

#linking library with target executable
target_link_libraries(
    ${PROJECT_NAME}
    ${JSONCPP_LIBRARIES}
    stdc++fs
    )

