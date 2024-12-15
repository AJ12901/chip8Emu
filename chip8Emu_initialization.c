#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>

#include "chip8Emu.h"



// Initialize user configuration settings received from the CLI
bool init_user_configuration(user_config_params_t* cfg_params, int num_args, char** args_array)
{
  // Setup Default User Parameters
  cfg_params->scale_factor = 15;
  cfg_params->window_height = 32;
  cfg_params->top_border = 8;
  cfg_params->side_border = 2;

  cfg_params->window_width = 64;
  cfg_params->pixel_outlines = true;
  cfg_params->instructions_per_second = 500;

  // cfg_params->fg_color = 0xFFFFFFFF;
  cfg_params->fg_color = 0x33FF3300;
  cfg_params->bg_color = 0x00000000;

  // If arguments are passed, override defaults
  for (int i=0; i<num_args; i++)
  {
    printf("Argument %d: %s\n", i, args_array[i]);
  }

  return true;
}


// Run once to initialize the SDL parameters (return true if initialized)
bool init_sdl(sdl_params_t* sdl_parameters, user_config_params_t config_parameters)
{
  // Try to initialize SDL
  if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
  {
    SDL_Log("Could not init SDL stuff ... exiting! %s\n", SDL_GetError());
    return false;
  }

  // Initialize Font
  if (TTF_Init() == -1) 
  {
    SDL_Log("Error initializing SDL_ttf: %s\n", TTF_GetError());
    return false;
  }

  sdl_parameters->main_window = SDL_CreateWindow(
    "Chip8Emu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
    ((config_parameters.window_width + config_parameters.side_border + config_parameters.side_border) * config_parameters.scale_factor), 
    ((config_parameters.window_height + config_parameters.top_border + config_parameters.side_border) * config_parameters.scale_factor), 0);

  // Try to create SDL Window
  if (sdl_parameters->main_window == NULL)
  {
    SDL_Log("Could not create window ... exiting! %s\n", SDL_GetError());
    return false;
  }

  sdl_parameters->main_renderer = SDL_CreateRenderer(sdl_parameters->main_window, -1, SDL_RENDERER_ACCELERATED);

  // Try to create window renderer
  if (sdl_parameters->main_renderer == NULL)
  { 
    SDL_Log("Could not create renderer ... exiting! %s\n", SDL_GetError());
    return false;
  }

  return true;
}


// Initialize an instance of a chip8
bool init_chip8(chip8_t* c8, const char rom_name[])
{
  // Programs are generally loaded at RAM location 0x200
  const uint16_t program_entry_point = 0x200;
  
  // Each symbol is represented by a list of 5 bytes representing which of the 40 bits are on/off for that symbol
  // For example, for the letter E, we can see that in binary, the 1's represent an "E" below:
  // 1111 0000
  // 1000 0000
  // 1111 0000
  // 1000 0000
  // 1111 0000

  const uint8_t system_font[] = 
  {
      0xF0, 0x90, 0x90, 0x90, 0xF0,   // 0
      0x20, 0x60, 0x20, 0x20, 0x70,   // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0,   // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0,   // 3
      0x90, 0x90, 0xF0, 0x10, 0x10,   // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0,   // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0,   // 6
      0xF0, 0x10, 0x20, 0x40, 0x40,   // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0,   // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0,   // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90,   // A
      0xF0, 0x90, 0xF0, 0x90, 0xF0,   // B
      0xF0, 0x80, 0x80, 0x80, 0xF0,   // C
      0xF0, 0x90, 0x90, 0x90, 0xF0,   // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0,   // E
      0xF0, 0x80, 0xF0, 0x80, 0x80    // F
  };

  // Load font set (digits 0-9 and letters A-F)
  memcpy(&c8->emu_ram[0], &system_font, sizeof(system_font));

  // Read ROM Data
  FILE* rom_data = fopen(rom_name, "rb");
  if (!rom_data)
  {
    SDL_Log("chip8 ROM file %s cannot be read", rom_name);
    return false;
  }

  // Get ROM size
  fseek(rom_data, 0, SEEK_END);             // Move filePtr to end of file
  const size_t rom_size = ftell(rom_data);  // Get current filePtr
  const size_t max_rom_size = sizeof(c8->emu_ram) - program_entry_point;
  rewind(rom_data);

  if (rom_size > max_rom_size)
  {
    SDL_Log("ROM file size %u ... max allowed size %u.", (unsigned int)rom_size, (unsigned int)max_rom_size);
    return false;
  }

  // Load ROM data
  if (fread(&c8->emu_ram[program_entry_point], rom_size, 1, rom_data) != 1)
  {
    SDL_Log("Error when reading ROM file into chip8 RAM");
    return false;
  }

  fclose(rom_data);

  // Set C8 defaults
  c8->emu_state = RUNNING;
  c8->emu_pc = program_entry_point;
  c8->emu_romName = rom_name;
  c8->emu_subrStack_ptr = &c8->emu_subrStack[0];

  return true;
}


// Clear window to the background color
void clear_window(sdl_params_t* sdl_parameters, user_config_params_t* cfg_params)
{
  uint8_t r = (cfg_params->bg_color >> (32- 8)) & 0xFF;
  uint8_t g = (cfg_params->bg_color >> (32-16)) & 0xFF;
  uint8_t b = (cfg_params->bg_color >> (32-24)) & 0xFF;
  uint8_t a = (cfg_params->bg_color >> (32-32)) & 0xFF;

  SDL_SetRenderDrawColor(sdl_parameters->main_renderer, r, g, b, a);
  SDL_RenderClear(sdl_parameters->main_renderer);
}