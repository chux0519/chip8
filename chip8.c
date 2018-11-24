#include <SDL2/SDL.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Definations
#define MEM_SIZE 4096
#define REGISTER_SIZE 16
#define STACK_SIZE 16
#define KEYBOARD_SIZE 16
#define WIDTH 64
#define HEIGHT 32
#define TIMES 8
#define FREQUENCY 60  // 60 hertz
#define FPS 30

const unsigned char fonts[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
    0x20, 0x60, 0x20, 0x20, 0x70,  // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
    0xF0, 0x80, 0xF0, 0x80, 0x80   // F
};

// For allocating memory
void *xmalloc(size_t size) {
  void *mem = (void *)malloc(size);
  if (mem == NULL) {
    fprintf(stderr, "fatal: memory exhausted (xmalloc of %zu bytes).\n", size);
    exit(-1);
  }
  return mem;
}

void log_unsupported_opcode(const short opcode) {
  printf("Opcode 0x%X is not supported.\n", opcode);
}

// Audio & Video
typedef struct chip8_av {
  SDL_Renderer **renderer;
  SDL_Window **window;
  SDL_Event *event;
} chip8_av;

void av_init(chip8_av *av, SDL_Window **window, SDL_Renderer **renderer,
             SDL_Event *event);
void av_clear_screen(chip8_av *av);
void av_beep(chip8_av *av);
void av_draw_pixels(chip8_av *av, const unsigned char *pixels);
void av_destroy(chip8_av *av);

// The Chip8
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
  chip8_av *av;           // For graphics and sound
} chip8;

// Initiate memory and registers
void chip8_init(chip8 *chip, chip8_av *av);

// Load ROM
void chip8_load_rom(chip8 *chip, const char *rom);

// Process CPU cycle
void chip8_process_cyle(chip8 *chip);

// Desctroy chip8
void chip8_destroy(chip8 *chip);

// Implementation
void chip8_init(chip8 *chip, chip8_av *av) {
  srand(time(0));
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
  chip->av = av;

  // Reset states
  for (int i = 0; i < MEM_SIZE; ++i) chip->memory[i] = 0;
  for (int i = 0; i < REGISTER_SIZE; ++i) chip->registers[i] = 0;
  for (int i = 0; i < STACK_SIZE; ++i) chip->stack[i] = 0;
  for (int i = 0; i < KEYBOARD_SIZE; ++i) chip->keyboard[i] = 0;
  for (int i = 0; i < WIDTH * HEIGHT; ++i) chip->pixels[i] = 0;  // 0 for black

  // Load fonts
  for (int i = 0; i < 80; ++i) chip->memory[i] = fonts[i];
}

void chip8_load_rom(chip8 *chip, const char *rom) {
  FILE *f = NULL;
  f = fopen(rom, "rb");
  if (f == NULL) {
    fprintf(stderr, "Error to load %s: %s", rom, strerror(errno));
    exit(-1);
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
  }
}

void chip8_process_cyle(chip8 *chip) {
  chip->opcode = chip->memory[chip->pc] << 8 | chip->memory[chip->pc + 1];
  const unsigned short opcode = chip->opcode;
  // printf("opcode: 0x%X\n", opcode);
  // Parse opcode here
  // For more infomation, see https://www.wikiwand.com/en/CHIP-8
  unsigned short nnn = opcode & 0x0FFF;
  unsigned short nn = opcode & 0x00FF;
  unsigned short n = opcode & 0x000F;
  unsigned short x = (opcode & 0x0F00) >> 8;
  unsigned short y = (opcode & 0x00F0) >> 4;

  switch (opcode & 0xF000) {
    case 0x0000: {
      switch (opcode) {
        case 0x00E0:
          // Clear pixels in chip
          for (int i = 0; i < WIDTH * HEIGHT; ++i) chip->pixels[i] = 0;
          // SDL clear
          av_clear_screen(chip->av);
          chip->pc += 2;
          break;
        case 0x00EE:
          --chip->sp;
          chip->pc = chip->stack[chip->sp];
          chip->pc += 2;
          break;
        default:
          log_unsupported_opcode(opcode);
      }
      break;
    }
    case 0x1000: {
      chip->pc = nnn;
      break;
    }
    case 0x2000: {
      chip->stack[chip->sp] = chip->pc;
      ++chip->sp;
      chip->pc = nnn;
      break;
    }
    case 0x3000: {
      if (chip->registers[x] == nn) chip->pc += 2;
      chip->pc += 2;
      break;
    }
    case 0x4000: {
      if (chip->registers[x] != nn) chip->pc += 2;
      chip->pc += 2;
      break;
    }
    case 0x5000: {
      if (chip->registers[x] == chip->registers[y]) chip->pc += 2;
      chip->pc += 2;
      break;
    }
    case 0x6000: {
      chip->registers[x] = nn;
      chip->pc += 2;
      break;
    }
    case 0x7000: {
      chip->registers[x] += nn;
      chip->pc += 2;
      break;
    }
    case 0x8000: {
      switch (n) {
        case 0:
          chip->registers[x] = chip->registers[y];
          chip->pc += 2;
          break;
        case 1:
          chip->registers[x] |= chip->registers[y];
          chip->pc += 2;
          break;
        case 2:
          chip->registers[x] &= chip->registers[y];
          chip->pc += 2;
          break;
        case 3:
          chip->registers[x] ^= chip->registers[y];
          chip->pc += 2;
          break;
        case 4:
          if (0xFF - chip->registers[x] >= chip->registers[y])
            chip->registers[0xF] = 0;  // No carry
          else
            chip->registers[0xF] = 1;
          chip->registers[x] += chip->registers[y];
          chip->pc += 2;
          break;
        case 5:
          if (chip->registers[x] >= chip->registers[y])
            chip->registers[0xF] = 1;  // No borrow
          else
            chip->registers[0xF] = 0;  // Borrow here
          chip->registers[x] -= chip->registers[y];
          chip->pc += 2;
          break;
        case 6:
          chip->registers[0xF] = chip->registers[x] & 0x1;
          chip->registers[x] >>= 1;
          chip->pc += 2;
          break;
        case 7:
          if (chip->registers[y] >= chip->registers[x])
            chip->registers[0xF] = 1;  // No borrow
          else
            chip->registers[0xF] = 0;
          chip->registers[x] = chip->registers[y] - chip->registers[x];
          chip->pc += 2;
          break;
        case 0xE:
          chip->registers[0xF] = chip->registers[x] >> 7;
          chip->registers[x] <<= 1;
          chip->pc += 2;
          break;
        default:
          log_unsupported_opcode(opcode);
      }
      break;
    }
    case 0x9000: {
      if (chip->registers[x] != chip->registers[y]) chip->pc += 2;
      chip->pc += 2;
      break;
    }
    case 0xA000: {
      chip->ir = nnn;
      chip->pc += 2;
      break;
    }
    case 0xB000: {
      chip->pc = chip->registers[0] + nnn;
      break;
    }
    case 0xC000: {
      unsigned short r = rand() % 0xFF;
      chip->registers[x] = r & nn;
      chip->pc += 2;
      break;
    }
    case 0xD000: {
      unsigned char pix;
      x = chip->registers[x];
      y = chip->registers[y];
      chip->registers[0xF] = 0;
      for (int h = 0; h < n; ++h) {
        pix = chip->memory[chip->ir + h];  // 8 bit width
        size_t idx = (y + h) * WIDTH + x;
        unsigned char mask = 0x80;  // 0b10000000
        for (int pos = 0; pos < 8; ++pos) {
          mask >>= pos;
          // Set p to 1 if not 0
          unsigned char p = (pix & mask) != 0;
          if (chip->pixels[idx + pos] && (chip->pixels[idx + pos] ^ p) == 0)
            chip->registers[0xF] = 1;  // Flip from set to unset
          // Use XOR to set pix
          // For more infomation see
          // http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#Dxyn
          chip->pixels[idx + pos] ^= p;
        }
      }
      // Draw in another timer
      chip->pc += 2;
      break;
    }
    case 0xE000: {
      unsigned short key_index;
      unsigned char key;
      switch (nn) {
        case 0x9E:
          key_index = chip->registers[x];
          key = chip->keyboard[key_index];
          if (key != 0) chip->pc += 2;
          chip->pc += 2;
          break;
        case 0xA1:
          key_index = chip->registers[x];
          key = chip->keyboard[key_index];
          if (key == 0) chip->pc += 2;
          chip->pc += 2;
          break;
        default:
          log_unsupported_opcode(opcode);
      }
      break;
    }
    case 0xF000: {
      unsigned short pressed;
      unsigned char bcd;
      switch (nn) {
        case 0x07:
          chip->registers[x] = chip->delay_timer;
          chip->pc += 2;
          break;
        case 0x0A:
          pressed = 0;
          for (int i = 0; i < 16; ++i) {
            if (chip->keyboard[i]) {
              pressed = 1;
              chip->registers[x] = i;
            }
          }
          if (pressed == 0)
            return;  // Return and will be called again in game loop
          chip->pc += 2;
          break;
        case 0x15:
          chip->delay_timer = chip->registers[x];
          chip->pc += 2;
          break;
        case 0x18:
          chip->sound_timer = chip->registers[x];
          chip->pc += 2;
          break;
        case 0x1E:
          if (chip->ir + chip->registers[x] > 0xFFF)
            chip->registers[0xF] = 1;
          else
            chip->registers[0xF] = 0;
          chip->ir += chip->registers[x];
          chip->pc += 2;
          break;
        case 0x29:
          chip->ir =
              chip->registers[x] * 5;  // Fonts loaded from 0x00 to 0xF * 5
          chip->pc += 2;
          break;
        case 0x33:
          bcd = chip->registers[x];
          chip->memory[chip->ir] = bcd / 100;
          chip->memory[chip->ir + 1] = (bcd - bcd / 100) / 10;
          chip->memory[chip->ir + 2] = bcd % 10;
          chip->pc += 2;
          break;
        case 0x55:
          for (int i = 0; i <= x; ++i) {
            chip->memory[chip->ir + i] = chip->registers[i];
          }
          chip->pc += 2;
          break;
        case 0x65:
          for (int i = 0; i <= x; ++i) {
            chip->registers[i] = chip->memory[chip->ir + i];
          }
          chip->pc += 2;
          break;
        default:
          log_unsupported_opcode(opcode);
      }
    }
  }
  // Update timers
  if (chip->delay_timer > 0) --chip->delay_timer;

  if (chip->sound_timer > 0) {
    if (chip->sound_timer == 1) av_beep(chip->av);
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
void av_init(chip8_av *av, SDL_Window **window, SDL_Renderer **renderer,
             SDL_Event *event) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(WIDTH * TIMES, HEIGHT * TIMES, 0, window,
                              renderer);
  av->window = window;
  av->renderer = renderer;
  av->event = event;
}

void av_clear_screen(chip8_av *av) {
  SDL_SetRenderDrawColor(*av->renderer, 0, 0, 0, 0);
  SDL_RenderClear(*av->renderer);
}

void av_beep(chip8_av *av) { printf("Beep.\n"); }

void av_draw_pixels(chip8_av *av, const unsigned char *pixels) {
  SDL_Rect r;
  for (int row = 0; row < HEIGHT; ++row) {
    for (int col = 0; col < WIDTH; ++col) {
      int index = row * WIDTH + col;
      if (pixels[index]) {
        // Draw white
        SDL_SetRenderDrawColor(*av->renderer, 255, 255, 255, 255);
      } else {
        // Draw black
        SDL_SetRenderDrawColor(*av->renderer, 0, 0, 0, 255);
      }
      r.x = col * TIMES;
      r.y = row * TIMES;
      r.w = TIMES;
      r.h = TIMES;
      SDL_RenderFillRect(*av->renderer, &r);
    }
  }
  SDL_RenderPresent(*av->renderer);
}

void av_destroy(chip8_av *av) {
  SDL_DestroyRenderer(*av->renderer);
  SDL_DestroyWindow(*av->window);
  SDL_Quit();
}

void chip8_key_change(chip8 *chip, SDL_Keycode code, unsigned char v) {
  switch (code) {
    case SDLK_1:
      chip->keyboard[0x1] = v;
      break;
    case SDLK_2:
      chip->keyboard[0x2] = v;
      break;
    case SDLK_3:
      chip->keyboard[0x3] = v;
      break;
    case SDLK_4:
      chip->keyboard[0xC] = v;
      break;
    case SDLK_q:
      chip->keyboard[0x4] = v;
      break;
    case SDLK_w:
      chip->keyboard[0x5] = v;
      break;
    case SDLK_e:
      chip->keyboard[0x6] = v;
      break;
    case SDLK_r:
      chip->keyboard[0xD] = v;
      break;
    case SDLK_a:
      chip->keyboard[0x7] = v;
      break;
    case SDLK_s:
      chip->keyboard[0x8] = v;
      break;
    case SDLK_d:
      chip->keyboard[0x9] = v;
      break;
    case SDLK_f:
      chip->keyboard[0xE] = v;
      break;
    case SDLK_z:
      chip->keyboard[0xA] = v;
      break;
    case SDLK_x:
      chip->keyboard[0x0] = v;
      break;
    case SDLK_c:
      chip->keyboard[0xB] = v;
      break;
    case SDLK_v:
      chip->keyboard[0xF] = v;
      break;
    default:
      break;
  }
}
void chip8_keydown(chip8 *chip, SDL_Keycode code) {
  chip8_key_change(chip, code, 1);
}
void chip8_keyup(chip8 *chip, SDL_Keycode code) {
  chip8_key_change(chip, code, 0);
}

unsigned int render(unsigned int interval, void *chip) {
  chip8 *c = (chip8 *)(chip);
  av_draw_pixels(c->av, c->pixels);
  return interval;
}

unsigned int process(unsigned int interval, void *chip) {
  chip8 *c = (chip8 *)(chip);
  chip8_process_cyle(c);
  return interval;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: chip8 xxx.rom\n");
    return -1;
  }
  SDL_Event event;
  SDL_Renderer *renderer;
  SDL_Window *window;

  chip8_av av;
  av_init(&av, &window, &renderer, &event);
  av_clear_screen(&av);

  chip8 chip;
  chip8_init(&chip, &av);
  chip8_load_rom(&chip, argv[1]);

  SDL_TimerID render_timer = SDL_AddTimer(1000 / FPS, render, &chip);
  SDL_TimerID process_timer = SDL_AddTimer(1000 / FREQUENCY, process, &chip);

  while (1) {
    SDL_Event *e = &event;
    while (SDL_PollEvent(e)) {
      switch (e->type) {
        case SDL_QUIT:
          goto quit;
        case SDL_KEYDOWN:
          chip8_keydown(&chip, e->key.keysym.sym);
          break;
        case SDL_KEYUP:
          chip8_keyup(&chip, e->key.keysym.sym);
          break;
        default:
          break;
      }
    }
    SDL_Delay(1000 / FPS);
  }
quit:
  chip8_destroy(&chip);
  av_destroy(&av);
  return EXIT_SUCCESS;
}
