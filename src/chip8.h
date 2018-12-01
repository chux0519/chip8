#ifndef _CHIP8
#define _CHIP8

// Definations
#define MEM_SIZE 4096
#define REGISTER_SIZE 16
#define STACK_SIZE 16
#define KEYBOARD_SIZE 16
#define WIDTH 64
#define HEIGHT 32
#define FREQUENCY 60  // 60 hertz

static const unsigned char fonts[80] = {
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

typedef struct chip8_s {
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
  unsigned char *pixels;  // Pixel buffer, 0 for black, 1 for white
} chip8_t;

#ifdef __cplusplus
extern "C" {
#endif

// Initiate
void chip8_init(chip8_t *chip);

// Load ROM
void chip8_load_rom(chip8_t *chip, const char *rom);

// Process CPU cycle
void chip8_step(chip8_t *chip);

// Desctroy chip8
void chip8_destroy(chip8_t *chip);

#ifdef __cplusplus
}
#endif

#endif