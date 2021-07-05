cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(minusd)

find_package(pxr REQUIRED)

add_executable(minusd minusd.cpp)

target_link_libraries(minusd usd usdImagingGL)
include_directories("/usr/include/python3.9")
