#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

typedef struct
{
  SDL_Window* main_window;
  SDL_Renderer* main_renderer;
} sdl_params;

typedef struct
{
  uint32_t window_width;
  uint32_t window_height;
  uint32_t fg_color;
  uint32_t bg_color;

} user_config_params;


bool init_sdl(sdl_params* sdl_parameters, user_config_params config_parameters)
{
  if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
  {
    SDL_Log("Could not init SDL stuff ... exiting! %s\n", SDL_GetError());
    return false;
  }

  sdl_parameters->main_window = SDL_CreateWindow(
    "Chip8Emu", 
    SDL_WINDOWPOS_CENTERED, 
    SDL_WINDOWPOS_CENTERED, 
    config_parameters.window_width, config_parameters.window_height, 
    0);

  if (sdl_parameters->main_window == NULL)
  {
    SDL_Log("Could not create window ... exiting! %s\n", SDL_GetError());
    return false;
  }

  sdl_parameters->main_renderer = SDL_CreateRenderer(sdl_parameters->main_window, -1, SDL_RENDERER_ACCELERATED);

  if (sdl_parameters->main_renderer == NULL)
  { 
    SDL_Log("Could not create renderer ... exiting! %s\n", SDL_GetError());
    return false;
  }







  return true;
}

void cleanup_sdl(sdl_params* sdl_parameters)
{
  SDL_DestroyRenderer(sdl_parameters->main_renderer);
  SDL_DestroyWindow(sdl_parameters->main_window);
  SDL_Quit();
}

void clear_screen_to_bg_color(sdl_params* sdl_parameters, user_config_params* cfg_params)
{
  uint8_t r = (cfg_params->bg_color >> (32-8))  & 0xFF;
  uint8_t g = (cfg_params->bg_color >> (32-16)) & 0xFF;
  uint8_t b = (cfg_params->bg_color >> (32-24)) & 0xFF;
  uint8_t a = (cfg_params->bg_color >> (32-32)) & 0xFF;



  SDL_SetRenderDrawColor(sdl_parameters->main_renderer, r, g, b, a);
  SDL_RenderClear(sdl_parameters->main_renderer);

}

void update_window(sdl_params* sdl_params, user_config_params* cfg_params)
{
  SDL_RenderPresent(sdl_params->main_renderer);
}

int main (int argc, char **argv)
{
  (void) argc;
  (void) argv;

  sdl_params sdl_parameters = {0};
  user_config_params config_parameters = {0};

  config_parameters.window_height = 32*10;
  config_parameters.window_width = 64*10;
  config_parameters.fg_color = 0x00000000;
  config_parameters.bg_color = 0xFFFFF0FF;



  // TODO, implement renderer and cleanup code


  if (!init_sdl(&sdl_parameters, config_parameters))
    return 1;

  clear_screen_to_bg_color(&sdl_parameters, &config_parameters);

  bool quit = false;
  SDL_Event sdl_event;

  //While application is running
  while( !quit )
  {
    //Handle events on queue
    while( SDL_PollEvent(&sdl_event) != 0) // poll for event
    {
      //User requests quit
      if( sdl_event.type == SDL_QUIT ) // unless player manually quits
        quit = true;

      puts("here1!");
        
    }

    puts("here2!");
    update_window(&sdl_parameters, &config_parameters); 


    // get time
    // emulate instructions
    // get time

    // Delay for 60 fps
    // SDL_Delay(16);
    
    // Update window with changes




  }

  cleanup_sdl(&sdl_parameters);

  puts("hehe!");
  return 0; 
}