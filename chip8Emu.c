#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "chip8Emu.h"

/*
 *
 *
 *    INITIALIZATION AND SETUP FUNCTIONS
 *
 * 
 */

// Initialize user configuration settings received from the CLI
bool init_user_configuration(user_config_params_t* cfg_params, int num_args, char** args_array)
{
  // Setup Default User Parameters
  cfg_params->window_height = 32 * 10;
  cfg_params->window_width = 64 * 10;
  cfg_params->fg_color = 0x00000000;
  cfg_params->bg_color = 0xFFFF00FF;

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

  sdl_parameters->main_window = SDL_CreateWindow("Chip8Emu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, config_parameters.window_width, config_parameters.window_height, 0);

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
bool init_chip8(chip8_t* c8_instance)
{
  c8_instance->emu_state = RUNNING;
  return true;
}


/*
 *
 *
 *    CLEANUP FUNCTIONS
 *
 * 
 */

// Clean up SDL Windows, Renderers, etc when SDL is about to quit
void cleanup_sdl(sdl_params_t* sdl_parameters)
{
  SDL_DestroyRenderer(sdl_parameters->main_renderer);
  SDL_DestroyWindow(sdl_parameters->main_window);
  SDL_Quit();
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



// Update the window in the main loop
void update_window(sdl_params_t* sdl_params)
{
  SDL_RenderPresent(sdl_params->main_renderer);
}



// Get User Input
void handle_user_input(chip8_t* c8_instance)
{
  SDL_Event main_events;

  while (SDL_PollEvent(&main_events))
  {
    switch (main_events.type)
    {
      case SDL_QUIT:
        c8_instance->emu_state = QUIT;
        return;
      
      case SDL_KEYDOWN:
        switch (main_events.key.keysym.sym)
        {
          case SDLK_ESCAPE:
            c8_instance->emu_state = QUIT;
            return;

          default:
            break;
        }
        break;

      case SDL_KEYUP:
        break;
      
      default:
        break;
    }
  }

}





int main (int argc, char** argv)
{
  user_config_params_t config_parameters = {0};
  if (!init_user_configuration(&config_parameters, argc, argv))
    exit(EXIT_FAILURE);

  // Exit if SDL not initialized
  sdl_params_t sdl_parameters = {0};
  if (!init_sdl(&sdl_parameters, config_parameters))
    exit(EXIT_FAILURE);

  // Exit if Chip8 not initialized
  chip8_t chip8_instnace = {0};
  if (!init_chip8(&chip8_instnace))
    exit(EXIT_FAILURE);

  clear_window(&sdl_parameters, &config_parameters);

  while (chip8_instnace.emu_state != QUIT)
  {
    // Handles all user input until nothing remains in the input queue
    handle_user_input(&chip8_instnace);

    // Delay by ((1/60) * 1000) to get delay in ms for 60HZ 
    SDL_Delay(16.66);

    update_window(&sdl_parameters);
  }

  if (chip8_instnace.emu_state == QUIT)
    printf("\nchip8Emu quiting ... bye :((\n");

  cleanup_sdl(&sdl_parameters);
  return 0; 
}