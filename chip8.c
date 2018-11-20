#include <SDL2/SDL.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Defs

#define MEM_SIZE 4096
#define REGISTER_SIZE 16
#define STACK_SIZE 16
#define KEYBOARD_SIZE 16
#define WIDTH 64
#define HEIGHT 32
#define TIMES 8

typedef struct chip8 {
  unsigned char *memory;     // 4k
  unsigned char *registers;  // 16
  unsigned short opcode;
  unsigned short ir;  // Index Register
  unsigned short pc;  // Program Counter
  unsigned char delay_timer;
  unsigned char sound_timer;
  unsigned short *stack;  // 16
  unsigned short sp;      // Stack Pointer
  unsigned char *keyboard;
  unsigned char *pixels;  // Pixel buffer for sdl
  void (*beep)();
  int should_update;
} chip8;

// Initiate memory and registers
void chip8_init(chip8 *chip, void (*beep)());

// Load ROM
int chip8_load_rom(chip8 *chip, const char *rom);

// Process CPU cycle
void chip8_process_cyle(chip8 *chip);

void chip8_destroy(chip8 *chip);

// Utils
void *xmalloc(size_t size);

// Implementation

void *xmalloc(size_t size) {
  void *mem = (void *)malloc(size);
  if (mem == NULL) {
    fprintf(stderr, "fatal: memory exhausted (xmalloc of %zu bytes).\n", size);
    exit(-1);
  }
  return mem;
}

void chip8_init(chip8 *chip, void (*beep)()) {
  chip->memory = (unsigned char *)xmalloc(sizeof(unsigned char) * MEM_SIZE);
  chip->registers =
      (unsigned char *)xmalloc(sizeof(unsigned char) * REGISTER_SIZE);
  chip->opcode = 0;
  chip->ir = 0;
  chip->pc = 0x200;
  chip->delay_timer = 0;
  chip->sound_timer = 0;
  chip->stack = (unsigned short *)xmalloc(sizeof(unsigned short) * STACK_SIZE);
  chip->sp = 0;
  chip->keyboard =
      (unsigned char *)xmalloc(sizeof(unsigned char) * KEYBOARD_SIZE);
  chip->pixels =
      (unsigned char *)xmalloc(sizeof(unsigned char) * WIDTH * HEIGHT);
  chip->beep = beep;
  chip->should_update = 0;
  for (int i = 0; i < MEM_SIZE; ++i) chip->memory[i] = 0;
  for (int i = 0; i < REGISTER_SIZE; ++i) chip->registers[i] = 0;
  for (int i = 0; i < STACK_SIZE; ++i) chip->stack[i] = 0;
  for (int i = 0; i < KEYBOARD_SIZE; ++i) chip->keyboard[i] = 0;
  for (int i = 0; i < WIDTH * HEIGHT; ++i) chip->pixels[i] = 0;  // 0 for black
}

int chip8_load_rom(chip8 *chip, const char *rom) {
  FILE *f = NULL;
  f = fopen(rom, "rb");
  if (f == NULL) {
    fprintf(stderr, "Error to load %s: %s", rom, strerror(errno));
    return -1;
  }
  fseek(f, 0, SEEK_END);
  long file_size = ftell(f);
  rewind(f);
  unsigned char *buf = (unsigned char *)xmalloc(file_size);  // At most 4k
  size_t bytes = fread(buf, 1, file_size, f);
  if (bytes == file_size && bytes < MEM_SIZE) {
    for (int i = 0; i < bytes; ++i) {
      chip->memory[i + 512] = buf[i];
    }
    return 0;
  }
  return -1;
}

void chip8_process_cyle(chip8 *chip) {
  chip->opcode = chip->memory[chip->pc] << 8 | chip->memory[chip->pc + 1];
  const unsigned short opcode = chip->opcode;
  printf("opcode: 0x%X\n", opcode);
  // TODO: parse opcode here
  // Update timers
  if (chip->delay_timer > 0) --chip->delay_timer;

  if (chip->sound_timer > 0) {
    if (chip->sound_timer == 1) chip->beep();
    --chip->sound_timer;
  }
}

void chip8_destroy(chip8 *chip) {
  free(chip->memory);
  free(chip->registers);
  free(chip->stack);
  free(chip->keyboard);
  free(chip->pixels);
}

// SDL stuff
void sdl_init_context(SDL_Window **window, SDL_Renderer **renderer) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(WIDTH * TIMES, HEIGHT * TIMES, 0, window,
                              renderer);
}

void sdl_clear(SDL_Renderer *renderer) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
}

void sdl_draw_pixels(SDL_Renderer *renderer, const unsigned char *pixels) {
  SDL_Rect r;
  for (int row = 0; row < HEIGHT; ++row) {
    for (int col = 0; col < WIDTH; ++col) {
      int index = row * WIDTH + col;
      if (pixels[index]) {
        // Draw white
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      } else {
        // Draw black
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      }
      r.x = col * TIMES;
      r.y = row * TIMES;
      r.w = TIMES;
      r.h = TIMES;
      SDL_RenderFillRect(renderer, &r);
    }
  }
  SDL_RenderPresent(renderer);
}

void sdl_destroy_context(SDL_Window **window, SDL_Renderer **renderer) {
  SDL_DestroyRenderer(*renderer);
  SDL_DestroyWindow(*window);
  SDL_Quit();
}

void beep() { printf("Beep\n"); }
int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: chip8 xxx.rom\n");
    return -1;
  }
  SDL_Event event;
  SDL_Renderer *renderer;
  SDL_Window *window;

  chip8 *chip = (chip8 *)xmalloc(sizeof(chip8));

  chip8_init(chip, beep);
  chip8_load_rom(chip, argv[1]);

  sdl_init_context(&window, &renderer);
  sdl_clear(renderer);

  while (1) {
    if (SDL_PollEvent(&event) && event.type == SDL_QUIT) break;
    chip8_process_cyle(chip);
    if (chip->should_update) {
      sdl_draw_pixels(renderer, chip->pixels);
      chip->should_update = 0;
    }
  }

  sdl_destroy_context(&window, &renderer);
  chip8_destroy(chip);
  return EXIT_SUCCESS;
}
