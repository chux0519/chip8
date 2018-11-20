#include <SDL2/SDL.h>
#include <stdio.h>

int main() {
  printf("hello sdl");
  SDL_Init(SDL_INIT_VIDEO);
  unsigned char buf[4096];
  printf("sizeof *buf: %lu, sizeof buf: %lu\n", sizeof *buf, sizeof buf);
  return 0;
}