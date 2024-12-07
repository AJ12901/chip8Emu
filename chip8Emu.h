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

typedef struct
{
  uint16_t inst_opcode;   // Entire Main Instruction
  
  uint16_t inst_nnn;  // 12 Bit Address
  uint8_t inst_nn;    // 8 Bit constant
  uint8_t inst_n;     // 4 Bit constant
  uint8_t inst_x;     // 4 Bit register identifier
  uint8_t inst_y;     // 4 Bit register identifier
  uint8_t inst_op;    // Identify type/category of instruction

} instruction_t;


// Could have multiple chip8_t instances for multiple windows simulatanoeusly 
typedef struct
{
  // Current state of the emulator
  // Name of the currently running rom
  // Currently executing instruction 
  current_state_t emu_state;
  const char* emu_romName;
  instruction_t instr;

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

#endif