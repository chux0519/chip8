cmake_minimum_required(VERSION 3.0)

project(chip8)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

set(CMAKE_BUILD_TYPE Debug)
add_executable(chip8 chip8.c)
target_link_libraries(chip8 ${SDL2_LIBRARIES})