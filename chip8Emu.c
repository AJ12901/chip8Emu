#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>

#include "chip8Emu.h"


int main (int argc, char** argv)
{
  const char* rom_name = argv[1];

  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s <rom_name>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Exit if user configuration parameters not initialized
  user_config_params_t config_parameters = {0};
  if (!init_user_configuration(&config_parameters, argc, argv))
    exit(EXIT_FAILURE);

  // Exit if SDL not initialized
  sdl_params_t sdl_parameters = {0};
  if (!init_sdl(&sdl_parameters, config_parameters))
    exit(EXIT_FAILURE);

  // Exit if Chip8 not initialized
  chip8_t chip8_instnace = {0};
  if (!init_chip8(&chip8_instnace, rom_name))
    exit(EXIT_FAILURE);

  clear_window(&sdl_parameters, &config_parameters);

  // Seed Random Number Generation
  srand(time(NULL));

  while (chip8_instnace.emu_state != QUIT)
  {
    // Handles all user input until nothing remains in the input queue
    handle_user_input(&chip8_instnace);

    if (chip8_instnace.emu_state == PAUSE) {continue;}

    uint64_t time_before_instructions = SDL_GetPerformanceCounter();

    // Emulate some instructions for this frame
    for (uint32_t i=0; i<(config_parameters.instructions_per_second)/60; i++)
    {
      emulate_instructions(&chip8_instnace, &config_parameters);
    }

    uint64_t time_after_instructions = SDL_GetPerformanceCounter();
    double time_emulating_instruction = (double)((time_after_instructions - time_before_instructions) / 1000) / SDL_GetPerformanceFrequency();

    // Delay by ((1/60) * 1000) to get delay in ms for 60HZ 
    SDL_Delay(16.66f > time_emulating_instruction ? 16.66f - time_emulating_instruction : 0);

    update_window(&sdl_parameters, &config_parameters, &chip8_instnace);
    update_timers(&chip8_instnace);
  }

  if (chip8_instnace.emu_state == QUIT)
    SDL_Log("\nchip8Emu quiting ... bye :((\n");

  SDL_DestroyRenderer(sdl_parameters.main_renderer);
  SDL_DestroyWindow(sdl_parameters.main_window);
  SDL_Quit();
  return 0; 
}