find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_INCLUDE_DIRS})

set(CMAKE_BUILD_TYPE Debug)
add_executable(chip8-sdl2 chip8-sdl2.c)
target_link_libraries(chip8-sdl2 ${SDL2_LIBRARIES} chip8)