For cloning repo: 
    git clone git@github.com:AJ12901/chip8Emu.git

MAC requires SDL_EVENT in order to show a window:
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
}

update_window(&sdl_parameters, &config_parameters); 
}
