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
  uint32_t top_border;
  uint32_t side_border;
  uint32_t scale_factor;
  uint32_t fg_color;
  uint32_t bg_color;
  bool pixel_outlines;
  uint32_t instructions_per_second;

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
  // Current state of the emulator
  // Name of the currently running rom
  current_state_t emu_state;
  const char* emu_romName;

  // Main System RAM
  uint8_t emu_ram[4096];
  
  // Using bool to store whether each pixel is on or off (instead of storing it in a part of RAM)
  // Originally, this was 256B and each pixel was represented by 1 bit (8b * 256B = 2048 pixels)
  bool emu_display[64*32];

  // Subroutine stack for 12 levels of subroutines (look into this more)
  uint16_t emu_subrStack[12];
  uint16_t* emu_subrStack_ptr;

  // V is 16 Data registers from V0 to VF
  // I is a 12 bit memory index/address register
  // PC is the program counter that holds the currently executing instruction (as wide as addr so 12b)
  uint8_t emu_V[16];
  uint16_t emu_I;
  uint16_t emu_pc;

  // Delay Timer: count from 60 to 0 (used for game delays like moving sprites every t ticks)
  // Sound Timer: Plays a tone when above 0
  uint8_t emu_delayTimer;
  uint8_t emu_soundTimer;

  // Whether each of the 16 keys is pressed or not
  bool emu_keypad[16];
 
} chip8_t;



/*
 *
 *
 *    INITIALIZATION AND SETUP FUNCTIONS
 *
 * 
 */

// Initialize user configuration settings received from the CLI
bool init_user_configuration(user_config_params_t* cfg_params, int num_args, char** args_array);

// Run once to initialize the SDL parameters (return true if initialized)
bool init_sdl(sdl_params_t* sdl_parameters, user_config_params_t config_parameters);

// Initialize an instance of a chip8
bool init_chip8(chip8_t* c8, const char rom_name[]);

// Clear window to the background color
void clear_window(sdl_params_t* sdl_parameters, user_config_params_t* cfg_params);



/*
 *
 *
 *    MAIN LOOP FUNCTIONS
 *
 * 
 */

// Get User Input
void handle_user_input(chip8_t* c8);

// Emulate Chip8 Instructions
void emulate_instructions(chip8_t* c8, user_config_params_t* cfg);

// Update the window
void update_window(sdl_params_t* sdl_params, user_config_params_t* cfg, chip8_t* c8);

// Update chip8 timers
void update_timers(chip8_t* c8);

#endif