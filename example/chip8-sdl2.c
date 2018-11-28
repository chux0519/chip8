#include <SDL.h>
#include "../src/chip8.h"

#define SCALE 8
#define FPS 60

#ifdef FREQUENCY
#undef FREQUENCY // 60 hertz by default
#define FREQUENCY 500
#endif

// Audio & Video
typedef struct chip8_sdl {
  SDL_Renderer **renderer;
  SDL_Window **window;
  SDL_Event *event;
} chip8_sdl;

void chip8_sdl_init(chip8_sdl *sdl, SDL_Window **window,
                    SDL_Renderer **renderer, SDL_Event *event);
void chip8_sdl_clear_screen(chip8_sdl *sdl);
void chip8_sdl_beep(chip8_sdl *sdl);
void chip8_sdl_draw_pixels(chip8_sdl *sdl, const unsigned char *pixels);
void chip8_sdl_destroy(chip8_sdl *sdl);

// SDL stuff
void chip8_sdl_init(chip8_sdl *sdl, SDL_Window **window,
                    SDL_Renderer **renderer, SDL_Event *event) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(WIDTH * SCALE, HEIGHT * SCALE, 0, window,
                              renderer);
  sdl->window = window;
  sdl->renderer = renderer;
  sdl->event = event;
}

void chip8_sdl_clear_screen(chip8_sdl *sdl) {
  SDL_SetRenderDrawColor(*sdl->renderer, 0, 0, 0, 0);
  SDL_RenderClear(*sdl->renderer);
}

void chip8_sdl_beep(chip8_sdl *sdl) { printf("Beep.\n"); }

void chip8_sdl_draw_pixels(chip8_sdl *sdl, const unsigned char *pixels) {
  SDL_Rect r;
  for (int row = 0; row < HEIGHT; ++row) {
    for (int col = 0; col < WIDTH; ++col) {
      int index = row * WIDTH + col;
      if (pixels[index]) {
        // Draw white
        SDL_SetRenderDrawColor(*sdl->renderer, 255, 255, 255, 255);
      } else {
        // Draw black
        SDL_SetRenderDrawColor(*sdl->renderer, 0, 0, 0, 255);
      }
      r.x = col * SCALE;
      r.y = row * SCALE;
      r.w = SCALE;
      r.h = SCALE;
      SDL_RenderFillRect(*sdl->renderer, &r);
    }
  }
  SDL_RenderPresent(*sdl->renderer);
}

void chip8_sdl_destroy(chip8_sdl *sdl) {
  SDL_DestroyRenderer(*sdl->renderer);
  SDL_DestroyWindow(*sdl->window);
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

typedef struct chip8_game {
  chip8 *chip;
  chip8_sdl *sdl;
} chip8_game;

unsigned int render(unsigned int interval, void *g) {
  chip8_game *game = (chip8_game *)(g);
  chip8_sdl_draw_pixels(game->sdl, game->chip->pixels);
  if (game->chip->sound_timer == 1) chip8_sdl_beep(game->sdl);
  return interval;
}

unsigned int process(unsigned int interval, void *chip) {
  chip8 *c = (chip8 *)(chip);
  chip8_step(c);
  return interval;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: chip8-sdl2 xxx.rom\n");
    return -1;
  }
  SDL_Event event;
  SDL_Renderer *renderer;
  SDL_Window *window;

  chip8_sdl sdl;
  chip8_sdl_init(&sdl, &window, &renderer, &event);
  chip8_sdl_clear_screen(&sdl);

  chip8 chip;
  chip8_init(&chip);
  chip8_load_rom(&chip, argv[1]);

  chip8_game game = {&chip, &sdl};

  SDL_TimerID render_timer = SDL_AddTimer(1000 / FPS, render, &game);
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
  SDL_RemoveTimer(process_timer);
  SDL_RemoveTimer(render_timer);
  chip8_destroy(&chip);
  chip8_sdl_destroy(&sdl);
  return EXIT_SUCCESS;
}
