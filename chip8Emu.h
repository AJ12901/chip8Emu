#ifndef CHIP8_EMU_H
#define CHIP8_EMU_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

// Main SDL Parameters used in a lot of functions
typedef struct
{
  SDL_Window* main_window;
  SDL_Renderer* main_renderer;
} sdl_params_t;


// User may want to pass these in as customisable parameters
typedef struct
{
  uint32_t window_width;
  uint32_t window_height;
  uint32_t fg_color;
  uint32_t bg_color;

} user_config_params_t;


// Emulator State
typedef enum
{
  QUIT = 0,
  RUNNING = 1,
  PAUSE = 2
} current_state_t;


// Could have multiple chip8_t instances for multiple windows simulatanoeusly 
typedef struct
{
  current_state_t emu_state;
} chip8_t;

#endif