# chip8

A simple implementation of the [chip8](https://en.wikipedia.org/wiki/CHIP-8). You can easily use this project as cmake subproject, by just including src/chip.h and linking to `chip8`.

See [example/chip8-sdl2.c](example/chip8-sdl2.c) for example.

## Quick start

build:

> mkdir build && cd build
>
> cmake ..
>
> make

run:

> ./build/example/chip8-sdl2 "xxx.rom"
