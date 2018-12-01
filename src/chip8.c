#include "chip8.h"
#include "errno.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"

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

void chip8_init(chip8_t *chip) {
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

  // Reset states
  for (int i = 0; i < MEM_SIZE; ++i) chip->memory[i] = 0;
  for (int i = 0; i < REGISTER_SIZE; ++i) chip->registers[i] = 0;
  for (int i = 0; i < STACK_SIZE; ++i) chip->stack[i] = 0;
  for (int i = 0; i < KEYBOARD_SIZE; ++i) chip->keyboard[i] = 0;
  for (int i = 0; i < WIDTH * HEIGHT; ++i) chip->pixels[i] = 0;  // 0 for black

  // Load fonts
  for (int i = 0; i < 80; ++i) chip->memory[i] = fonts[i];
}

void chip8_load_rom(chip8_t *chip, const char *rom) {
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

void chip8_step(chip8_t *chip) {
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

  if (chip->sound_timer > 0) --chip->sound_timer;
}

void chip8_destroy(chip8_t *chip) {
  free(chip->memory);
  free(chip->registers);
  free(chip->stack);
  free(chip->keyboard);
  free(chip->pixels);
}