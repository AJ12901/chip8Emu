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
void update_window(sdl_params_t* sdl_params, user_config_params_t* cfg_params)
{
  SDL_RenderPresent(sdl_params->main_renderer);
}

int main (int argc, char **argv)
{
  (void) argc;
  (void) argv;

  sdl_params_t sdl_parameters = {0};
  user_config_params_t config_parameters = {0};

  // Setup User Parameters
  config_parameters.window_height = 32 * 10;
  config_parameters.window_width = 64 * 10;
  config_parameters.fg_color = 0x00000000;
  config_parameters.bg_color = 0xFFFF00FF;

  // Exit if not initialized
  if (!init_sdl(&sdl_parameters, config_parameters))
    return 1;

  clear_window(&sdl_parameters, &config_parameters);

  while (true)
  {
    // Delay by ((1/60) * 1000) to get delay in ms for 60HZ 
    SDL_Delay(16.66);

    update_window(&sdl_parameters, &config_parameters);
  }

  cleanup_sdl(&sdl_parameters);
  return 0; 
}





/*
// MAC requires SDL_EVENT in order to show a window:
bool quit = false;
SDL_Event sdl_event;

// While application is running
while( !quit )
{
// Handle events on queue
  while( SDL_PollEvent(&sdl_event) != 0) // poll for event
  {
      // User requests quit
      if( sdl_event.type == SDL_QUIT ) // unless player manually quits
      quit = true;
  }

update_window(&sdl_parameters, &config_parameters); 
}
*/